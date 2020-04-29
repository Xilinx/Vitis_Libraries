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

void get_join_cfg_dat(ap_uint<512>* hbuf, const char* cfgdat, int i) {
    int size = 64 * 9;
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);
    std::ifstream p;
    p.open(cfgdat, std::ios::in | std::ios::binary);
    if (p.is_open()) {
        p.seekg(i * size);
        char* buffer = new char[size];
        p.read(buffer, size);
        memcpy(&b[0], buffer, size);
        std::cout << "Tags(1x5):" << std::endl;
        std::cout << std::hex << b[0].range(7, 0) << std::endl;
        std::cout << "The scanai(8x8)" << std::endl;
        for (int i = 56; i < 119; i += 8) {
            std::cout << std::hex << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The scanb(8x8)" << std::endl;
        for (int i = 120; i < 183; i += 8) {
            std::cout << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The write(1x8)" << std::endl;
        for (int i = 184; i < 191; i += 8) {
            std::cout << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The shuffle1a(8x8)" << std::endl;
        for (int i = 192; i < 255; i += 8) {
            std::cout << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The shuffle1b(8x8)" << std::endl;
        for (int i = 256; i < 319; i += 8) {
            std::cout << std::hex << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The shuffle2(8x8)" << std::endl;
        for (int i = 320; i < 383; i += 8) {
            std::cout << std::hex << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The shuffle3(8x8)" << std::endl;
        for (int i = 384; i < 447; i += 8) {
            std::cout << std::hex << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "The shuffle4(8x8)" << std::endl;
        for (int i = 448; i < 512; i += 8) {
            std::cout << std::hex << b[0].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "alu1" << std::endl;
        for (int i = 0; i < 512; i += 8) {
            if (i % 64 == 0) std::cout << std::endl;
            std::cout << std::hex << b[1].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        std::cout << std::endl << "alu2" << std::endl;
        for (int i = 0; i < 512; i += 8) {
            if (i % 64 == 0) std::cout << std::endl;
            std::cout << std::hex << b[2].range(i + 7, i) << " ";
            // printf("%d ",b[0].range(i+7,i));
        }
        for (int i = 3; i < 6; i++) {
            for (int j = 0; j < 512; j += 32) {
                if (j % 256 == 0) std::cout << std::endl;
                std::cout << std::hex << b[i].range(j + 31, j) << " ";
                // printf("%d ",b[0].range(i+7,i));
            }
        }
        printf("\n");
        p.close();
    }
}

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

void get_aggr_cfg(ap_uint<32>* buf, int i) {
    ap_uint<32>* config = buf;
    memset(config, 0, sizeof(ap_uint<32>) * 81);

    // scan and shuffle for eval0
    ap_uint<32> t;
    signed char id[] = {0, 1, -1, -1, -1, -1, -1, -1}; // Lineitem, 6col
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
    shuffle1_cfg(7, 0) = 0;   //
    shuffle1_cfg(15, 8) = 1;  //
    shuffle1_cfg(23, 16) = 2; //
    shuffle1_cfg(31, 24) = 3; //
    shuffle1_cfg(39, 32) = 4; //
    shuffle1_cfg(47, 40) = 5; //
    shuffle1_cfg(55, 48) = 6; //
    shuffle1_cfg(63, 56) = 7; //
    config[67] = shuffle1_cfg(31, 0);
    config[68] = shuffle1_cfg(63, 32);

    // shuffle2 for filter
    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 0;   //
    shuffle2_cfg(15, 8) = 1;  //
    shuffle2_cfg(23, 16) = 2; //
    shuffle2_cfg(31, 24) = 3; //
    shuffle2_cfg(39, 32) = 4; //
    shuffle2_cfg(47, 40) = 5; //
    shuffle2_cfg(55, 48) = 6; //
    shuffle2_cfg(63, 56) = 7; //
    config[69] = shuffle2_cfg(31, 0);
    config[70] = shuffle2_cfg(63, 32);

    // shuffle3 to shuffle key
    ap_int<64> shuffle3_cfg;
    shuffle3_cfg(7, 0) = 0; // l_orderkey
    shuffle3_cfg(15, 8) = -1;
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
    shuffle4_cfg(7, 0) = 1; // l_quantity
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

    config[76] = 1; // key column
    config[77] = 1; // pld column
    config[78] = 0; // aggr num

    // column merge
    ap_uint<8> merge[5];
    merge[0] = 0;
    merge[1] = 0;
    merge[2] = 0x80; // 1000_0000
    merge[3] = 0;
    merge[4] = 0;
    config[79] = (1, merge[2], merge[1], merge[0]);
    config[80] = (1, merge[4], merge[3]);

    // demux mux direct_aggr
    config[81] = 0;

    // write out
    config[82] = 0x8101; // 1000_0001_0000_0001
    /*
        config[80][0]=1;//sum1l
        config[80][1]=0;//
        config[80][2]=0;//
        config[80][3]=0;//
        config[80][4]=0;//
        config[80][5]=0;//
        config[80][6]=0;//
        config[80][7]=0;//
        config[80][8]=1;//sum1h
        config[80][9]=0;//
        config[80][10]=0;//
        config[80][11]=0;//
        config[80][12]=0;//
        config[80][13]=0;//
        config[80][14]=0;//
        config[80][15]=1;//key1
    */
}
