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

#ifndef _DATAMOVER_CFG_GEN_HPP_
#define _DATAMOVER_CFG_GEN_HPP_

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

#include <iostream>
template <int WDATA>
class DatamoverCfgGen {
   private:
    int* pattern_buf[4];
    int* pattern[4];
    int* URAM_offsets[4];
    uint32_t pm_sz[4];
    uint32_t* pm[4];
    int* cur_pattern;
    int buf_ptr;

    const int OCP = 0xFFFF;
    const int template_1D_pattern[24] = {
        OCP, 1, 1, 1, // buffer_dim[4]
        OCP, 0, 0, 0, // offset[4]
        OCP, 1, 1, 1, // tiling[4]
        0,   1, 2, 3, // dim_idx[4]
        OCP, 0, 0, 0, // stride[4]
        OCP, 1, 1, 1  // wrap[4]
    };

    uint32_t MOVE(int mode, int rd, int rs) {
        return ((0x0 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs & 0xFF) << 12));
    }

    uint32_t POP(int mode, int rd) { return ((0x1 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8)); }

    uint32_t PUSH(int mode, int rs) { return ((0x2 & 0x1F) | ((mode & 0x7) << 5) | ((rs & 0xF) << 8)); }

    uint32_t ADD(int mode, int rd, int rs1, int rs2) {
        return ((0x3 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs1 & 0xFF) << 12) | ((rs2 & 0xFF) << 20));
    }

    uint32_t JUMP(int mode, int rs1, int rs2, int imm) {
        return ((0x4 & 0x1F) | ((mode & 0x7) << 5) | ((rs1 & 0xF) << 8) | ((rs2 & 0xF) << 12) | ((imm & 0xFFFF) << 16));
    }

    uint32_t EXIT(void) { return (0x5 & 0x1F); }

    void pm_gen(const int* num_of_pattern,
                uint32_t* pm0_vec,
                uint32_t* pm1_vec,
                uint32_t* pm2_vec,
                uint32_t* pm3_vec,
                uint32_t& pm0_sz,
                uint32_t& pm1_sz,
                uint32_t& pm2_sz,
                uint32_t& pm3_sz,
                uint32_t loop) {
        /*
            connection view:
        -------------------
               AIE
             ↓ u2s ↑
            pm0 → pm2
             ↓     ↑
            ---------
               uram
            ---------
             ↓     ↑
            pm1   pm3
             ↓ m2u ↑
               DDR
        -------------------
            regs function:
                R0: pm source / target
                R1: init the intial index of pattern
                R2: recieve signal from other pm
                R3: record how many iteration done
                R4: record the number of iteration
                R5: record how many pattern transfered
                R6: record the number of pattern
                R7: record the state
        */
        // pm3 m2u once
        uint32_t ptr3 = 0;
        uint32_t m2u_pattern_ptr = 0;
        pm3_vec[ptr3++] = MOVE(1, R0, 0x30 | 0x02);       // R0=0x32, # source_ID=3, target_id=2(ctrl-2)
        pm3_vec[ptr3++] = PUSH(1, R0);                    // dm.write(R0)
        pm3_vec[ptr3++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
        pm3_vec[ptr3++] = MOVE(1, R5, 0);                 // count for ptn_finished
        pm3_vec[ptr3++] = MOVE(1, R6, num_of_pattern[3]); // LIMIT of ptn-id

        m2u_pattern_ptr = ptr3;
        pm3_vec[ptr3++] = PUSH(0, R1);                      // trigger ptn-1
        pm3_vec[ptr3++] = POP(0, R0);                       // wait ack
        pm3_vec[ptr3++] = ADD(1, R5, R5, 1);                // count_for_ptn_finished++
        pm3_vec[ptr3++] = ADD(1, R1, R1, 1);                // increase ptn-id, to 1
        pm3_vec[ptr3++] = PUSH(1, R1);                      // notify pm2 to read uram
        pm3_vec[ptr3++] = JUMP(1, R5, R6, m2u_pattern_ptr); // if R5  < num_of_pattern[3]: jump -> m2u_loop_ptr

        pm3_vec[ptr3++] = MOVE(1, R0, 0xFF); // end flag
        pm3_vec[ptr3++] = PUSH(0, R0);
        pm3_vec[ptr3++] = PUSH(1, R0);
        pm3_vec[ptr3++] = EXIT();
        pm3_sz = ptr3;
        //------------------------------------------------------------------
        // pm2 u2s iteration
        uint32_t ptr2 = 0;
        uint32_t u2s_loop_ptr = 0, u2s_pattern_ptr = 0;
        pm2_vec[ptr2++] = MOVE(1, R0, 0x20 | 0x00);       // source pm2 target pm0
        pm2_vec[ptr2++] = PUSH(1, R0);                    // dm.write(R0)
        pm2_vec[ptr2++] = MOVE(1, R4, loop);              // R4=loop
        pm2_vec[ptr2++] = MOVE(1, R3, 0);                 // R3=0 count for loop
        pm2_vec[ptr2++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
        pm2_vec[ptr2++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
        pm2_vec[ptr2++] = MOVE(1, R5, 0);                 // count for ptn_finished

        u2s_loop_ptr = ptr2;
        u2s_pattern_ptr = ptr2;
        pm2_vec[ptr2++] = POP(1, R2);                       // wait pm3/pm0 ack to start each loop
        pm2_vec[ptr2++] = PUSH(0, R1);                      // trigger ptn-1
        pm2_vec[ptr2++] = POP(0, R0);                       // wait ack
        pm2_vec[ptr2++] = ADD(1, R5, R5, 1);                // count_for_ptn_finished++
        pm2_vec[ptr2++] = ADD(1, R1, R1, 1);                // increase ptn-id, to 1
        pm2_vec[ptr2++] = JUMP(1, R5, R6, u2s_pattern_ptr); // if R5  < num_of_pattern: jump -> u2s_loop_ptr
        pm2_vec[ptr2++] = ADD(1, R3, R3, 1);                // loop ++
        pm2_vec[ptr2++] = MOVE(1, R1, 0);                   // R1=0, initial ptn_id
        pm2_vec[ptr2++] = MOVE(1, R5, 0);                   // count for ptn_finished
        pm2_vec[ptr2++] = JUMP(1, R3, R4, u2s_loop_ptr);

        pm2_vec[ptr2++] = MOVE(1, R0, 0xFF); // end flag
        pm2_vec[ptr2++] = PUSH(0, R0);
        pm2_vec[ptr2++] = PUSH(1, R0);
        pm2_vec[ptr2++] = EXIT();
        pm2_sz = ptr2;
        //------------------------------------------------------------------
        // pm1 u2m once
        uint32_t ptr1 = 0;
        uint32_t u2m_pattern_ptr = 0;
        pm1_vec[ptr1++] = MOVE(1, R0, 0x10 | 0x00);       // source pm1 target pm0
        pm1_vec[ptr1++] = PUSH(1, R0);                    // dm.write(R0)
        pm1_vec[ptr1++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
        pm1_vec[ptr1++] = MOVE(1, R6, num_of_pattern[1]); // LIMIT of ptn-id
        pm1_vec[ptr1++] = MOVE(1, R5, 0);                 // count for ptn_finished

        u2m_pattern_ptr = ptr1;
        pm1_vec[ptr1++] = POP(1, R2);                       // wait pm0 ack to start
        pm1_vec[ptr1++] = PUSH(0, R1);                      // trigger ptn-1
        pm1_vec[ptr1++] = POP(0, R0);                       // wait ack
        pm1_vec[ptr1++] = ADD(1, R5, R5, 1);                // count_for_ptn_finished++
        pm1_vec[ptr1++] = ADD(1, R1, R1, 1);                // increase ptn-id, to 1
        pm1_vec[ptr1++] = JUMP(1, R5, R6, u2m_pattern_ptr); // if R5  < num_of_pattern: jump -> u2m_loop_ptr

        pm1_vec[ptr1++] = MOVE(1, R0, 0xFF); // end flag
        pm1_vec[ptr1++] = PUSH(0, R0);
        pm1_vec[ptr1++] = PUSH(1, R0);
        pm1_vec[ptr1++] = EXIT();
        pm1_sz = ptr1;
        //------------------------------------------------------------------
        // pm0 s2u iteration
        uint32_t ptr0 = 0;
        uint32_t s2u_loop_ptr = 0, s2u_pattern_ptr = 0;
        uint32_t target1 = 18;                            /* to modify */
        pm0_vec[ptr0++] = MOVE(1, R0, 0x00 | 0x02);       // source pm0 target pm1
        pm0_vec[ptr0++] = PUSH(1, R0);                    // dm.write(R0)
        pm0_vec[ptr0++] = MOVE(1, R4, loop - 1);          // R4=loop-1
        pm0_vec[ptr0++] = MOVE(1, R3, 0);                 // R3=0 count for loop
        pm0_vec[ptr0++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
        pm0_vec[ptr0++] = MOVE(1, R5, 0);                 // count for ptn_finished
        pm0_vec[ptr0++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id

        pm0_vec[ptr0++] = JUMP(3, R3, R4, target1);
        s2u_loop_ptr = ptr0;
        s2u_pattern_ptr = ptr0;
        pm0_vec[ptr0++] = PUSH(0, R1);                      // trigger ptn-1
        pm0_vec[ptr0++] = POP(0, R0);                       // wait ack
        pm0_vec[ptr0++] = ADD(1, R5, R5, 1);                // count_for_ptn_finished++
        pm0_vec[ptr0++] = ADD(1, R1, R1, 1);                // increase ptn-id, to 1
        pm0_vec[ptr0++] = PUSH(1, R1);                      // notify pm2
        pm0_vec[ptr0++] = JUMP(1, R5, R6, s2u_pattern_ptr); // if R5  < num_of_pattern: jump -> u2m_loop_ptr
        pm0_vec[ptr0++] = ADD(1, R3, R3, 1);
        pm0_vec[ptr0++] = MOVE(1, R1, 0); // R1=0, initial ptn_id
        pm0_vec[ptr0++] = MOVE(1, R5, 0); // count for ptn_finished
        pm0_vec[ptr0++] = JUMP(1, R3, R4, s2u_loop_ptr);
        // 18
        pm0_vec[ptr0++] = MOVE(1, R0, 0xFE);
        pm0_vec[ptr0++] = PUSH(1, R0);
        pm0_vec[ptr0++] = MOVE(1, R0, 0x00 | 0x01);
        pm0_vec[ptr0++] = PUSH(1, R0);
        s2u_pattern_ptr = ptr0;
        pm0_vec[ptr0++] = PUSH(0, R1);                      // trigger ptn-1
        pm0_vec[ptr0++] = POP(0, R0);                       // wait ack
        pm0_vec[ptr0++] = ADD(1, R5, R5, 1);                // count_for_ptn_finished++
        pm0_vec[ptr0++] = ADD(1, R1, R1, 1);                // increase ptn-id, to 1
        pm0_vec[ptr0++] = PUSH(1, R1);                      // notify pm1
        pm0_vec[ptr0++] = JUMP(1, R5, R6, s2u_pattern_ptr); // if R5  < num_of_pattern: jump -> u2m_loop_ptr

        pm0_vec[ptr0++] = MOVE(1, R0, 0xFF); // end flag
        pm0_vec[ptr0++] = PUSH(0, R0);
        pm0_vec[ptr0++] = PUSH(1, R0);
        pm0_vec[ptr0++] = EXIT();
        pm0_sz = ptr0;
    };

    void Pattern1DGen(int* pattern, int buffer_dim, int offset, int tiling, int stride, int wrap) {
        std::memcpy(pattern, template_1D_pattern, 24 * sizeof(int));
        pattern[0] = buffer_dim; // dim
        pattern[4] = offset;     // offset
        pattern[8] = tiling;     // tiling tile size
        pattern[16] = stride;    // stride
        pattern[20] = wrap;      // wrap how many tile
    }

   public:
    uint32_t* cfg_buf;
    int cfg_size_in_byte;
    DatamoverCfgGen(){};

    DatamoverCfgGen(int pattern_size, int loop) {
        int num_of_pattern[4] = {1, 1, 1, 1};

        for (int i = 0; i < 4; i++) {
            pattern[i] = (int*)malloc(num_of_pattern[i] * 24 * sizeof(int));
            URAM_offsets[i] = (int*)malloc(num_of_pattern[i] * sizeof(int));
            pattern_buf[i] = (int*)malloc(num_of_pattern[i] * 25 * sizeof(int));
        }

        int URAM_address = 0;
        for (int j = 0; j < num_of_pattern[0]; j++) {
            URAM_offsets[0][j] = URAM_address;
            URAM_offsets[1][j] = URAM_address;
            URAM_offsets[2][j] = URAM_address;
            URAM_offsets[3][j] = URAM_address;
            URAM_address += pattern_size;
        }

        cur_pattern = (int*)malloc(24 * sizeof(int));
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < num_of_pattern[i]; j++) {
                Pattern1DGen(cur_pattern, pattern_size, 0, pattern_size, pattern_size, 1);
                memcpy(pattern[i] + j * 24, cur_pattern, 24 * sizeof(int));
            }
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < num_of_pattern[i]; j++) {
                pattern_buf[i][j * 25] = URAM_offsets[i][j];
                for (int k = 0; k < 24; k++) {
                    pattern_buf[i][j * 25 + k + 1] = pattern[i][j * 24 + k];
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            pm_sz[i] = 0;
            pm[i] = (uint32_t*)malloc(1024 * sizeof(uint32_t));
            for (int j = 0; j < 1024; j++) {
                pm[i][j] = 0;
            }
        }
        pm_gen(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3], loop);

        cfg_buf = (uint32_t*)malloc(1024 * sizeof(uint32_t));
        for (int i = 0; i < 1024; i++) {
            cfg_buf[i] = 0;
        }
        buf_ptr = 0;

        for (int i = 0; i < 4; i++) {
            int ptn_sz = num_of_pattern[i] * 25;
            cfg_buf[buf_ptr] = ptn_sz;
            buf_ptr += WDATA / 32;
            if (num_of_pattern[i]) {
                std::memcpy(cfg_buf + buf_ptr, pattern_buf[i], ptn_sz * sizeof(uint32_t));
                buf_ptr += ptn_sz;
                if (ptn_sz % (WDATA / 32) != 0) buf_ptr += (WDATA / 32) - ptn_sz % (WDATA / 32);
            }
        }

        for (int i = 0; i < 4; i++) {
            if (pm_sz[i] % (WDATA / 32) != 0) pm_sz[i] += (WDATA / 32) - pm_sz[i] % (WDATA / 32);
            cfg_buf[buf_ptr] = pm_sz[i];
            buf_ptr += WDATA / 32;
            std::memcpy(cfg_buf + buf_ptr, pm[i], pm_sz[i] * sizeof(uint32_t));
            buf_ptr += pm_sz[i];
        }

        cfg_size_in_byte = buf_ptr * sizeof(uint32_t);
    }

    ~DatamoverCfgGen() {
        for (int i = 0; i < 4; i++) {
            free(pattern_buf[i]);
            free(pattern[i]);
            free(URAM_offsets[i]);
            free(pm[i]);
        }
        free(cur_pattern);
        free(cfg_buf);
    }
};
#endif