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
#ifndef GQE_Q6_CFG_H
#define GQE_Q6_CFG_H

#include "ap_int.h"

#include "xf_database/enums.hpp"
#include "xf_database/dynamic_alu_host.hpp"

/* filter
 * (5 <= col:1 << 7) AND (19940101 <= col:2 < 19950101) AND (col:3 < 24)
 */
static void gen_q6_fcfg(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // 5 <= cond_1 <= 7, 2nd col in input.
    cfg[n++] = (uint32_t)5L;
    cfg[n++] = (uint32_t)7L;
    cfg[n++] = 0UL | (FOP_GE << FilterOpWidth) | (FOP_LE);
    // 19940101 <= cond_2 < 19950101
    cfg[n++] = (uint32_t)19940101L;
    cfg[n++] = (uint32_t)19950101L;
    cfg[n++] = 0UL | (FOP_GEU << FilterOpWidth) | (FOP_LTU);
    // cond_3 < 24
    cfg[n++] = (uint32_t)0L;
    cfg[n++] = (uint32_t)24L;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_LTU);

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
 * select
 *   sum(l_extendedprice * l_discount) as revenue
 * from
 *   lineitem
 * where
 *   l_shipdate >= '1994-01-01'
 *   and l_shipdate < '1995-01-01'
 *   and l_discount between 0.05 and 0.07
 *   and l_quantity < 24;
 *
 * col:0 l_extendedprice            (eval var a)
 * col:1 l_discount (filter cond 1) (eval var b)
 * col:2 l_shipdate (filter cond 2) (eval var c)
 * col:3 l_quantity (filter cond 3) (eval var d)
 * col:4 NA         (filter cond 4)
 */
void get_q6_cfg(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 0; // join off
    t.set_bit(1, 1);    // aggr on
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 1, 2, 3, -1, -1, -1, -1};
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {-1, -1, -1, -1, -1, -1, -1, -1}; //  not used.
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = 0x10; // write only col:5 (eval result)

    b[0] = t;

    // 512b word
    // alu: a * b
    ap_uint<289> op;
    xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>("strm1*strm2", 0, 0, 0, 0, op);
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter a
    uint32_t cfg[45];
    gen_q6_fcfg(cfg);
    memcpy(&b[3], cfg, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter b
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
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = 1;
    shuffle4_cfg(23, 16) = 2;
    shuffle4_cfg(31, 24) = 3;
    shuffle4_cfg(39, 32) = 8;
    shuffle4_cfg(47, 40) = 5;
    shuffle4_cfg(55, 48) = 6;
    shuffle4_cfg(63, 56) = 7;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

#endif // GQE_Q6_CFG_H
