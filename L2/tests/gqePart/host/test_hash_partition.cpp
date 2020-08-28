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
#include "xf_database/hash_lookup3.hpp"
#include "table_dt.hpp"
#include "utils.hpp"
#include "test_cfg.hpp"
#include "tpch_read_2.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <ap_int.h>

#define COL_NUM 8

const int HASHWH = 0;
const int HASHWL = 8;
const int PU = (1 << HASHWH);

#ifdef HLS_TEST
extern "C" void gqePart(const int k_depth,
                        const int col_index,
                        const int bit_num,
                        ap_uint<512> buf_A[],
                        ap_uint<512> buf_B[],
                        ap_uint<512> buf_D[]);
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

struct rlt_pair {
    std::string name;
    TPCH_INT nationkey;
    long long group_result;
};

typedef struct print_buf_result_data_ {
    int i;
    TPCH_INT* nrow;
} print_buf_result_data_t;

#ifdef HLS_TEST
void print_buf_result(void* user_data)
#else
void CL_CALLBACK print_buf_result(cl_event event, cl_int cmd_exec_status, void* user_data)
#endif // HLS_TEST
{
    print_buf_result_data_t* d = (print_buf_result_data_t*)user_data;

    // header
    int nm = *(d->nrow);
    printf("FPGA result of run %d:\n", d->i);
    printf("  %d\n", nm);
}

void compload(int* a, int* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) std::cout << i << " :  " << a[i] << " " << b[i] << std::endl;
    }
}

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

ap_uint<512> get_table_header(int hp_size, int blk_size, int nrow) {
    ap_uint<512> th = 0;
    th.range(31, 0) = nrow;
    th.range(63, 32) = blk_size;
    th.range(95, 64) = hp_size;
    return th;
}

int main(int argc, const char* argv[]) {
    std::cout << "\n------------ HASH PARTITION Beta Test with TPC-H (SF = 100) "
                 "-------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

#ifndef HLS_TEST
    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 0;
    }
#endif

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

    std::vector<std::string> tb2load; // table to load
    tb2load.push_back("Lineitem");

    int k_depth = 512;
    int l_nrow = L_MAX_ROW;

    std::cout << "Lineitem " << l_nrow << " rows\n";

    const size_t l_depth = L_MAX_ROW + VEC_LEN * 2 - 1;
    ap_uint<512>* table_l;
    size_t size_table_l;
    const size_t table_l_int_depth = size_t((sizeof(TPCH_INT) * l_depth + 64 - 1) / 64);

    int err;
    if (std::find(tb2load.begin(), tb2load.end(), "Lineitem") != tb2load.end()) {
        std::vector<std::string> column_names;
        column_names.push_back("l_orderkey");
        column_names.push_back("l_orderkey");
        column_names.push_back("l_suppkey");
        column_names.push_back("l_extendedprice");
        column_names.push_back("l_quantity");
        column_names.push_back("l_tax");
        column_names.push_back("l_discount");
        column_names.push_back("l_discount");
        size_table_l = (table_l_int_depth + 1) * COL_NUM; // 8 columns
        table_l = aligned_alloc<ap_uint<512> >(size_table_l);
        std::cout << "Malloced memory size for table_l in 512b: " << size_table_l << std::endl;

        for (int c = 0; c < COL_NUM; c++) {
            table_l[table_l_int_depth * c] = get_table_header(table_l_int_depth, l_nrow);
            err = generate_data<TPCH_INT>((int*)(table_l + table_l_int_depth * c + 1), 1000000, l_nrow);
            if (err) return err;
        }
        std::cout << "Lineitem table has been read from disk\n\n";
    }

    ap_uint<512>* table_cfg = aligned_alloc<ap_uint<512> >(9);
    get_q5simple_cfg(table_cfg);

    const int bit_num = 3;
    const int BK = 1 << bit_num;
    const int total_bucket = BK * PU;
    const float F = 2.0; // overflow size ratio

    const size_t result_depth = l_depth;
    const size_t table_result_depth = size_t((result_depth * sizeof(TPCH_INT) + 64 - 1) / 64); // base & overflow area
    const int BLOCK_SIZE = (F * table_result_depth + total_bucket - 1) / total_bucket;
    const size_t table_result_size =
        BLOCK_SIZE * COL_NUM * total_bucket + total_bucket; // 8 column, and heading area for each bucket
    const int PARTITION_SIZE = table_result_size / total_bucket;
    ap_uint<512>* table_out = aligned_alloc<ap_uint<512> >(table_result_size);
    table_out[0] = get_table_header(PARTITION_SIZE, BLOCK_SIZE, 0);

    std::cout << "Total size for each column in 512b: " << table_result_depth
              << "\nTotal malloced memory size in 512b: " << table_result_size << std::endl;
    std::cout << "BLOCK SIZE: " << BLOCK_SIZE << "\nPARTITION SIZE: " << PARTITION_SIZE << std::endl;
    std::cout << "Host map buffer has been allocated.\n\n";

#ifdef HLS_TEST
    hls::stream<ap_uint<64> > key_in;
    hls::stream<ap_uint<64> > key_out;
    gqePart(k_depth, 0, bit_num, (ap_uint<512>*)table_l, (ap_uint<512>*)table_out, (ap_uint<512>*)table_cfg);
    {
        std::cout << "------------------------HLS Csim Result "
                     "Checking-------------------------\n";
        for (int j = 0; j < PU; j++) {
            for (int i = 0; i < BK; i++) {
                int loc = j * BK + i;
                int prow = (int32_t)table_out[loc * PARTITION_SIZE];
                const int nread = (prow + 15) / 16;
                int abc = 0;
                ap_uint<HASHWH + HASHWL> golden = 0;
                for (int n = 0; n < nread; n++) {
                    const int len = (abc + VEC_LEN) > prow ? (prow - abc) : VEC_LEN;
                    // std::cout << "loc:" << loc << ",nread:" << nread << ",n:" << n
                    // << std::endl;
                    for (int m = 0; m < len; m++) {
                        ap_uint<64> k1 =
                            (table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE + 1](32 * (m + 1) - 1, 32 * m),
                             table_out[PARTITION_SIZE * loc + n + 1](32 * (m + 1) - 1, 32 * m));
                        key_in.write(k1);
                        xf::database::hashLookup3<64>(13, key_in, key_out);
                        ap_uint<64> k_g = key_out.read();
                        if ((n == 0) && (m == 0)) {
                            golden = k_g(bit_num - 1, 0);
                            // std::cout << std::hex << "Golden hash: " << golden <<
                            // std::endl;
                        }
                        if (golden != k_g(bit_num - 1, 0)) {
                            std::cout << std::hex << "golden:" << golden << ",bad:" << k_g << ",offset:" << n
                                      << ",vrow:" << m << std::endl;
                            return 1;
                        } else {
                            if (loc == 1) {
                                ap_uint<128> pdata =
                                    (table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE * 3 + 1](32 * (m + 1) - 1, 32 * m),
                                     table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE * 2 + 1](32 * (m + 1) - 1, 32 * m),
                                     table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE * 1 + 1](32 * (m + 1) - 1, 32 * m),
                                     table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE + 1](32 * (m + 1) - 1, 32 * m));
                            }
                        }
                    }
                    abc += len;
                }
                std::cout << "Hash partition " << loc << " is verified.\n";
            }
        }
    }
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

    // 2 for ping-pong, 4 for four joins.
    cl::Kernel kernel0table = cl::Kernel(program, "gqePart");

    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_l, mext_table_out, mext_cfg;

    mext_table_l = {3, table_l, kernel0table()};
    mext_table_out = {4, table_out, kernel0table()};
    mext_cfg = {5, table_cfg, kernel0table()};
    // mext_table_l = {XCL_BANK(33), table_l, 0};
    // mext_table_out = {XCL_BANK(32), table_out, 0};
    // mext_cfg = {XCL_BANK(32), table_cfg, 0};

    // Map buffers
    cl::Buffer buf_table_l(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                           (size_t)(sizeof(ap_uint<512>) * size_table_l), &mext_table_l);
    cl::Buffer buf_table_out(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             (size_t)(sizeof(ap_uint<512>) * table_result_size), &mext_table_out);
    cl::Buffer buf_cfg(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                       (size_t)(sizeof(ap_uint<512>) * 9), &mext_cfg);

    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events_l(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        write_events[i].resize(1);
        kernel_events_l[i].resize(1);
        read_events[i].resize(1);
    }

    std::vector<print_buf_result_data_t> cbd(num_rep);
    std::vector<print_buf_result_data_t>::iterator it = cbd.begin();
    print_buf_result_data_t* cbd_ptr = &(*it);

    std::vector<cl::Memory> ibtable;
    ibtable.push_back(buf_table_l);
    ibtable.push_back(buf_cfg);

    // set args and enqueue kernel
    int j = 0;
    kernel0table.setArg(j++, k_depth);
    kernel0table.setArg(j++, 0);
    kernel0table.setArg(j++, bit_num);
    kernel0table.setArg(j++, buf_table_l);
    kernel0table.setArg(j++, buf_table_out);
    kernel0table.setArg(j++, buf_cfg);

    struct timeval tv0;
    int exec_us;
    gettimeofday(&tv0, 0);
    for (int i = 0; i < num_rep; ++i) {
        // write data to DDR
        q.enqueueMigrateMemObjects(ibtable, 0, nullptr, &write_events[i][0]);

        // enqueue kernel
        q.enqueueTask(kernel0table, &write_events[i], &kernel_events_l[i][0]);

        // read data from DDR
        std::vector<cl::Memory> obtable;
        obtable.push_back(buf_table_out);

        q.enqueueMigrateMemObjects(obtable, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events_l[i], &read_events[i][0]);

        cbd_ptr[i].i = i;
        cbd_ptr[i].nrow = (TPCH_INT*)(table_out);
        read_events[i][0].setCallback(CL_COMPLETE, print_buf_result, cbd_ptr + i);
    }

    // wait all to finish.
    q.flush();
    q.finish();

    struct timeval tv3;
    gettimeofday(&tv3, 0);
    exec_us = tvdiff(&tv0, &tv3);
    std::cout << "FPGA execution time of " << num_rep << " runs: " << exec_us / 1000 << " ms\n"
              << "Average execution per run: " << exec_us / num_rep / 1000 << " ms\n";

    hls::stream<ap_uint<64> > key_in;
    hls::stream<ap_uint<64> > key_out;
    std::cout << "------------------------Result Checking-------------------------\n";
    for (int j = 0; j < PU; j++) {
        for (int i = 0; i < BK; i++) {
            int loc = j * BK + i;
            int prow = (int32_t)table_out[loc * PARTITION_SIZE];
            const int nread = (prow + 15) / 16;
            int abc = 0;
            ap_uint<HASHWH + HASHWL> golden = 0;
            for (int n = 0; n < nread; n++) {
                const int len = (abc + VEC_LEN) > prow ? (prow - abc) : VEC_LEN;
                for (int m = 0; m < len; m++) {
                    // ap_uint<64> k1 = (table_out[PARTITION_SIZE * loc + n + BLOCK_SIZE + 1](32 * (m + 1) - 1, 32 * m),
                    //                   table_out[PARTITION_SIZE * loc + n + 1](32 * (m + 1) - 1, 32 * m));
                    ap_uint<64> k1 =
                        (ap_uint<32>(0), table_out[PARTITION_SIZE * loc + n + 1](32 * (m + 1) - 1, 32 * m));
                    key_in.write(k1);
                    xf::database::hashLookup3<64>(13, key_in, key_out);
                    ap_uint<64> k_g = key_out.read();
                    if ((n == 0) && (m == 0)) {
                        golden = k_g(bit_num - 1, 0);
                    }
                    if (golden != k_g(bit_num - 1, 0)) {
                        std::cout << std::dec << "Partition: " << loc << std::hex << " <--> golden:" << golden
                                  << ",bad:" << k_g << std::endl;
                        return 1;
                    }
                }
                abc += len;
            }
            // std::cout << "Hash partition " << loc << " is verified.\n";
        }
    }
    std::cout << "All partitions are checked.\nSuccessfully";
#endif

    return 0;
}
