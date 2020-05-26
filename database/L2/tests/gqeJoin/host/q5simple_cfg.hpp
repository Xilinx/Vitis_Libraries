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
#ifndef GQE_Q5SIMPLE_CFG_H
#define GQE_Q5SIMPLE_CFG_H

#include "q5_cfg.hpp"

/* filter (19940101<= col:1 < 19950101) */
static void gen_q5simple_orders_fcfg(uint32_t cfg[]) {
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
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
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

/*
 * col:0                 (join key)     (eval var a) (aggr 0)
 * col:1 (filter cond 1) (join key/pay) (eval var b) (aggr 1)
 * col:2 (filter cond 2) (join pay)     (eval var c) (aggr 2)
 * col:3 (filter cond 3) (join pay)     (eval var d) (aggr 3)
 * col:4 (filter cond 4) (join pay)                  (aggr 4)
 */
/*
 * select
 *   sum(l_extendedprice * (1 - l_discount)) as revenue
 * from
 *   orders,
 *   lineitem
 * where
*    l_orderkey = o_orderkey
 *   and o_orderdate >= '1994-01-01'
 *   and o_orderdate < '1995-01-01';
 *
 * Orders
 * 0 o_orderkey
 * 1 o_orderdate
 *
 * Lineitem
 * 0 l_orderkey
 * 1 l_extendedprice
 * 2 l_discount
 *
 * --
 *
 * joined
 * 0 l_extendedprice (big payload first)
 * 1 l_discount
 * 2 -
 * 3 o_orderkey (small payload last)
 *
 * --
 *
 * Result
 * 0 -
 * 1 -
 * 2 -
 * 3 -
 * 4 sum(aggr)
 */

void get_q5simple_cfg(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 1);    // aggr on
    t.set_bit(2, 0);    // dura key off
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti

    signed char id_a[] = {0, 1, -2, -2, -1, -1, -1, -1}; // Orders, 2col
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 1, 2, -1, -1, -1, -1, -1}; // Lineitem, 3col
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = 0x13; // write only col:4,1,0 (eval result)

    b[0] = t;

    // 512b word
    // alu: a * (100 - b)
    ap_uint<289> op;
    xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>("strm1*(-strm2+c2)", 0, 100, 0, 0,
                                                                               //"(-strm1+c1)*strm3", 100, 0, 0, 0,
                                                                               op);
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter Orders
    uint32_t cfg[45];
    gen_q5simple_orders_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter Lineitem
    gen_pass_fcfg(cfg);
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
    shuffle2_cfg(31, 24) = 3;
    shuffle2_cfg(39, 32) = 4;
    shuffle2_cfg(47, 40) = 5;
    shuffle2_cfg(55, 48) = 6;
    shuffle2_cfg(63, 56) = 7;

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
    shuffle4_cfg(7, 0) = 8;
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

#endif // GQE_Q5SIMPLE_CFG_H
