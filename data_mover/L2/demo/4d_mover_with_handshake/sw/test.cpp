/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>
#include <experimental/xrt_bo.h>
#include <experimental/xrt_device.h>
#include <experimental/xrt_kernel.h>

static const int OCP = 0xFFFF;
static const int template_1D_pattern[24] = {
    OCP, 1, 1, 1, // buffer_dim[4]
    OCP, 0, 0, 0, // offset[4]
    OCP, 1, 1, 1, // tiling[4]
    0,   1, 2, 3, // dim_idx[4]
    OCP, 0, 0, 0, // stride[4]
    OCP, 1, 1, 1  // wrap[4]
};
static const int template_2D_pattern[24] = {
    OCP, OCP, 1, 1, // buffer_dim[4]
    OCP, OCP, 0, 0, // offset[4]
    OCP, OCP, 1, 1, // tiling[4]
    OCP, OCP, 2, 3, // dim_idx[4]
    OCP, OCP, 0, 0, // stride[4]
    OCP, OCP, 1, 1  // wrap[4]
};
static const int template_3D_pattern[24] = {
    OCP, OCP, OCP, 1, // buffer_dim[4]
    OCP, OCP, OCP, 0, // offset[4]
    OCP, OCP, OCP, 1, // tiling[4]
    OCP, OCP, OCP, 3, // dim_idx[4]
    OCP, OCP, OCP, 0, // stride[4]
    OCP, OCP, OCP, 1  // wrap[4]
};
static const int template_4D_pattern[24] = {
    OCP, OCP, OCP, OCP, // buffer_dim[4]
    OCP, OCP, OCP, OCP, // offset[4]
    OCP, OCP, OCP, OCP, // tiling[4]
    OCP, OCP, OCP, OCP, // dim_idx[4]
    OCP, OCP, OCP, OCP, // stride[4]
    OCP, OCP, OCP, OCP  // wrap[4]
};

// return the sum of number of element for all pattern
int pattern_gen(const int num_of_pattern, int* pattern_vec) {
    int sum_elem = 0;
    for (int p = 0; p < num_of_pattern;) {
        // int dim = rand() % 4;
        int dim = 3; // fixed to 4D
        int ptn_template[24];
        switch (dim) {
            case 0:
                std::memcpy(ptn_template, template_1D_pattern, 24 * sizeof(int));
                break;
            case 1:
                std::memcpy(ptn_template, template_2D_pattern, 24 * sizeof(int));
                break;
            case 2:
                std::memcpy(ptn_template, template_3D_pattern, 24 * sizeof(int));
                break;
            case 3:
                std::memcpy(ptn_template, template_4D_pattern, 24 * sizeof(int));
                break;
        }

        // for buffer dim
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 0;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 8;
            }
        }
        // for offset
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 4;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3;
            }
        }
        // for stride
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 16;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // for wrap
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 20;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // Check: offset + stride * (wrap - 1) <= buf_dim
        bool k0 = (ptn_template[0] < (ptn_template[16] * (ptn_template[20] - 1) + ptn_template[4]));
        bool k1 = (ptn_template[1] < (ptn_template[17] * (ptn_template[21] - 1) + ptn_template[5]));
        bool k2 = (ptn_template[2] < (ptn_template[18] * (ptn_template[22] - 1) + ptn_template[6]));
        bool k3 = (ptn_template[3] < (ptn_template[19] * (ptn_template[23] - 1) + ptn_template[7]));
        if (k0 || k1 || k2 || k3) continue;

        // for tiling
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 8;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // for dim_idx
        bool valid[4] = {true, true, true, true};
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 12;
            if (tmp[i] == OCP) {
                while (1) {
                    int id = rand() % (dim + 1);
                    if (valid[id]) {
                        tmp[i] = id;
                        valid[id] = false;
                        break;
                    }
                }
            }
        }

        std::memcpy(pattern_vec + p * 24, ptn_template, 24 * sizeof(int));
        sum_elem += (ptn_template[8] * ptn_template[9] * ptn_template[10] * ptn_template[11] * ptn_template[20] *
                     ptn_template[21] * ptn_template[22] * ptn_template[23]);
        // print the 4D pattern
        std::cout << "buffer_dim[4]={" << pattern_vec[p * 24 + 0] << "," << pattern_vec[p * 24 + 1] << ","
                  << pattern_vec[p * 24 + 2] << "," << pattern_vec[p * 24 + 3] << "}" << std::endl;
        std::cout << "buffer_offset[4]={" << pattern_vec[p * 24 + 4] << "," << pattern_vec[p * 24 + 5] << ","
                  << pattern_vec[p * 24 + 6] << "," << pattern_vec[p * 24 + 7] << "}" << std::endl;
        std::cout << "tiling[4]={" << pattern_vec[p * 24 + 8] << "," << pattern_vec[p * 24 + 9] << ","
                  << pattern_vec[p * 24 + 10] << "," << pattern_vec[p * 24 + 11] << "}" << std::endl;
        std::cout << "dim_idx[4]={" << pattern_vec[p * 24 + 12] << "," << pattern_vec[p * 24 + 13] << ","
                  << pattern_vec[p * 24 + 14] << "," << pattern_vec[p * 24 + 15] << "}" << std::endl;
        std::cout << "stride[4]={" << pattern_vec[p * 24 + 16] << "," << pattern_vec[p * 24 + 17] << ","
                  << pattern_vec[p * 24 + 18] << "," << pattern_vec[p * 24 + 19] << "}" << std::endl;
        std::cout << "wrap[4]={" << pattern_vec[p * 24 + 20] << "," << pattern_vec[p * 24 + 21] << ","
                  << pattern_vec[p * 24 + 22] << "," << pattern_vec[p * 24 + 23] << "}" << std::endl;
        p++;
    }

    return sum_elem;
}

// flatten one 4D pattern to 1D pattern, with the same number of element
void flatten_4D_pattern(const int num_in_pattern, int* pattern_src, int* pattern_dst) {
    int num_per_tiling = pattern_src[8] * pattern_src[9] * pattern_src[10] * pattern_src[11];
    int num_of_wrap = pattern_src[20] * pattern_src[21] * pattern_src[22] * pattern_src[23];
    std::memcpy(pattern_dst, template_1D_pattern, 24 * sizeof(int));
    pattern_dst[0] = num_in_pattern;  // dim
    pattern_dst[4] = 0;               // offset
    pattern_dst[8] = num_per_tiling;  // tiling
    pattern_dst[16] = num_per_tiling; // stride
    pattern_dst[20] = num_of_wrap;    // wrap
}

// generate 4D data and store to buffer continously
void populate_data_gen(int* current_ptn, uint64_t* dm_buf) {
    int ptr = 0;
    int* buf_dim = current_ptn;
    int* offset = current_ptn + 4;
    int* tiling = current_ptn + 8;
    int* dim_idx = current_ptn + 12;
    int* stride = current_ptn + 16;
    int* wrap = current_ptn + 20;
    for (int w = 0; w < wrap[dim_idx[3]]; w++) {
        for (int z = 0; z < wrap[dim_idx[2]]; z++) {
            for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                    int bias[4];
                    bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;
                    bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                    bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                    bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
                    for (int l = 0; l < tiling[3]; l++) {
                        for (int k = 0; k < tiling[2]; k++) {
                            for (int j = 0; j < tiling[1]; j++) {
                                for (int i = 0; i < tiling[0]; i++) {
                                    dm_buf[ptr++] = (bias[3] + l) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                    (bias[2] + k) * (buf_dim[1] * buf_dim[0]) +
                                                    (bias[1] + j) * buf_dim[0] + (bias[0] + i);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

// mode=0, R[rd]=R[rs]
// mode=1, R[rd]=rs
uint32_t MOVE(int mode, int rd, int rs) {
    return ((0x0 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs & 0xFF) << 12));
}

// mode=0, R[rd]=ack.read()
// mode=1, R[rd]=adj_ctrl.read()
// mode=2, R[rd]=adj_dm.read()
uint32_t POP(int mode, int rd) {
    return ((0x1 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8));
}

// mode=0, ptn.write(R[rs])
// mode=1, adj_ctrl.write(R[rs])
// mode=2, adj_dm.write(R[rs])
uint32_t PUSH(int mode, int rs) {
    return ((0x2 & 0x1F) | ((mode & 0x7) << 5) | ((rs & 0xF) << 8));
}

// mode=0, R[rd]=R[rs1]+R[rs2]
// mode=1, R[rd]=R[rs1]+rs2
uint32_t ADD(int mode, int rd, int rs1, int rs2) {
    return ((0x3 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs1 & 0xFF) << 12) | ((rs2 & 0xFF) << 20));
}

// mode=0, pc=#imm
// mode=1, pc=#imm if R[rs1]<R[rs2]
// mode=2, pc=#imm if R[rs1]>=R[rs2]
// mode=3, pc=#imm if R[rs1]==R[rs2]
// mode=4, pc=#imm if R[rs1]!=R[rs2]
// mode=5, pc=#imm if E[rs1]
uint32_t JUMP(int mode, int rs1, int rs2, int imm) {
    return ((0x4 & 0x1F) | ((mode & 0x7) << 5) | ((rs1 & 0xF) << 8) | ((rs2 & 0xF) << 12) | ((imm & 0xFFFF) << 16));
}

uint32_t EXIT(void) {
    return (0x5 & 0x1F);
}

// Internal read4D
// Master m2s firstly feed the URAM by 2 4D patterns, and trigger u2s once each pattern is finished
// Slave u2s translate and read the URAM to AXI-stream once receive trigger from m2s, according to its 4D pattern
// m2s & u2s access the URAM by ping-pong
//
// Work with write4D
// Master write4D/s2u firstly feed the ping & pong buffer by 2 4D patterns, and trigger read4D/m2s for each pattern
// Slave read4D/m2s start one DM once received trigger from write4D/s2u
//
// Master/slave m2s exit if patterns in both itw own and read4D/u2s are finished
// Slave u2s exit if all its pattern are finished
void pm_for_read4D(int num_of_pattern, uint32_t* pm0_vec, uint32_t* pm1_vec, uint32_t& pm0_sz, uint32_t& pm1_sz) {
    uint32_t ptr = 0;
    uint32_t TAG_M2S_ACK0, TAG_M2S_ACK1, TAG_M2S_ACK2, TAG_M2S_ACK3, TAG_M2S_ACK4;
    uint32_t TAG_M2S_WAIT0 = 22, TAG_M2S_WAIT1 = 41, TAG_M2S_EXIT = 45;
    pm0_vec[ptr++] = MOVE(1, R0, 1);              // Corp mode
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R0, 1);              // own ID is 1
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R0, 0);              // target ID is 0
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R1, 0);              // R1=0, pattern_id
    pm0_vec[ptr++] = MOVE(1, R3, 2);              // first 2 ping-pong
    pm0_vec[ptr++] = MOVE(1, R4, 0xAA);           // flag
    pm0_vec[ptr++] = MOVE(1, R5, 0);              // temp count
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern); // LIMIT
    pm0_vec[ptr++] = MOVE(1, R7, 0);              // m2s own state, from 0
    TAG_M2S_ACK0 = ptr;
    pm0_vec[ptr++] = POP(2, R0);        // wait write4D
    pm0_vec[ptr++] = PUSH(0, R1);       // trigger one pattern
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // move to next pattern
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_WAIT0);
    TAG_M2S_ACK1 = ptr;
    pm0_vec[ptr++] = POP(0, R0);                     // wait ack
    pm0_vec[ptr++] = PUSH(2, R7);                    // notify write4D/s2m
    pm0_vec[ptr++] = PUSH(1, R7);                    // notify s2S
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);              // increase state
    pm0_vec[ptr++] = JUMP(3, R7, R6, TAG_M2S_EXIT);  // all m2s is done, wait for u2S
    pm0_vec[ptr++] = JUMP(2, R7, R3, TAG_M2S_WAIT1); // [TAG_M2S_WAIT1]
    TAG_M2S_WAIT0 = ptr;
    pm0_vec[ptr++] = JUMP(5, 0, 0, TAG_M2S_ACK1);  // [TAG_M2S_ACK1]
    pm0_vec[ptr++] = JUMP(5, 2, 0, TAG_M2S_ACK0);  // [TAG_M2S_ACK0]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_WAIT0); // [TAG_M2S_WAIT0]

    TAG_M2S_ACK4 = ptr;
    pm0_vec[ptr++] = PUSH(0, R1);                  // trigger
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);            // next pattern
    pm0_vec[ptr++] = MOVE(1, R5, 0);               // clear the dependence
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_WAIT1); // [TAG_M2S_WAIT1]
    TAG_M2S_ACK2 = ptr;
    pm0_vec[ptr++] = MOVE(1, R0, 0x0A);
    pm0_vec[ptr++] = JUMP(3, R5, R0, TAG_M2S_WAIT1 + 2); // [TAG_M2S_ACK3]
    pm0_vec[ptr++] = POP(1, R0);                         // wait u2S
    pm0_vec[ptr++] = ADD(1, R5, R5, 0x0A);               // dependence +1
    pm0_vec[ptr++] = JUMP(4, R5, R4, TAG_M2S_WAIT1);     // [TAG_M2S_WAIT1]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_ACK4);        // [TAG_M2S_ACK4]
    TAG_M2S_ACK3 = ptr;
    pm0_vec[ptr++] = MOVE(1, R0, 0xA0);
    pm0_vec[ptr++] = JUMP(3, R5, R0, TAG_M2S_WAIT1);
    pm0_vec[ptr++] = POP(2, R0);                     //  wait write4D/m2s
    pm0_vec[ptr++] = ADD(1, R5, R5, 0xA0);           // dependence +1
    pm0_vec[ptr++] = JUMP(4, R5, R4, TAG_M2S_WAIT1); // [TAG_M2S_WAIT1]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_ACK4);    // [TAG_M2S_ACK4]
    TAG_M2S_WAIT1 = ptr;
    pm0_vec[ptr++] = JUMP(5, 0, 0, TAG_M2S_ACK1);  // wait ack
    pm0_vec[ptr++] = JUMP(5, 1, 0, TAG_M2S_ACK2);  // u2S ready
    pm0_vec[ptr++] = JUMP(5, 2, 0, TAG_M2S_ACK3);  // write4D/s2m ready
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_M2S_WAIT1); // [TAG_M2S_WAIT1]

    TAG_M2S_EXIT = ptr;
    pm0_vec[ptr++] = POP(1, R0); // wait last ack from u2S
    pm0_vec[ptr++] = MOVE(1, R1, num_of_pattern - 1);
    pm0_vec[ptr++] = JUMP(3, R0, R1, TAG_M2S_EXIT + 4); // the last ack from m2u later than the 2nd last ack from u2S
    pm0_vec[ptr++] = POP(1, R0);                        // wait last ack from u2S
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF);                 // end flag
    pm0_vec[ptr++] = PUSH(0, R0);                       // finish m2s
    pm0_vec[ptr++] = PUSH(2, R0);                       // finish sync
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    ptr = 0;
    uint32_t TAG_S2S_ACK0, TAG_S2S_ACK1;
    uint32_t TAG_S2S_WAIT = 11, TAG_S2S_EXIT = 14;
    pm1_vec[ptr++] = MOVE(1, R1, 0);              // R1=0, pattern_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern); // LIMIT
    pm1_vec[ptr++] = MOVE(1, R7, 0);              // s2s own state, from 0
    TAG_S2S_ACK0 = ptr;
    pm1_vec[ptr++] = POP(1, R0);                  // [TAG_S2S_ACK0]: wait m2s
    pm1_vec[ptr++] = PUSH(0, R1);                 // trigger
    pm1_vec[ptr++] = ADD(1, R1, R1, 1);           // point to next pattern
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_S2S_WAIT); //
    TAG_S2S_ACK1 = ptr;
    pm1_vec[ptr++] = POP(0, R0);                    // [TAG_S2S_ACK1]: wait ack
    pm1_vec[ptr++] = PUSH(1, R7);                   // notify m2s
    pm1_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm1_vec[ptr++] = JUMP(3, R7, R6, TAG_S2S_EXIT); // [TAG_S2S_EXIT] if no pattern left
    TAG_S2S_WAIT = ptr;
    pm1_vec[ptr++] = JUMP(5, 0, 0, TAG_S2S_ACK1); // [TAG_S2S_ACK1]
    pm1_vec[ptr++] = JUMP(5, 1, 0, TAG_S2S_ACK0); // [TAG_S2S_ACK0]
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_S2S_WAIT); // [TAG_S2S_WAIT]
    TAG_S2S_EXIT = ptr;
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm1_vec[ptr++] = PUSH(0, R0);       // finish s2s
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;
}

// Internal write4D
// Master s2u firstly feed the URAM by 2 4D patterns, and trigger s2m once the 1st pattern finished
// Slave s2m translate its 4D pattern and read the URAM to AXI-stream once receive trigger from m2s
// s2u & s2m access the URAM by ping-pong
//
// Work with read4D
// Master write4D/s2u firstly feed the ping & pong buffer by 2 4D patterns, and trigger read4D/m2s for each pattern
// Slave read4D/m2s start one DM once received trigger from write4D/s2u

// Master write4D/s2u exit if patterns in both s2m and s2u are finished
// Slave/Master s2m exit if patterns in both its own and read4D/m2s are finished
void pm_for_write4D(int num_of_pattern, uint32_t* pm0_vec, uint32_t* pm1_vec, uint32_t& pm0_sz, uint32_t& pm1_sz) {
    uint32_t ptr = 0;
    uint32_t TAG_S2M_ACK0, TAG_S2M_ACK1, TAG_S2M_ACK2, TAG_S2M_ACK3, TAG_S2M_ACK4;
    uint32_t TAG_S2M_WAIT0 = 22, TAG_S2M_WAIT1 = 41, TAG_S2M_EXIT = 45;
    pm0_vec[ptr++] = MOVE(1, R0, 1);              // Corp mode
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R0, 0);              // own ID is 0
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R0, 1);              // target ID is 1
    pm0_vec[ptr++] = PUSH(2, R0);                 // set
    pm0_vec[ptr++] = MOVE(1, R1, 0);              // R1=0, initial pattern_id
    pm0_vec[ptr++] = MOVE(1, R3, 2);              // first 2 ping-pong
    pm0_vec[ptr++] = MOVE(1, R4, 0xAA);           // flag
    pm0_vec[ptr++] = MOVE(1, R5, 0);              // temp count
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern); // LIMIT
    pm0_vec[ptr++] = MOVE(1, R7, 0);              // s2m own state, from 0
    TAG_S2M_ACK0 = ptr;
    pm0_vec[ptr++] = POP(1, R0);                   // [TAG_S2M_ACK0]: wait s2m
    pm0_vec[ptr++] = PUSH(0, R1);                  // trigger next pattern
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);            // point to next pattern
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_WAIT0); //
    TAG_S2M_ACK1 = ptr;
    pm0_vec[ptr++] = POP(0, R0);                     // [TAG_S2M_ACK1]: wait ack
    pm0_vec[ptr++] = PUSH(2, R7);                    // notify read4D/m2s
    pm0_vec[ptr++] = PUSH(1, R7);                    // notify s2u
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);              // increase state
    pm0_vec[ptr++] = JUMP(3, R7, R6, TAG_S2M_EXIT);  // all s2m is done, wait for read4D
    pm0_vec[ptr++] = JUMP(2, R7, R3, TAG_S2M_WAIT1); // until 2 patterns are egressed
    TAG_S2M_WAIT0 = ptr;
    pm0_vec[ptr++] = JUMP(5, 0, 0, TAG_S2M_ACK1);  // [TAG_S2M_ACK1]
    pm0_vec[ptr++] = JUMP(5, 1, 0, TAG_S2M_ACK0);  // [TAG_S2M_ACK0]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_WAIT0); // [TAG_S2M_WAIT0]

    TAG_S2M_ACK4 = ptr;
    pm0_vec[ptr++] = PUSH(0, R1);                  // trigger next pattern
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);            // point to next pattern
    pm0_vec[ptr++] = MOVE(1, R5, 0);               // clear the flag
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_WAIT1); // [TAG_S2M_WAIT1]
    TAG_S2M_ACK2 = ptr;
    pm0_vec[ptr++] = MOVE(1, R0, 0x0A);
    pm0_vec[ptr++] = JUMP(3, R5, R0, TAG_S2M_WAIT1 + 2); // [TAG_S2M_ACK3]
    pm0_vec[ptr++] = POP(1, R0);
    pm0_vec[ptr++] = ADD(1, R5, R5, 0x0A);           // dependence +1
    pm0_vec[ptr++] = JUMP(4, R5, R4, TAG_S2M_WAIT1); // [TAG_S2M_WAIT1]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_ACK4);    // [TAG_S2M_ACK4]
    TAG_S2M_ACK3 = ptr;
    pm0_vec[ptr++] = MOVE(1, R0, 0xA0);
    pm0_vec[ptr++] = JUMP(3, R5, R0, TAG_S2M_WAIT1);
    pm0_vec[ptr++] = POP(2, R0);
    pm0_vec[ptr++] = ADD(1, R5, R5, 0xA0);           // dependence +1
    pm0_vec[ptr++] = JUMP(4, R5, R4, TAG_S2M_WAIT1); // [TAG_S2M_WAIT1]
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_ACK4);    // [TAG_S2M_ACK4]
    TAG_S2M_WAIT1 = ptr;
    pm0_vec[ptr++] = JUMP(5, 0, 0, TAG_S2M_ACK1);  // wait ack
    pm0_vec[ptr++] = JUMP(5, 1, 0, TAG_S2M_ACK2);  // s2m ready
    pm0_vec[ptr++] = JUMP(5, 2, 0, TAG_S2M_ACK3);  // read4D/m2s ready
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2M_WAIT1); // [TAG_S2M_WAIT1]

    TAG_S2M_EXIT = ptr;
    pm0_vec[ptr++] = POP(2, R0);        // wait last ack from read4D
    pm0_vec[ptr++] = POP(2, R0);        // wait last ack from read4D
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm0_vec[ptr++] = PUSH(0, R0);       // finish s2m
    pm0_vec[ptr++] = PUSH(2, R0);       // finish sync
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    ptr = 0;
    uint32_t TAG_S2U_ACK0, TAG_S2U_ACK1;
    uint32_t TAG_S2U_WAIT = 15, TAG_S2U_EXIT = 18;
    pm1_vec[ptr++] = MOVE(1, R1, 0);                  // R1=0, pattern_id
    pm1_vec[ptr++] = PUSH(0, R1);                     // trigger ptn-0, [ping]
    pm1_vec[ptr++] = ADD(1, R1, R1, 1);               // increase ptn-id
    pm1_vec[ptr++] = PUSH(0, R1);                     // trigger ptn-1, [pong]
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern - 1); // LIMIT
    pm1_vec[ptr++] = MOVE(1, R7, 0);                  // s2u own state, from 0
    TAG_S2U_ACK0 = ptr;
    pm1_vec[ptr++] = POP(0, R0);                  // [TAG_S2U_ACK0]: wait ack
    pm1_vec[ptr++] = ADD(1, R7, R7, 1);           // increase state
    pm1_vec[ptr++] = PUSH(1, R7);                 // notify S2M
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_WAIT); // [TAG_S2U_WAIT]
    TAG_S2U_ACK1 = ptr;
    pm1_vec[ptr++] = POP(1, R0);                    // [TAG_S2U_ACK1]: wait s2m
    pm1_vec[ptr++] = JUMP(3, R0, R6, TAG_S2U_EXIT); // [EXIT] if s2m is done
    pm1_vec[ptr++] = JUMP(3, R1, R6, TAG_S2U_WAIT); // [TAG_S2U_WAIT] to wait last pattern
    pm1_vec[ptr++] = ADD(1, R1, R1, 1);             // next pattern
    pm1_vec[ptr++] = PUSH(0, R1);                   // trigger
    TAG_S2U_WAIT = ptr;
    pm1_vec[ptr++] = JUMP(5, 0, 0, TAG_S2U_ACK0); // [TAG_S2U_ACK0]
    pm1_vec[ptr++] = JUMP(5, 1, 0, TAG_S2U_ACK1); // [TAG_S2U_ACK1]
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_WAIT); // [TAG_S2U_WAIT]
    TAG_S2U_EXIT = ptr;
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm1_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    char* xclbinFilename = argv[1];
    auto device = xrt::device(0); // device index=0
    auto uuid = device.load_xclbin(xclbinFilename);
    xrt::kernel* dm0_krnl = new xrt::kernel(device, uuid, "s2mm_4d_hsk", true);
    xrt::kernel* dm1_krnl = new xrt::kernel(device, uuid, "mm2s_4d_hsk", true);
    xrt::kernel* sink0_krnl = new xrt::kernel(device, uuid, "mm2s_mp", true);
    xrt::kernel* sink1_krnl = new xrt::kernel(device, uuid, "s2mm_mp", true);

    int cfg_sz = 256 * 1024 * sizeof(uint32_t); // 1MB
    int data_sz = 128 * 1024 * 1024;            // 128MB
    xrt::bo* cfg0_bo = new xrt::bo(device, cfg_sz, dm0_krnl->group_id(0));
    xrt::bo* cfg1_bo = new xrt::bo(device, cfg_sz, dm1_krnl->group_id(0));
    xrt::bo* dm_data_bo = new xrt::bo(device, data_sz, dm0_krnl->group_id(1));
    xrt::bo* sink0_in_bo = new xrt::bo(device, data_sz, sink0_krnl->group_id(0));
    xrt::bo* sink1_out_bo = new xrt::bo(device, data_sz, sink1_krnl->group_id(1));
    auto cfg0_buf = cfg0_bo->map<uint32_t*>();
    auto cfg1_buf = cfg1_bo->map<uint32_t*>();
    auto sink0_buf = sink0_in_bo->map<uint64_t*>();

    const int num_of_pattern = 4;
    int sum_of_all_pattern = 0;
    int* pattern_ddr = (int*)malloc(num_of_pattern * 24 * sizeof(int));
    int* pattern_uram = (int*)malloc(num_of_pattern * 24 * sizeof(int));
    // pattern generation
    unsigned int pp_offset = data_sz / (2 * sizeof(uint64_t)); // 64MB for ping/pong
    for (int i = 0; i < num_of_pattern; i++) {
        int* cur_ptn_m2s = pattern_ddr + i * 24;
        int* cur_ptn_s2m = pattern_uram + i * 24;
        std::cout << "************Pattern-" << i << "************" << std::endl;
        int elem_in_pattern = pattern_gen(1, cur_ptn_m2s);
        populate_data_gen(cur_ptn_m2s, sink0_buf + sum_of_all_pattern);
        if (i % 2 == 1) cur_ptn_m2s[4] += pp_offset;
        flatten_4D_pattern(elem_in_pattern, cur_ptn_m2s, cur_ptn_s2m);
        sum_of_all_pattern += elem_in_pattern;
        std::cout << "Total element: " << elem_in_pattern << std::endl;
    }
    // program
    uint32_t pm0_sz, pm1_sz;
    uint32_t* pm0 = (uint32_t*)malloc(1024 * sizeof(uint32_t));
    uint32_t* pm1 = (uint32_t*)malloc(1024 * sizeof(uint32_t));
    for (int i = 0; i < 2; i++) {
        uint32_t* cfg_buf;
        if (i == 0) {
            cfg_buf = cfg0_buf;
            pm_for_write4D(num_of_pattern, pm0, pm1, pm0_sz, pm1_sz);
        } else if (i == 1) {
            cfg_buf = cfg1_buf;
            pm_for_read4D(num_of_pattern, pm0, pm1, pm0_sz, pm1_sz);
        }

        // config m2s pattern
        cfg_buf[0] = num_of_pattern * 24;
        std::memcpy(cfg_buf + 2, pattern_ddr, num_of_pattern * 24 * sizeof(uint32_t));

        // config s2m pattern
        cfg_buf[2 + num_of_pattern * 24] = num_of_pattern * 24;
        std::memcpy(cfg_buf + 2 * 2 + num_of_pattern * 24, pattern_uram, num_of_pattern * 24 * sizeof(uint32_t));

        // config m2s program
        cfg_buf[2 * 2 + 2 * num_of_pattern * 24] = pm0_sz;
        std::memcpy(cfg_buf + 3 * 2 + 2 * num_of_pattern * 24, pm0, pm0_sz * sizeof(uint32_t));

        // config s2m program
        if (pm0_sz % 2 != 0) pm0_sz += 1;
        cfg_buf[3 * 2 + 2 * num_of_pattern * 24 + pm0_sz] = pm1_sz;
        std::memcpy(cfg_buf + 4 * 2 + 2 * num_of_pattern * 24 + pm0_sz, pm1, pm1_sz * sizeof(uint32_t));
    }
    free(pm0);
    free(pm1);

    cfg0_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, cfg_sz, /*OFFSET=*/0);
    cfg1_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, cfg_sz, /*OFFSET=*/0);
    sink0_in_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, data_sz, /*OFFSET=*/0);
    sum_of_all_pattern *= sizeof(uint64_t);

    /*
// set Arg
    auto run_dm0 = new xrt::run(*dm0_krnl); // write4D
    auto run_dm1 = new xrt::run(*dm1_krnl); // read4D
    auto run_sk0 = new xrt::run(*sink0_krnl); // mm2s
    auto run_sk1 = new xrt::run(*sink1_krnl); // s2mm
    run_dm0->set_arg(0, cfg0_bo);
    run_dm0->set_arg(1, dm_data_bo);
    run_sk0->set_arg(0, sink0_in_bo);
    run_sk0->set_arg(2, sum_of_all_pattern);
    run_dm1->set_arg(0, cfg1_bo);
    run_dm1->set_arg(1, dm_data_bo);
    run_sk1->set_arg(1, sink1_out_bo);
    run_sk1->set_arg(2, sum_of_all_pattern);

// start kernel
    run_dm0->start();
    run_dm1->start();
    run_sk0->start();
    run_sk1->start();

    // wait done
    run_dm0->wait();
    run_dm1->wait();
    run_sk0->wait();
    run_sk1->wait();
    */

    uint64_t cfg0_bo_addr = cfg0_bo->address();
    uint64_t cfg1_bo_addr = cfg1_bo->address();
    uint64_t dm_data_bo_addr = dm_data_bo->address();
    uint64_t sink_in_bo_addr = sink0_in_bo->address();
    uint64_t sink_out_bo_addr = sink1_out_bo->address();
    dm0_krnl->write_register(0x10, cfg0_bo_addr);
    dm0_krnl->write_register(0x14, (cfg0_bo_addr >> 32));
    dm0_krnl->write_register(0x1C, dm_data_bo_addr);
    dm0_krnl->write_register(0x20, (dm_data_bo_addr >> 32));
    sink0_krnl->write_register(0x1C, sum_of_all_pattern); // num of data to produce
    sink0_krnl->write_register(0x10, sink_in_bo_addr);
    sink0_krnl->write_register(0x14, (sink_in_bo_addr >> 32));
    dm1_krnl->write_register(0x10, cfg1_bo_addr);
    dm1_krnl->write_register(0x14, (cfg1_bo_addr >> 32));
    dm1_krnl->write_register(0x1C, dm_data_bo_addr);
    dm1_krnl->write_register(0x20, (dm_data_bo_addr >> 32));
    sink1_krnl->write_register(0x1C, sum_of_all_pattern); // num of data to consume
    sink1_krnl->write_register(0x10, sink_out_bo_addr);
    sink1_krnl->write_register(0x14, (sink_out_bo_addr >> 32));

    // start kernel
    dm0_krnl->write_register(0x0, 0x1);   // ap_start
    dm1_krnl->write_register(0x0, 0x1);   // ap_start
    sink0_krnl->write_register(0x0, 0x1); // ap_start
    sink1_krnl->write_register(0x0, 0x1); // ap_start
    while (true) {
        uint32_t status = dm0_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink0_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = dm1_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink1_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    dm0_krnl->write_register(0x10, 0x1);   // ap_continue
    sink0_krnl->write_register(0x10, 0x1); // ap_continue
    dm1_krnl->write_register(0x10, 0x1);   // ap_continue
    sink1_krnl->write_register(0x10, 0x1); // ap_continue

    // check result
    int total_nerr = 0;
    int hw_ptr = 0;
    sink1_out_bo->sync(XCL_BO_SYNC_BO_FROM_DEVICE, data_sz, /*OFFSET=*/0);
    auto sink_out_buf = sink1_out_bo->map<uint64_t*>();
    for (int p = 0; p < num_of_pattern; p++) {
        int* current_ptn = pattern_ddr + 24 * p;
        int* buf_dim = current_ptn;
        int* offset = current_ptn + 4;
        int* tiling = current_ptn + 8;
        int* dim_idx = current_ptn + 12;
        int* stride = current_ptn + 16;
        int* wrap = current_ptn + 20;
        if (p % 2 == 1) offset[0] -= pp_offset;
        int nerr = 0;
        for (int w = 0; w < wrap[dim_idx[3]]; w++) {
            for (int z = 0; z < wrap[dim_idx[2]]; z++) {
                for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                    for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                        int bias[4];
                        bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;
                        bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                        bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                        bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
                        for (int l = 0; l < tiling[3]; l++) {
                            for (int k = 0; k < tiling[2]; k++) {
                                for (int j = 0; j < tiling[1]; j++) {
                                    for (int i = 0; i < tiling[0]; i++) {
                                        uint64_t golden = (bias[3] + l) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                          (bias[2] + k) * (buf_dim[1] * buf_dim[0]) +
                                                          (bias[1] + j) * buf_dim[0] + (bias[0] + i);
                                        uint64_t hw_result = sink_out_buf[hw_ptr++];
                                        if (golden != hw_result) nerr++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        total_nerr += nerr;
        if (nerr == 0)
            std::cout << "Check passed for pattern " << p << std::endl;
        else
            std::cout << "nerr:" << nerr << std::endl;
    }

    free(pattern_ddr);
    free(pattern_uram);

    delete cfg0_bo;
    delete cfg1_bo;
    delete dm_data_bo;
    delete sink0_in_bo;
    delete sink1_out_bo;
    delete dm0_krnl;
    delete dm1_krnl;
    delete sink0_krnl;
    delete sink1_krnl;
    // delete run_dm0;
    // delete run_dm1;
    // delete run_sk0;
    // delete run_sk1;

    return total_nerr;
}
