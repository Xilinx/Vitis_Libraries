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
#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include "xf_data_mover/bi_pl_4d_data_mover.hpp"

void dut(ap_uint<32>* pattern_0,
         ap_uint<32>* pattern_1,
         ap_uint<32>* pattern_2,
         ap_uint<32>* pattern_3,
         ap_uint<32>* pm_0,
         ap_uint<32>* pm_1,
         ap_uint<32>* pm_2,
         ap_uint<32>* pm_3) {
    std::cout << "pattern_0=" << pattern_0 << std::endl;
    std::cout << "pattern_1=" << pattern_1 << std::endl;
    std::cout << "pattern_2=" << pattern_2 << std::endl;
    std::cout << "pattern_3=" << pattern_3 << std::endl;
    std::cout << "pm_0=" << pm_0 << std::endl;
    std::cout << "pm_1=" << pm_1 << std::endl;
    std::cout << "pm_2=" << pm_2 << std::endl;
    std::cout << "pm_3=" << pm_3 << std::endl;

#pragma HLS dataflow

    xf::data_mover::bi_details::bi_dm<64, 65536, 16, 32, 16>(pattern_0, pattern_1, pattern_2, pattern_3, pm_0, pm_1,
                                                             pm_2, pm_3);
}

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
int pattern_gen(const int num_of_pattern, int* pattern_vec, int* uram_offset) {
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
        std::memcpy(pattern_vec + p * 25, uram_offset + p, 1 * sizeof(int)); // write URAM access address into ROM
        std::memcpy(pattern_vec + p * 25 + 1, ptn_template, 24 * sizeof(int));
        sum_elem += (ptn_template[8] * ptn_template[9] * ptn_template[10] * ptn_template[11] * ptn_template[20] *
                     ptn_template[21] * ptn_template[22] * ptn_template[23]);
        // print the 4D pattern
        std::cout << "******************generated 4D-patterns in host******************" << std::endl;
        std::cout << "URAM access addr: 0x" << std::hex << pattern_vec[p * 25] << std::endl;
        std::cout << "buffer_dim[4]={" << pattern_vec[p * 25 + 1] << "," << pattern_vec[p * 25 + 2] << ","
                  << pattern_vec[p * 25 + 3] << "," << pattern_vec[p * 25 + 4] << "}" << std::endl;
        std::cout << "buffer_offset[4]={" << pattern_vec[p * 25 + 5] << "," << pattern_vec[p * 25 + 6] << ","
                  << pattern_vec[p * 25 + 7] << "," << pattern_vec[p * 25 + 8] << "}" << std::endl;
        std::cout << "tiling[4]={" << pattern_vec[p * 25 + 9] << "," << pattern_vec[p * 25 + 10] << ","
                  << pattern_vec[p * 25 + 11] << "," << pattern_vec[p * 25 + 12] << "}" << std::endl;
        std::cout << "dim_idx[4]={" << pattern_vec[p * 25 + 13] << "," << pattern_vec[p * 25 + 14] << ","
                  << pattern_vec[p * 25 + 15] << "," << pattern_vec[p * 25 + 16] << "}" << std::endl;
        std::cout << "stride[4]={" << pattern_vec[p * 25 + 17] << "," << pattern_vec[p * 25 + 18] << ","
                  << pattern_vec[p * 25 + 19] << "," << pattern_vec[p * 25 + 20] << "}" << std::endl;
        std::cout << "wrap[4]={" << pattern_vec[p * 25 + 21] << "," << pattern_vec[p * 25 + 22] << ","
                  << pattern_vec[p * 25 + 23] << "," << pattern_vec[p * 25 + 24] << "}" << std::endl;
        p++;
    }

    return sum_elem;
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
// mode=1, R[rd]=ctrl.read()
uint32_t POP(int mode, int rd) {
    return ((0x1 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8));
}

// mode=0, ptn.write(R[rs])
// mode=1, ctrl.write(R[rs])
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

/*
 *    To test handshake only, we assume all acks are received,
 *    so ack CMDs are all commentted.[e.g. PUSH(0,x) and POP(0,x) ]
 */

/* test handshake [case_0: ctrls just exit, no dependences]
In this case, no pattern is parsed in DM, ctrls inside DM just bypass EXIT signal
to the others. Once some ctrl knows all the others are ready to exit,
this ctrl just kill itself and broadcast it's death signal to the others.
In this case, the sequence of EXIT is random.
*/
void pm_gen_case_0(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    /* pm0: ALU0 */
    uint32_t ptr = 0;
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x02); // R0=0x4A ( 0x4<<4 | 0xA=0x4A ),  # source_ID=0, target_id=2(ctrl-2)
    pm0_vec[ptr++] = PUSH(1, R0);              // ctrl.write(R0)

    pm0_vec[ptr++] = MOVE(1, R0, 0xFE); // update target-id to ctrl-1
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);
    pm0_vec[ptr++] = PUSH(1, R0);

    // update end flag and trigger EXIT
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x03); // R0=0x13, # source_ID=5, target_id=11(ctrl-3)
    pm1_vec[ptr++] = PUSH(1, R0);              // dm.write(R0)

    // Do nothing, just bypass. After all the other ctrls idle, kill itself.

    // update end flag and trigger EXIT
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x00); // R0=0xA4, # source_ID=10, target_id=4(ctrl-0)
    pm2_vec[ptr++] = PUSH(1, R0);              // dm.write(R0)

    // update end flag and trigger EXIT
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x01); // R0=0xB4, # source_ID=11, target_id=4(ctrl-0)
    pm3_vec[ptr++] = PUSH(1, R0);              // dm.write(R0)

    // Do nothing, just bypass. After all the other ctrls idle, kill itself.

    // update end flag and trigger EXIT
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT();
    pm3_sz = ptr;
}

/* test handshake [case_1: multiple patterns, operations with pendings/dependences]
In this case, axis2cache unit has 5 patterns(ptn-0 ~ ptn-4), cache2axim has 1 pattern, cache2axis has 3 patterns,
and axim2cache has no pattern.

As for axis2cache, the first ptn is disguarded as rubbish(it means the data described in ptn-0
is going to be written into cache and no one would read it), and the 2nd-4th patterns are parsed and writed to cache,
Once some ptn of them is parsed and writen, a ready signal will be sent to cache2axis unit.

As for cache2axis, once it received a ready signal from axis2cache, it will do the data-moving work from cache to axis.

After all 3 patterns between axis2cache and cache2axis are done, axis2cache will trigger the last pattern. After this,
cache2axim will move the data of the last pattern to ddr.

After processing  all the patterns, the ctrls will exit in some sequence.
*/
void pm_gen_case_1(const int* num_of_pattern, // int array, fixed size = 5
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-axis&axis-axim ##" << std::endl;

    uint32_t TAG_S2U_ACK0, TAG_S2U_ACK1 = 12 - 3, TAG_S2U_ACK2 = 19 - 5, TAG_S2U_ACK3 = 28 - 6, TAG_S2U_EXIT = 32 - 7,
                           TAG_U2M_ACK0, TAG_U2M_ACK1 = 10 - 1, TAG_U2M_EXIT = 12 - 2, TAG_U2S_ACK0 = 6,
                           TAG_U2S_WAIT0 = 14 - 2, TAG_U2S_EXIT = 16 - 2, TAG_M2U_EXIT = 7;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    // init REG
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x02);           // R0=0x02,  # source_ID=0, target_id=2(ctrl-2)
    pm0_vec[ptr++] = PUSH(1, R0);                        // ctrl.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                     // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]);     // count for ptn
    pm0_vec[ptr++] = MOVE(1, R5, num_of_pattern[0] - 2); // limit for ptn-id
    pm0_vec[ptr++] = MOVE(1, R7, 0);                     // s2u own state, from 0
    // pm0_vec[ptr++] = PUSH(0, R1);                 // trigger ptn-0

    // let ptn-0 be rubbish, just ignore it
    TAG_S2U_ACK0 = ptr;
    // 7(6)
    //  pm0_vec[ptr++] = POP(0, R0);                   // [TAG_S2U_ACK0]: wait ack of ptn-0
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    // pm0_vec[ptr++] = PUSH(0, R1);                  // trigger ptn-1
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK1);

    TAG_S2U_ACK1 = ptr;
    // 12(9)
    //  pm0_vec[ptr++] = POP(0, R0);                     // [TAG_S2U_ACK1]: wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);             // 0x107723     // increase state
    pm0_vec[ptr++] = PUSH(1, R7);                   // notify u2s
    pm0_vec[ptr++] = JUMP(3, R1, R5, TAG_S2U_ACK2); // 0xd5764// until ptn-2,3 are egressed
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    // pm0_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-2,3
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK1);

    // wait u2s all done
    TAG_S2U_ACK2 = ptr;
    // 19(14)
    pm0_vec[ptr++] = POP(1, R0);        // 0x21             // [TAG_S2U_ACK2]: wait u2s
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm0_vec[ptr++] = MOVE(1, R0, 0xFE); // update target-id to ctrl-1
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    // pm0_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-4(the last)
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK3);

    // wait s2u last ptn
    TAG_S2U_ACK3 = ptr;
    // 28(22)
    //  pm0_vec[ptr++] = POP(0, R0);                     // [TAG_S2U_ACK3]: wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);           // increase state
    pm0_vec[ptr++] = PUSH(1, R7);                 // notify u2m(ctrl-1)
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_EXIT); // jump to exit

    TAG_S2U_EXIT = ptr;
    // 32(25)
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x10, # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    TAG_U2M_ACK0 = ptr;
    // 5
    pm1_vec[ptr++] = POP(1, R0);        // [TAG_U2M_ACK0]: wait signal from ctrl-0
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    // pm1_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    pm1_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm1_vec[ptr++] = JUMP(1, R7, R6, TAG_U2M_ACK1);

    TAG_U2M_ACK1 = ptr;
    // 10(9)
    //  pm1_vec[ptr++] = POP(0, R0);                     // [TAG_U2M_ACK0]: wait ack
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_EXIT);

    TAG_U2M_EXIT = ptr;
    // 12(10)
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;
    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x00);       // R0=0xA4, # source_ID=2, target_id=0(ctrl-0)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 2);                 // count for ptn
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    pm2_vec[ptr++] = POP(1, R0);        // 0x21               // [TAG_U2S_ACK0]: wait signal from ctrl-0
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // 0x107723    // increase state
    // pm2_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0,1,2
    // pm2_vec[ptr++] = POP(0, R0);                     // [TAG_U2S_ACK0]: wait ack
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);              // increase state
    pm2_vec[ptr++] = JUMP(3, R1, R5, TAG_U2S_WAIT0); // until ptn-2 are egressed
    pm2_vec[ptr++] = ADD(1, R1, R1, 1);              // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_WAIT0 = ptr;
    // 14(12)
    pm2_vec[ptr++] = PUSH(1, R7); // send end signal to ctrl-0
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_EXIT);

    TAG_U2S_EXIT = ptr;
    // update end flag and trigger EXIT
    // 16(14)
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x01); // R0=0x31, # source_ID=3, target_id=1(ctrl-1)
    pm3_vec[ptr++] = PUSH(1, R0);              // dm.write(R0)

    pm3_vec[ptr++] = MOVE(1, R0, 0xFE); // update target-id to ctrl-1
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x02);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_EXIT);

    // update end flag and trigger EXIT
    TAG_M2U_EXIT = ptr;
    // 7
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT();
    pm3_sz = ptr;
}

/* test handshake [case_2: multiple patterns, operations with pendings/dependences]
In this case, axis2cache unit has no pattern, cache2axim has 3 pattern, cache2axis has 1 pattern,
and axim2cache has 5 patterns(ptn-0 ~ ptn-4)).

This case is similar to [case_1], the only difference is that the data-flow-path changes from
"axis2axis, then axis2axim" to "axim2axim, then axim2axis".
*/
void pm_gen_case_2(const int* num_of_pattern, // int array, fixed size = 5
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axim-axim&axim-axis ##" << std::endl;

    uint32_t TAG_S2U_EXIT = 3, TAG_U2M_ACK0, TAG_U2M_EXIT = 15 - 2, TAG_U2S_ACK0 = 6, TAG_U2S_WAIT0 = 15 - 2,
             TAG_U2S_EXIT = 17 - 2, TAG_M2U_ACK0, TAG_M2U_ACK1 = 13 - 3, TAG_M2U_ACK2 = 21 - 5, TAG_M2U_ACK3 = 31 - 6,
             TAG_M2U_ACK4 = 36 - 7, TAG_M2U_EXIT = 38 - 7;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);    // R0=0x01, # source_ID=0, target_id=1(ctrl-1)
    pm0_vec[ptr++] = PUSH(1, R0);                 // dm.write(R0)
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_EXIT); // dm.write(R0)

    /* Do nothing, just bypass handshakes */

    // update end flag and trigger EXIT
    TAG_S2U_EXIT = ptr;
    // 3
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x03);       // R0=0x13, # source_ID=1, target_id=3(ctrl-3)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    TAG_U2M_ACK0 = ptr;
    // 6
    pm1_vec[ptr++] = POP(1, R0);        // [TAG_U2M_ACK0]: wait signal from ctrl-3
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    // pm1_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    // pm1_vec[ptr++] = POP(0, R0);                     // wait ack
    pm1_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm1_vec[ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT);
    pm1_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

    TAG_U2M_EXIT = ptr;
    // 15(13)
    pm1_vec[ptr++] = PUSH(1, R7);       // u2m all done, notify ctrl-3
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);       // finish u2m
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;
    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x03);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_fininshed
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    pm2_vec[ptr++] = POP(1, R0);        // 0x21      // [TAG_U2S_ACK0]: wait signal from ctrl-3
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // 0x107723  // increase state
    // pm2_vec[ptr++] = PUSH(0, R1);                 // trigger ptn-0
    // pm2_vec[ptr++] = POP(0, R0);                  // wait ack
    pm2_vec[ptr++] = ADD(1, R5, R5, 1);              // count_for_ptn++
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);              // increase state
    pm2_vec[ptr++] = JUMP(3, R5, R6, TAG_U2S_WAIT0); // until ptn-0 are egressed
    pm2_vec[ptr++] = ADD(1, R1, R1, 1);              // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_WAIT0 = ptr;
    // 15(13)
    pm2_vec[ptr++] = PUSH(1, R7); // send end signal to ctrl-3
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_EXIT);

    TAG_U2S_EXIT = ptr;
    // update end flag and trigger EXIT
    // 17(15)
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x01);           // R0=0x31,  # source_ID=3, target_id=1(ctrl-1)
    pm3_vec[ptr++] = PUSH(1, R0);                        // ctrl.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                     // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                     // count_for_ptn_finished
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3] - 1); // limit for ptn-id
    pm3_vec[ptr++] = MOVE(1, R7, 0);                     // m2m own state, from 0

    // let ptn-0 be rubbish, just ignore it
    TAG_M2U_ACK0 = ptr;
    // 6
    //  pm3_vec[ptr++] = PUSH(0, R1);                  // trigger ptn-0
    //  pm3_vec[ptr++] = POP(0, R0);                   // [TAG_M2M_ACK0]: wait ack of ptn-0
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    // pm3_vec[ptr++] = PUSH(0, R1);                   // trigger ptn-1
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK1);

    TAG_M2U_ACK1 = ptr;
    // 13(10)
    //  pm3_vec[ptr++] = POP(0, R0);                     // [TAG_M2U_ACK1]: wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1);             // 0x107723     // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm3_vec[ptr++] = PUSH(1, R7);                   // notify u2m
    pm3_vec[ptr++] = JUMP(3, R5, R6, TAG_M2U_ACK2); // 0xd5764// until ptn-2,3 are egressed
    pm3_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    // pm3_vec[ptr++] = PUSH(0, R1);                   // trigger ptn-2,3
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK1);

    // wait u2m all done
    TAG_M2U_ACK2 = ptr;
    // 21(16)
    pm3_vec[ptr++] = POP(1, R0);        // 0x21             // [TAG_M2M_ACK2]: wait u2m all done
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm3_vec[ptr++] = MOVE(1, R0, 0xFE); // update target-id to ctrl-2
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x02);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    // pm3_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-4(the last)
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK3);

    // wait m2s last ptn
    TAG_M2U_ACK3 = ptr;
    // 31(25)
    //  pm3_vec[ptr++] = POP(0, R0);                     // [TAG_U2S_ACK3]: wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = PUSH(1, R7);       // notify u2s(ctrl-2)
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK4);

    TAG_M2U_ACK4 = ptr;
    // 36(29)
    pm3_vec[ptr++] = POP(1, R0);                  // 0x21                // [TAG_M2M_ACK4]: wait u2s all done
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_EXIT); // jump to exit

    TAG_M2U_EXIT = ptr;
    // 38(31)
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT();
    pm3_sz = ptr;
}

/* test handshake [case_3: write into cache from both AIE and DDR, no dependences between them]
In this case, axis2cache and axim2cache will write data into cache independently according to
the patterns of themselves.
*/
void pm_gen_case_3(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-cache&axim-cache ##" << std::endl;

    uint32_t TAG_S2U_ACK0, TAG_S2U_EXIT = 13 - 2, TAG_U2M_ACK0, TAG_U2M_EXIT = 13 - 2;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    // init REG
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x03);       // R0=0x01, # source_ID=0, target_id=3(ctrl-3)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // s2u own state, from 0

    // trigger patterns
    TAG_S2U_ACK0 = ptr;
    // 6
    // pm0_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    // pm0_vec[ptr++] = POP(0, R0);                     // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm0_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm0_vec[ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT);
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    // idle, waiting for exit
    TAG_S2U_EXIT = ptr;
    // 13(11)
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x13, # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);       // finish u2m
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;
    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x03);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_fininshed
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);       // finish u2m
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x00);       // R0=0x31,  # source_ID=3, target_id=0(ctrl-0)
    pm3_vec[ptr++] = PUSH(1, R0);                    // ctrl.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                 // count_for_ptn_finished
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3]); // limit for ptn-id
    pm3_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    TAG_U2M_ACK0 = ptr;
    // 6
    //  pm3_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    //  pm3_vec[ptr++] = POP(0, R0);                     // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT);
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

    TAG_U2M_EXIT = ptr;
    // 13(11)
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);       // finish m2u
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT();
    pm3_sz = ptr;
}

/* test handshake [case_4: read data from cache to both AIE and DDR, no dependences between them]
In this case, cache2axis and cache2axim will read data from cache into AIE and DDR
independently according to the patterns of themselves.
*/
void pm_gen_case_4(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for cache-axis&cache-axis ##" << std::endl;

    uint32_t TAG_U2M_ACK0, TAG_U2M_EXIT = 13 - 2, TAG_U2S_ACK0, TAG_U2S_EXIT = 13 - 2;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    // init REG
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x03);       // R0=0x01, # source_ID=0, target_id=3(ctrl-3)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // s2u own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x13, # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    // trigger patterns
    TAG_U2M_ACK0 = ptr;
    // 6
    // pm1_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    // pm1_vec[ptr++] = POP(0, R0);                     // wait ack
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm1_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm1_vec[ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT);
    pm1_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

    TAG_U2M_EXIT = ptr;
    // 13(11)
    // idle, waiting for exit
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);       // finish u2m
    pm1_vec[ptr++] = PUSH(1, R0); // finish u2m
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;
    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x03);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_fininshed
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    // pm2_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0
    // pm2_vec[ptr++] = POP(0, R0);                     // wait ack
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm2_vec[ptr++] = JUMP(3, R5, R6, TAG_U2S_EXIT);
    pm2_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_EXIT = ptr;
    // 13(11)
    // idle, waiting for exit
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);       // finish u2m
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x00);       // R0=0x31,  # source_ID=3, target_id=0(ctrl-0)
    pm3_vec[ptr++] = PUSH(1, R0);                    // ctrl.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                 // count_for_ptn_finished
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3]); // limit for ptn-id
    pm3_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    /* Do nothing, just bypass handshakes */

    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);       // finish m2u
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT();
    pm3_sz = ptr;
}

/* test handshake [case_5: write data into cache from AIE, and move target data of cache into DDR]
In this case, axis2cache will read data from AIE and write them into cache. While cache2axim will read some of
data writed into cache by axis2cache, and write these data into DDR.

axis2cache and cache2axim are working in parallel.
*/
void pm_gen_case_5(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-cache&cache-axim ##" << std::endl;

    uint32_t TAG_S2U_ACK0 = 13 - 3, TAG_S2U_EXIT = 21 - 5, TAG_U2M_ACK0, TAG_U2M_EXIT = 15 - 2;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);       // R0=0x01, # source_ID=0, target_id=1(ctrl-1)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // u2c own state, from 0

    // let ptn-0 be rubbish, just ignore it
    // 6
    // pm0_vec[ptr++] = PUSH(0, R1);                  // trigger ptn-0
    // pm0_vec[ptr++] = POP(0, R0);                   // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm0_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    // pm0_vec[ptr++] = PUSH(0, R1);                   // trigger ptn-1
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_ACK0 = ptr;
    // 13(10)
    //  pm0_vec[ptr++] = POP(0, R0);                // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm0_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm0_vec[ptr++] = PUSH(1, R7);                   // notify u2m
    pm0_vec[ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT); // until all patterns are egressed
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    // pm0_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_EXIT = ptr;
    // 21(16)
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);       // finish s2u
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x10, # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    TAG_U2M_ACK0 = ptr;
    // 6
    pm1_vec[ptr++] = POP(1, R0);        // wait signal from ctrl-0
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    // pm1_vec[ptr++] = PUSH(0, R1);                    // trigger ptn-0,1,2
    // pm1_vec[ptr++] = POP(0, R0);                     // wait ack
    pm1_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm1_vec[ptr++] = ADD(1, R5, R5, 1);             // count_of_ptn++
    pm1_vec[ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT); // until ptn-2 are egressed
    pm1_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

    TAG_U2M_EXIT = ptr;
    // update end flag and trigger EXIT
    // 15(13)
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;

    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x03);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_fininshed
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // s2u own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT(); // finish s2u
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;
    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x01);       // R0=0x31,  # source_ID=3, target_id=1(ctrl-1)
    pm3_vec[ptr++] = PUSH(1, R0);                    // ctrl.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                 // count_for_ptn_finished
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3]); // limit for ptn-id
    pm3_vec[ptr++] = MOVE(1, R7, 0);                 // m2m own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish u2m
    pm3_sz = ptr;
}

/* test handshake [case_6: write data into cache from DDR, and move target data of cache into AIE]
In this case, axim2cache will read data from DDR and write them into cache. While cache2axis will read some of
data writed into cache by axim2cache, and write these data into AIE.

axim2cache and cache2axis are working in parallel.
*/
void pm_gen_case_6(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axim-cache&cache-axis ##" << std::endl;

    uint32_t TAG_U2S_ACK0, TAG_U2S_EXIT = 15 - 2, TAG_M2U_ACK0 = 13 - 3, TAG_M2U_WAIT = 21 - 5, TAG_M2U_EXIT = 25 - 7;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    // init REG
    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);       // R0=0x01, # source_ID=0, target_id=1(ctrl-1)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count_for_ptn_fininshed
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // s2u own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);
    pm0_vec[ptr++] = PUSH(1, R0);
    pm0_vec[ptr++] = EXIT(); // finish s2u
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x10,  # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // ctrl.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // limit for ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count_for_ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT(); // finish u2m
    pm1_sz = ptr;

    /* pm0: ALU2 */
    ptr = 0;

    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x03);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    pm2_vec[ptr++] = POP(1, R0);        // wait signal from ctrl-3
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    // pm2_vec[ptr++] = PUSH(0, R1);                    // trigger next ptn
    // pm2_vec[ptr++] = POP(0, R0);                     // wait ack
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm2_vec[ptr++] = ADD(1, R5, R5, 1);             // count_of_ptn++
    pm2_vec[ptr++] = JUMP(3, R5, R6, TAG_U2S_EXIT); // until ptn-2 are egressed
    pm2_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_EXIT = ptr;
    // update end flag and trigger EXIT
    // 15(13)
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;

    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x02);           // R0=0x32, # source_ID=3, target_id=2(ctrl-2)
    pm3_vec[ptr++] = PUSH(1, R0);                        // dm.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                     // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3] - 1); // LIMIT of ptn-id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                     // count for ptn_finished
    pm3_vec[ptr++] = MOVE(1, R7, 0);                     // m2u own state, from 0

    // let ptn-0 be rubbish, just ignore it
    // 6
    // pm3_vec[ptr++] = PUSH(0, R1);                  // trigger ptn-0
    // pm3_vec[ptr++] = POP(0, R0);                   // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    // pm3_vec[ptr++] = PUSH(0, R1);                   // trigger ptn-1
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

    TAG_M2U_ACK0 = ptr;
    // 13(10)
    //  pm3_vec[ptr++] = POP(0, R0);                // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm3_vec[ptr++] = PUSH(1, R7);                   // notify u2s
    pm3_vec[ptr++] = JUMP(3, R5, R6, TAG_M2U_WAIT); // until all patterns are egressed
    pm3_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    // pm3_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

    // 21(16)
    TAG_M2U_WAIT = ptr;
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    // pm3_vec[ptr++] = PUSH(0, R1);              // trigger next ptn
    // pm3_vec[ptr++] = POP(0, R0);               // wait ack
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_EXIT);

    // 25(18)
    TAG_M2U_EXIT = ptr;
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish m2u
    pm3_sz = ptr;
}

void pm_gen_case_7(const int* num_of_pattern, // int array, fixed size = 5
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-axis&axis-axim ##" << std::endl;

    uint32_t TAG_S2U_ACK0, TAG_S2U_EXIT = 14 - 2, TAG_U2M_ACK0, TAG_U2M_EXIT = 14 - 2, TAG_U2S_ACK0,
                           TAG_U2S_EXIT = 14 - 2, TAG_M2U_ACK0, TAG_M2U_EXIT = 14 - 2;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x01);       // R0=0x01, # source_ID=0, target_id=1(ctrl-1)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // u2c own state, from 0

    TAG_S2U_ACK0 = ptr;
    // 6
    // pm0_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    // pm0_vec[ptr++] = POP(0, R1);                    // wait ack
    pm0_vec[ptr++] = PUSH(1, R1);                   // notify u2m
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm0_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm0_vec[ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT); // until all patterns are egressed
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_EXIT = ptr;
    // 14-2
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm0_vec[ptr++] = PUSH(0, R0);
    pm0_vec[ptr++] = PUSH(1, R0); // finish s2u
    pm0_vec[ptr++] = EXIT();
    pm0_sz = ptr;

    /* pm1: ALU1 */
    ptr = 0;

    // init REG
    pm1_vec[ptr++] = MOVE(1, R0, 0x10 | 0x00);       // R0=0x10, # source_ID=1, target_id=0(ctrl-0)
    pm1_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm1_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm1_vec[ptr++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    TAG_U2M_ACK0 = ptr;
    // 6
    pm1_vec[ptr++] = POP(1, R0); // wait signal from ctrl-0
    // pm1_vec[ptr++] = PUSH(0, R1);                    // trigger ptn
    // pm1_vec[ptr++] = POP(0, R0);                     // wait ack
    pm1_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm1_vec[ptr++] = ADD(1, R5, R5, 1);             // count_of_ptn++
    pm1_vec[ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT); // until ptns are all egressed
    pm1_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm1_vec[ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

    TAG_U2M_EXIT = ptr;
    // 14-2
    pm1_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm1_vec[ptr++] = PUSH(0, R0);
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT();
    pm1_sz = ptr;

    /* pm0: ALU2 */
    ptr = 0;

    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x00);       // R0=0x23, # source_ID=2, target_id=3(ctrl-3)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    pm2_vec[ptr++] = POP(1, R0); // wait signal from ctrl-3
    // pm2_vec[ptr++] = PUSH(0, R1);                    // trigger next ptn
    // pm2_vec[ptr++] = POP(0, R0);                     // wait ack
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm2_vec[ptr++] = ADD(1, R5, R5, 1);             // count_of_ptn++
    pm2_vec[ptr++] = JUMP(3, R5, R6, TAG_U2S_EXIT); // until ptns are all egressed
    pm2_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_EXIT = ptr;
    // 14-2
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
    pm2_sz = ptr;

    /* pm3: ALU3 */
    ptr = 0;

    // init REG
    pm3_vec[ptr++] = MOVE(1, R0, 0x30 | 0x02);       // R0=0x32, # source_ID=3, target_id=2(ctrl-2)
    pm3_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm3_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm3_vec[ptr++] = MOVE(1, R6, num_of_pattern[3]); // LIMIT of ptn-id
    pm3_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm3_vec[ptr++] = MOVE(1, R7, 0);                 // m2u own state, from 0

    TAG_M2U_ACK0 = ptr;
    // 6
    // pm3_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    // pm3_vec[ptr++] = POP(0, R0);                    // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm3_vec[ptr++] = PUSH(1, R1);                   // notify u2s
    pm3_vec[ptr++] = JUMP(3, R5, R6, TAG_M2U_EXIT); // until all patterns are egressed
    pm3_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

    // 14-2
    TAG_M2U_EXIT = ptr;
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    // pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish m2u
    pm3_sz = ptr;
}

/* ################### MAIN ################### */
int main() {
    // pattern generation
    int ptr = 0;

// num of patterns for 4 ctrls inside bi-DM
#if defined(TEST_BYPASS_ONLY)
    const int num_of_pattern[4] = {0, 0, 0, 0};
#elif defined(TEST_S2U_U2S_S2M)
    const int num_of_pattern[4] = {5, 1, 3, 0};
#elif defined(TEST_M2U_U2M_M2S)
    const int num_of_pattern[4] = {0, 3, 1, 5};
#elif defined(TEST_S2U_M2U)
    const int num_of_pattern[4] = {5, 0, 0, 4};
#elif defined(TEST_U2S_U2M)
    const int num_of_pattern[4] = {0, 5, 4, 0};
#elif defined(TEST_S2U_U2M)
    const int num_of_pattern[4] = {5, 3, 0, 0};
#elif defined(TEST_M2U_U2S)
    const int num_of_pattern[4] = {0, 0, 3, 5};
#elif defined(TEST_M2S_S2M)
    const int num_of_pattern[4] = {5, 5, 5, 5};
#endif

    // allocate memory for pm
    int* pattern[4] = {
        (int*)malloc(num_of_pattern[0] * 25 * sizeof(int)), (int*)malloc(num_of_pattern[1] * 25 * sizeof(int)),
        (int*)malloc(num_of_pattern[2] * 25 * sizeof(int)), (int*)malloc(num_of_pattern[3] * 25 * sizeof(int))};

    /* URAM access addr of data described in each pattern */
    int URAM_offsets[][5] = {0x00000000, 0x00010000, 0x00100000, 0x00110000, 0x01000000, 0x01000000, 0x0,
                             0x0,        0x0,        0x0,        0x00010000, 0x00100000, 0x00110000, 0x0,
                             0x0,        0x0,        0x0,        0x0,        0x0,        0x0};

    // generate patterns for ALUs
    int sum_of_all_pattern = 0;
    for (int i = 0; i < 4; ++i) {
        int* p_ptn = pattern[i];
        int elem_in_pattern = pattern_gen(num_of_pattern[i], p_ptn, URAM_offsets[i]);
        sum_of_all_pattern += elem_in_pattern;
        std::cout << "element: " << sum_of_all_pattern << std::endl;
    }
    std::cout << "Total element: " << sum_of_all_pattern << std::endl;

    // generate pm
    uint32_t pm_sz[4] = {0, 0, 0, 0};
    uint32_t* pm[4] = {(uint32_t*)malloc(1024 * sizeof(uint32_t)), (uint32_t*)malloc(1024 * sizeof(uint32_t)),
                       (uint32_t*)malloc(1024 * sizeof(uint32_t)), (uint32_t*)malloc(1024 * sizeof(uint32_t))};

    std::cout << "generate pm..." << std::endl;
#if defined(TEST_BYPASS_ONLY)
    pm_gen_case_0(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_S2U_U2S_S2M)
    pm_gen_case_1(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_M2U_U2M_M2S)
    pm_gen_case_2(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_S2U_M2U)
    pm_gen_case_3(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_U2S_U2M)
    pm_gen_case_4(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_S2U_U2M)
    pm_gen_case_5(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_M2U_U2S)
    pm_gen_case_6(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_M2S_S2M)
    pm_gen_case_7(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#endif

    for (int i = 0; i < 4; ++i) {
        std::cout << "pm_sz[" << std::to_string(i) << "]=" << pm_sz[i] << std::endl;
    }

    // dump pm to stdout
    for (int i = 0; i < 4; ++i) {
        std::cout << std::endl << "##pm[" << std::to_string(i) << "]##" << std::endl;
        for (uint32_t j = 0; j < pm_sz[i]; ++j) {
            std::cout << "0x" << std::hex << *(pm[i] + j) << std::endl;
        }
    }

    std::cout << "All pm generated" << std::endl;

    dut((ap_uint<32>*)pattern[0], (ap_uint<32>*)pattern[1], (ap_uint<32>*)pattern[2], (ap_uint<32>*)pattern[3],
        (ap_uint<32>*)pm[0], (ap_uint<32>*)pm[1], (ap_uint<32>*)pm[2], (ap_uint<32>*)pm[3]);

    return 0;
}
