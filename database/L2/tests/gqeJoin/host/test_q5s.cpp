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

// OpenCL C API utils
#include "xclhost.hpp"
#include "x_utils.hpp"
// GQE L2
#include "xf_database/meta_table.hpp"
#include "xf_database/join_command.hpp"
// HLS
#include "ap_int.h"

#include "table_dt.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdio>

const int PU_NM = 8;
const int VEC_SCAN = 8; // 256-bit column.

inline int tvdiff(const timeval& tv0, const timeval& tv1) {
    return (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec);
}

inline int tvdiff(const timeval& tv0, const timeval& tv1, const char* name) {
    int exec_us = tvdiff(tv0, tv1);
    printf("%s: %d.%03d msec\n", name, (exec_us / 1000), (exec_us % 1000));
    return exec_us;
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
            // std::cout << p << ", " << d << std::endl;
            sum += (p * (100 - d));
            ++cnt;
        }
    }

    std::cout << "INFO: CPU ref matched " << cnt << " rows, sum = " << sum << std::endl;
    return sum;
}

class MM {
   private:
    size_t _total;
    std::vector<void*> _pvec;

   public:
    MM() : _total(0) {}
    ~MM() {
        for (void* p : _pvec) {
            if (p) free(p);
        }
    }
    size_t size() const { return _total; }
    template <typename T>
    T* aligned_alloc(std::size_t num) {
        void* ptr = nullptr;
        size_t sz = num * sizeof(T);
        if (posix_memalign(&ptr, 4096, sz)) throw std::bad_alloc();
        _pvec.push_back(ptr);
        _total += sz;
        printf("align_alloc %lu Bytes\n", sz);
        return reinterpret_cast<T*>(ptr);
    }
};

int main(int argc, const char* argv[]) {
    std::cout << "\n--------- TPC-H Query 5 Simplified (1G) ---------\n";

    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

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

    // default to have

    int l_nrow = L_MAX_ROW / sim_scale;
    int o_nrow = O_MAX_ROW / sim_scale;

    std::cout << "Lineitem " << l_nrow << " rows\n"
              << "Orders " << o_nrow << " rows\n";

    MM mm;

    // Number of vec in input buf. Add some extra and align.
    size_t table_l_depth = (l_nrow + VEC_SCAN * 9 - 1) / VEC_SCAN;
    size_t table_o_depth = (o_nrow + VEC_SCAN * 9 - 1) / VEC_SCAN;

    size_t table_l_size = table_l_depth * VEC_SCAN * TPCH_INT_SZ;
    size_t table_o_size = table_o_depth * VEC_SCAN * TPCH_INT_SZ;

    // data load from disk. will re-use in each call, but assumed to be different.
    TPCH_INT* table_o_user_0 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_o_depth);
    TPCH_INT* table_o_user_1 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_o_depth);

    TPCH_INT* table_l_user_0 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);
    TPCH_INT* table_l_user_1 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);
    TPCH_INT* table_l_user_2 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);

    int error = 0;
    error += generate_data<TPCH_INT>((int*)(table_o_user_0), 10000000, o_nrow);
    error += generate_data<TPCH_INT>((int*)(table_o_user_1), 10000000, o_nrow);
    if (error) return error;
    std::cout << "Orders table has been read from disk\n";

    error += generate_data<TPCH_INT>((int*)(table_l_user_0), 10000000, l_nrow);
    error += generate_data<TPCH_INT>((int*)(table_l_user_1), 10000000, l_nrow);
    error += generate_data<TPCH_INT>((int*)(table_l_user_2), 10000000, l_nrow);
    if (error) return error;
    std::cout << "Lineitem table has been read from disk\n";

    long long golden = get_golden_sum(l_nrow, (ap_uint<32>*)(table_l_user_0), (ap_uint<32>*)(table_l_user_1),
                                      (ap_uint<32>*)(table_l_user_2), o_nrow, (ap_uint<32>*)(table_o_user_0),
                                      (ap_uint<32>*)(table_o_user_1));

    // host buffer to be mapped with device buffer through OpenCL

    TPCH_INT* table_l_0[2] = {0};
    TPCH_INT* table_l_1[2] = {0};
    TPCH_INT* table_l_2[2] = {0};
    TPCH_INT* table_l_3[2] = {0};
    TPCH_INT* table_l_4[2] = {0};
    TPCH_INT* table_l_5[2] = {0};
    TPCH_INT* table_l_6[2] = {0};
    TPCH_INT* table_l_7[2] = {0};

    TPCH_INT* table_o_0;
    TPCH_INT* table_o_1;
    TPCH_INT* table_o_2;
    TPCH_INT* table_o_3;
    TPCH_INT* table_o_4;
    TPCH_INT* table_o_5;
    TPCH_INT* table_o_6;
    TPCH_INT* table_o_7;

    for (int i = 0; i < 2; i++) {
        table_l_0[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);
        table_l_1[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);
        table_l_2[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_l_depth);
        table_l_3[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
        table_l_4[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
        table_l_5[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
        table_l_6[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
        table_l_7[i] = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    }

    table_o_0 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_o_depth);
    table_o_1 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN * table_o_depth);
    table_o_2 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    table_o_3 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    table_o_4 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    table_o_5 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    table_o_6 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);
    table_o_7 = mm.aligned_alloc<TPCH_INT>(VEC_SCAN);

    size_t result_nrow = (1 << 20);

    // Num of vecs.
    size_t table_result_depth = result_nrow / VEC_LEN; // 8 columns in one buffer
    size_t table_result_size = table_result_depth * VEC_LEN * TPCH_INT_SZ;

    ap_uint<512>* table_out_0[2];
    ap_uint<512>* table_out_1[2];
    ap_uint<512>* table_out_2[2];
    ap_uint<512>* table_out_3[2];
    ap_uint<512>* table_out_4[2];
    ap_uint<512>* table_out_5[2];
    ap_uint<512>* table_out_6[2];
    ap_uint<512>* table_out_7[2];
    for (int i = 0; i < 2; i++) {
        table_out_0[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_1[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_2[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_3[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_4[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_5[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_6[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_7[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
    }
    std::vector<ap_uint<512>*> table_out_user_0(num_rep);
    std::vector<ap_uint<512>*> table_out_user_1(num_rep);
    std::vector<ap_uint<512>*> table_out_user_2(num_rep);
    std::vector<ap_uint<512>*> table_out_user_3(num_rep);
    std::vector<ap_uint<512>*> table_out_user_4(num_rep);
    std::vector<ap_uint<512>*> table_out_user_5(num_rep);
    std::vector<ap_uint<512>*> table_out_user_6(num_rep);
    std::vector<ap_uint<512>*> table_out_user_7(num_rep);
    size_t result_size = sizeof(ap_uint<512>) * table_result_depth;
    for (int i = 0; i < num_rep; i++) {
        table_out_user_0[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_1[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_2[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_3[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_4[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_5[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_6[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        table_out_user_7[i] = mm.aligned_alloc<ap_uint<512> >(table_result_depth);
        // XXX ensure the out buffer is allocated
        timeval t0, t1;
        gettimeofday(&t0, 0);
        memset(table_out_user_0[i], 0, result_size);
        memset(table_out_user_1[i], 0, result_size);
        memset(table_out_user_2[i], 0, result_size);
        memset(table_out_user_3[i], 0, result_size);
        memset(table_out_user_4[i], 0, result_size);
        memset(table_out_user_5[i], 0, result_size);
        memset(table_out_user_6[i], 0, result_size);
        memset(table_out_user_7[i], 0, result_size);
        gettimeofday(&t1, 0);
        tvdiff(t0, t1, "memset user output");
    }

    // one config is enough
    using jcmdclass = xf::database::gqe::JoinCommand;
    jcmdclass jcmd = jcmdclass();

    jcmd.setJoinType(xf::database::INNER_JOIN);
    jcmd.Scan(0, {0, 1});
    jcmd.Scan(1, {0, 1, 2});
    jcmd.setWriteCol({0, 1, 2, 3});

    // jcmd.setEvaluation(0, "strm1*(-strm2+c2)", {0, 100});
    // jcmd.setFilter(0, "19940101<=b && b<19950101");

    jcmd.setShuffle0(0, {0, 1});
    jcmd.setShuffle0(1, {0, 1, 2});

    jcmd.setShuffle1(0, {0, 1});    // setJoinKeys
    jcmd.setShuffle1(1, {0, 1, 2}); // setJoinKeys

    // jcmd.setShuffle2({0, 1});
    // jcmd.setShuffle3({8});
    // jcmd.setShuffle4({0});
    jcmd.setShuffle2({0, 1, 6, 12});
    jcmd.setShuffle3({0, 1, 2, 3});
    jcmd.setShuffle4({0, 1, 2, 3});

    ap_uint<512>* table_cfg5s = jcmd.getConfigBits();
    std::cout << "Host map buffer has been allocated.\n";

    std::cout << "Total aligned alloc size: " << mm.size() << std::endl;

    //--------------- metabuffer setup -----------------
    // using col0 and col1 buffer during build
    // setup build used meta input
    xf::database::gqe::MetaTable meta_build_in;
    meta_build_in.setColNum(2);
    meta_build_in.setCol(0, 0, o_nrow);
    meta_build_in.setCol(1, 1, o_nrow);

    // setup probe used meta input
    xf::database::gqe::MetaTable meta_probe_in;
    meta_probe_in.setColNum(3);
    meta_probe_in.setCol(0, 0, l_nrow);
    meta_probe_in.setCol(1, 1, l_nrow);
    meta_probe_in.setCol(2, 2, l_nrow);

    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3. (When aggr is off)
    // when aggr is on, actually only using col0 is enough.
    // below example only illustrates the output buffers can be shuffled.
    // setup probe used meta output
    xf::database::gqe::MetaTable meta_probe_out;
    meta_probe_out.setColNum(4);
    meta_probe_out.setCol(0, 0, result_nrow);
    meta_probe_out.setCol(1, 1, result_nrow);
    meta_probe_out.setCol(2, 2, result_nrow);
    meta_probe_out.setCol(3, 3, result_nrow);
    //--------------------------------------------
    //
    size_t build_probe_flag_0 = 0;
    size_t build_probe_flag_1 = 1;

    // Get CL devices.
    cl_int err;
    cl_context ctx;
    cl_device_id dev_id;
    cl_command_queue cmq;
    cl_program prg;

    err = xclhost::init_hardware(&ctx, &dev_id, &cmq,
                                 CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, MSTR(XDEVICE));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to init OpenCL with " MSTR(XDEVICE) "\n");
        return err;
    }

    err = xclhost::load_binary(&prg, ctx, dev_id, xclbin_path.c_str());
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to program PL\n");
        return err;
    }

    // build kernel
    cl_kernel bkernel;
    bkernel = clCreateKernel(prg, "gqeJoin", &err);
    // probe kernel, pipeline used handle
    cl_kernel jkernel[2];
    for (int i = 0; i < 2; i++) {
        jkernel[i] = clCreateKernel(prg, "gqeJoin", &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to create kernel.\n");
            return err;
        }
    }
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_o[8], mext_table_l[2][8], mext_cfg5s;
    cl_mem_ext_ptr_t mext_table_out_0[2], mext_table_out_1[2], mext_table_out_2[2], mext_table_out_3[2];
    cl_mem_ext_ptr_t mext_table_out_4[2], mext_table_out_5[2], mext_table_out_6[2], mext_table_out_7[2];
    cl_mem_ext_ptr_t memExt[2][PU_NM * 2];

    mext_table_o[0] = {0, table_o_0, bkernel};
    mext_table_o[1] = {1, table_o_1, bkernel};
    mext_table_o[2] = {2, table_o_2, bkernel};
    mext_table_o[3] = {3, table_o_3, bkernel};
    mext_table_o[4] = {4, table_o_4, bkernel};
    mext_table_o[5] = {5, table_o_5, bkernel};
    mext_table_o[6] = {6, table_o_6, bkernel};
    mext_table_o[7] = {7, table_o_7, bkernel};
    mext_cfg5s = {19, table_cfg5s, bkernel};

    for (int i = 0; i < 2; i++) {
        mext_table_l[i][0] = {0, table_l_0[i], jkernel[i]};
        mext_table_l[i][1] = {1, table_l_1[i], jkernel[i]};
        mext_table_l[i][2] = {2, table_l_2[i], jkernel[i]};
        mext_table_l[i][3] = {3, table_l_3[i], jkernel[i]};
        mext_table_l[i][4] = {4, table_l_4[i], jkernel[i]};
        mext_table_l[i][5] = {5, table_l_5[i], jkernel[i]};
        mext_table_l[i][6] = {6, table_l_6[i], jkernel[i]};
        mext_table_l[i][7] = {7, table_l_7[i], jkernel[i]};

        mext_table_out_0[i] = {11, table_out_0[i], jkernel[i]};
        mext_table_out_1[i] = {12, table_out_1[i], jkernel[i]};
        mext_table_out_2[i] = {13, table_out_2[i], jkernel[i]};
        mext_table_out_3[i] = {14, table_out_3[i], jkernel[i]};
        mext_table_out_4[i] = {15, table_out_4[i], jkernel[i]};
        mext_table_out_5[i] = {16, table_out_5[i], jkernel[i]};
        mext_table_out_6[i] = {17, table_out_6[i], jkernel[i]};
        mext_table_out_7[i] = {18, table_out_7[i], jkernel[i]};
        for (int j = 0; j < 16; j++) {
            memExt[i][j] = {20 + j, NULL, jkernel[i]};
        }
    }

    cl_mem_ext_ptr_t mext_meta_build_in, mext_meta_probe_in, mext_meta_probe_out;

    mext_meta_build_in = {9, meta_build_in.meta(), bkernel};
    mext_meta_probe_in = {9, meta_probe_in.meta(), jkernel[0]};
    mext_meta_probe_out = {10, meta_probe_out.meta(), jkernel[0]};

    // Map buffers
    cl_mem buf_table_o_0;
    cl_mem buf_table_o_1;
    cl_mem buf_table_o_2;
    cl_mem buf_table_o_3;
    cl_mem buf_table_o_4;
    cl_mem buf_table_o_5;
    cl_mem buf_table_o_6;
    cl_mem buf_table_o_7;
    cl_mem buf_table_l_0[2];
    cl_mem buf_table_l_1[2];
    cl_mem buf_table_l_2[2];
    cl_mem buf_table_l_3[2];
    cl_mem buf_table_l_4[2];
    cl_mem buf_table_l_5[2];
    cl_mem buf_table_l_6[2];
    cl_mem buf_table_l_7[2];
    cl_mem buf_table_out_0[2];
    cl_mem buf_table_out_1[2];
    cl_mem buf_table_out_2[2];
    cl_mem buf_table_out_3[2];
    cl_mem buf_table_out_4[2];
    cl_mem buf_table_out_5[2];
    cl_mem buf_table_out_6[2];
    cl_mem buf_table_out_7[2];
    cl_mem buf_cfg5s;

    buf_table_o_0 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, table_o_size,
                                   &mext_table_o[0], &err);
    buf_table_o_1 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, table_o_size,
                                   &mext_table_o[1], &err);
    buf_table_o_2 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[2], &err);
    buf_table_o_3 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[3], &err);
    buf_table_o_4 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[4], &err);
    buf_table_o_5 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[5], &err);
    buf_table_o_6 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[6], &err);
    buf_table_o_7 = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   (TPCH_INT_SZ * VEC_SCAN), &mext_table_o[7], &err);
    for (int i = 0; i < 2; i++) {
        buf_table_l_0[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          table_l_size, &mext_table_l[i][0], &err);
        buf_table_l_1[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          table_l_size, &mext_table_l[i][1], &err);
        buf_table_l_2[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          table_l_size, &mext_table_l[i][2], &err);
        buf_table_l_3[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (TPCH_INT_SZ * VEC_SCAN), &mext_table_l[i][3], &err);
        buf_table_l_4[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (TPCH_INT_SZ * VEC_SCAN), &mext_table_l[i][4], &err);
        buf_table_l_5[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (TPCH_INT_SZ * VEC_SCAN), &mext_table_l[i][5], &err);
        buf_table_l_6[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (TPCH_INT_SZ * VEC_SCAN), &mext_table_l[i][6], &err);
        buf_table_l_7[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (TPCH_INT_SZ * VEC_SCAN), &mext_table_l[i][7], &err);

        buf_table_out_0[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_0[i], &err);
        buf_table_out_1[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_1[i], &err);
        buf_table_out_2[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_2[i], &err);
        buf_table_out_3[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_3[i], &err);
        buf_table_out_4[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_4[i], &err);
        buf_table_out_5[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_5[i], &err);
        buf_table_out_6[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_6[i], &err);
        buf_table_out_7[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            table_result_size, &mext_table_out_7[i], &err);
    }

    buf_cfg5s = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                               (sizeof(ap_uint<512>) * 9), &mext_cfg5s, &err);
    // htb stb buffers
    cl_mem buf_tmp[PU_NM * 2];
    for (int j = 0; j < PU_NM; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(sizeof(ap_uint<64>) * HT_BUFF_DEPTH / 2), &memExt[0][j], &err);
    }
    for (int j = PU_NM; j < PU_NM * 2; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(KEY_SZ * 2 * S_BUFF_DEPTH / 2), &memExt[0][j], &err);
    }
    cl_mem buf_meta_build_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (sizeof(ap_uint<512>) * 8), &mext_meta_build_in, &err);

    cl_mem buf_meta_probe_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (sizeof(ap_uint<512>) * 8), &mext_meta_probe_in, &err);
    cl_mem buf_meta_probe_out = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                               (sizeof(ap_uint<512>) * 8), &mext_meta_probe_out, &err);

    std::cout << "buffers have been mapped.\n";

    // helper buffer sets
    std::vector<cl_mem> non_loop_bufs;
    non_loop_bufs.push_back(buf_table_o_0);
    non_loop_bufs.push_back(buf_table_o_1);
    non_loop_bufs.push_back(buf_table_o_2);
    non_loop_bufs.push_back(buf_table_o_3);
    non_loop_bufs.push_back(buf_table_o_4);
    non_loop_bufs.push_back(buf_table_o_5);
    non_loop_bufs.push_back(buf_table_o_6);
    non_loop_bufs.push_back(buf_table_o_7);
    non_loop_bufs.push_back(buf_cfg5s);
    non_loop_bufs.push_back(buf_meta_build_in);
    non_loop_bufs.push_back(buf_meta_probe_out);

    std::vector<cl_mem> loop_in_bufs[2];
    for (int k = 0; k < 2; k++) {
        loop_in_bufs[k].push_back(buf_table_l_0[k]);
        loop_in_bufs[k].push_back(buf_table_l_1[k]);
        loop_in_bufs[k].push_back(buf_table_l_2[k]);
        loop_in_bufs[k].push_back(buf_table_l_3[k]);
        loop_in_bufs[k].push_back(buf_table_l_4[k]);
        loop_in_bufs[k].push_back(buf_table_l_5[k]);
        loop_in_bufs[k].push_back(buf_table_l_6[k]);
        loop_in_bufs[k].push_back(buf_table_l_7[k]);
        loop_in_bufs[k].push_back(buf_meta_probe_in);
    }

    std::vector<cl_mem> loop_out_bufs[2];
    for (int k = 0; k < 2; k++) {
        loop_out_bufs[k].push_back(buf_table_out_0[k]);
        loop_out_bufs[k].push_back(buf_table_out_1[k]);
        loop_out_bufs[k].push_back(buf_table_out_2[k]);
        loop_out_bufs[k].push_back(buf_table_out_3[k]);
        loop_out_bufs[k].push_back(buf_table_out_4[k]);
        loop_out_bufs[k].push_back(buf_table_out_5[k]);
        loop_out_bufs[k].push_back(buf_table_out_6[k]);
        loop_out_bufs[k].push_back(buf_table_out_7[k]);
        loop_out_bufs[k].push_back(buf_meta_probe_out);
    }

    clEnqueueMigrateMemObjects(cmq, loop_in_bufs[0].size(), loop_in_bufs[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, loop_in_bufs[1].size(), loop_in_bufs[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, loop_out_bufs[0].size(), loop_out_bufs[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, loop_out_bufs[1].size(), loop_out_bufs[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    // set args and enqueue kernel
    int j = 0;
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_0);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_1);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_2);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_3);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_4);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_5);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_6);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_7);
    clSetKernelArg(bkernel, j++, sizeof(size_t), &build_probe_flag_0);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_build_in);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_probe_out);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_0[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_1[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_2[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_3[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_4[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_5[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_6[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out_7[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }

    // set args and enqueue kernel
    for (int i = 0; i < 2; i++) {
        int j = 0;
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_0[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_1[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_2[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_3[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_4[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_5[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_6[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_l_7[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(size_t), &build_probe_flag_1);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_meta_probe_in);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_meta_probe_out);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_0[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_1[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_2[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_3[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_4[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_5[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_6[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_table_out_7[i]);
        clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_cfg5s);
        for (int k = 0; k < PU_NM * 2; k++) {
            clSetKernelArg(jkernel[i], j++, sizeof(cl_mem), &buf_tmp[k]);
        }
    }

    timeval tr0, tr1, tr2, tr3, tr4, tr5;

    int k_id = 0;
    std::array<cl_event, 1> evt_tb_o;
    std::array<cl_event, 1> evt_bkrn;
    std::array<cl_event, 1> evt_tb_l;
    std::array<cl_event, 1> evt_pkrn;
    std::array<cl_event, 1> evt_tb_out;

    // 1) copy Order table from host DDR to build kernel pinned host buffer
    gettimeofday(&tr0, 0);
    memcpy(table_o_0, table_o_user_0, sizeof(TPCH_INT) * VEC_SCAN * table_o_depth);
    memcpy(table_o_1, table_o_user_1, sizeof(TPCH_INT) * VEC_SCAN * table_o_depth);
    gettimeofday(&tr1, 0);

    // 2) migrate order table data from host buffer to device buffer
    clEnqueueMigrateMemObjects(cmq, non_loop_bufs.size(), non_loop_bufs.data(), 0, 0, nullptr, &evt_tb_o[0]);

    // 3) launch build kernel
    clEnqueueTask(cmq, bkernel, 1, evt_tb_o.data(), &evt_bkrn[0]);
    clWaitForEvents(1, &evt_bkrn[0]);

    // 4) copy L table from host DDR to build kernel pinned host buffer
    gettimeofday(&tr2, 0);
    memcpy(table_l_0[k_id], table_l_user_0, sizeof(TPCH_INT) * VEC_SCAN * table_l_depth);
    memcpy(table_l_1[k_id], table_l_user_1, sizeof(TPCH_INT) * VEC_SCAN * table_l_depth);
    memcpy(table_l_2[k_id], table_l_user_2, sizeof(TPCH_INT) * VEC_SCAN * table_l_depth);
    gettimeofday(&tr3, 0);

    // 5) migrate L table data from host buffer to device buffer
    clEnqueueMigrateMemObjects(cmq, loop_in_bufs[k_id].size(), loop_in_bufs[k_id].data(), 0, 0, nullptr, &evt_tb_l[0]);

    // 6) launch probe kernel
    clEnqueueTask(cmq, jkernel[k_id], 1, evt_tb_l.data(), &evt_pkrn[0]);

    // 7) migrate result data from device buffer to host buffer
    clEnqueueMigrateMemObjects(cmq, loop_out_bufs[k_id].size(), loop_out_bufs[k_id].data(), CL_MIGRATE_MEM_OBJECT_HOST,
                               1, evt_pkrn.data(), &evt_tb_out[0]);

    // 8) copy output data from pinned host buffer to user host buffer
    clWaitForEvents(1, &evt_tb_out[0]);
    gettimeofday(&tr4, 0);
    memcpy(table_out_user_0[0], table_out_0[k_id], result_size);
    memcpy(table_out_user_1[0], table_out_1[k_id], result_size);
    memcpy(table_out_user_2[0], table_out_2[k_id], result_size);
    memcpy(table_out_user_3[0], table_out_3[k_id], result_size);
    memcpy(table_out_user_4[0], table_out_4[k_id], result_size);
    memcpy(table_out_user_5[0], table_out_5[k_id], result_size);
    memcpy(table_out_user_6[0], table_out_6[k_id], result_size);
    memcpy(table_out_user_7[0], table_out_7[k_id], result_size);
    gettimeofday(&tr5, 0);

    // 9) calc and print the execution time of each phase
    cl_ulong start, end;
    long ev_ns;

    std::cout << std::endl << "============== execution time ==================" << std::endl;
    // 9.0) memcpy O
    tvdiff(tr0, tr1, "memcpy O table time");
    // 9.1) migrate O
    clGetEventProfilingInfo(evt_tb_o[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_o[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate O: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.2) build kernel
    clGetEventProfilingInfo(evt_bkrn[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_bkrn[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("build kernel: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.3) memcpy L
    tvdiff(tr2, tr3, "memcpy L table time");
    // 9.4) migrate L
    clGetEventProfilingInfo(evt_tb_l[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_l[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate L: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.5) probe kernel
    clGetEventProfilingInfo(evt_pkrn[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_pkrn[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("probe kernel: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.6) migreate output
    clGetEventProfilingInfo(evt_tb_out[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_out[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate output: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.7) memcpy output
    tvdiff(tr4, tr5, "memcpy Output time");

    // =========== print result ===========
    printf("\n");
    // check the probe updated meta
    int out_nrow = meta_probe_out.getColLen();
    std::cout << "Output buffer has " << out_nrow << " rows." << std::endl;
    std::cout << "---------------------------------Checking result---------------------------------" << std::endl;
    long long sum = 0;
    int p_nrow = out_nrow;
    for (int n = 0; n < p_nrow / 16; n++) {
        for (int i = 0; i < 16; i++) {
            uint32_t extendedprice = table_out_user_0[0][n](31 + 32 * i, 32 * i);
            uint32_t discount = table_out_user_1[0][n](31 + 32 * i, 32 * i);
            sum += extendedprice * (100 - discount);
            // std::cout << extendedprice << ", " << discount << std::endl;
        }
    }
    for (int n = 0; n < p_nrow % 16; n++) {
        uint32_t extendedprice = table_out_user_0[0][p_nrow / 16](31 + 32 * n, 32 * n);
        uint32_t discount = table_out_user_1[0][p_nrow / 16](31 + 32 * n, 32 * n);
        sum += extendedprice * (100 - discount);
        // std::cout << extendedprice << ", " << discount << std::endl;
    }
    std::cout << "Golen value: " << golden << ", FPGA value: " << sum << std::endl;
    if (sum == golden) {
        std::cout << "Validate Pass" << std::endl;
    } else {
        std::cout << "Validate Failed" << std::endl;
        return 1;
    }
    //--------------release---------------
    clReleaseMemObject(buf_table_o_0);
    clReleaseMemObject(buf_table_o_1);
    clReleaseMemObject(buf_table_o_2);
    clReleaseMemObject(buf_table_o_3);
    clReleaseMemObject(buf_table_o_4);
    clReleaseMemObject(buf_table_o_5);
    clReleaseMemObject(buf_table_o_6);
    clReleaseMemObject(buf_table_o_7);
    for (int k = 0; k < 2; k++) {
        clReleaseMemObject(buf_table_l_0[k]);
        clReleaseMemObject(buf_table_l_1[k]);
        clReleaseMemObject(buf_table_l_2[k]);
        clReleaseMemObject(buf_table_l_3[k]);
        clReleaseMemObject(buf_table_l_4[k]);
        clReleaseMemObject(buf_table_l_5[k]);
        clReleaseMemObject(buf_table_l_6[k]);
        clReleaseMemObject(buf_table_l_7[k]);

        clReleaseMemObject(buf_table_out_0[k]);
        clReleaseMemObject(buf_table_out_1[k]);
        clReleaseMemObject(buf_table_out_2[k]);
        clReleaseMemObject(buf_table_out_3[k]);
        clReleaseMemObject(buf_table_out_4[k]);
        clReleaseMemObject(buf_table_out_5[k]);
        clReleaseMemObject(buf_table_out_6[k]);
        clReleaseMemObject(buf_table_out_7[k]);
    }
    clReleaseMemObject(buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clReleaseMemObject(buf_tmp[k]);
    }
    clReleaseMemObject(buf_meta_build_in);
    clReleaseMemObject(buf_meta_probe_in);
    clReleaseMemObject(buf_meta_probe_out);

    clReleaseEvent(evt_tb_o[0]);
    clReleaseEvent(evt_bkrn[0]);
    clReleaseEvent(evt_tb_l[0]);
    clReleaseEvent(evt_pkrn[0]);
    clReleaseEvent(evt_tb_out[0]);

    clReleaseKernel(bkernel);
    clReleaseKernel(jkernel[0]);
    clReleaseKernel(jkernel[1]);

    err = clReleaseProgram(prg);
    if (err != CL_SUCCESS) {
        printf("ERROR: fail to release program. \n");
    }

    clReleaseCommandQueue(cmq);
    clReleaseContext(ctx);

    return 0;
}
