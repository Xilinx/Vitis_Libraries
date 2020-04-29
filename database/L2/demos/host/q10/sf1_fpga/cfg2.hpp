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
#include <fstream>
#endif
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
        b[0].range(5, 3) = 0;
        b[0].range(39, 8) = 32;
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
        std::cout << std::dec;
        printf("\n");
        p.close();
    }
}
