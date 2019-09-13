/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// clang-format off
#include "table_dt.hpp"
#include "join_kernel.hpp"
#include "utils.hpp"
// clang-format on

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/time.h>

#ifndef HLS_TEST
#include <xcl2.hpp>

#define XCL_BANK(n) (XCL_MEM_TOPOLOGY | unsigned(n))

typedef struct print_buf_result_data_ {
    int i;
    long long* v;
} print_buf_result_data_t;

void CL_CALLBACK print_buf_result(cl_event event, cl_int cmd_exec_status, void* user_data) {
    print_buf_result_data_t* d = (print_buf_result_data_t*)user_data;
    printf("FPGA result %d: %lld.%lld\n", d->i, *(d->v) / 10000, *(d->v) % 10000);
}
#endif

template <typename T>
int load_dat(void* data, const std::string& name, const std::string& dir, size_t n) {
    if (!data) {
        return -1;
    }
    std::string fn = dir + "/" + name + ".dat";
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary read." << std::endl;
    }
    size_t cnt = fread(data, sizeof(T), n, f);
    fclose(f);
    if (cnt != n) {
        std::cerr << "ERROR: " << cnt << " entries read from " << fn << ", " << n << " entries required." << std::endl;
        return -1;
    }
    return 0;
}

int main(int argc, const char* argv[]) {
    std::cout << "\n------------- Hash-Join Test ----------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string mode;
    std::string xclbin_path; // eg. kernel.xclbin

    if (parser.getCmdOption("-mode", mode) && mode != "fpga") {
        std::cout << "ERROR: CPU mode is not available yet.\n";
        return 1;
    }

    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
    }

    std::string num_str;

    int num_rep = 1;
#ifndef HLS_TEST
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 1;
        }
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times\n.";
    }
#endif

    int sim_scale = 1;
    if (parser.getCmdOption("-scale", num_str)) {
        try {
            sim_scale = std::stoi(num_str);
        } catch (...) {
            sim_scale = 1;
        }
    }

    const int k_bucket = 1;
    const int ht_hbm_size = 64 / 8 * PU_HT_DEPTH;
    const int s_hbm_size = 64 / 8 * PU_S_DEPTH;

    const size_t l_depth = L_MAX_ROW + VEC_LEN - 1;
    KEY_T* col_l_orderkey = aligned_alloc<KEY_T>(l_depth);
    MONEY_T* col_l_extendedprice = aligned_alloc<MONEY_T>(l_depth);
    MONEY_T* col_l_discount = aligned_alloc<MONEY_T>(l_depth);

    const size_t o_depth = O_MAX_ROW + VEC_LEN - 1;
    KEY_T* col_o_orderkey = aligned_alloc<KEY_T>(o_depth);

    MONEY_T* row_result_a = aligned_alloc<MONEY_T>(2);
    MONEY_T* row_result_b = aligned_alloc<MONEY_T>(2);

    std::cout << "Data integer width is " << 8 * sizeof(KEY_T) << ".\n";

    std::cout << "Host map buffer has been allocated.\n";

    int join_flag = 1; // 0-hash_join,1-semi_join,2-anti_join

    int l_nrow = L_MAX_ROW / sim_scale;
    int o_nrow = O_MAX_ROW / sim_scale;
    std::cout << "Lineitem " << l_nrow << " rows\nOrders " << o_nrow << "rows" << std::endl;

    int err;
    err = load_dat<TPCH_INT>(col_l_orderkey, "l_orderkey", in_dir, l_nrow);
    if (err) return err;
    err = load_dat<TPCH_INT>(col_l_extendedprice, "l_extendedprice", in_dir, l_nrow);
    if (err) return err;
    err = load_dat<TPCH_INT>(col_l_discount, "l_discount", in_dir, l_nrow);
    if (err) return err;

    std::cout << "Lineitem table has been read from disk\n";

    err = load_dat<TPCH_INT>(col_o_orderkey, "o_orderkey", in_dir, o_nrow);
    if (err) return err;

    std::cout << "Orders table has been read from disk\n";

    const int PU_NM = 8;

#ifdef HLS_TEST
    ap_uint<64>* tb_ht[PU_NM];
    ap_uint<64>* tb_s[PU_NM];
    for (int i = 0; i < PU_NM; i++) {
        tb_ht[i] = aligned_alloc<ap_uint<64> >(PU_HT_DEPTH);
        tb_s[i] = aligned_alloc<ap_uint<64> >(PU_S_DEPTH);
    }
    join_kernel(join_flag, (ap_uint<W_TPCH_INT * VEC_LEN>*)col_o_orderkey, o_nrow,
                (ap_uint<W_TPCH_INT * VEC_LEN>*)col_l_orderkey, (ap_uint<W_TPCH_INT * VEC_LEN>*)col_l_extendedprice,
                (ap_uint<W_TPCH_INT * VEC_LEN>*)col_l_discount, l_nrow, k_bucket, (ap_uint<64>*)tb_ht[0],
                (ap_uint<64>*)tb_ht[1], (ap_uint<64>*)tb_ht[2], (ap_uint<64>*)tb_ht[3], (ap_uint<64>*)tb_ht[4],
                (ap_uint<64>*)tb_ht[5], (ap_uint<64>*)tb_ht[6], (ap_uint<64>*)tb_ht[7], (ap_uint<64>*)tb_s[0],
                (ap_uint<64>*)tb_s[1], (ap_uint<64>*)tb_s[2], (ap_uint<64>*)tb_s[3], (ap_uint<64>*)tb_s[4],
                (ap_uint<64>*)tb_s[5], (ap_uint<64>*)tb_s[6], (ap_uint<64>*)tb_s[7],
                (ap_uint<W_TPCH_INT * 2>*)row_result_a);
    long long* rv = (long long*)row_result_a;
    printf("FPGA result: %lld.%lld\n", *rv / 10000, *rv % 10000);
#else
    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device,
                       // CL_QUEUE_PROFILING_ENABLE);
                       CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel0(program, "join_kernel"); // XXX must match
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_o_orderkey = {XCL_BANK(0), col_o_orderkey, 0};
    cl_mem_ext_ptr_t mext_l_orderkey = {XCL_BANK(1), col_l_orderkey, 0};
    cl_mem_ext_ptr_t mext_l_extendedprice = {XCL_BANK(2), col_l_extendedprice, 0};
    cl_mem_ext_ptr_t mext_l_discount = {XCL_BANK(3), col_l_discount, 0};
    cl_mem_ext_ptr_t mext_result_a = {XCL_BANK(4), row_result_a, 0};
    cl_mem_ext_ptr_t mext_result_b = {XCL_BANK(4), row_result_b, 0};

    // Map buffers
    // a
    cl::Buffer buf_l_orderkey_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                (size_t)(KEY_SZ * l_depth), &mext_l_orderkey);

    cl::Buffer buf_l_extendedprice_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                     (size_t)(MONEY_SZ * l_depth), &mext_l_extendedprice);

    cl::Buffer buf_l_discout_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                               (size_t)(MONEY_SZ * l_depth), &mext_l_discount);

    cl::Buffer buf_o_orderkey_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                (size_t)(KEY_SZ * o_depth), &mext_o_orderkey);

    cl::Buffer buf_result_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                            (size_t)(MONEY_SZ * 2), &mext_result_a);

    // b (need to copy input)
    cl::Buffer buf_l_orderkey_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                (size_t)(KEY_SZ * l_depth), &mext_l_orderkey);

    cl::Buffer buf_l_extendedprice_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                     (size_t)(MONEY_SZ * l_depth), &mext_l_extendedprice);

    cl::Buffer buf_l_discout_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                               (size_t)(MONEY_SZ * l_depth), &mext_l_discount);

    cl::Buffer buf_o_orderkey_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                (size_t)(KEY_SZ * o_depth), &mext_o_orderkey);

    cl::Buffer buf_result_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                            (size_t)(MONEY_SZ * 2), &mext_result_b);

    cl::Buffer buf_ht[PU_NM];
    cl::Buffer buf_s[PU_NM];
    std::vector<cl::Memory> tb;
    for (int i = 0; i < PU_NM; i++) {
        // even
        cl_mem_ext_ptr_t me_ht = {0};
        me_ht.banks = XCL_BANK(8 + i * 2);
        buf_ht[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, (size_t)ht_hbm_size, &me_ht);
        tb.push_back(buf_ht[i]);
        // odd
        cl_mem_ext_ptr_t me_s = {0};
        me_s.banks = XCL_BANK(8 + i * 2 + 1);
        buf_s[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, (size_t)s_hbm_size, &me_s);
        tb.push_back(buf_s[i]);
    }
    q.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);

    q.finish();
    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    struct timeval tv0;
    int exec_us;
    gettimeofday(&tv0, 0);

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        write_events[i].resize(1);
        kernel_events[i].resize(1);
        read_events[i].resize(1);
    }

    /*
     * W0-. W1----.     W2-.     W3-.
     *    '-K0--. '-K1-/-. '-K2-/-. '-K3---.
     *          '---R0-  '---R1-  '---R2   '--R3
     */

    std::vector<print_buf_result_data_t> cbd(num_rep);
    std::vector<print_buf_result_data_t>::iterator it = cbd.begin();
    print_buf_result_data_t* cbd_ptr = &(*it);

    for (int i = 0; i < num_rep; ++i) {
        int use_a = i & 1;

        // write data to DDR
        std::vector<cl::Memory> ib;
        if (use_a) {
            ib.push_back(buf_o_orderkey_a);
            ib.push_back(buf_l_orderkey_a);
            ib.push_back(buf_l_extendedprice_a);
            ib.push_back(buf_l_discout_a);
        } else {
            ib.push_back(buf_o_orderkey_b);
            ib.push_back(buf_l_orderkey_b);
            ib.push_back(buf_l_extendedprice_b);
            ib.push_back(buf_l_discout_b);
        }
        if (i > 1) {
            q.enqueueMigrateMemObjects(ib, 0, &read_events[i - 2], &write_events[i][0]);
        } else {
            q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_events[i][0]);
        }

        // set args and enqueue kernel
        if (use_a) {
            int j = 0;
            kernel0.setArg(j++, join_flag);
            kernel0.setArg(j++, buf_o_orderkey_a);
            kernel0.setArg(j++, o_nrow);
            kernel0.setArg(j++, buf_l_orderkey_a);
            kernel0.setArg(j++, buf_l_extendedprice_a);
            kernel0.setArg(j++, buf_l_discout_a);
            kernel0.setArg(j++, l_nrow);
            kernel0.setArg(j++, k_bucket);
            kernel0.setArg(j++, buf_ht[0]);
            kernel0.setArg(j++, buf_ht[1]);
            kernel0.setArg(j++, buf_ht[2]);
            kernel0.setArg(j++, buf_ht[3]);
            kernel0.setArg(j++, buf_ht[4]);
            kernel0.setArg(j++, buf_ht[5]);
            kernel0.setArg(j++, buf_ht[6]);
            kernel0.setArg(j++, buf_ht[7]);
            kernel0.setArg(j++, buf_s[0]);
            kernel0.setArg(j++, buf_s[1]);
            kernel0.setArg(j++, buf_s[2]);
            kernel0.setArg(j++, buf_s[3]);
            kernel0.setArg(j++, buf_s[4]);
            kernel0.setArg(j++, buf_s[5]);
            kernel0.setArg(j++, buf_s[6]);
            kernel0.setArg(j++, buf_s[7]);
            kernel0.setArg(j++, buf_result_a);
        } else {
            int j = 0;
            kernel0.setArg(j++, join_flag);
            kernel0.setArg(j++, buf_o_orderkey_b);
            kernel0.setArg(j++, o_nrow);
            kernel0.setArg(j++, buf_l_orderkey_b);
            kernel0.setArg(j++, buf_l_extendedprice_b);
            kernel0.setArg(j++, buf_l_discout_b);
            kernel0.setArg(j++, l_nrow);
            kernel0.setArg(j++, k_bucket);
            kernel0.setArg(j++, buf_ht[0]);
            kernel0.setArg(j++, buf_ht[1]);
            kernel0.setArg(j++, buf_ht[2]);
            kernel0.setArg(j++, buf_ht[3]);
            kernel0.setArg(j++, buf_ht[4]);
            kernel0.setArg(j++, buf_ht[5]);
            kernel0.setArg(j++, buf_ht[6]);
            kernel0.setArg(j++, buf_ht[7]);
            kernel0.setArg(j++, buf_s[0]);
            kernel0.setArg(j++, buf_s[1]);
            kernel0.setArg(j++, buf_s[2]);
            kernel0.setArg(j++, buf_s[3]);
            kernel0.setArg(j++, buf_s[4]);
            kernel0.setArg(j++, buf_s[5]);
            kernel0.setArg(j++, buf_s[6]);
            kernel0.setArg(j++, buf_s[7]);
            kernel0.setArg(j++, buf_result_b);
        }
        q.enqueueTask(kernel0, &write_events[i], &kernel_events[i][0]);

        // read data from DDR
        std::vector<cl::Memory> ob;
        if (use_a) {
            ob.push_back(buf_result_a);
        } else {
            ob.push_back(buf_result_b);
        }
        q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        if (use_a) {
            cbd_ptr[i].i = i;
            cbd_ptr[i].v = (long long*)row_result_a;
            read_events[i][0].setCallback(CL_COMPLETE, print_buf_result, cbd_ptr + i);
        } else {
            cbd_ptr[i].i = i;
            cbd_ptr[i].v = (long long*)row_result_b;
            read_events[i][0].setCallback(CL_COMPLETE, print_buf_result, cbd_ptr + i);
        }
    }

    // wait all to finish.
    q.flush();
    q.finish();

    struct timeval tv3;
    gettimeofday(&tv3, 0);
    exec_us = tvdiff(&tv0, &tv3);
    std::cout << "FPGA execution time of " << num_rep << " runs: " << exec_us << " usec\n"
              << "Average execution per run: " << exec_us / num_rep << " usec\n";

    for (int i = 0; i < num_rep; ++i) {
        cl_ulong ts, te;
        kernel_events[i][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
        kernel_events[i][0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
        unsigned long t = (te - ts) / 1000;
        printf("INFO: kernel %d: execution time %lu usec\n", i, t);
    }
#endif

    if (l_nrow == 100 && o_nrow == 100) {
        std::cout << "---------------------------------------------\n\n";
        std::cout << "PostgreSQL ref result: 582130.7687\n";
    } else if (l_nrow == L_MAX_ROW && o_nrow == O_MAX_ROW) {
        std::cout << "---------------------------------------------\n\n";
        std::cout << "PostgreSQL ref result: 33093172473.7938\n";
    }

    std::cout << "---------------------------------------------\n\n";
    return 0;
}
