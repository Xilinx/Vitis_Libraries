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
#include "table_dt.hpp"
#include "utils.hpp"
#include "q5simple_cfg.hpp"

#include <unordered_map>
#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

const int PU_NM = 8;

#include <ap_int.h>

#ifdef HLS_TEST
#include "gqe_join.hpp"
#else
#include <CL/cl_ext_xilinx.h>
#include <xcl2.hpp>

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)

#endif // HLS_TEST

template <typename T>
int generate_data(T* data, int range, size_t n) {
    if (!data) {
        return -1;
    }

    for (size_t i = 0; i < n; i++) {
        data[i] = (T)(rand() % range + 1);
    }

    return 0;
}

ap_uint<512> get_table_header(int n512b, int nrow) {
    ap_uint<512> th = 0;
    th.range(31, 0) = nrow;
    th.range(63, 32) = n512b;
    return th;
}

ap_uint<64> get_golden_sum(int l_row,
                           ap_uint<32>* col_l_orderkey,
                           ap_uint<32>* col_l_extendedprice,
                           ap_uint<32>* col_l_discount,
                           int o_row,
                           ap_uint<32>* col_o_orderkey,
                           ap_uint<32>* col_o_orderdate) {
    ap_uint<64> sum = 0;
    int cnt = 0;

    std::unordered_multimap<uint32_t, uint32_t> ht1;

    {
        for (int i = 0; i < o_row; ++i) {
            uint32_t k = col_o_orderkey[i];
            uint32_t date = col_o_orderdate[i];
            // insert into hash table
            ht1.insert(std::make_pair(k, date));
        }
    }
    // read t once
    for (int i = 0; i < l_row; ++i) {
        uint32_t k = col_l_orderkey[i];
        uint32_t p = col_l_extendedprice[i];
        uint32_t d = col_l_discount[i];
        // check hash table
        auto its = ht1.equal_range(k);
        for (auto it = its.first; it != its.second; ++it) {
            sum += (p * (100 - d));
            ++cnt;
        }
    }

    std::cout << "INFO: CPU ref matched " << cnt << " rows, sum = " << sum << std::endl;
    return sum;
}
int main(int argc, const char* argv[]) {
    std::cout << "\n--------- TPC-H Query 5 Simplified (1G) ---------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string scale;
    int sim_scale = 1;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }

    int num_rep = 1;
#ifndef HLS_TEST
    std::string num_str;
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

    int l_nrow = L_MAX_ROW / sim_scale;
    int o_nrow = O_MAX_ROW / sim_scale;

    std::cout << "Lineitem " << l_nrow << " rows\n"
              << "Orders " << o_nrow << " rows\n";

    const size_t l_depth = L_MAX_ROW + VEC_LEN * 2 - 1;

    const size_t o_depth = O_MAX_ROW + VEC_LEN * 2 - 1;

    ap_uint<512>* table_l;
    ap_uint<512>* table_o;

    size_t size_table_l;
    size_t size_table_o;

    const size_t table_l_depth = size_t((sizeof(TPCH_INT) * l_depth + 64 - 1) / 64);
    const size_t table_o_depth = size_t((sizeof(TPCH_INT) * o_depth + 64 - 1) / 64);

    size_table_l = table_l_depth * 3;
    table_l = aligned_alloc<ap_uint<512> >(size_table_l);
    table_l[0] = get_table_header(table_l_depth, l_nrow);
    table_l[table_l_depth] = 0;
    table_l[2 * table_l_depth] = 0;
    table_l[3 * table_l_depth] = 0;

    size_table_o = table_o_depth * 2;
    table_o = aligned_alloc<ap_uint<512> >(size_table_o);
    table_o[0] = get_table_header(table_o_depth, o_nrow);
    table_o[table_o_depth] = 0;
    table_o[2 * table_o_depth] = 0;

    const size_t result_depth = 180000 + VEC_LEN;
    const size_t table_result_depth = size_t((result_depth * sizeof(TPCH_INT) + 64 - 1) / 64);
    const size_t table_result_size = table_result_depth * 8; // XXX aggr out buf must be 5col

    ap_uint<512>* table_out_a = aligned_alloc<ap_uint<512> >(table_result_size);
    table_out_a[0] = get_table_header(table_result_depth, 0);

    ap_uint<512>* table_out_b = aligned_alloc<ap_uint<512> >(table_result_size);
    table_out_b[0] = get_table_header(table_result_depth, 0);

    ap_uint<512>* table_cfg5s = aligned_alloc<ap_uint<512> >(9);
    get_q5simple_cfg(table_cfg5s);

    ap_uint<64>* htb_buf[PU_NM];
    ap_uint<64>* stb_buf[PU_NM];
    for (int i = 0; i < PU_NM; i++) {
        htb_buf[i] = aligned_alloc<ap_uint<64> >(HT_BUFF_DEPTH);
        stb_buf[i] = aligned_alloc<ap_uint<64> >(S_BUFF_DEPTH);
    }

    std::cout << "Host map buffer has been allocated.\n";

    int err;

    memcpy(table_l, &l_nrow, sizeof(TPCH_INT));
    err = generate_data<TPCH_INT>((int*)(table_l + 1), 100000, l_nrow);
    if (err) return err;

    memcpy(table_l + table_l_depth * 1, &l_nrow, sizeof(TPCH_INT));
    err = generate_data<TPCH_INT>((int*)(table_l + 1 + table_l_depth * 1), 10000000, l_nrow);
    if (err) return err;

    memcpy(table_l + table_l_depth * 2, &l_nrow, sizeof(TPCH_INT));
    err = generate_data<TPCH_INT>((int*)(table_l + 1 + table_l_depth * 2), 10, l_nrow);
    if (err) return err;

    std::cout << "Lineitem table has been read from disk\n";

    memcpy(table_o, &o_nrow, sizeof(TPCH_INT));
    err = generate_data<TPCH_INT>((int*)(table_o + 1), 100000, o_nrow);
    if (err) return err;

    memcpy(table_o + table_o_depth, &o_nrow, sizeof(TPCH_INT));
    err = generate_data<TPCH_INT>((int*)(table_o + 1 + table_o_depth), 20000000, o_nrow);
    if (err) return err;

    std::cout << "Orders table has been read from disk\n";

    long long golden = get_golden_sum(l_nrow, (ap_uint<32>*)(table_l + 1), (ap_uint<32>*)(table_l + 1 + table_l_depth),
                                      (ap_uint<32>*)(table_l + 1 + table_l_depth * 2), o_nrow,
                                      (ap_uint<32>*)(table_o + 1), (ap_uint<32>*)(table_o + 1 + table_o_depth));
    long long v = 0;

#ifdef HLS_TEST
    gqeJoin((ap_uint<512>*)table_o, (ap_uint<512>*)table_l, (ap_uint<512>*)table_out_b, (ap_uint<512>*)table_cfg5s,
            (ap_uint<64>*)htb_buf[0], (ap_uint<64>*)htb_buf[1], (ap_uint<64>*)htb_buf[2], (ap_uint<64>*)htb_buf[3],
            (ap_uint<64>*)htb_buf[4], (ap_uint<64>*)htb_buf[5], (ap_uint<64>*)htb_buf[6], (ap_uint<64>*)htb_buf[7],
            (ap_uint<64>*)stb_buf[0], (ap_uint<64>*)stb_buf[1], (ap_uint<64>*)stb_buf[2], (ap_uint<64>*)stb_buf[3],
            (ap_uint<64>*)stb_buf[4], (ap_uint<64>*)stb_buf[5], (ap_uint<64>*)stb_buf[6], (ap_uint<64>*)stb_buf[7]);
    v = table_out_b[1].range(TPCH_INT_SZ * 8 * 4 - 1, TPCH_INT_SZ * 8 * 2).to_int64();
#else
    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);

    cl::Kernel kernel0table[2];
    for (int i = 0; i < 2; i++) {
        kernel0table[i] = cl::Kernel(program, "gqeJoin");
    };

    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_o, mext_table_l, mext_table_out_a, mext_table_out_b, mext_cfg5s;
    cl_mem_ext_ptr_t memExt[PU_NM * 2];

    mext_table_o = {0, table_o, kernel0table[0]()};
    mext_table_l = {1, table_l, kernel0table[0]()};
    mext_table_out_a = {2, table_out_a, kernel0table[0]()};
    mext_table_out_b = {2, table_out_b, kernel0table[0]()};
    mext_cfg5s = {3, table_cfg5s, kernel0table[0]()};

    memExt[0].flags = (unsigned int)(2) | XCL_MEM_TOPOLOGY;
    memExt[1].flags = (unsigned int)(3) | XCL_MEM_TOPOLOGY;
    memExt[2].flags = (unsigned int)(10) | XCL_MEM_TOPOLOGY;
    memExt[3].flags = (unsigned int)(11) | XCL_MEM_TOPOLOGY;
    memExt[4].flags = (unsigned int)(18) | XCL_MEM_TOPOLOGY;
    memExt[5].flags = (unsigned int)(19) | XCL_MEM_TOPOLOGY;
    memExt[6].flags = (unsigned int)(26) | XCL_MEM_TOPOLOGY;
    memExt[7].flags = (unsigned int)(27) | XCL_MEM_TOPOLOGY;
    memExt[8].flags = (unsigned int)(6) | XCL_MEM_TOPOLOGY;
    memExt[9].flags = (unsigned int)(7) | XCL_MEM_TOPOLOGY;
    memExt[10].flags = (unsigned int)(14) | XCL_MEM_TOPOLOGY;
    memExt[11].flags = (unsigned int)(15) | XCL_MEM_TOPOLOGY;
    memExt[12].flags = (unsigned int)(22) | XCL_MEM_TOPOLOGY;
    memExt[13].flags = (unsigned int)(23) | XCL_MEM_TOPOLOGY;
    memExt[14].flags = (unsigned int)(30) | XCL_MEM_TOPOLOGY;
    memExt[15].flags = (unsigned int)(31) | XCL_MEM_TOPOLOGY;

    for (int i = 0; i < PU_NM; ++i) {
        memExt[i].param = 0;
        memExt[i].obj = htb_buf[i];
    }
    for (int i = PU_NM; i < PU_NM * 2; ++i) {
        memExt[i].param = 0;
        memExt[i].obj = stb_buf[i - PU_NM];
    }

    // Map buffers
    cl::Buffer buf_table_o_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             (size_t)(sizeof(ap_uint<512>) * size_table_o), &mext_table_o);

    cl::Buffer buf_table_l_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             (size_t)(sizeof(ap_uint<512>) * size_table_l), &mext_table_l);

    cl::Buffer buf_table_out_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               (size_t)(sizeof(ap_uint<512>) * table_result_size), &mext_table_out_a);

    cl::Buffer buf_cfg5s_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                           (sizeof(ap_uint<512>) * 9), &mext_cfg5s);

    cl::Buffer buff_a[PU_NM * 2];
    std::vector<cl::Memory> tb;
    // XXX force to write from hbuf to dbuf, to work-around ECC problem.
    for (int i = 0; i < PU_NM; i++) {
        buff_a[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(sizeof(ap_uint<64>) * HT_BUFF_DEPTH), &memExt[i]);
        tb.push_back(buff_a[i]);
    }
    for (int i = PU_NM; i < PU_NM * 2; i++) {
        buff_a[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(KEY_SZ * 2 * S_BUFF_DEPTH), &memExt[i]);
        tb.push_back(buff_a[i]);
    }

    q.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);

    q.finish();
    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events_l(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        write_events[i].resize(1);
        kernel_events_l[i].resize(1);
        read_events[i].resize(1);
    }

    std::vector<cl::Memory> ibtable[2];
    ibtable[0].push_back(buf_table_o_a);
    ibtable[0].push_back(buf_table_l_a);
    ibtable[0].push_back(buf_table_out_a);
    ibtable[0].push_back(buf_cfg5s_a);

    // set args and enqueue kernel
    int j = 0;
    kernel0table[0].setArg(j++, buf_table_o_a);
    kernel0table[0].setArg(j++, buf_table_l_a);
    kernel0table[0].setArg(j++, buf_table_out_a);
    kernel0table[0].setArg(j++, buf_cfg5s_a);
    for (int i = 0; i < PU_NM * 2; i++) {
        kernel0table[0].setArg(j++, buff_a[i]);
    }

    struct timeval tv0;
    int exec_us;
    gettimeofday(&tv0, 0);
    for (int i = 0; i < num_rep; ++i) {
        q.enqueueMigrateMemObjects(ibtable[0], 0, nullptr, &write_events[i][0]);

        q.enqueueTask(kernel0table[0], &write_events[i], &kernel_events_l[i][0]);

        std::vector<cl::Memory> obtable;
        obtable.push_back(buf_table_out_a);
        q.enqueueMigrateMemObjects(obtable, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events_l[i], &read_events[i][0]);
    }

    // wait all to finish.
    q.flush();
    q.finish();

    struct timeval tv3;
    gettimeofday(&tv3, 0);
    exec_us = tvdiff(&tv0, &tv3);
    v = table_out_a[1].range(TPCH_INT_SZ * 8 * 4 - 1, TPCH_INT_SZ * 8 * 2).to_int64();
    std::cout << "FPGA execution time of " << num_rep << " runs: " << exec_us / 1000 << " ms\n"
              << "Average execution per run: " << exec_us / num_rep / 1000 << " ms\n"
              << "FPGA result: " << v / 10000 << "." << v % 10000 << " \n";

#endif // HLS_TEST

    return golden - v;
}
