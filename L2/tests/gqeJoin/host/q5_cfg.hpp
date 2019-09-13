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
#ifndef GQE_Q5_CFG_H
#define GQE_Q5_CFG_H

#include "ap_int.h"

#include "xf_database/enums.hpp"
#include "xf_database/dynamic_alu_host.hpp"

/* filter (true) */
static void gen_pass_fcfg(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_2
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_3
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_4
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

/* filter (19940101<= col:2 < 19950101) */
static void gen_q5_orders_fcfg(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_2
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // 19940101 <= cond_2 < 19950101
    cfg[n++] = (uint32_t)19940101UL;
    cfg[n++] = (uint32_t)19950101UL;
    cfg[n++] = 0UL | (FOP_GEU << FilterOpWidth) | (FOP_LTU);
    // cond_4
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

/* n_out
 * 0 c_nationkey
 *
 * Customer
 * 0 c_nationkey
 * 1 c_custkey
 *
 * --
 *
 * joined
 * 0 c_custkey (big payload first)
 * 1 -
 * 2 -
 * 3 c_nationkey (small payload last)
 * 4 -
 *
 * --
 *
 * out1
 * 0 c_custkey
 * 1
 * 2
 * 3 c_nationkey
 */
void get_q5_cfg_n_c_out1(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 0, -1, -1, -1, -1, -1, -1}; // dup key to payload
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 1, -1, -1, -1, -1, -1, -1}; // not used.
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (0 + 8 + 0 + 0 + 1);

    b[0] = t;

    // 512b word
    // alu:
    ap_uint<289> op = 0;
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter a
    uint32_t cfg[45];
    gen_pass_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter b
    memcpy(&b[6], cfg, sizeof(uint32_t) * 45);
    // --
    ap_int<64> shuffle1a_cfg;
    shuffle1a_cfg(7, 0) = 0;
    shuffle1a_cfg(15, 8) = 1;
    shuffle1a_cfg(23, 16) = 2;
    shuffle1a_cfg(31, 24) = 3;
    shuffle1a_cfg(39, 32) = 4;
    shuffle1a_cfg(47, 40) = 5;
    shuffle1a_cfg(55, 48) = 6;
    shuffle1a_cfg(63, 56) = 7;

    ap_int<64> shuffle1b_cfg;
    shuffle1b_cfg(7, 0) = 0;
    shuffle1b_cfg(15, 8) = 1;
    shuffle1b_cfg(23, 16) = 2;
    shuffle1b_cfg(31, 24) = 3;
    shuffle1b_cfg(39, 32) = 4;
    shuffle1b_cfg(47, 40) = 5;
    shuffle1b_cfg(55, 48) = 6;
    shuffle1b_cfg(63, 56) = 7;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 0;
    shuffle2_cfg(15, 8) = 1;
    shuffle2_cfg(23, 16) = 2;
    shuffle2_cfg(31, 24) = 6;
    shuffle2_cfg(39, 32) = 7;
    shuffle2_cfg(47, 40) = 8;
    shuffle2_cfg(55, 48) = 9;
    shuffle2_cfg(63, 56) = 10;

    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0;
    shuffle3_cfg(15, 8) = 1;
    shuffle3_cfg(23, 16) = 2;
    shuffle3_cfg(31, 24) = 3;
    shuffle3_cfg(39, 32) = 4;
    shuffle3_cfg(47, 40) = 5;
    shuffle3_cfg(55, 48) = 6;
    shuffle3_cfg(63, 56) = 7;

    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = 1;
    shuffle4_cfg(23, 16) = 2;
    shuffle4_cfg(31, 24) = 3;
    shuffle4_cfg(39, 32) = 4;
    shuffle4_cfg(47, 40) = 5;
    shuffle4_cfg(55, 48) = 6;
    shuffle4_cfg(63, 56) = 7;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

/*
 * out1
 * 0 c_custkey
 * 1
 * 2
 * 3 c_nationkey
 *
 * Orders
 * 0 o_custkey
 * 1 o_orderkey
 * 2 o_orderdate
 *
 * --
 *
 * joined
 * 0 o_orderkey (big payload first)
 * 1 o_orderdate
 * 2 -
 * 3 c_nationkey (small payload last)
 * 4 -
 *
 * --
 *
 * out2
 * 0 o_orderkey
 * 1
 * 2
 * 3 c_nationkey
 */
void get_q5_cfg_out1_o_out2(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 3, -1, -1, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 1, 2, -1, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (0 + 8 + 0 + 0 + 1);

    b[0] = t;

    // 512b word
    // alu:
    ap_uint<289> op = 0;
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter out1
    uint32_t cfg[45];
    gen_pass_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter orders
    gen_q5_orders_fcfg(cfg);
    memcpy(&b[6], cfg, sizeof(uint32_t) * 45);
    // --
    ap_int<64> shuffle1a_cfg;
    shuffle1a_cfg(7, 0) = 0;
    shuffle1a_cfg(15, 8) = 1;
    shuffle1a_cfg(23, 16) = 2;
    shuffle1a_cfg(31, 24) = 3;
    shuffle1a_cfg(39, 32) = 4;
    shuffle1a_cfg(47, 40) = 5;
    shuffle1a_cfg(55, 48) = 6;
    shuffle1a_cfg(63, 56) = 7;

    ap_int<64> shuffle1b_cfg;
    shuffle1b_cfg(7, 0) = 0;
    shuffle1b_cfg(15, 8) = 1;
    shuffle1b_cfg(23, 16) = 2;
    shuffle1b_cfg(31, 24) = 3;
    shuffle1b_cfg(39, 32) = 4;
    shuffle1b_cfg(47, 40) = 5;
    shuffle1b_cfg(55, 48) = 6;
    shuffle1b_cfg(63, 56) = 7;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 0;
    shuffle2_cfg(15, 8) = 1;
    shuffle2_cfg(23, 16) = 2;
    shuffle2_cfg(31, 24) = 6;
    shuffle2_cfg(39, 32) = 7;
    shuffle2_cfg(47, 40) = 8;
    shuffle2_cfg(55, 48) = 9;
    shuffle2_cfg(63, 56) = 10;

    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0;
    shuffle3_cfg(15, 8) = 1;
    shuffle3_cfg(23, 16) = 2;
    shuffle3_cfg(31, 24) = 3;
    shuffle3_cfg(39, 32) = 4;
    shuffle3_cfg(47, 40) = 5;
    shuffle3_cfg(55, 48) = 6;
    shuffle3_cfg(63, 56) = 7;

    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = 1;
    shuffle4_cfg(23, 16) = 2;
    shuffle4_cfg(31, 24) = 3;
    shuffle4_cfg(39, 32) = 4;
    shuffle4_cfg(47, 40) = 5;
    shuffle4_cfg(55, 48) = 6;
    shuffle4_cfg(63, 56) = 7;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

/*
 * out2
 * 0 o_orderkey
 * 1
 * 2
 * 3 c_nationkey
 *
 * Lineitem
 * 0 l_orderkey
 * 1 l_suppkey
 * 2 l_extendedprice
 * 3 l_discount
 *
 * --
 *
 * joined
 * 0 l_extendedprice (big payload first)
 * 1 l_discount
 * 2 l_suppkey
 * 3 c_nationkey (small payload last)
 * 4 l_extendedprice*(1-l_discount)
 *
 * --
 *
 * out1
 * 0 -
 * 1 -
 * 2 l_suppkey
 * 3 c_nationkey
 * 4 l_extendedprice*(1-l_discount)
 */
void get_q5_cfg_out2_l_out1(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 3, -1, -1, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 1, 2, 3, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (16 + 8 + 4 + 0 + 0);

    b[0] = t;

    // 512b word
    // alu:
    ap_uint<289> op = 0;
    xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>("strm1*(-strm2+c2)", 0, 100, 0, 0,
                                                                               //"(-strm1+c1)*strm3", 100, 0, 0, 0,
                                                                               op);
    b[1] = op;
    b[2] = 0;

    // 512b word * 3
    // filter
    uint32_t cfg[45];
    gen_pass_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter
    memcpy(&b[6], cfg, sizeof(uint32_t) * 45);
    // --
    ap_int<64> shuffle1a_cfg;
    shuffle1a_cfg(7, 0) = 0;
    shuffle1a_cfg(15, 8) = 1;
    shuffle1a_cfg(23, 16) = 2;
    shuffle1a_cfg(31, 24) = 3;
    shuffle1a_cfg(39, 32) = 4;
    shuffle1a_cfg(47, 40) = 5;
    shuffle1a_cfg(55, 48) = 6;
    shuffle1a_cfg(63, 56) = 7;

    ap_int<64> shuffle1b_cfg;
    shuffle1b_cfg(7, 0) = 0;
    shuffle1b_cfg(15, 8) = 1;
    shuffle1b_cfg(23, 16) = 2;
    shuffle1b_cfg(31, 24) = 3;
    shuffle1b_cfg(39, 32) = 4;
    shuffle1b_cfg(47, 40) = 5;
    shuffle1b_cfg(55, 48) = 6;
    shuffle1b_cfg(63, 56) = 7;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 1;
    shuffle2_cfg(15, 8) = 2;
    shuffle2_cfg(23, 16) = 0;
    shuffle2_cfg(31, 24) = 6;
    shuffle2_cfg(39, 32) = 7;
    shuffle2_cfg(47, 40) = 8;
    shuffle2_cfg(55, 48) = 9;
    shuffle2_cfg(63, 56) = 10;

    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0;
    shuffle3_cfg(15, 8) = 1;
    shuffle3_cfg(23, 16) = 2;
    shuffle3_cfg(31, 24) = 3;
    shuffle3_cfg(39, 32) = 8;
    shuffle3_cfg(47, 40) = 5;
    shuffle3_cfg(55, 48) = 6;
    shuffle3_cfg(63, 56) = 7;

    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = 1;
    shuffle4_cfg(23, 16) = 2;
    shuffle4_cfg(31, 24) = 3;
    shuffle4_cfg(39, 32) = 4;
    shuffle4_cfg(47, 40) = 5;
    shuffle4_cfg(55, 48) = 6;
    shuffle4_cfg(63, 56) = 7;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

/*
 * Supplier
 * 0 s_suppkey (k0)
 * 1 s_nationkey (k1) also spay
 *
 * out1 (need shift order)
 * 0 -
 * 1 -
 * 2 l_suppkey       0 (k0)
 * 3 c_nationkey     1 (k1)
 * 4 l_extendedprice*(1-l_discount) (p3)
 * --
 *
 * joined
 * 0 - (big payload first)
 * 1 -
 * 2 l_extendedprice*(1-l_discount)
 * 3 s_nationkey (small payload)
 *
 * --
 *
 * out2
 * 0 - (big payload first)
 * 1 -
 * 2 l_extendedprice*(1-l_discount)
 * 3 s_nationkey (small payload)
 * 4 -
 */
void get_q5_cfg_s_out1_out2(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.set_bit(2, 1);    // dual-col key on
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 1, 1, -1, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {2, 3, -1, -1, 4, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (0 + 8 + 4 + 0 + 0);

    b[0] = t;

    // 512b word
    // alu:
    ap_uint<289> op = 0;
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter
    uint32_t cfg[45];
    gen_pass_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter
    memcpy(&b[6], cfg, sizeof(uint32_t) * 45);
    // --
    ap_int<64> shuffle1a_cfg;
    shuffle1a_cfg(7, 0) = 0;
    shuffle1a_cfg(15, 8) = 1;
    shuffle1a_cfg(23, 16) = 2;
    shuffle1a_cfg(31, 24) = 3;
    shuffle1a_cfg(39, 32) = 4;
    shuffle1a_cfg(47, 40) = 5;
    shuffle1a_cfg(55, 48) = 6;
    shuffle1a_cfg(63, 56) = 7;

    ap_int<64> shuffle1b_cfg;
    shuffle1b_cfg(7, 0) = 0;
    shuffle1b_cfg(15, 8) = 1;
    shuffle1b_cfg(23, 16) = 2;
    shuffle1b_cfg(31, 24) = 3;
    shuffle1b_cfg(39, 32) = 4;
    shuffle1b_cfg(47, 40) = 5;
    shuffle1b_cfg(55, 48) = 6;
    shuffle1b_cfg(63, 56) = 7;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 0;
    shuffle2_cfg(15, 8) = 1;
    shuffle2_cfg(23, 16) = 2;
    shuffle2_cfg(31, 24) = 6;
    shuffle2_cfg(39, 32) = 7;
    shuffle2_cfg(47, 40) = 8;
    shuffle2_cfg(55, 48) = 9;
    shuffle2_cfg(63, 56) = 10;

    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0;
    shuffle3_cfg(15, 8) = 1;
    shuffle3_cfg(23, 16) = 2;
    shuffle3_cfg(31, 24) = 3;
    shuffle3_cfg(39, 32) = 4;
    shuffle3_cfg(47, 40) = 5;
    shuffle3_cfg(55, 48) = 6;
    shuffle3_cfg(63, 56) = 7;

    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = 1;
    shuffle4_cfg(23, 16) = 2;
    shuffle4_cfg(31, 24) = 3;
    shuffle4_cfg(39, 32) = 4;
    shuffle4_cfg(47, 40) = 5;
    shuffle4_cfg(55, 48) = 6;
    shuffle4_cfg(63, 56) = 7;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

#endif // GQE_Q5_CFG_H
