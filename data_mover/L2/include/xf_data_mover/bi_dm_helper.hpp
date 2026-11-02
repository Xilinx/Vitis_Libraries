#pragma once
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

class Bi_DM_Helper {
   private:
    int loop_num;       // the number of pattern in cache2axis and axis2cache call
    int mm2s_bd[2];     // [address, length] for DDR reading in 1D, elem-wise length
    int s2mm_bd[2];     // [address, length] for DDR writing in 1D, elem-wise length
    int* cache_loop_bd; // [address, length],[],.. for each loopable caching access in loop_num times

    int cache_len;                        // elem-wise
    int num_of_pattern[4] = {0, 1, 0, 1}; // num of patterns for each ctrl
    int pm_sz[4] = {0, 0, 0, 0};          // num of instructions of program memory for each ALU
    int* ptn[4];
    uint32_t* pm[4];

    const int OCP = 0xFFFF;
    const int template_1D_pattern[24] = {
        OCP, 1, 1, 1, // buffer_dim[4]
        OCP, 0, 0, 0, // offset[4]
        OCP, 1, 1, 1, // tiling[4]
        0,   1, 2, 3, // dim_idx[4]
        OCP, 0, 0, 0, // stride[4]
        OCP, 1, 1, 1  // wrap[4]
    };

   public:
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

    Bi_DM_Helper(const int cache_len, int* mm2s_bd, int* s2mm_bd, const int loop_num, int* cache_loop_bd) {
        this->cache_len = cache_len;
        this->loop_num = loop_num;
        this->cache_loop_bd = (int*)malloc(2 * 2 * this->loop_num * sizeof(int));
        std::memcpy(this->mm2s_bd, mm2s_bd, 2 * sizeof(int));
        std::memcpy(this->s2mm_bd, s2mm_bd, 2 * sizeof(int));
        std::memcpy(this->cache_loop_bd, cache_loop_bd, 2 * 2 * this->loop_num * sizeof(int));

        num_of_pattern[0] = loop_num;
        num_of_pattern[2] = loop_num;

        for (int i = 0; i < 4; i++) {
            pm[i] = (uint32_t*)malloc(1024 * sizeof(uint32_t));
        }
    }

    ~Bi_DM_Helper() {
        delete cache_loop_bd;

        for (int i = 0; i < 4; i++) {
            delete pm[i];
            delete ptn[i];
        }
    }

    template <int WDATA>
    bool genCacheLoopCFG(uint32_t* cfg_buf, const int cfg_len, bool handshake_enable = false) {
        // validate user's input cfg
        if (cache_len < mm2s_bd[1] || cache_len < s2mm_bd[1]) {
            std::cout << "ERROR: Data described in 'this->mm2s_bd, this->s2mm_bd' exceeds maximum cache length"
                      << std::endl;
            return false;
        }
        for (int i = 0; i < loop_num; i++) {
            if ((cache_loop_bd[i * 2] + cache_loop_bd[i * 2 + 1]) > cache_len ||
                (cache_loop_bd[loop_num * 2 + i * 2] + cache_loop_bd[loop_num * 2 + i * 2 + 1]) > cache_len) {
                std::cout << "ERROR: Data described in 'this->cache_loop_bd' exceeds maximum cache length" << std::endl;
                return false;
            }
        }

        // generate the PM for all 4 components
        genLoopPM(num_of_pattern, handshake_enable);

        for (int i = 0; i < 4; i++) {
            if (pm_sz[i] > 1024) {
                std::cout << "ERROR: Program memory for ALU-" << i << " should not exceed 1024 instructions"
                          << std::endl;
                return false;
            }
        }

        // generate the 1D pattern for all 4 components
        gen1DPtn();

        for (int i = 0; i < 4; i++) {
            int cache_offset = ptn[i][0 + 0];
            int dim = ptn[i][1 + 20];
            int offset = ptn[i][1 + 20];
            int tiling = ptn[i][1 + 20];
            int stride = ptn[i][1 + 20];
            int wrap = ptn[i][1 + 20];
            for (int j = 0; j < wrap; j++) {
                int bias = cache_offset + offset + stride * j;
                if (bias + tiling > cache_len) {
                    std::cout << "ERROR: Data described in pattern of ctrl-" << i << " exceeds maximum cache length"
                              << std::endl;
                    return false;
                }
            }
        }

        // generate the cfg_buf for L2 kernel run
        auto len = genCFG<WDATA>(cfg_buf);

        // check if the aligned CFG exceeds cfg_sz in host
        if (len > cfg_len) {
            std::cout << "ERROR: Generated CFG exceeds maximum cfg_sz, please allocate more memory for CFG in host"
                      << std::endl;
            return false;
        }

        return true;
    }

    template <int WDATA>
    int genCFG(uint32_t* cfg_buf) {
        const int W_URAM_ADDR = 32;
        const int W_ADDR = WDATA / W_URAM_ADDR;

        int wr_ptr = 0;
        for (int i = 0; i < 8; ++i) {
            if (i < 4) {
                // write pattern into CFG
                int ptn_sz = num_of_pattern[i] * 25;
                cfg_buf[wr_ptr] = ptn_sz;
                wr_ptr += W_ADDR;
                if (num_of_pattern[i]) {
                    std::memcpy(cfg_buf + wr_ptr, ptn[i], ptn_sz * sizeof(uint32_t));
                    wr_ptr += ptn_sz;
                    int frag = ptn_sz % W_ADDR;
                    if (frag) wr_ptr += (W_ADDR - frag);
                }
            } else {
                // write pm into CFG
                int frag = pm_sz[i - 4] % W_ADDR;
                if (frag) pm_sz[i - 4] += (W_ADDR - frag);
                cfg_buf[wr_ptr] = pm_sz[i - 4];
                wr_ptr += W_ADDR;
                std::memcpy(cfg_buf + wr_ptr, pm[i - 4], pm_sz[i - 4] * sizeof(uint32_t));
                wr_ptr += pm_sz[i - 4];
            }
        }
        return wr_ptr;
    }

    int* genData(uint64_t* data_buf) {
        populate_data_gen(ptn[3], data_buf);
        return ptn[3];
    }

    void populate_data_gen(int* current_ptn, uint64_t* dm_buf) {
        current_ptn += 1;
        int* buf_dim = current_ptn;
        int* offset = current_ptn + 4;
        int* tiling = current_ptn + 8;
        int* dim_idx = current_ptn + 12;
        int* stride = current_ptn + 16;
        int* wrap = current_ptn + 20;

        int bias[4];
        for (int w = 0; w < wrap[dim_idx[3]]; w++) {
            bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
            for (int z = 0; z < wrap[dim_idx[2]]; z++) {
                bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                    bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                    for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                        bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;

                        for (int l = 0; l < tiling[3]; l++) {
                            for (int k = 0; k < tiling[2]; k++) {
                                for (int j = 0; j < tiling[1]; j++) {
                                    for (int i = 0; i < tiling[0]; i++) {
                                        int data = (bias[3] + l) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                   (bias[2] + k) * (buf_dim[1] * buf_dim[0]) +
                                                   (bias[1] + j) * buf_dim[0] + (bias[0] + i);
                                        dm_buf[data] = data;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void print_ptn(int* p_ptn) {
        // print the 4D pattern
        std::cout << "****generated patterns in host: " << p_ptn << "****" << std::endl;
        std::cout << "URAM access addr: 0x" << std::hex << p_ptn[0] << std::endl;
        std::cout << "buffer_dim[4]={" << p_ptn[1] << "," << p_ptn[2] << "," << p_ptn[3] << "," << p_ptn[4] << "}"
                  << std::endl;
        std::cout << "buffer_offset[4]={" << p_ptn[5] << "," << p_ptn[6] << "," << p_ptn[7] << "," << p_ptn[8] << "}"
                  << std::endl;
        std::cout << "tiling[4]={" << p_ptn[9] << "," << p_ptn[10] << "," << p_ptn[11] << "," << p_ptn[12] << "}"
                  << std::endl;
        std::cout << "dim_idx[4]={" << p_ptn[13] << "," << p_ptn[14] << "," << p_ptn[15] << "," << p_ptn[16] << "}"
                  << std::endl;
        std::cout << "stride[4]={" << p_ptn[17] << "," << p_ptn[18] << "," << p_ptn[19] << "," << p_ptn[20] << "}"
                  << std::endl;
        std::cout << "wrap[4]={" << p_ptn[21] << "," << p_ptn[22] << "," << p_ptn[23] << "," << p_ptn[24] << "}"
                  << std::endl;
        return;
    }

    void gen1DPtn() {
        for (int i = 0; i < 4; i++) {
            ptn[i] = (int*)malloc(num_of_pattern[i] * 25 * sizeof(int));
            for (int j = 0; j < num_of_pattern[i]; j++) {
                std::memcpy(ptn[i] + j * 25 + 1, template_1D_pattern, 24 * sizeof(int));
            }
        }

        ptn[1][0 + 0] = s2mm_bd[0];
        ptn[1][1 + 0] = s2mm_bd[1];
        ptn[1][1 + 4] = 0;
        ptn[1][1 + 8] = s2mm_bd[1];
        ptn[1][1 + 16] = s2mm_bd[1];
        ptn[1][1 + 20] = 1;

        ptn[3][0 + 0] = mm2s_bd[0];
        ptn[3][1 + 0] = mm2s_bd[1];
        ptn[3][1 + 4] = 0;
        ptn[3][1 + 8] = mm2s_bd[1];
        ptn[3][1 + 16] = mm2s_bd[1];
        ptn[3][1 + 20] = 1;

        for (int i = 0; i < loop_num; i++) {
            (ptn[0] + i * 25)[0 + 0] = cache_loop_bd[i * 2];
            (ptn[0] + i * 25)[1 + 0] = cache_loop_bd[i * 2 + 1];
            (ptn[0] + i * 25)[1 + 4] = 0;
            (ptn[0] + i * 25)[1 + 8] = cache_loop_bd[i * 2 + 1];
            (ptn[0] + i * 25)[1 + 16] = cache_loop_bd[i * 2 + 1];
            (ptn[0] + i * 25)[1 + 20] = 1;

            (ptn[2] + i * 25)[0 + 0] = cache_loop_bd[loop_num * 2 + i * 2];
            (ptn[2] + i * 25)[1 + 0] = cache_loop_bd[loop_num * 2 + i * 2 + 1];
            (ptn[2] + i * 25)[1 + 4] = 0;
            (ptn[2] + i * 25)[1 + 8] = cache_loop_bd[loop_num * 2 + i * 2 + 1];
            (ptn[2] + i * 25)[1 + 16] = cache_loop_bd[loop_num * 2 + i * 2 + 1];
            (ptn[2] + i * 25)[1 + 20] = 1;
        }

        for (int i = 0; i < 4; i++) {
            std::cout << "print ptn of ctrl-" << i << std::endl;
            for (int j = 0; j < num_of_pattern[i]; j++) {
                print_ptn(ptn[i] + j * 25);
            }
        }
    }

    void genLoopPM(int* num_of_pattern, const bool hsk_enable) {
        uint32_t TAG_S2U_ACK0, TAG_S2U_EXIT = hsk_enable ? 14 : 13, TAG_U2M_ACK0, TAG_U2M_EXIT = 14, TAG_U2S_ACK0,
                               TAG_U2S_EXIT = hsk_enable ? 15 : 14, TAG_M2U_ACK0, TAG_M2U_EXIT = 14;

        /* pm0: ALU0 */
        uint32_t ptr = 0;

        pm[0][ptr++] = MOVE(1, R0, 0x00 | 0x01);
        pm[0][ptr++] = PUSH(1, R0);
        pm[0][ptr++] = MOVE(1, R1, 0);
        pm[0][ptr++] = MOVE(1, R6, num_of_pattern[0]);
        pm[0][ptr++] = MOVE(1, R5, 0);
        pm[0][ptr++] = MOVE(1, R7, 0);

        TAG_S2U_ACK0 = ptr;
        // 6
        if (hsk_enable) pm[0][ptr++] = POP(1, R0);
        pm[0][ptr++] = PUSH(0, R1);
        pm[0][ptr++] = POP(0, R0);
        pm[0][ptr++] = ADD(1, R7, R7, 1);
        pm[0][ptr++] = ADD(1, R5, R5, 1);
        pm[0][ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT);
        pm[0][ptr++] = ADD(1, R1, R1, 1);
        pm[0][ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

        TAG_S2U_EXIT = ptr;
        // 13
        pm[0][ptr++] = PUSH(1, R1);
        pm[0][ptr++] = MOVE(1, R0, 0xFF);
        pm[0][ptr++] = PUSH(0, R0);
        pm[0][ptr++] = PUSH(1, R0);
        pm[0][ptr++] = EXIT();
        pm_sz[0] = ptr;

        /* pm1: ALU1 */
        ptr = 0;

        // init REG
        pm[1][ptr++] = MOVE(1, R0, 0x10 | 0x00);
        pm[1][ptr++] = PUSH(1, R0);
        pm[1][ptr++] = MOVE(1, R1, 0);
        pm[1][ptr++] = MOVE(1, R6, num_of_pattern[1]);
        pm[1][ptr++] = MOVE(1, R5, 0);
        pm[1][ptr++] = MOVE(1, R7, 0);

        TAG_U2M_ACK0 = ptr;
        // 6
        pm[1][ptr++] = POP(1, R0);
        pm[1][ptr++] = PUSH(0, R1);
        pm[1][ptr++] = POP(0, R0);
        pm[1][ptr++] = ADD(1, R7, R7, 1);
        pm[1][ptr++] = ADD(1, R5, R5, 1);
        pm[1][ptr++] = JUMP(3, R5, R6, TAG_U2M_EXIT);
        pm[1][ptr++] = ADD(1, R1, R1, 1);
        pm[1][ptr++] = JUMP(0, 0, 0, TAG_U2M_ACK0);

        TAG_U2M_EXIT = ptr;
        // 14
        pm[1][ptr++] = ADD(1, R7, R7, 1);
        pm[1][ptr++] = MOVE(1, R0, 0xFF);
        pm[1][ptr++] = PUSH(0, R0);
        pm[1][ptr++] = PUSH(1, R0);
        pm[1][ptr++] = EXIT();
        pm_sz[1] = ptr;

        /* pm0: ALU2 */
        ptr = 0;

        // init REG
        pm[2][ptr++] = MOVE(1, R0, 0x20 | 0x00);
        pm[2][ptr++] = PUSH(1, R0);
        pm[2][ptr++] = MOVE(1, R1, 0);
        pm[2][ptr++] = MOVE(1, R6, num_of_pattern[2]);
        pm[2][ptr++] = MOVE(1, R5, 0);
        pm[2][ptr++] = MOVE(1, R7, 0);

        pm[2][ptr++] = POP(1, R0);

        TAG_U2S_ACK0 = ptr;
        // 7
        pm[2][ptr++] = PUSH(0, R1);
        pm[2][ptr++] = POP(0, R0);
        if (hsk_enable) pm[0][ptr++] = PUSH(1, R1);
        pm[2][ptr++] = ADD(1, R7, R7, 1);
        pm[2][ptr++] = ADD(1, R5, R5, 1);
        pm[2][ptr++] = JUMP(3, R5, R6, TAG_U2S_EXIT);
        pm[2][ptr++] = ADD(1, R1, R1, 1);
        pm[2][ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

        TAG_U2S_EXIT = ptr;
        // 14
        pm[2][ptr++] = ADD(1, R7, R7, 1);
        pm[2][ptr++] = MOVE(1, R0, 0xFF);
        pm[2][ptr++] = PUSH(0, R0);
        pm[2][ptr++] = PUSH(1, R0);
        pm[2][ptr++] = EXIT();
        pm_sz[2] = ptr;

        /* pm3: ALU3 */
        ptr = 0;

        // init REG
        pm[3][ptr++] = MOVE(1, R0, 0x30 | 0x02);
        pm[3][ptr++] = PUSH(1, R0);
        pm[3][ptr++] = MOVE(1, R1, 0);
        pm[3][ptr++] = MOVE(1, R6, num_of_pattern[3]);
        pm[3][ptr++] = MOVE(1, R5, 0);
        pm[3][ptr++] = MOVE(1, R7, 0);

        TAG_M2U_ACK0 = ptr;
        // 6
        pm[3][ptr++] = PUSH(0, R1);
        pm[3][ptr++] = POP(0, R0);
        pm[3][ptr++] = ADD(1, R7, R7, 1);
        pm[3][ptr++] = ADD(1, R5, R5, 1);
        pm[3][ptr++] = PUSH(1, R1);
        pm[3][ptr++] = JUMP(3, R5, R6, TAG_M2U_EXIT);
        pm[3][ptr++] = ADD(1, R1, R1, 1);
        pm[3][ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

        // 14
        TAG_M2U_EXIT = ptr;
        pm[3][ptr++] = MOVE(1, R0, 0xFF);
        pm[3][ptr++] = PUSH(0, R0);
        pm[3][ptr++] = PUSH(1, R0);
        pm[3][ptr++] = EXIT();
        pm_sz[3] = ptr;
    }
};
