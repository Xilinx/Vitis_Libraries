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
#include "q6_cfg.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/time.h>

// XXX work-around gmp.h issue.
#include <cstddef>

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

ap_uint<512> get_table_header(int n512b, int nrow) {
    ap_uint<512> th = 0;
    th.range(31, 0) = nrow;
    th.range(63, 32) = n512b;
    return th;
}

void compload(int* a, int* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) std::cout << i << " :  " << a[i] << " " << b[i] << std::endl;
    }
}

int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H Query 6 (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string mode;
    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin

    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
    }

    int board = 0;
    std::string board_s;
    if (parser.getCmdOption("-b", board_s)) {
        try {
            board = std::stoi(board_s);
        } catch (...) {
            board = 0;
        }
    }

#ifdef HLS_TEST
    int num_rep = 1;
#else
    int num_rep = 1;
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

    // ************* //

    int l_nrow = L_MAX_ROW;
    std::cout << "Lineitem " << l_nrow << " rows\n";

    const size_t l_depth = L_MAX_ROW + VEC_LEN * 2 - 1;
    const size_t table_l_depth = size_t((sizeof(TPCH_INT) * l_depth + 64 - 1) / 64);
    const size_t size_table_l = table_l_depth * 4; // 4 columns
    ap_uint<512>* table_l = aligned_alloc<ap_uint<512> >(size_table_l);
    table_l[0] = get_table_header(table_l_depth, l_nrow);
    table_l[1 * table_l_depth] = 0;
    table_l[2 * table_l_depth] = 0;
    table_l[3 * table_l_depth] = 0;

    std::cout << "table_l_depth=" << table_l_depth << std::endl;

    const size_t table_result_depth = 32;                    // XXX at least enough for burst.
    const size_t size_table_result = table_result_depth * 8; // XXX 5 column, required by write_out
    ap_uint<512>* table_result_a = aligned_alloc<ap_uint<512> >(size_table_result);
    table_result_a[0] = get_table_header(table_result_depth, 0);
    table_result_a[1 * table_result_depth] = 0;
    table_result_a[2 * table_result_depth] = 0;
    table_result_a[3 * table_result_depth] = 0;
    table_result_a[4 * table_result_depth] = 0;
    ap_uint<512>* table_result_b = aligned_alloc<ap_uint<512> >(size_table_result);
    table_result_b[0] = get_table_header(table_result_depth, 0);
    table_result_b[1 * table_result_depth] = 0;
    table_result_b[2 * table_result_depth] = 0;
    table_result_b[3 * table_result_depth] = 0;
    table_result_b[4 * table_result_depth] = 0;

    std::cout << "table_result_depth=" << table_result_depth << std::endl;

    ap_uint<512>* table_cfg6 = aligned_alloc<ap_uint<512> >(9); //(size_table_result);
    get_q6_cfg(table_cfg6);

    ap_uint<64>* htb_buf[PU_NM];
    ap_uint<64>* stb_buf[PU_NM];
    for (int i = 0; i < PU_NM; i++) {
        htb_buf[i] = aligned_alloc<ap_uint<64> >(HT_BUFF_DEPTH);
        stb_buf[i] = aligned_alloc<ap_uint<64> >(S_BUFF_DEPTH);
    }

    std::cout << "Host map buffer has been allocated.\n";

    int err;

    {
        // 0
        memcpy(table_l, &l_nrow, sizeof(TPCH_INT));
        err = load_dat<TPCH_INT>(table_l + 1, "l_extendedprice", in_dir, l_nrow);
        if (err) return err;
        // 1
        memcpy(table_l, &l_nrow, sizeof(TPCH_INT));
        err = load_dat<TPCH_INT>(table_l + 1 + table_l_depth * 1, "l_discount", in_dir, l_nrow);
        if (err) return err;
        // 2
        memcpy(table_l, &l_nrow, sizeof(TPCH_INT));
        err = load_dat<TPCH_INT>(table_l + 1 + table_l_depth * 2, "l_shipdate", in_dir, l_nrow);
        if (err) return err;
        // 2
        memcpy(table_l, &l_nrow, sizeof(TPCH_INT));
        err = load_dat<TPCH_INT>(table_l + 1 + table_l_depth * 3, "l_quantity", in_dir, l_nrow);
        if (err) return err;
    }

    std::cout << "Lineitem table has been read from disk\n";

#ifdef HLS_TEST
    gqeJoin((ap_uint<512>*)table_l, (ap_uint<512>*)NULL, (ap_uint<512>*)table_result_b, (ap_uint<512>*)table_cfg6,
            (ap_uint<64>*)htb_buf[0], (ap_uint<64>*)htb_buf[1], (ap_uint<64>*)htb_buf[2], (ap_uint<64>*)htb_buf[3],
            (ap_uint<64>*)htb_buf[4], (ap_uint<64>*)htb_buf[5], (ap_uint<64>*)htb_buf[6], (ap_uint<64>*)htb_buf[7],
            (ap_uint<64>*)stb_buf[0], (ap_uint<64>*)stb_buf[1], (ap_uint<64>*)stb_buf[2], (ap_uint<64>*)stb_buf[3],
            (ap_uint<64>*)stb_buf[4], (ap_uint<64>*)stb_buf[5], (ap_uint<64>*)stb_buf[6], (ap_uint<64>*)stb_buf[7]);
#else
    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[board];

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device,
                       // CL_QUEUE_PROFILING_ENABLE);
                       CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    std::vector<cl::Device> devices_;
    devices.resize(1);
    devices_.push_back(device);
    cl::Program program(context, devices_, xclBins);

    cl::Kernel kernel0tableq6[2];
    for (int i = 0; i < 2; i++) {
        kernel0tableq6[i] = cl::Kernel(program, "gqeJoin");
    };

    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_l, mext_table_x, mext_table_result_a, mext_table_result_b, mext_cfg6;
    cl_mem_ext_ptr_t memExt[PU_NM * 2];

#ifdef USE_DDR
    mext_table_l = {XCL_MEM_DDR_BANK0, table_l, 0};
    mext_table_x = {XCL_MEM_DDR_BANK0, 0, 0};
    mext_table_result_a = {XCL_MEM_DDR_BANK0, table_result_a, 0};
    mext_table_result_b = {XCL_MEM_DDR_BANK0, table_result_b, 0};
    mext_cfg6 = {XCL_MEM_DDR_BANK0, table_cfg6, 0};
    memExt[0].flags = XCL_MEM_DDR_BANK1;
    memExt[1].flags = XCL_MEM_DDR_BANK1;
    memExt[2].flags = XCL_MEM_DDR_BANK1;
    memExt[3].flags = XCL_MEM_DDR_BANK2;
    memExt[4].flags = XCL_MEM_DDR_BANK2;
    memExt[5].flags = XCL_MEM_DDR_BANK2;
    memExt[6].flags = XCL_MEM_DDR_BANK3;
    memExt[7].flags = XCL_MEM_DDR_BANK3;

    memExt[8].flags = XCL_MEM_DDR_BANK1;
    memExt[9].flags = XCL_MEM_DDR_BANK1;
    memExt[10].flags = XCL_MEM_DDR_BANK1;
    memExt[11].flags = XCL_MEM_DDR_BANK2;
    memExt[12].flags = XCL_MEM_DDR_BANK2;
    memExt[13].flags = XCL_MEM_DDR_BANK2;
    memExt[14].flags = XCL_MEM_DDR_BANK3;
    memExt[15].flags = XCL_MEM_DDR_BANK3;
#else // HBM
    mext_table_l = {XCL_MEM_TOPOLOGY | unsigned(32), table_l, 0};
    mext_table_x = {XCL_MEM_TOPOLOGY | unsigned(32), 0, 0};
    mext_table_result_a = {XCL_MEM_TOPOLOGY | unsigned(32), table_result_a, 0};
    mext_table_result_b = {XCL_MEM_TOPOLOGY | unsigned(32), table_result_b, 0};
    mext_cfg6 = {XCL_MEM_TOPOLOGY | unsigned(32), table_cfg6, 0};
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
#endif
    for (int i = 0; i < PU_NM; ++i) {
        memExt[i].param = 0;
        memExt[i].obj = htb_buf[i];
    }
    for (int i = PU_NM; i < PU_NM * 2; ++i) {
        memExt[i].param = 0;
        memExt[i].obj = stb_buf[i - PU_NM];
    }

    // Map buffers
    cl::Buffer buf_table_l_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             (sizeof(ap_uint<512>) * size_table_l), &mext_table_l);
    cl::Buffer buf_table_l_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             (sizeof(ap_uint<512>) * size_table_l), &mext_table_l);

    cl::Buffer buf_table_x_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY, (sizeof(ap_uint<512>) * 8),
                             &mext_table_x);
    cl::Buffer buf_table_x_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY, (sizeof(ap_uint<512>) * 8),
                             &mext_table_x);

    cl::Buffer buf_table_result_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  (sizeof(ap_uint<512>) * size_table_result), &mext_table_result_a);
    cl::Buffer buf_table_result_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  (sizeof(ap_uint<512>) * size_table_result), &mext_table_result_b);

    cl::Buffer buf_cfg6_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                          (sizeof(ap_uint<512>) * 9), &mext_cfg6);
    cl::Buffer buf_cfg6_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                          (sizeof(ap_uint<512>) * 9), &mext_cfg6);

    cl::Buffer buff_a[PU_NM * 2];
    cl::Buffer buff_b[PU_NM * 2];
    std::vector<cl::Memory> tb;
    // XXX force to write from hbuf to dbuf, to work-around ECC problem.
    for (int i = 0; i < PU_NM; i++) {
        buff_a[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(KEY_SZ * 2 * (1 << 20)), &memExt[i]);
        tb.push_back(buff_a[i]);
        buff_b[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(KEY_SZ * 2 * (1 << 20)), &memExt[i]);
        tb.push_back(buff_b[i]);
    }
    for (int i = PU_NM; i < PU_NM * 2; i++) {
        buff_a[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(KEY_SZ * 2 * (1 << 20)), &memExt[i]);
        tb.push_back(buff_a[i]);
        buff_b[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                               (size_t)(KEY_SZ * 2 * (1 << 20)), &memExt[i]);
        tb.push_back(buff_b[i]);
    }
    q.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);

    q.finish();
    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        write_events[i].resize(1);
        kernel_events[i].resize(1);
        read_events[i].resize(1);
    }

    // std::vector<print_buf_result_data_t> cbd(num_rep);
    // std::vector<print_buf_result_data_t>::iterator it = cbd.begin();
    // print_buf_result_data_t* cbd_ptr = &(*it);

    std::vector<cl::Memory> ibtable[2];
    ibtable[0].push_back(buf_table_l_a);
    ibtable[0].push_back(buf_table_x_a);
    ibtable[0].push_back(buf_table_result_a);
    ibtable[0].push_back(buf_cfg6_a);

    ibtable[1].push_back(buf_table_l_b);
    ibtable[1].push_back(buf_table_x_b);
    ibtable[1].push_back(buf_table_result_b);
    ibtable[1].push_back(buf_cfg6_b);

    int j = 0;
    kernel0tableq6[0].setArg(j++, buf_table_l_a);
    kernel0tableq6[0].setArg(j++, buf_table_x_a);
    kernel0tableq6[0].setArg(j++, buf_table_result_a);
    kernel0tableq6[0].setArg(j++, buf_cfg6_a);
    for (int i = 0; i < PU_NM * 2; ++i) {
        kernel0tableq6[0].setArg(j++, buff_a[i]);
    }
    j = 0;
    kernel0tableq6[1].setArg(j++, buf_table_l_b);
    kernel0tableq6[1].setArg(j++, buf_table_x_b);
    kernel0tableq6[1].setArg(j++, buf_table_result_b);
    kernel0tableq6[1].setArg(j++, buf_cfg6_b);
    for (int i = 0; i < PU_NM * 2; ++i) {
        kernel0tableq6[1].setArg(j++, buff_b[i]);
    }

    struct timeval tv0;
    int exec_us;
    gettimeofday(&tv0, 0);
    for (int i = 0; i < num_rep; ++i) {
        int use_a = i & 1;

        // write data to DDR
        if (use_a) {
            if (i > 1) {
                q.enqueueMigrateMemObjects(ibtable[0], 0, &read_events[i - 2], &write_events[i][0]);
            } else {
                q.enqueueMigrateMemObjects(ibtable[0], 0, nullptr, &write_events[i][0]);
            };
        } else {
            if (i > 1) {
                q.enqueueMigrateMemObjects(ibtable[1], 0, &read_events[i - 2], &write_events[i][0]);
            } else {
                q.enqueueMigrateMemObjects(ibtable[1], 0, nullptr, &write_events[i][0]);
            };
        }

        // enqueue kernel
        if (use_a) {
            q.enqueueTask(kernel0tableq6[0], &write_events[i], &kernel_events[i][0]);
        } else {
            q.enqueueTask(kernel0tableq6[1], &write_events[i], &kernel_events[i][0]);
        }

        // read data from DDR
        std::vector<cl::Memory> obtable;
        if (use_a) {
            obtable.push_back(buf_table_result_a);
        } else {
            obtable.push_back(buf_table_result_b);
        }

        q.enqueueMigrateMemObjects(obtable, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
    }

    // wait all to finish.
    q.flush();
    q.finish();

    struct timeval tv1;
    gettimeofday(&tv1, 0);
    exec_us = tvdiff(&tv0, &tv1);
    /*    std::cout << "FPGA execution time of " << num_rep << " runs: " << exec_us / 1000 << " ms\n"
                  << "Average execution per run: " << exec_us / num_rep / 1000 << " ms\n";*/
    std::cout << "All execution time of Host/Kernel " << std::dec << exec_us / 1000 << " ms" << std::endl;

#endif // HLS_TEST

    if (num_rep > 1) {
        ap_uint<512> d = table_result_a[1 + table_result_depth * 4];
        // printf("min=%d, ", d.range(31, 0).to_int());
        // printf("max=%d, ", d.range(63, 32).to_int());
        // printf("sum=%lld, ", d.range(127, 64).to_int64());
        // printf("cnt=%lld\n", d.range(192, 128).to_int64());

        long long v = d.range(127, 64).to_int64();
        printf("\n\nFPGA result: %lld.%lld\n", v / 10000, v % 10000);
    }

    {
        ap_uint<512> d = table_result_b[1 + table_result_depth * 4];
        // printf("min=%d, ", d.range(31, 0).to_int());
        // printf("max=%d, ", d.range(63, 32).to_int());
        // printf("sum=%lld, ", d.range(127, 64).to_int64());
        // printf("cnt=%lld\n", d.range(192, 128).to_int64());

        long long v = d.range(127, 64).to_int64();
        printf("\n\nFPGA result: %lld.%lld\n", v / 10000, v % 10000);
    }

    std::cout << "---------------------------------------------"
                 "\nCPU reference:\n"
                 "\nselect"
                 "\n  sum(l_extendedprice * l_discount) as revenue"
                 "\nfrom"
                 "\n  lineitem"
                 "\nwhere"
                 "\n  l_shipdate >= date '1994-01-01'"
                 "\n  and l_shipdate < date '1994-01-01' + interval '1' year"
                 "\n  and l_discount between 0.06 - 0.01 and 0.06 + 0.01"
                 "\n  and l_quantity < 24;"
                 "\n"
                 "\n>>  123141078.2283"
              << std::endl;

    std::cout << "---------------------------------------------\n\n";
    return 0;
}
