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

#ifndef HLS_TEST
// OpenCL C API utils
#include "xclhost.hpp"
#endif

#include "xf_utils_sw/logger.hpp"

#include "x_utils.hpp"

// for hash-lookup3 in L1
#include "xf_database/hash_lookup3.hpp"

// GQE common
#include "xf_database/meta_table.hpp"
#include "xf_database/kernel_command.hpp"
#include "xf_database/enums.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <unordered_map>

#include <ap_int.h>

// number of processing unit
const int PU_NM = 8;
// build l_nrow / BUILD_FACTOR into bloom-filter
const int BUILD_FACTOR = 10;

#define HASH_WIDTH_LOW 61
#define HASH_WIDTH_HIGH 3

// number of hash functions in bloom-filter
#define NUM_HASH_FUNCS 4

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))));

#ifndef __SYNTHESIS__
extern "C" void gqeKernel(
    // input data columns
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col0,
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col1,
    ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* din_col2,

    // validation buffer
    ap_uint<64>* din_val,

    // kernel config
    ap_uint<64> din_krn_cfg[14 * (512 / 64)],

    // meta input buffer
    ap_uint<64> din_meta[24 * (512 / 64)],
    // meta output buffer
    ap_uint<8 * TPCH_INT_SZ * VEC_LEN> dout_meta[48],

    //  output data columns
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

// int to 256 bits
void int_to_vec(int nrow, int64_t* col_in, ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* col_out) {
    int t = 0;
    for (int i = 0; i < nrow; i = i + VEC_SCAN) {
        ap_uint<8 * TPCH_INT_SZ* VEC_SCAN> tmp = 0;
        for (int j = 0; j < VEC_SCAN; ++j) {
            tmp.range(64 * j + 63, 64 * j) = col_in[i + j];
        }
        col_out[t] = tmp;
        t++;
    }
}

#endif

inline int tvdiff(const timeval& tv0, const timeval& tv1) {
    return (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec);
}

inline int tvdiff(const timeval& tv0, const timeval& tv1, const char* name) {
    int exec_us = tvdiff(tv0, tv1);
    printf("%s: %d.%03d msec\n", name, (exec_us / 1000), (exec_us % 1000));
    return exec_us;
}

// load one col data into 1 buffer
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

// hash helper for building bloom-filter
ap_uint<64> hashlookup3_64(ap_uint<64> key0_val, ap_uint<64> key1_val) {
    const int key96blen = 128 / 96;

    // key8blen is the BYTE len of the key.
    const int key8blen = 128 / 8;
    const ap_uint<32> c1 = 0xdeadbeef;

    // use magic word(seed) to initial the output
    ap_uint<64> hash1 = 1032032634;
    ap_uint<64> hash2 = 2818135537;

    ap_uint<32> a = c1 + ((ap_uint<32>)key8blen) + ((ap_uint<32>)hash1);
    ap_uint<32> b = c1 + ((ap_uint<32>)key8blen) + ((ap_uint<32>)hash1);
    ap_uint<32> c = c1 + ((ap_uint<32>)key8blen) + ((ap_uint<32>)hash1);
    c += (ap_uint<32>)hash2;

    a += (ap_uint<32>)(key0_val & 0x00000000ffffffffUL);
    b += (ap_uint<32>)(key0_val >> 32);
    c += (ap_uint<32>)(key1_val & 0x00000000ffffffffUL);

    a -= c;
    a ^= rot(c, 4);
    c += b;

    b -= a;
    b ^= rot(a, 6);
    a += c;

    c -= b;
    c ^= rot(b, 8);
    b += a;

    a -= c;
    a ^= rot(c, 16);
    c += b;

    b -= a;
    b ^= rot(a, 19);
    a += c;

    c -= b;
    c ^= rot(b, 4);
    b += a;

    a += (ap_uint<32>)(key1_val >> 32);

    // finalization
    c ^= b;
    c -= rot(b, 14);

    a ^= c;
    a -= rot(c, 11);

    b ^= a;
    b -= rot(a, 25);

    c ^= b;
    c -= rot(b, 16);

    a ^= c;
    a -= rot(c, 4);

    b ^= a;
    b -= rot(a, 14);

    c ^= b;
    c -= rot(b, 24);

    hash1 = (ap_uint<64>)c;
    hash2 = (ap_uint<64>)b;

    ap_uint<64> hash_val = hash1 << 32 | hash2;

    return hash_val;
}

/* filter (19940101<= col:1 < 19950101) */
static void gen_q5simple_orders_fcfg(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);

    // 19940101 <= cond_1 < 19950101
    uint64_t dat1 = (uint64_t)19940101UL;
    uint64_t dat2 = (uint64_t)19950101UL;
    // uint64_t dat1 = (uint64_t)700;
    // uint64_t dat2 = (uint64_t)800;
    cfg[n++] = dat1 & 0xffffffff;
    cfg[n++] = dat1 >> 32;
    cfg[n++] = dat2 & 0xffffffff;
    cfg[n++] = dat2 >> 32;
    cfg[n++] = 0UL | (FOP_GEU << FilterOpWidth) | (FOP_LTU);
    // cond_3
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_4
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
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
    std::cout << "\n--------- TPC-H Query 5 Simplified (1G) ---------\n";

    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

#ifndef HLS_TEST
    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin
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
    int factor_o = 1;
    int factor_l = 1;
#ifndef HLS_TEST
    if (parser.getCmdOption("-O", scale)) {
        try {
            factor_o = std::stoi(scale);
        } catch (...) {
            factor_o = 1;
        }
    }
    if (parser.getCmdOption("-L", scale)) {
        try {
            factor_l = std::stoi(scale);
        } catch (...) {
            factor_l = 1;
        }
    }
#endif

    int32_t table_o_nrow = 1500000 * factor_o;
    // int32_t table_o_nrow = 400 * factor_o;
    int32_t table_l_nrow;
    switch (factor_l) {
        case 1:
            table_l_nrow = 6001215;
            // table_l_nrow = 100000;
            break;
        case 2:
            table_l_nrow = 11997941;
            break;
        case 4:
            table_l_nrow = 23996604;
            break;
        case 8:
            table_l_nrow = 47989007;
            break;
        case 10:
            table_l_nrow = 59986052;
            break;
        case 12:
            table_l_nrow = 71985077;
            break;
        case 20:
            table_l_nrow = 119994608;
            break;
        case 30:
            table_l_nrow = 179998372;
            break;
        case 32:
            table_l_nrow = 192000000;
            break;
        case 33:
            table_l_nrow = 198000000;
            break;
        case 34:
            table_l_nrow = 204000000;
            break;
        case 35:
            table_l_nrow = 210000000;
            break;
        case 36:
            table_l_nrow = 216000000;
            break;
        case 37:
            table_l_nrow = 222000000;
            break;
        case 38:
            table_l_nrow = 228000000;
            break;
        case 39:
            table_l_nrow = 234000000;
            break;
        case 40:
            table_l_nrow = 240012290;
            break;
        case 60:
            table_l_nrow = 360011594;
            break;
        case 80:
            table_l_nrow = 480025129;
            break;
        case 100:
            table_l_nrow = 600037902;
            break;
        case 150:
            table_l_nrow = 900035147;
            break;
        case 200:
            table_l_nrow = 1200018434;
            break;
        case 250:
            table_l_nrow = 1500000714;
            break;
        default:
            table_l_nrow = 6001215;
            std::cerr << "L factor not supported, using SF1" << std::endl;
    }
    if (factor_l > 30 && factor_l < 40) {
        factor_l = 40;
    }

    int sim_scale = 1000;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }
    table_o_nrow /= sim_scale;
    table_l_nrow /= sim_scale;
    int o_nrow = table_o_nrow;
    int l_nrow = table_l_nrow;

    std::cout << "Orders SF(" << factor_o << ")\t" << table_o_nrow << " rows\n"
              << "Lineitem SF(" << factor_l << ")\t" << table_l_nrow << " rows\n";

    x_utils::MM mm;

    // Number of vec in input buf. Add some extra and align.
    size_t table_l_depth = (l_nrow + VEC_SCAN - 1) / VEC_SCAN;

    size_t table_l_size = table_l_depth * VEC_SCAN * TPCH_INT_SZ;

    int32_t* table_l_in_0 = mm.aligned_alloc<int32_t>(table_l_depth * VEC_SCAN);
    int32_t* table_l_in_1 = mm.aligned_alloc<int32_t>(table_l_depth * VEC_SCAN);
    int32_t* table_l_in_2 = mm.aligned_alloc<int32_t>(table_l_depth * VEC_SCAN);

    // key column
    TPCH_INT* table_l_user_0 = mm.aligned_alloc<TPCH_INT>(table_l_depth * VEC_SCAN);
    // price column
    TPCH_INT* table_l_user_1 = mm.aligned_alloc<TPCH_INT>(table_l_depth * VEC_SCAN);
    // discount column
    TPCH_INT* table_l_user_2 = mm.aligned_alloc<TPCH_INT>(table_l_depth * VEC_SCAN);

    for (int i = 0; i < l_nrow; i++) {
        table_l_in_0[i] = i;
        table_l_in_1[i] = i;
        table_l_in_2[i] = i;
    }
    std::cout << "LineItem table has been generated" << std::endl;

    for (int i = 0; i < l_nrow; ++i) {
        table_l_user_0[i] = table_l_in_0[i];
        table_l_user_1[i] = table_l_in_1[i];
        table_l_user_2[i] = table_l_in_2[i];
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

    // build bloom-filter, using multi-hash algorithm build in Hive (number of hash functions is 4, default cache line
    // size is 512-bit)
    for (int i = 0; i < l_nrow / BUILD_FACTOR; i++) {
        ap_uint<32> seed = 0xdeadbeef;
        hls::stream<ap_uint<128> > key_strm;
        ap_uint<128> key = 0;
        key.range(63, 0) = table_l_user_0[i];
        key_strm.write(key);
        hls::stream<ap_uint<64> > hash_strm;
        xf::database::hashLookup3(seed, key_strm, hash_strm);
        uint64_t hash = hash_strm.read();
        uint64_t idx = hash >> HASH_WIDTH_LOW;
        hash = hash & 0x1FFFFFFFFFFFFFFFUL;
        // first hash is used to locate start of the block
        uint32_t hash1 = (uint32_t)hash;
        uint32_t hash2 = (uint32_t)(hash >> 32);
        uint32_t firstHash = hash1 + hash2;
        uint32_t totalBlockCountEach = bf_size / 64 / 8;
        uint32_t blockIdx = firstHash % totalBlockCountEach;
        uint32_t blockBaseOffset = blockIdx & 0xFFFFFFF8;
        // subsequent K hashes are used to generate K bits within a cache line (4 blocks)
        for (int i = 1; i <= NUM_HASH_FUNCS; i++) {
            uint32_t combinedHash = hash1 + ((i + 1) * hash2);
            uint32_t absOffset = blockBaseOffset + (combinedHash & 0x00000007);
            int bitPos = (combinedHash >> 3) & 63;
            htb_buf[idx][absOffset] |= ((uint64_t)1 << bitPos);
        }
    }
    std::cout << "Bloom-filter created and built" << std::endl;
    // copy higher space of htb_buf to stb_buf
    uint32_t totalBlockCountEach = bf_size / 64 / 16;
    for (int i = 0; i < PU_NM; i++) {
        for (int j = 0; j < totalBlockCountEach; j++) {
            stb_buf[i][j] = htb_buf[i][j + totalBlockCountEach];
        }
    }

    ap_uint<64>* din_val = mm.aligned_alloc<ap_uint<64> >((l_nrow + 63) / 64);
    for (int i = 0; i < (l_nrow + 63) / 64; i++) {
        din_val[i] = 0x5555555555555555UL;
    }

    // result buff
    size_t result_nrow = l_nrow;
    size_t table_result_depth =
        (result_nrow / (8 * TPCH_INT_SZ * VEC_LEN) + ((result_nrow % (8 * TPCH_INT_SZ * VEC_LEN)) > 0));
    size_t table_result_size = table_result_depth * VEC_LEN * TPCH_INT_SZ;
    ap_uint<8 * TPCH_INT_SZ* VEC_LEN>* table_out_0 =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(table_result_depth);
    ap_uint<8 * TPCH_INT_SZ* VEC_LEN>* table_out_1 =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(table_result_depth);
    ap_uint<8 * TPCH_INT_SZ* VEC_LEN>* table_out_2 =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(table_result_depth);
    ap_uint<8 * TPCH_INT_SZ* VEC_LEN>* table_out_3 =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> >(table_result_depth);
    memset(table_out_0, 0, table_result_size);
    memset(table_out_1, 0, table_result_size);
    memset(table_out_2, 0, table_result_size);
    memset(table_out_3, 0, table_result_size);
    std::cout << "Result buffer set" << std::endl;

    // 2 for gqeFilter
    const int krn_id = 2;
    // 1 for big table
    const int table_id = 1;

    // kernel command object
    xf::database::gqe::KernelCommand krn_cmd;

    // for enabling input channels
    std::vector<int8_t> ch_in;
    ch_in.push_back(0);
    ch_in.push_back(1);
    ch_in.push_back(2);
    krn_cmd.setScanColEnable(table_id, ch_in);

    // for enableing bloom-filter
    krn_cmd.setBloomfilterOn(1);
    // set bloom filter size
    krn_cmd.setBloomfilterSize(bf_size);
    // set bloom filter to probe mode
    krn_cmd.setBloomfilterBuildProbe(1);

    // table_id, gen_rowID_en, validEn,
    krn_cmd.setRowIDValidEnable(1, 1, 1);

    // for enabling output channels
    std::vector<int8_t> ch_out;
    ch_out.push_back(0);
    ch_out.push_back(1);
    ch_out.push_back(2);
    ch_out.push_back(3);
    krn_cmd.setWriteColEnable(krn_id, table_id, ch_out);

    // no dynamic filtering by default, put the condition into the second paramter of setFilter for setting up the
    // filter
    // krn_cmd.setFilter(1, "c > 0");

    // set build probe flag
    // 0 for build, 1 for probe
    krn_cmd.setJoinBuildProbe(1);

    //--------------- metabuffer setup -----------------
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

    std::cout << "Kernel config & Meta set" << std::endl;

    using namespace xf::common::utils_sw;
    Logger logger(std::cout, std::cerr);

#ifdef HLS_TEST
    ap_uint<8 * TPCH_INT_SZ* VEC_SCAN>* table_l_0_new =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> >(table_l_depth);
    ap_uint<8 * TPCH_INT_SZ* VEC_SCAN>* table_l_1_new =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> >(table_l_depth);
    ap_uint<8 * TPCH_INT_SZ* VEC_SCAN>* table_l_2_new =
        mm.aligned_alloc<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> >(table_l_depth);

    std::cout << "Table B col0: " << std::endl;
    int_to_vec(l_nrow, table_l_user_0, table_l_0_new);
    std::cout << "Table B col1: " << std::endl;
    int_to_vec(l_nrow, table_l_user_1, table_l_1_new);
    std::cout << "Table B col2: " << std::endl;
    int_to_vec(l_nrow, table_l_user_2, table_l_2_new);
    /*
    // bloom-filter build
    std::cout << std::endl << "build starts................." << std::endl;
    krn_cmd.setBloomfilterBuildProbe(0);
    meta_probe_in.setCol(0, 0, l_nrow / BUILD_FACTOR);
    meta_probe_in.setCol(1, 1, l_nrow / BUILD_FACTOR);
    meta_probe_in.setCol(2, 2, l_nrow / BUILD_FACTOR);
    gqeKernel(table_l_0_new, table_l_1_new, table_l_2_new, din_val, krn_cmd.getConfigBits(), meta_probe_in.meta(),
              meta_probe_out.meta(), table_out_0, table_out_1, table_out_2, table_out_3, (ap_uint<256>*)htb_buf[0],
              (ap_uint<256>*)htb_buf[1], (ap_uint<256>*)htb_buf[2], (ap_uint<256>*)htb_buf[3],
              (ap_uint<256>*)htb_buf[4], (ap_uint<256>*)htb_buf[5], (ap_uint<256>*)htb_buf[6],
              (ap_uint<256>*)htb_buf[7], (ap_uint<256>*)stb_buf[0], (ap_uint<256>*)stb_buf[1],
              (ap_uint<256>*)stb_buf[2], (ap_uint<256>*)stb_buf[3], (ap_uint<256>*)stb_buf[4],
              (ap_uint<256>*)stb_buf[5], (ap_uint<256>*)stb_buf[6], (ap_uint<256>*)stb_buf[7]);
    std::cout << "build ends................." << std::endl;
    krn_cmd.setBloomfilterBuildProbe(1);
    meta_probe_in.setCol(0, 0, l_nrow);
    meta_probe_in.setCol(1, 1, l_nrow);
    meta_probe_in.setCol(2, 2, l_nrow);
    */

    // bloom-filter probe
    std::cout << std::endl << "probe starts................." << std::endl;
    gqeKernel(table_l_0_new, table_l_1_new, table_l_2_new, din_val, (ap_uint<64>*)krn_cmd.getConfigBits(),
              (ap_uint<64>*)meta_probe_in.meta(), (ap_uint<TPCH_INT_SZ * 8 * VEC_LEN>*)meta_probe_out.meta(),
              table_out_0, table_out_1, table_out_2, table_out_3, (ap_uint<256>*)htb_buf[0], (ap_uint<256>*)htb_buf[1],
              (ap_uint<256>*)htb_buf[2], (ap_uint<256>*)htb_buf[3], (ap_uint<256>*)htb_buf[4],
              (ap_uint<256>*)htb_buf[5], (ap_uint<256>*)htb_buf[6], (ap_uint<256>*)htb_buf[7],
              (ap_uint<256>*)stb_buf[0], (ap_uint<256>*)stb_buf[1], (ap_uint<256>*)stb_buf[2],
              (ap_uint<256>*)stb_buf[3], (ap_uint<256>*)stb_buf[4], (ap_uint<256>*)stb_buf[5],
              (ap_uint<256>*)stb_buf[6], (ap_uint<256>*)stb_buf[7]);
    std::cout << "probe ends................." << std::endl;

#else
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

    // filter kernel
    cl_kernel fkernel;
    fkernel = clCreateKernel(prg, "gqeKernel", &err);
    logger.logCreateKernel(err);
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_l[3], mext_cfg5s, mext_valid_in_col;
    mext_table_l[0] = {0, table_l_user_0, fkernel};
    mext_table_l[1] = {1, table_l_user_1, fkernel};
    mext_table_l[2] = {2, table_l_user_2, fkernel};

    mext_valid_in_col = {3, din_val, fkernel};

    mext_cfg5s = {4, krn_cmd.getConfigBits(), fkernel};

    cl_mem_ext_ptr_t memExt[PU_NM * 2];
    memExt[0] = {11, htb_buf[0], fkernel};
    memExt[1] = {12, htb_buf[1], fkernel};
    memExt[2] = {13, htb_buf[2], fkernel};
    memExt[3] = {14, htb_buf[3], fkernel};
    memExt[4] = {15, htb_buf[4], fkernel};
    memExt[5] = {16, htb_buf[5], fkernel};
    memExt[6] = {17, htb_buf[6], fkernel};
    memExt[7] = {18, htb_buf[7], fkernel};
    memExt[8] = {19, stb_buf[0], fkernel};
    memExt[9] = {20, stb_buf[1], fkernel};
    memExt[10] = {21, stb_buf[2], fkernel};
    memExt[11] = {22, stb_buf[3], fkernel};
    memExt[12] = {23, stb_buf[4], fkernel};
    memExt[13] = {24, stb_buf[5], fkernel};
    memExt[14] = {25, stb_buf[6], fkernel};
    memExt[15] = {26, stb_buf[7], fkernel};

    cl_mem_ext_ptr_t mext_table_out[5];
    mext_table_out[0] = {7, table_out_0, fkernel};
    mext_table_out[1] = {8, table_out_1, fkernel};
    mext_table_out[2] = {9, table_out_2, fkernel};
    mext_table_out[3] = {10, table_out_3, fkernel};

    cl_mem_ext_ptr_t mext_meta_probe_in, mext_meta_probe_out;

    mext_meta_probe_in = {5, meta_probe_in.meta(), fkernel};
    mext_meta_probe_out = {6, meta_probe_out.meta(), fkernel};

    // Map buffers
    cl_mem buf_table_l[3];
    cl_mem buf_table_out[4];

    for (int c = 0; c < 3; c++) {
        buf_table_l[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                        table_l_size, &mext_table_l[c], &err);
    }
    cl_mem buf_valid_in_col = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                             ((l_nrow + 63) / 64) * sizeof(ap_uint<64>), &mext_valid_in_col, &err);
    for (int c = 0; c < 4; c++) {
        buf_table_out[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          table_result_size, &mext_table_out[c], &err);
    }

    cl_mem buf_cfg5s = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                      (sizeof(ap_uint<512>) * 14), &mext_cfg5s, &err);

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

    cl_mem buf_meta_probe_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (sizeof(ap_uint<512>) * 8), &mext_meta_probe_in, &err);
    cl_mem buf_meta_probe_out = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                               (sizeof(ap_uint<512>) * 8), &mext_meta_probe_out, &err);

    std::cout << "buffers have been mapped.\n";

    // helper buffer sets
    std::vector<cl_mem> loop_in_hbms;
    for (int c = 0; c < 2 * PU_NM; c++) {
        loop_in_hbms.push_back(buf_tmp[c]);
    }
    std::vector<cl_mem> loop_in_bufs;
    for (int c = 0; c < 3; c++) {
        loop_in_bufs.push_back(buf_table_l[c]);
    }
    loop_in_bufs.push_back(buf_cfg5s);
    loop_in_bufs.push_back(buf_meta_probe_in);
    loop_in_bufs.push_back(buf_meta_probe_out);

    std::vector<cl_mem> loop_out_bufs;
    for (int c = 0; c < 4; c++) {
        loop_out_bufs.push_back(buf_table_out[c]);
    }
    loop_out_bufs.push_back(buf_meta_probe_out);

    // make resident
    clEnqueueMigrateMemObjects(cmq, loop_in_hbms.size(), loop_in_hbms.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, loop_in_bufs.size(), loop_in_bufs.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, loop_out_bufs.size(), loop_out_bufs.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);
    std::cout << "make buffer resident.\n";

    // set args for fkernel
    int j = 0;
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_l[0]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_l[1]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_l[2]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_valid_in_col);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_cfg5s);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_meta_probe_in);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_meta_probe_out);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_out[0]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_out[1]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_out[2]);
    clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_table_out[3]);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(fkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }
    std::cout << "kernel args set.\n";

    int k_id = 0;
    std::array<cl_event, 1> evt_hbm;
    std::array<cl_event, 1> evt_tb_l;
    std::array<cl_event, 1> evt_pkrn;
    std::array<cl_event, 1> evt_tb_out;

    timeval t0, t1;
    // --- probe ---
    // 5) migrate hast-tables into HBMs
    gettimeofday(&t0, 0);
    clEnqueueMigrateMemObjects(cmq, loop_in_hbms.size(), loop_in_hbms.data(), 0, 0, nullptr, &evt_hbm[0]);
    clFinish(cmq);
    gettimeofday(&t1, 0);
    tvdiff(t0, t1, "probe hbm: ");

    // 6) migrate L table data from host buffer to device buffer
    gettimeofday(&t0, 0);
    clEnqueueMigrateMemObjects(cmq, loop_in_bufs.size(), loop_in_bufs.data(), 0, 0, nullptr, &evt_tb_l[0]);
    clFinish(cmq);
    gettimeofday(&t1, 0);
    tvdiff(t0, t1, "probe h2d: ");

    // 7) launch bloom-filter probe kernel
    gettimeofday(&t0, 0);
    clEnqueueTask(cmq, fkernel, 1, evt_tb_l.data(), &evt_pkrn[0]);
    clFinish(cmq);
    gettimeofday(&t1, 0);
    tvdiff(t0, t1, "probe krn time: ");

    // 8) migrate result data from device buffer to host buffer
    gettimeofday(&t0, 0);
    clEnqueueMigrateMemObjects(cmq, loop_out_bufs.size(), loop_out_bufs.data(), CL_MIGRATE_MEM_OBJECT_HOST, 1,
                               evt_pkrn.data(), &evt_tb_out[0]);
    clFinish(cmq);
    gettimeofday(&t1, 0);
    tvdiff(t0, t1, "probe d2h time: ");

    // 9) copy output data from pinned host buffer to user host buffer
    clWaitForEvents(1, &evt_tb_out[0]);
#endif

    // =========== print result ===========
    // check the probe updated meta
    int out_nrow = l_nrow;
    int p_nrow = out_nrow;
    std::cout << "Output buffer has " << out_nrow << " rows." << std::endl;

    std::cout << "------------Checking result-------------" << std::endl;
    // save filtered key/payload to std::unordered_map for checking
    std::unordered_map<int64_t, int64_t> filtered_pairs;
    for (int n = 0; n < p_nrow; n++) {
        // valid key flag
        if (table_out_0[n / (sizeof(ap_uint<8 * TPCH_INT_SZ * VEC_LEN>) * 8)]
                       [n % (sizeof(ap_uint<8 * TPCH_INT_SZ * VEC_LEN>) * 8)]) {
            filtered_pairs.insert(
                std::make_pair<int64_t, int64_t>((int64_t)table_l_user_0[n], (int64_t)table_l_user_2[n]));
        }
    }
    // test each added key in the filtered key list
    int nerror = 0;
    for (int i = 0; i < l_nrow / BUILD_FACTOR; i++) {
        // valid input row
        if (din_val[i / 64][i % 64]) {
            std::unordered_map<int64_t, int64_t>::const_iterator got = filtered_pairs.find((int64_t)table_l_user_0[i]);
            if (got == filtered_pairs.end()) {
                nerror++;
                std::cout << "Missing key = " << table_l_user_0[i] << " in bloom-filter." << std::endl;
            }
        }
    }

    if (nerror) std::cout << nerror << " errors found in " << l_nrow << " inputs.\n";

    nerror ? logger.error(Logger::Message::TEST_FAIL) : logger.info(Logger::Message::TEST_PASS);

    return nerror;
}
