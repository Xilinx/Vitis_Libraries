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

#ifndef HLS_TEST
// OpenCL C API utils
#include "xclhost.hpp"
#endif

#include "xf_utils_sw/logger.hpp"

#include "x_utils.hpp"

// L1
#include "xf_database/hash_lookup3.hpp"
// GQE L2
#include "xf_database/meta_table.hpp"
#include "xf_database/kernel_command.hpp"
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
#include <thread>
#include <unistd.h>
#include <map>

// number of processing unit
const int PU_NM = 8;

#define BUILD_FACTOR 10

#ifdef HLS_TEST
extern "C" void gqeKernel(
    // input data columns
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col0,
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col1,
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col2,

    ap_uint<64>* din_val,

    // kernel config
    ap_uint<64> din_krn_cfg[14 * 512 / 64],

    // meta input buffer
    ap_uint<64> din_meta[24 * 512 / 64],
    // meta output buffer
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN> dout_meta[48],

    // output data columns
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* dout_col0,
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* dout_col1,
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* dout_col2,
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* dout_col3,
    // hbm buffers used to save build table key/payload
    ap_uint<256>* htb_buf0,
    ap_uint<256>* htb_buf1,
    ap_uint<256>* htb_buf2,
    ap_uint<256>* htb_buf3,
    ap_uint<256>* htb_buf4,
    ap_uint<256>* htb_buf5,
    ap_uint<256>* htb_buf6,
    ap_uint<256>* htb_buf7,

    // overflow buffers
    ap_uint<256>* stb_buf0,
    ap_uint<256>* stb_buf1,
    ap_uint<256>* stb_buf2,
    ap_uint<256>* stb_buf3,
    ap_uint<256>* stb_buf4,
    ap_uint<256>* stb_buf5,
    ap_uint<256>* stb_buf6,
    ap_uint<256>* stb_buf7);
#endif

inline int tvdiff(const timeval& tv0, const timeval& tv1) {
    return (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec);
}

inline int tvdiff(const timeval& tv0, const timeval& tv1, const char* name) {
    int exec_us = tvdiff(tv0, tv1);
    printf("%s: %d.%03d msec\n", name, (exec_us / 1000), (exec_us % 1000));
    return exec_us;
}

template <typename T>
int generate_data(T* data, int64_t range, size_t n) {
    if (!data) {
        return -1;
    }

    for (size_t i = 0; i < n; i++) {
        data[i] = (T)(rand() % range + 1);
    }

    return 0;
}

template <typename T>
int load_dat(void* data, const std::string& name, const std::string& dir, const int sf, const size_t n) {
    if (!data) {
        return -1;
    }
    std::string fn = dir + "/dat" + std::to_string(sf) + "/" + name + ".dat";
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

void gen_pass_fcfg(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_2
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_3
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_4
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);

    uint32_t r = 0;
    int sh = 0;
    // cond_1 -- cond_2
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;
    // cond_1 -- cond_3
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;
    // cond_1 -- cond_4
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;

    // cond_2 -- cond_3
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;
    // cond_2 -- cond_4
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;

    // cond_3 -- cond_4
    r |= ((uint32_t)(FOP_DC << sh));
    sh += FilterOpWidth;

    cfg[n++] = r;

    // 4 true and 6 true
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)(1UL << 31);
}

int main(int argc, const char* argv[]) {
    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

#ifndef HLS_TEST
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }
#endif

    // set up bloom-filter size
    std::string in_size;
    long bf_size = 256 * 1024;
#ifndef HLS_TEST
    if (parser.getCmdOption("-size", in_size)) {
        try {
            bf_size = std::stol(in_size);
        } catch (...) {
            // 256k-bit by default
            bf_size = 256 * 1024;
        }
    }
#endif
    if (bf_size % 4096 > 0) {
        bf_size = bf_size / 4096 * 4096 + ((bf_size % 4096) > 0) * 4096;
        std::cout << "Bloom filter hash table size should be 4096-bit aligned, automatically changed the size to "
                  << bf_size << "bits.\n";
    }
    if (bf_size > 16UL * 1024 * 1024 * 1024) {
        std::cout << "16Gbits is the current hardware limitation of bloom filter hash table size.\n";
        exit(1);
    }

    std::string scale;
    int sim_scale = 10000;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }

    int l_nrow = L_MAX_ROW / sim_scale;
    std::cout << "Lineitem " << l_nrow << " rows\n";

    int log_part = 3;
    if (parser.getCmdOption("-log_part", scale)) {
        try {
            log_part = std::stoi(scale);
        } catch (...) {
            log_part = 3;
        }
    }
    if (log_part < 3) {
        std::cout << "ERROR: partition number only supports >= 8 !!" << std::endl;
        return -1;
    }
    if (log_part > 8) {
        std::cout << "ERROR: partition number only supports <= 256 !!" << std::endl;
        return -1;
    }

    // --------- partitioning Table L ----------
    // partition setups
    const int tab_index = 1;
    const int partition_num = 1 << log_part;

    // the col nrow of each section
    int tab_part_sec_nrow_each = l_nrow;
    int tab_part_sec_size = tab_part_sec_nrow_each * TPCH_INT_SZ;

    x_utils::MM mm;
    int err = 0;

    // data load from disk. due to table size, data read into several sections
    // L host side pinned buffers for partition kernel
    TPCH_INT* tab_part_in_col[3];
    for (int i = 0; i < 3; i++) {
        tab_part_in_col[i] = mm.aligned_alloc<TPCH_INT>(tab_part_sec_nrow_each);
        if (i < 1) {
            err += generate_data<TPCH_INT>((TPCH_INT*)(tab_part_in_col[i]), 1000, l_nrow);
        } else {
            for (int tt = 0; tt < l_nrow; tt++) {
                tab_part_in_col[i][tt] = tt;
            }
        }
    }
    if (err) {
        fprintf(stderr, "ERROR: failed to gen data.\n");
        return 1;
    }
    ap_uint<64>* valid_in_col = mm.aligned_alloc<ap_uint<64> >((tab_part_sec_nrow_each + 63) / 64);
    for (int i = 0; i < (l_nrow + 63) / 64; i++) {
        // valid_in_col[i] = 0xffffffffffffffff;
        valid_in_col[i] = 0x5555555555555555;
    }
    for (int i = 0; i < 6; i++) {
        std::cout << "key = " << tab_part_in_col[0][i] << std::endl;
    }

    // HBM storage allocations
    // total size split to 8 individual HBM pseudo channels with 256-bit for each block
    int htb_buf_depth = bf_size / 8 / 256; // HT_BUFF_DEPTH;
    int stb_buf_depth = bf_size / 8 / 256; // HT_BUFF_DEPTH;
    // htb buff holds lower space of the hash table of the bloom filter
    int htb_buf_size = sizeof(ap_uint<256>) * htb_buf_depth / 2;
    // stb buff holds higher space of the hash table of the bloom filter
    int stb_buf_size = sizeof(ap_uint<256>) * stb_buf_depth / 2;
    uint64_t* htb_buf[PU_NM];
    uint64_t* stb_buf[PU_NM];
    for (int i = 0; i < PU_NM; i++) {
        htb_buf[i] = (uint64_t*)mm.aligned_alloc<uint64_t>(htb_buf_depth / (sizeof(uint64_t) * 8) * 256);
        stb_buf[i] = (uint64_t*)mm.aligned_alloc<uint64_t>(stb_buf_depth / (sizeof(uint64_t) * 8) * 256);
    }

    std::cout << "finished dat loading/generating" << std::endl;

    // partition output data
    int tab_part_out_col_nrow_256_init = tab_part_sec_nrow_each * 2 / VEC_LEN;
    assert(tab_part_out_col_nrow_256_init > 0 && "Error: table output col size must > 0");
    // the depth of each partition in each col
    int tab_part_out_col_eachpart_nrow_256 = (tab_part_out_col_nrow_256_init + partition_num - 1) / partition_num;
    std::cout << "tab_part_out_col_eachpart_nrow_256: " << tab_part_out_col_eachpart_nrow_256 << std::endl;
    // total output data nrow, aligned by 256
    int tab_part_out_col_nrow_256 = partition_num * tab_part_out_col_eachpart_nrow_256;
    int tab_part_out_col_size = tab_part_out_col_nrow_256 * TPCH_INT_SZ * VEC_LEN;

    // partition_output data
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* tab_part_out_col_b[4];
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* tab_part_out_col_p[4];
    for (int i = 0; i < 4; i++) {
        tab_part_out_col_b[i] = mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(tab_part_out_col_nrow_256);
        tab_part_out_col_p[i] = mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(tab_part_out_col_nrow_256);
    }

    // kernel command object
    xf::database::gqe::KernelCommand krn_cmd_b;
    xf::database::gqe::KernelCommand krn_cmd_p;
    // for enabling input channels
    std::vector<int8_t> enable_A;
    enable_A.push_back(0);
    enable_A.push_back(-1);
    enable_A.push_back(-1);
    // table_id, index
    krn_cmd_b.setScanColEnable(tab_index, enable_A);
    krn_cmd_p.setScanColEnable(tab_index, enable_A);
    std::vector<int8_t> enable_out;
    enable_out.push_back(0);
    enable_out.push_back(1);
    enable_out.push_back(2);
    // krn_id, table_id, index
    krn_cmd_b.setWriteColEnable(1, tab_index, enable_out);
    krn_cmd_p.setWriteColEnable(1, tab_index, enable_out);
    // table_id, gen_rowID_en, valid_en
    krn_cmd_b.setRowIDValidEnable(tab_index, 1, 1);
    krn_cmd_p.setRowIDValidEnable(tab_index, 1, 1);
    // part on
    krn_cmd_b.setPartOn(1);
    krn_cmd_p.setPartOn(1);
    // set log_part
    krn_cmd_b.setLogPart(log_part);
    krn_cmd_p.setLogPart(log_part);
    // set build probe flag
    krn_cmd_b.setJoinBuildProbe(tab_index);
    krn_cmd_p.setJoinBuildProbe(tab_index);

    // for enableing bloom-filter
    krn_cmd_b.setBloomfilterOn(1);
    krn_cmd_p.setBloomfilterOn(1);
    // set bloom filter size
    krn_cmd_b.setBloomfilterSize(bf_size);
    krn_cmd_p.setBloomfilterSize(bf_size);
    // set bloom filter to build mode
    krn_cmd_b.setBloomfilterBuildProbe(0);
    krn_cmd_p.setBloomfilterBuildProbe(1);

    // no dynamic filtering by default, put the condition into the second paramter of setFilter for setting up the
    // filter
    // krn_cmd_b.setFilter(0, "c > 0");
    // krn_cmd_p.setFilter(0, "c > 0");

    //--------------- metabuffer setup L -----------------
    xf::database::gqe::MetaTable meta_part_in_b;
    xf::database::gqe::MetaTable meta_part_in_p;
    xf::database::gqe::MetaTable meta_part_out_b;
    xf::database::gqe::MetaTable meta_part_out_p;
    meta_part_in_b.setSecID(0);
    meta_part_in_p.setSecID(0);
    meta_part_in_b.setColNum(1);
    meta_part_in_p.setColNum(1);
    meta_part_in_b.setCol(0, 0, l_nrow / BUILD_FACTOR);
    meta_part_in_p.setCol(0, 0, l_nrow);
    meta_part_in_b.setCol(1, 1, l_nrow / BUILD_FACTOR);
    meta_part_in_p.setCol(1, 1, l_nrow);
    meta_part_in_b.setCol(2, 2, l_nrow / BUILD_FACTOR);
    meta_part_in_p.setCol(2, 2, l_nrow);
    meta_part_in_b.meta();
    meta_part_in_p.meta();

    // setup partition kernel used meta output
    meta_part_out_b.setColNum(2);
    meta_part_out_p.setColNum(2);
    meta_part_out_b.setPartition(partition_num, tab_part_out_col_eachpart_nrow_256);
    meta_part_out_p.setPartition(partition_num, tab_part_out_col_eachpart_nrow_256);
    meta_part_out_b.setCol(0, 0, tab_part_out_col_nrow_256);
    meta_part_out_p.setCol(0, 0, tab_part_out_col_nrow_256);
    meta_part_out_b.setCol(1, 1, tab_part_out_col_nrow_256);
    meta_part_out_p.setCol(1, 1, tab_part_out_col_nrow_256);
    meta_part_out_b.setCol(2, 2, tab_part_out_col_nrow_256);
    meta_part_out_p.setCol(2, 2, tab_part_out_col_nrow_256);
    meta_part_out_b.meta();
    meta_part_out_p.meta();

    int nerror = 0;
    using namespace xf::common::utils_sw;
    Logger logger(std::cout, std::cerr);

#ifdef HLS_TEST

    std::cout << std::endl << "build + partition starts................." << std::endl;
    gqeKernel((ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[0],
              (ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[1],
              (ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[2], (ap_uint<64>*)valid_in_col,
              (ap_uint<64>*)krn_cmd_b.getConfigBits(), (ap_uint<64>*)meta_part_in_b.meta(),
              (ap_uint<8 * TPCH_INT_SZ * VEC_LEN>*)meta_part_out_b.meta(), tab_part_out_col_b[0], tab_part_out_col_b[1],
              tab_part_out_col_b[2], tab_part_out_col_b[3], (ap_uint<256>*)htb_buf[0], (ap_uint<256>*)htb_buf[1],
              (ap_uint<256>*)htb_buf[2], (ap_uint<256>*)htb_buf[3], (ap_uint<256>*)htb_buf[4],
              (ap_uint<256>*)htb_buf[5], (ap_uint<256>*)htb_buf[6], (ap_uint<256>*)htb_buf[7],
              (ap_uint<256>*)stb_buf[0], (ap_uint<256>*)stb_buf[1], (ap_uint<256>*)stb_buf[2],
              (ap_uint<256>*)stb_buf[3], (ap_uint<256>*)stb_buf[4], (ap_uint<256>*)stb_buf[5],
              (ap_uint<256>*)stb_buf[6], (ap_uint<256>*)stb_buf[7]);
    std::cout << "build + partition ends................." << std::endl;

    std::cout << std::endl << "probe + partition starts................." << std::endl;
    gqeKernel((ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[0],
              (ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[1],
              (ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>*)tab_part_in_col[2], (ap_uint<64>*)valid_in_col,
              (ap_uint<64>*)krn_cmd_p.getConfigBits(), (ap_uint<64>*)meta_part_in_p.meta(),
              (ap_uint<8 * TPCH_INT_SZ * VEC_LEN>*)meta_part_out_p.meta(), tab_part_out_col_p[0], tab_part_out_col_p[1],
              tab_part_out_col_p[2], tab_part_out_col_p[3], (ap_uint<256>*)htb_buf[0], (ap_uint<256>*)htb_buf[1],
              (ap_uint<256>*)htb_buf[2], (ap_uint<256>*)htb_buf[3], (ap_uint<256>*)htb_buf[4],
              (ap_uint<256>*)htb_buf[5], (ap_uint<256>*)htb_buf[6], (ap_uint<256>*)htb_buf[7],
              (ap_uint<256>*)stb_buf[0], (ap_uint<256>*)stb_buf[1], (ap_uint<256>*)stb_buf[2],
              (ap_uint<256>*)stb_buf[3], (ap_uint<256>*)stb_buf[4], (ap_uint<256>*)stb_buf[5],
              (ap_uint<256>*)stb_buf[6], (ap_uint<256>*)stb_buf[7]);
    std::cout << "probe + partition ends................." << std::endl;

#else

    // setup OpenCL related
    cl_context ctx;
    cl_device_id dev_id;
    cl_command_queue cmq;
    cl_program prg;
    err += xclhost::init_hardware(&ctx, &dev_id, &cmq,
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
    // partition kernel
    cl_kernel bkernel;
    bkernel = clCreateKernel(prg, "gqeKernel", &err);
    // will not exit with failure by default
    logger.logCreateKernel(err);
    cl_kernel pkernel;
    pkernel = clCreateKernel(prg, "gqeKernel", &err);
    // will not exit with failure by default
    logger.logCreateKernel(err);

    cl_mem_ext_ptr_t mext_tab_part_in_col[3];
    for (int i = 0; i < 3; ++i) {
        mext_tab_part_in_col[i] = {(0 + i), tab_part_in_col[i], bkernel};
        mext_tab_part_in_col[i] = {(0 + i), tab_part_in_col[i], pkernel};
    }

    cl_mem_ext_ptr_t mext_tab_part_out_col_b[4];
    cl_mem_ext_ptr_t mext_tab_part_out_col_p[4];
    for (int i = 0; i < 4; ++i) {
        mext_tab_part_out_col_b[i] = {(7 + i), tab_part_out_col_b[i], bkernel};
        mext_tab_part_out_col_p[i] = {(7 + i), tab_part_out_col_p[i], pkernel};
    }

    cl_mem_ext_ptr_t mext_valid_in_col;
    mext_valid_in_col = {3, valid_in_col, bkernel};
    mext_valid_in_col = {3, valid_in_col, pkernel};

    cl_mem_ext_ptr_t mext_cfg_part_b = {4, krn_cmd_b.getConfigBits(), bkernel};
    cl_mem_ext_ptr_t mext_cfg_part_p = {4, krn_cmd_p.getConfigBits(), pkernel};

    cl_mem_ext_ptr_t memExt[PU_NM * 2];
    memExt[0] = {11, htb_buf[0], bkernel};
    memExt[1] = {12, htb_buf[1], bkernel};
    memExt[2] = {13, htb_buf[2], bkernel};
    memExt[3] = {14, htb_buf[3], bkernel};
    memExt[4] = {15, htb_buf[4], bkernel};
    memExt[5] = {16, htb_buf[5], bkernel};
    memExt[6] = {17, htb_buf[6], bkernel};
    memExt[7] = {18, htb_buf[7], bkernel};
    memExt[8] = {19, stb_buf[0], bkernel};
    memExt[9] = {20, stb_buf[1], bkernel};
    memExt[10] = {21, stb_buf[2], bkernel};
    memExt[11] = {22, stb_buf[3], bkernel};
    memExt[12] = {23, stb_buf[4], bkernel};
    memExt[13] = {24, stb_buf[5], bkernel};
    memExt[14] = {25, stb_buf[6], bkernel};
    memExt[15] = {26, stb_buf[7], bkernel};
    memExt[0] = {11, htb_buf[0], pkernel};
    memExt[1] = {12, htb_buf[1], pkernel};
    memExt[2] = {13, htb_buf[2], pkernel};
    memExt[3] = {14, htb_buf[3], pkernel};
    memExt[4] = {15, htb_buf[4], pkernel};
    memExt[5] = {16, htb_buf[5], pkernel};
    memExt[6] = {17, htb_buf[6], pkernel};
    memExt[7] = {18, htb_buf[7], pkernel};
    memExt[8] = {19, stb_buf[0], pkernel};
    memExt[9] = {20, stb_buf[1], pkernel};
    memExt[10] = {21, stb_buf[2], pkernel};
    memExt[11] = {22, stb_buf[3], pkernel};
    memExt[12] = {23, stb_buf[4], pkernel};
    memExt[13] = {24, stb_buf[5], pkernel};
    memExt[14] = {25, stb_buf[6], pkernel};
    memExt[15] = {26, stb_buf[7], pkernel};

    // dev buffers, part in
    cl_mem buf_tab_part_in_col[3];
    for (int c = 0; c < 3; c++) {
        buf_tab_part_in_col[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                tab_part_sec_size, &mext_tab_part_in_col[c], &err);
    }

    cl_mem buf_valid_in_col = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                             (l_nrow + 63) / 64 * sizeof(ap_uint<64>), &mext_valid_in_col, &err);

    // dev buffers, part out
    std::cout << "Input device buffer has been created" << std::endl;
    cl_mem buf_tab_part_out_col_b[4];
    cl_mem buf_tab_part_out_col_p[4];
    for (int c = 0; c < 4; c++) {
        buf_tab_part_out_col_b[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                   tab_part_out_col_size, &mext_tab_part_out_col_b[c], &err);
        buf_tab_part_out_col_p[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                   tab_part_out_col_size, &mext_tab_part_out_col_p[c], &err);
    }
    std::cout << "Output device buffer has been created" << std::endl;

    cl_mem buf_cfg_part_b = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           (sizeof(ap_uint<512>) * 14), &mext_cfg_part_b, &err);
    cl_mem buf_cfg_part_p = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           (sizeof(ap_uint<512>) * 14), &mext_cfg_part_p, &err);

    // htb stb buffers
    cl_mem buf_tmp[PU_NM * 2];
    for (int j = 0; j < PU_NM; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    (size_t)(htb_buf_size), &memExt[j], &err);
    }
    for (int j = PU_NM; j < 2 * PU_NM; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    (size_t)(stb_buf_size), &memExt[j], &err);
    }

    cl_mem_ext_ptr_t mext_meta_part_in_b, mext_meta_part_out_b;
    cl_mem_ext_ptr_t mext_meta_part_in_p, mext_meta_part_out_p;

    mext_meta_part_in_b = {5, meta_part_in_b.meta(), bkernel};
    mext_meta_part_in_p = {5, meta_part_in_p.meta(), pkernel};
    mext_meta_part_out_b = {6, meta_part_out_b.meta(), bkernel};
    mext_meta_part_out_p = {6, meta_part_out_p.meta(), pkernel};

    cl_mem buf_meta_part_in_b;
    buf_meta_part_in_b = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                        (sizeof(ap_uint<512>) * 8), &mext_meta_part_in_b, &err);
    cl_mem buf_meta_part_in_p;
    buf_meta_part_in_p = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                        (sizeof(ap_uint<512>) * 8), &mext_meta_part_in_p, &err);
    cl_mem buf_meta_part_out_b;
    buf_meta_part_out_b = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         (sizeof(ap_uint<512>) * 24), &mext_meta_part_out_b, &err);
    cl_mem buf_meta_part_out_p;
    buf_meta_part_out_p = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         (sizeof(ap_uint<512>) * 24), &mext_meta_part_out_p, &err);

    //----------------------partition L run-----------------------------
    std::cout << "------------------- Partitioning L table -----------------" << std::endl;
    // set up build kernel args
    int j = 0;
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_valid_in_col);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_cfg_part_b);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_part_in_b);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_part_out_b);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_b[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_b[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_b[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_b[3]);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }
    // set up probe kernel args
    j = 0;
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[0]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[1]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[2]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_valid_in_col);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_cfg_part_p);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_meta_part_in_p);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_meta_part_out_p);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_p[0]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_p[1]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_p[2]);
    clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col_p[3]);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(pkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }

    // partition h2d
    std::vector<cl_mem> part_in_vec_b;
    part_in_vec_b.push_back(buf_tab_part_in_col[0]);
    part_in_vec_b.push_back(buf_tab_part_in_col[1]);
    part_in_vec_b.push_back(buf_tab_part_in_col[2]);
    part_in_vec_b.push_back(buf_valid_in_col);
    part_in_vec_b.push_back(buf_meta_part_in_b);
    part_in_vec_b.push_back(buf_meta_part_out_b);
    part_in_vec_b.push_back(buf_cfg_part_b);

    std::vector<cl_mem> part_in_vec_p;
    part_in_vec_p.push_back(buf_tab_part_in_col[0]);
    part_in_vec_p.push_back(buf_tab_part_in_col[1]);
    part_in_vec_p.push_back(buf_tab_part_in_col[2]);
    part_in_vec_p.push_back(buf_valid_in_col);
    part_in_vec_p.push_back(buf_meta_part_in_p);
    part_in_vec_p.push_back(buf_meta_part_out_p);
    part_in_vec_p.push_back(buf_cfg_part_p);

    // partition d2h
    std::vector<cl_mem> part_out_vec_b;
    part_out_vec_b.push_back(buf_tab_part_out_col_b[0]);
    part_out_vec_b.push_back(buf_tab_part_out_col_b[1]);
    part_out_vec_b.push_back(buf_tab_part_out_col_b[2]);
    part_out_vec_b.push_back(buf_tab_part_out_col_b[3]);
    part_out_vec_b.push_back(buf_meta_part_out_b);

    std::vector<cl_mem> part_out_vec_p;
    part_out_vec_p.push_back(buf_tab_part_out_col_p[0]);
    part_out_vec_p.push_back(buf_tab_part_out_col_p[1]);
    part_out_vec_p.push_back(buf_tab_part_out_col_p[2]);
    part_out_vec_p.push_back(buf_tab_part_out_col_p[3]);
    part_out_vec_p.push_back(buf_meta_part_out_p);

    cl_event evt_part_h2d, evt_part_krn, evt_part_d2h;

    timeval tv_part_start, tv_part_end;
    timeval tv1, tv2;

    // bloom filter build + partition flow
    clEnqueueMigrateMemObjects(cmq, part_in_vec_b.size(), part_in_vec_b.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, part_out_vec_b.size(), part_out_vec_b.data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    gettimeofday(&tv_part_start, 0);
    clEnqueueMigrateMemObjects(cmq, part_in_vec_b.size(), part_in_vec_b.data(), 0, 0, nullptr, &evt_part_h2d);
    clFinish(cmq);
    gettimeofday(&tv1, 0);
    tvdiff(tv_part_start, tv1, "h2d");
    std::cout << "build h2d done " << std::endl;
    clEnqueueTask(cmq, bkernel, 1, &evt_part_h2d, &evt_part_krn);
    clFinish(cmq);
    gettimeofday(&tv2, 0);
    tvdiff(tv1, tv2, "krn: ");
    std::cout << "build krn done " << std::endl;
    clEnqueueMigrateMemObjects(cmq, part_out_vec_b.size(), part_out_vec_b.data(), 1, 1, &evt_part_krn, &evt_part_d2h);
    clFinish(cmq);
    gettimeofday(&tv1, 0);
    tvdiff(tv2, tv1, "d2h: ");
    std::cout << "build d2h done " << std::endl;

    clFlush(cmq);
    clFinish(cmq);
    gettimeofday(&tv_part_end, 0);

    // bloom filter probe + partition flow
    clEnqueueMigrateMemObjects(cmq, part_in_vec_p.size(), part_in_vec_p.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, part_out_vec_p.size(), part_out_vec_p.data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    gettimeofday(&tv_part_start, 0);
    clEnqueueMigrateMemObjects(cmq, part_in_vec_p.size(), part_in_vec_p.data(), 0, 0, nullptr, &evt_part_h2d);
    clFinish(cmq);
    gettimeofday(&tv1, 0);
    tvdiff(tv_part_start, tv1, "h2d");
    std::cout << "probe h2d done " << std::endl;
    clEnqueueTask(cmq, pkernel, 1, &evt_part_h2d, &evt_part_krn);
    clFinish(cmq);
    gettimeofday(&tv2, 0);
    tvdiff(tv1, tv2, "krn: ");
    std::cout << "probe krn done " << std::endl;
    clEnqueueMigrateMemObjects(cmq, part_out_vec_p.size(), part_out_vec_p.data(), 1, 1, &evt_part_krn, &evt_part_d2h);
    clFinish(cmq);
    gettimeofday(&tv1, 0);
    tvdiff(tv2, tv1, "d2h: ");
    std::cout << "probe d2h done " << std::endl;

#endif

    std::cout << "Checking result...." << std::endl;

    // get number of rows from each partition
    std::map<int64_t, int64_t> key_part_map_b;
    std::map<int64_t, int64_t> key_part_map_p;
    // save filtered key/payload to std::unordered_map for checking
    std::unordered_map<int64_t, int64_t> filtered_pairs;
    int* nrow_per_part_b = meta_part_out_b.getPartLen();
    int* nrow_per_part_p = meta_part_out_p.getPartLen();

    std::cout << "Checking BF build + PART stage...\n";
    for (int i = 0; i < partition_num; i++) {
        int offset = tab_part_out_col_eachpart_nrow_256 * i;
        int prow = nrow_per_part_b[i];
        std::cout << "Partition " << i << " nrow: " << prow << std::endl;

        if (tab_part_out_col_eachpart_nrow_256 * VEC_LEN < prow) {
            std::cout << "ERROR: the output buffer nrow for each partition: "
                      << (tab_part_out_col_eachpart_nrow_256 * VEC_LEN);
            std::cout << " is smaller than the resulting nrow: " << prow << std::endl;
            exit(1);
        }

        std::cout << "       Key0         Key1      Row-ID\n";
        const int nread = (prow + VEC_LEN - 1) / VEC_LEN;
        int abc = 0;
        for (int n = 0; n < nread; n++) {
            const int len = (abc + VEC_LEN) > prow ? (prow - abc) : VEC_LEN;
            for (int m = 0; m < len; m++) {
                ap_uint<64> key = tab_part_out_col_b[0][offset + n](m * 64 + 63, m * 64);
                ap_uint<64> key2 = tab_part_out_col_b[1][offset + n](m * 64 + 63, m * 64);
                ap_uint<64> rowid = tab_part_out_col_b[2][offset + n](m * 64 + 63, m * 64);
                // pre-check whether same key are in the same partition
                if (key_part_map_b.find(key) != key_part_map_b.end()) {
                    // to make sure every key in each partition is orthogonal to those in the different partitions
                    if (i != key_part_map_b[key]) {
                        std::cout << "Find Error, Error key is " << key << std::endl;
                        nerror++;
                    }
                } else {
                    // new key found
                    key_part_map_b.insert(std::make_pair(key, i));
                }
            }
            abc += len;
        }
    }

    std::cout << "Checking BF probe + PART stage...\n";
    for (int i = 0; i < partition_num; i++) {
        int offset = tab_part_out_col_eachpart_nrow_256 * i;
        int prow = nrow_per_part_p[i];
        std::cout << "Partition " << i << " nrow: " << prow << std::endl;

        if (tab_part_out_col_eachpart_nrow_256 * VEC_LEN < prow) {
            std::cout << "ERROR: the output buffer nrow for each partition: "
                      << (tab_part_out_col_eachpart_nrow_256 * VEC_LEN);
            std::cout << " is smaller than the resulting nrow: " << prow << std::endl;
            exit(1);
        }

        std::cout << "       Key0         Key1      Row-ID\n";
        const int nread = (prow + VEC_LEN) / VEC_LEN;
        int abc = 0;
        for (int n = 0; n < nread; n++) {
            const int len = (abc + VEC_LEN) > prow ? (prow - abc) : VEC_LEN;
            for (int m = 0; m < len; m++) {
                ap_uint<64> key = tab_part_out_col_p[0][offset + n](m * 64 + 63, m * 64);
                ap_uint<64> key2 = tab_part_out_col_p[1][offset + n](m * 64 + 63, m * 64);
                ap_uint<64> rowid = tab_part_out_col_p[2][offset + n](m * 64 + 63, m * 64);
                // insert key/pld paris for bloom filter verification
                filtered_pairs.insert(std::make_pair<int64_t, int64_t>((int64_t)key, (int64_t)rowid));
                // std::cout << std::setw(10) << (int64_t)key << ", ";
                // std::cout << std::setw(10) << (int64_t)key2 << ", ";
                // std::cout << std::setw(10) << (int64_t)rowid << std::endl;
                // pre-check whether same key are in the same partition
                if (key_part_map_p.find(key) != key_part_map_p.end()) {
                    // to make sure every key in each partition is orthogonal to those in the different partitions
                    if (i != key_part_map_p[key]) {
                        std::cout << "Find Error, Error key is " << key << std::endl;
                        nerror++;
                    }
                } else {
                    // new key found
                    key_part_map_p.insert(std::make_pair(key, i));
                }
            }
            abc += len;
        }
    }

    // test each added key in the filtered key list for bloom filter
    for (int i = 0; i < l_nrow / BUILD_FACTOR; i++) {
        // valid input row
        if (valid_in_col[i / 64][i % 64]) {
            std::unordered_map<int64_t, int64_t>::const_iterator got =
                filtered_pairs.find((int64_t)tab_part_in_col[0][i]);
            if (got == filtered_pairs.end()) {
                nerror++;
                std::cout << "Missing key = " << tab_part_in_col[0][i] << " in bloom-filter." << std::endl;
            }
        }
    }
    (nerror > 0) ? logger.error(Logger::Message::TEST_FAIL) : logger.info(Logger::Message::TEST_PASS);

    return nerror;
}
