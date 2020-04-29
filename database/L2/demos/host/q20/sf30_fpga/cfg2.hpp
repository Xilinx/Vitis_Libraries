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
#ifndef GQE_CFG_H
#define GQE_CFG_H

#include "ap_int.h"
#include "xf_database/enums.hpp"
#include "xf_database/dynamic_alu_host.hpp"
#include <fstream>
#endif

void gen_filter_cfg(uint32_t cfg[]) {
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
    cfg[n++] = (uint32_t)19940101;
    cfg[n++] = (uint32_t)19950101;
    cfg[n++] = 0UL | (FOP_GE << FilterOpWidth) | (FOP_LT);
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

void get_partition_cfg(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.range(5, 3) = 1;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 1, 2, 3, -1, -1, -1, -1}; // dup key to payload
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 1, 2, -1, -1, -1, -1, -1}; // not used.
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (128 * 0 + 64 * 0 + 32 * 0 + 16 * 0 + 8 * 0 + 4 * 0 + 2 * 0 + 1 * 1);

    b[0] = t;

    // 512b word
    // alu:
    ap_uint<289> op = 0;
    b[1] = op;
    b[2] = op;

    // 512b word * 3
    // filter a
    uint32_t cfga[45];
    gen_filter_cfg(cfga);
    memcpy(&b[3], cfga, sizeof(uint32_t) * 45);

    // 512b word * 3
    // filter b
    uint32_t cfgb[45];
    gen_filter_cfg(cfgb);
    memcpy(&b[6], cfgb, sizeof(uint32_t) * 45);
    // --
    ap_int<64> shuffle1a_cfg;
    shuffle1a_cfg(7, 0) = 0;
    shuffle1a_cfg(15, 8) = 1;
    shuffle1a_cfg(23, 16) = 2;
    shuffle1a_cfg(31, 24) = -1;
    shuffle1a_cfg(39, 32) = -1;
    shuffle1a_cfg(47, 40) = -1;
    shuffle1a_cfg(55, 48) = -1;
    shuffle1a_cfg(63, 56) = -1;

    ap_int<64> shuffle1b_cfg;
    shuffle1b_cfg(7, 0) = 0;
    shuffle1b_cfg(15, 8) = 1;
    shuffle1b_cfg(23, 16) = 2;
    shuffle1b_cfg(31, 24) = -1;
    shuffle1b_cfg(39, 32) = -1;
    shuffle1b_cfg(47, 40) = -1;
    shuffle1b_cfg(55, 48) = -1;
    shuffle1b_cfg(63, 56) = -1;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 1;
    shuffle2_cfg(15, 8) = -1;
    shuffle2_cfg(23, 16) = -1;
    shuffle2_cfg(31, 24) = -1;
    shuffle2_cfg(39, 32) = -1;
    shuffle2_cfg(47, 40) = -1;
    shuffle2_cfg(55, 48) = -1;
    shuffle2_cfg(63, 56) = -1;

    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0;
    shuffle3_cfg(15, 8) = -1;
    shuffle3_cfg(23, 16) = -1;
    shuffle3_cfg(31, 24) = -1;
    shuffle3_cfg(39, 32) = -1;
    shuffle3_cfg(47, 40) = -1;
    shuffle3_cfg(55, 48) = -1;
    shuffle3_cfg(63, 56) = -1;

    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 0;
    shuffle4_cfg(15, 8) = -1;
    shuffle4_cfg(23, 16) = -1;
    shuffle4_cfg(31, 24) = -1;
    shuffle4_cfg(39, 32) = -1;
    shuffle4_cfg(47, 40) = -1;
    shuffle4_cfg(55, 48) = -1;
    shuffle4_cfg(63, 56) = -1;

    b[0].range(255, 192) = shuffle1a_cfg;
    b[0].range(319, 256) = shuffle1b_cfg;
    b[0].range(383, 320) = shuffle2_cfg;
    b[0].range(447, 384) = shuffle3_cfg;
    b[0].range(511, 448) = shuffle4_cfg;
}

void get_aggr_cfg(ap_uint<32>* buf, int i) {
    ap_uint<32>* config = buf;
    memset(config, 0, sizeof(ap_uint<32>) * 81);

    // scan and shuffle for eval0
    ap_uint<32> t;
    signed char id[] = {0, 1, 2, 3, -1, -1, -1, -1}; // Lineitem, 6col
    for (int c = 0; c < 4; ++c) {
        t.range(8 * c + 7, 8 * c) = id[c];
    }
    config[0] = t;

    for (int c = 0; c < 4; ++c) {
        t.range(8 * c + 7, 8 * c) = id[c + 4];
    }
    config[1] = t;

    // alu0: a * (1 - b)
    ap_uint<289> op;
    xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>("strm1+strm2", 0, 1, 0, 0, op);

    for (int i = 0; i < 9; i++) {
        config[i + 2] = op(32 * (i + 1) - 1, 32 * i);
    }
    config[11] = op[288];

    // alu1: a * (1 - b) * (1 + c)
    xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>("strm1+strm2", 0, 1, 1, 0, op);

    for (int i = 0; i < 9; i++) {
        config[i + 12] = op(32 * (i + 1) - 1, 32 * i);
    }
    config[21] = op[288];

    // filter
    uint32_t fcfg[45];
    gen_filter_cfg(fcfg);
    memcpy(&config[22], fcfg, sizeof(uint32_t) * 45);

    // shuffle1 for eval1
    ap_int<64> shuffle1_cfg;
    shuffle1_cfg(7, 0) = 0;   // extenderprice
    shuffle1_cfg(15, 8) = 1;  // discount
    shuffle1_cfg(23, 16) = 2; // tax
    shuffle1_cfg(31, 24) = 3; // shipdate
    shuffle1_cfg(39, 32) = 4; // returnflag
    shuffle1_cfg(47, 40) = 5; // linestatus
    shuffle1_cfg(55, 48) = 6; // quantity
    shuffle1_cfg(63, 56) = 7; // eva0
    config[67] = shuffle1_cfg(31, 0);
    config[68] = shuffle1_cfg(63, 32);

    // shuffle2 for filter
    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 0;   // extenderprice
    shuffle2_cfg(15, 8) = 1;  // discount
    shuffle2_cfg(23, 16) = 2; // eva1
    shuffle2_cfg(31, 24) = 3; // shipdate
    shuffle2_cfg(39, 32) = 4; // returnflag
    shuffle2_cfg(47, 40) = 5; // linestatus
    shuffle2_cfg(55, 48) = 6; // quantity
    shuffle2_cfg(63, 56) = 7; // eva0
    config[69] = shuffle2_cfg(31, 0);
    config[70] = shuffle2_cfg(63, 32);

    // shuffle3 to shuffle key
    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0; // returnflag
    shuffle3_cfg(15, 8) = 1;
    shuffle3_cfg(23, 16) = -1;
    shuffle3_cfg(31, 24) = -1;
    shuffle3_cfg(39, 32) = -1;
    shuffle3_cfg(47, 40) = -1;
    shuffle3_cfg(55, 48) = -1;
    shuffle3_cfg(63, 56) = -1;
    config[71] = shuffle3_cfg(31, 0);
    config[72] = shuffle3_cfg(63, 32);

    // shuffle4 to shuffle pld
    ap_int<64> shuffle4_cfg;
    shuffle4_cfg(7, 0) = 3; // cnt
    shuffle4_cfg(15, 8) = -1;
    shuffle4_cfg(23, 16) = -1;
    shuffle4_cfg(31, 24) = -1;
    shuffle4_cfg(39, 32) = -1;
    shuffle4_cfg(47, 40) = -1;
    shuffle4_cfg(55, 48) = -1;
    shuffle4_cfg(63, 56) = -1;
    config[73] = shuffle4_cfg(31, 0);
    config[74] = shuffle4_cfg(63, 32);

    // group aggr
    ap_uint<4> aggr_op[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    aggr_op[0] = xf::database::enums::AOP_SUM;
    aggr_op[1] = xf::database::enums::AOP_SUM;
    aggr_op[2] = xf::database::enums::AOP_SUM;
    aggr_op[3] = xf::database::enums::AOP_SUM;
    aggr_op[4] = xf::database::enums::AOP_SUM;
    aggr_op[5] = xf::database::enums::AOP_SUM;
    aggr_op[6] = xf::database::enums::AOP_SUM;
    aggr_op[7] = xf::database::enums::AOP_SUM;
    config[75] = (aggr_op[7], aggr_op[6], aggr_op[5], aggr_op[4], aggr_op[3], aggr_op[2], aggr_op[1], aggr_op[0]);

    config[76] = 2; // key column
    config[77] = 1; // pld column
    config[78] = 0; // aggr num

    // column merge
    ap_uint<8> merge[5];
    merge[0] = 0x3; // 0000_0011
    merge[1] = 0;
    merge[2] = 0;
    merge[3] = 0xc0; // 1100_0000
    merge[4] = 0;
    config[79] = (0, merge[2], merge[1], merge[0]);
    config[80] = (1, merge[4], merge[3]);

    // demux mux direct_aggr
    config[81] = 0;

    // write out
    config[82] = 0x01c1; // 0000_0001_1100_0001

    /*
        config[80][0]=1;//sum1l
        config[80][1]=0;//
        config[80][2]=0;//
        config[80][3]=0;//
        config[80][4]=0;//
        config[80][5]=0;//
        config[80][6]=1;//key1
        config[80][7]=1;//key0
        config[80][8]=1;//sum1h
        config[80][9]=0;//
        config[80][10]=0;//
        config[80][11]=0;//
        config[80][12]=0;//
        config[80][13]=0;//
        config[80][14]=0;//
        config[80][15]=0;//
    */
}
