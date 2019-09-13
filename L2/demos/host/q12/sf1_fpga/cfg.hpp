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

#include "xf_database/dynamic_alu_host.hpp"
#include "xf_database/enums.hpp"
#include <fstream>
void get_cfg_dat(ap_uint<512>* hbuf, const char* cfgdat, int i) {
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

static void gen_fcfg_1(uint32_t cfg[]) {
    using namespace xf::database;
    int n = 0;

    // cond_1
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_2 commit
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = (uint32_t)0UL;
    cfg[n++] = 0UL | (FOP_DC << FilterOpWidth) | (FOP_DC);
    // cond_3 receipt
    cfg[n++] = (uint32_t)19940101UL;
    cfg[n++] = (uint32_t)19950101UL;
    cfg[n++] = 0UL | (FOP_GEU << FilterOpWidth) | (FOP_LTU);
    // cond_4 ship
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
    r |= ((uint32_t)(FOP_LTU << sh));
    sh += FilterOpWidth;
    // cond_2 -- cond_4
    r |= ((uint32_t)(FOP_GTU << sh));
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

void get_cfg_dat_1(ap_uint<512>* hbuf) {
    ap_uint<512>* b = hbuf;
    memset(b, 0, sizeof(ap_uint<512>) * 9);

    // 512b word
    ap_uint<512> t = 1; // join on
    t.set_bit(1, 0);    // aggr off
    t.range(5, 3) = 0;  // hash join flag = 0 for normal, 1 for semi, 2 for anti
    t.range(39, 8) = 32;
    signed char id_a[] = {0, 1, -1, -1, -1, -1, -1, -1}; // dup key to payload
    for (int c = 0; c < 8; ++c) {
        t.range(56 + 8 * c + 7, 56 + 8 * c) = id_a[c];
    }

    signed char id_b[] = {0, 2, 3, 4, 1, -1, -1, -1, -1}; // not used.
    for (int c = 0; c < 8; ++c) {
        t.range(120 + 8 * c + 7, 120 + 8 * c) = id_b[c];
    }

    t.range(191, 184) = (128 * 0 + 64 * 0 + 32 * 0 + 16 * 0 + 8 * 0 + 4 * 0 + 2 + 1);

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
    gen_fcfg_1(cfg);
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
    shuffle1b_cfg(15, 8) = 4;
    shuffle1b_cfg(23, 16) = -1;
    shuffle1b_cfg(31, 24) = -1;
    shuffle1b_cfg(39, 32) = -1;
    shuffle1b_cfg(47, 40) = -1;
    shuffle1b_cfg(55, 48) = -1;
    shuffle1b_cfg(63, 56) = -1;

    ap_int<64> shuffle2_cfg;
    shuffle2_cfg(7, 0) = 6;
    shuffle2_cfg(15, 8) = 0;
    shuffle2_cfg(23, 16) = -1;
    shuffle2_cfg(31, 24) = -1;
    shuffle2_cfg(39, 32) = -1;
    shuffle2_cfg(47, 40) = -1;
    shuffle2_cfg(55, 48) = -1;
    shuffle2_cfg(63, 56) = -1;

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

#endif
