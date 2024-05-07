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

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

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

uint32_t MOVE(int mode, int rd, int rs) {
    return ((0x0 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs & 0xFF) << 12));
}

uint32_t POP(int mode, int rd) {
    return ((0x1 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8));
}

uint32_t PUSH(int mode, int rs) {
    return ((0x2 & 0x1F) | ((mode & 0x7) << 5) | ((rs & 0xF) << 8));
}

uint32_t ADD(int mode, int rd, int rs1, int rs2) {
    return ((0x3 & 0x1F) | ((mode & 0x7) << 5) | ((rd & 0xF) << 8) | ((rs1 & 0xFF) << 12) | ((rs2 & 0xFF) << 20));
}

uint32_t JUMP(int mode, int rs1, int rs2, int imm) {
    return ((0x4 & 0x1F) | ((mode & 0x7) << 5) | ((rs1 & 0xF) << 8) | ((rs2 & 0xF) << 12) | ((imm & 0xFFFF) << 16));
}

uint32_t EXIT(void) {
    return (0x5 & 0x1F);
}

// flatten one 4D pattern to 1D pattern, with the same number of element
void flatten_4D_pattern(const int num_in_pattern, int* pattern_src, int* pattern_dst) {
    pattern_src += 1;
    pattern_dst += 1;
    int num_per_tiling = pattern_src[8] * pattern_src[9] * pattern_src[10] * pattern_src[11];
    int num_of_wrap = pattern_src[20] * pattern_src[21] * pattern_src[22] * pattern_src[23];
    std::memcpy(pattern_dst, template_1D_pattern, 24 * sizeof(int));
    pattern_dst[0] = num_in_pattern;  // dim
    pattern_dst[4] = 0;               // offset
    pattern_dst[8] = num_per_tiling;  // tiling
    pattern_dst[16] = num_per_tiling; // stride
    pattern_dst[20] = num_of_wrap;    // wrap
}

void print_ptn(int* p_ptn) {
    // print the 4D pattern
    std::cout << "****generated 4D-patterns in host: " << p_ptn << "****" << std::endl;
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

// return the sum of number of element for all pattern
int* pattern_gen(const int num_of_pattern, int* pattern_vec, int* uram_offset) {
    int* sum_elem = (int*)malloc(sizeof(int) * num_of_pattern);
    std::memset(sum_elem, 0, sizeof(int) * num_of_pattern);

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
                tmp[i] = rand() % 3 + 3;
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
                tmp[i] = rand() % 3 + 1;
            }
        }
        // for wrap
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 20;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 1;
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
        sum_elem[p] += (ptn_template[8] * ptn_template[9] * ptn_template[10] * ptn_template[11] * ptn_template[20] *
                        ptn_template[21] * ptn_template[22] * ptn_template[23]);
        print_ptn(pattern_vec + p * 25);
        p++;
    }
    return sum_elem;
}

// generate 4D data and store to buffer continously
void populate_data_gen(int* current_ptn, uint64_t* dm_buf, bool s2m, bool enable_dump, std::ofstream& fout) {
    if (!enable_dump)
        std::cout << std::endl << "data gen: " << current_ptn << std::endl;
    else {
        if (!fout && !fout.is_open()) {
            std::cout << "Dump-file opening failure in populate_data_gen()" << std::endl;
            exit(-1);
        } else {
            fout << std::endl;
        }
    }
    int ptr = 0;
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
                                               (bias[2] + k) * (buf_dim[1] * buf_dim[0]) + (bias[1] + j) * buf_dim[0] +
                                               (bias[0] + i);
                                    if (enable_dump) {
                                        fout << std::hex << dm_buf[data] << " ";
                                    } else {
                                        std::cout << std::hex << data << " ";
                                        if (s2m) {
                                            dm_buf[ptr] = data;
                                        } else {
                                            dm_buf[data] = data;
                                        }
                                    }
                                    ptr++;
                                    if (ptr && ptr % 40 == 0) {
                                        if (enable_dump)
                                            fout << std::endl;
                                        else
                                            std::cout << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (enable_dump)
        fout << std::endl << "count: " << std::dec << ptr << std::endl;
    else
        std::cout << std::endl << "count: " << std::dec << ptr << std::endl;
}

/* [Test case] write data into cache from AIE, and move target data of cache into AIE
In this case, axis2cache will read data from AIE and write them into cache. While cache2axis will read some of
data writed into cache by axis2cache, and forward these data into AIE.

axis2cache and cache2axis are working in parallel.
*/
void pm_gen_case_1(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-cache&cache-axis ##" << std::endl;

    uint32_t TAG_S2U_ACK0 = 13, TAG_S2U_EXIT = 21, TAG_U2S_ACK0, TAG_U2S_EXIT = 15;

    /* pm0: ALU0 */
    uint32_t ptr = 0;

    pm0_vec[ptr++] = MOVE(1, R0, 0x00 | 0x02);       // R0=0x01, # source_ID=0, target_id=2(ctrl-2)
    pm0_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm0_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm0_vec[ptr++] = MOVE(1, R6, num_of_pattern[0]); // LIMIT of ptn-id
    pm0_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm0_vec[ptr++] = MOVE(1, R7, 0);                 // u2c own state, from 0

    // let ptn-0 be rubbish, just ignore it
    // 6
    pm0_vec[ptr++] = PUSH(0, R1);       // trigger ptn-0
    pm0_vec[ptr++] = POP(0, R0);        // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm0_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    pm0_vec[ptr++] = PUSH(0, R1);       // trigger ptn-1
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_ACK0 = ptr;
    // 13
    pm0_vec[ptr++] = POP(0, R0);                    // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm0_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm0_vec[ptr++] = PUSH(1, R1);                   // notify u2s
    pm0_vec[ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT); // until all patterns are egressed
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm0_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_EXIT = ptr;
    // 21
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm0_vec[ptr++] = PUSH(0, R0);
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
    pm1_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_fininshed
    pm1_vec[ptr++] = MOVE(1, R7, 0);                 // u2m own state, from 0

    /* Do nothing, just bypass handshakes */

    // idle, waiting for exit
    pm1_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm1_vec[ptr++] = PUSH(0, R0);
    pm1_vec[ptr++] = PUSH(1, R0);
    pm1_vec[ptr++] = EXIT(); // finish s2u
    pm1_sz = ptr;

    /* pm2: ALU2 */
    ptr = 0;

    // init REG
    pm2_vec[ptr++] = MOVE(1, R0, 0x20 | 0x00);       // R0=0x20, # source_ID=2, target_id=0(ctrl-0)
    pm2_vec[ptr++] = PUSH(1, R0);                    // dm.write(R0)
    pm2_vec[ptr++] = MOVE(1, R1, 0);                 // R1=0, initial ptn_id
    pm2_vec[ptr++] = MOVE(1, R6, num_of_pattern[2]); // LIMIT of ptn-id
    pm2_vec[ptr++] = MOVE(1, R5, 0);                 // count for ptn_finished
    pm2_vec[ptr++] = MOVE(1, R7, 0);                 // u2s own state, from 0

    TAG_U2S_ACK0 = ptr;
    // 6
    pm2_vec[ptr++] = POP(1, R0);                    // wait signal from ctrl-0
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm2_vec[ptr++] = PUSH(0, R1);                   // trigger ptn
    pm2_vec[ptr++] = POP(0, R0);                    // wait ack
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm2_vec[ptr++] = ADD(1, R5, R5, 1);             // count_of_ptn++
    pm2_vec[ptr++] = JUMP(3, R5, R6, TAG_U2S_EXIT); // until ptn-2 are egressed
    pm2_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm2_vec[ptr++] = JUMP(0, 0, 0, TAG_U2S_ACK0);

    TAG_U2S_EXIT = ptr;
    // update end flag and trigger EXIT
    // 15
    pm2_vec[ptr++] = ADD(1, R7, R7, 1); // increase state
    pm2_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm2_vec[ptr++] = PUSH(0, R0);
    pm2_vec[ptr++] = PUSH(1, R0);
    pm2_vec[ptr++] = EXIT();
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
    pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish u2m
    pm3_sz = ptr;
}

/* [Test case] write data into cache from AIE, and move target data of cache into DDR
In this case, axis2cache will read data from AIE and write them into cache. While cache2axim will read some of
data writed into cache by axis2cache, and write these data into DDR.

axis2cache and cache2axim are working in parallel.
*/
void pm_gen_case_2(const int* num_of_pattern,
                   uint32_t* pm0_vec,
                   uint32_t* pm1_vec,
                   uint32_t* pm2_vec,
                   uint32_t* pm3_vec,
                   uint32_t& pm0_sz,
                   uint32_t& pm1_sz,
                   uint32_t& pm2_sz,
                   uint32_t& pm3_sz) {
    std::cout << "## Test program memory for axis-cache&cache-axim ##" << std::endl;

    uint32_t TAG_S2U_ACK0 = 13, TAG_S2U_EXIT = 21, TAG_U2M_ACK0, TAG_U2M_EXIT = 15;

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
    pm0_vec[ptr++] = PUSH(0, R1);       // trigger ptn-0
    pm0_vec[ptr++] = POP(0, R0);        // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm0_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm0_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    pm0_vec[ptr++] = PUSH(0, R1);       // trigger ptn-1
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_ACK0 = ptr;
    // 13(10)
    pm0_vec[ptr++] = POP(0, R0);                    // wait ack
    pm0_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm0_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm0_vec[ptr++] = PUSH(1, R1);                   // notify u2m
    pm0_vec[ptr++] = JUMP(3, R5, R6, TAG_S2U_EXIT); // until all patterns are egressed
    pm0_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm0_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm0_vec[ptr++] = JUMP(0, 0, 0, TAG_S2U_ACK0);

    TAG_S2U_EXIT = ptr;
    // 21(16)
    pm0_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm0_vec[ptr++] = PUSH(0, R0);
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
    pm1_vec[ptr++] = POP(1, R0);                    // wait signal from ctrl-0
    pm1_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm1_vec[ptr++] = PUSH(0, R1);                   // trigger ptn-0,1
    pm1_vec[ptr++] = POP(0, R0);                    // wait ack
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
    pm1_vec[ptr++] = PUSH(0, R0);
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
    pm2_vec[ptr++] = PUSH(0, R0);
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
    pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish u2m
    pm3_sz = ptr;
}

/* [Test case] write data into cache from DDR, and move target data of cache into AIE
In this case, axim2cache will read data from DDR and write them into cache. While cache2axis will read some of
data writed into cache by axim2cache, and write these data into AIE.

axim2cache and cache2axis are working in parallel.
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
    std::cout << "## Test program memory for axim-cache&cache-axis ##" << std::endl;

    uint32_t TAG_U2S_ACK0, TAG_U2S_EXIT = 15, TAG_M2U_ACK0 = 13, TAG_M2U_WAIT = 21, TAG_M2U_EXIT = 26;

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
    pm0_vec[ptr++] = PUSH(0, R0);
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
    pm1_vec[ptr++] = PUSH(0, R0);
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
    pm2_vec[ptr++] = POP(1, R0);                    // wait signal from ctrl-3
    pm2_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm2_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm2_vec[ptr++] = POP(0, R0);                    // wait ack
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
    pm2_vec[ptr++] = PUSH(0, R0);
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
    pm3_vec[ptr++] = PUSH(0, R1);       // trigger ptn-0
    pm3_vec[ptr++] = POP(0, R0);        // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1); // increase state,  to 1
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id, to 1
    pm3_vec[ptr++] = PUSH(0, R1);       // trigger ptn-1
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

    TAG_M2U_ACK0 = ptr;
    // 13(10)
    pm3_vec[ptr++] = POP(0, R0);                    // wait ack
    pm3_vec[ptr++] = ADD(1, R7, R7, 1);             // increase state
    pm3_vec[ptr++] = ADD(1, R5, R5, 1);             // count_for_ptn_finished++
    pm3_vec[ptr++] = PUSH(1, R1);                   // notify u2s
    pm3_vec[ptr++] = JUMP(3, R5, R6, TAG_M2U_WAIT); // until all patterns are egressed
    pm3_vec[ptr++] = ADD(1, R1, R1, 1);             // increase ptn-id
    pm3_vec[ptr++] = PUSH(0, R1);                   // trigger next ptn
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_ACK0);

    // 21(16)
    TAG_M2U_WAIT = ptr;
    pm3_vec[ptr++] = ADD(1, R1, R1, 1); // increase ptn-id
    pm3_vec[ptr++] = PUSH(0, R1);       // trigger next ptn
    pm3_vec[ptr++] = POP(0, R0);        // wait ack
    pm3_vec[ptr++] = ADD(1, R5, R5, 1); // count_for_ptn_finished++
    pm3_vec[ptr++] = JUMP(0, 0, 0, TAG_M2U_EXIT);

    // 26(19)
    TAG_M2U_EXIT = ptr;
    pm3_vec[ptr++] = MOVE(1, R0, 0xFF); // end flag
    pm3_vec[ptr++] = PUSH(0, R0);
    pm3_vec[ptr++] = PUSH(1, R0);
    pm3_vec[ptr++] = EXIT(); // finish m2u
    pm3_sz = ptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ofstream fout("host_dump.log");
    if (fout.is_open()) {
        fout << "## HOST DUMP FILE ##" << std::endl;
    } else {
        std::cout << "host dump-file creating failure..." << std::endl;
    }

    std::cout << "[0.0] testing in hw_emu..." << std::endl;
    std::cout << "[1.0] creating kernels..." << std::endl;

    char* xclbinFilename = argv[1];
    std::cout << "[1.1] xclbinFilename: " << xclbinFilename << std::endl;
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbinFilename);
    xrt::kernel* dm_krnl = new xrt::kernel(device, uuid, "bi_dm_4d_hsk", true);
#if defined(TEST_AXIS_CACHE_AXIS)
    xrt::kernel* sink_s2u_krnl = new xrt::kernel(device, uuid, "mm2s_mp", true); // sink for s2u's AXIS-Stream input
    xrt::kernel* sink_u2s_krnl = new xrt::kernel(device, uuid, "s2mm_mp", true); // sink for u2s's AXIS-Stream output
#elif defined(TEST_AXIS_TO_DDR)
    xrt::kernel* sink_s2u_krnl = new xrt::kernel(device, uuid, "mm2s_mp", true); // sink for s2u's AXIS-Stream input
#elif defined(TEST_DDR_TO_AXIS)
    xrt::kernel* sink_u2s_krnl = new xrt::kernel(device, uuid, "s2mm_mp", true); // sink for u2s's AXIS-Stream output
#endif
    std::cout << "[2.0] creating buffer..." << std::endl;

    int cfg_sz = 256 * 1024 * sizeof(uint32_t);                               // 1MB
    int data_sz = 128 * 1024 * 1024;                                          // 128MB
    xrt::bo* cfg_bo = new xrt::bo(device, cfg_sz, dm_krnl->group_id(0));      // Store config in BiD-DM's buffer_0
    xrt::bo* dm_data_bo = new xrt::bo(device, data_sz, dm_krnl->group_id(1)); // Use BiD-DM's buffer_1 as DDR (emulate)
#if defined(TEST_AXIS_CACHE_AXIS)
    xrt::bo* sink_in_bo = new xrt::bo(device, data_sz, sink_s2u_krnl->group_id(0));
    xrt::bo* sink_out_bo = new xrt::bo(device, data_sz, sink_u2s_krnl->group_id(1));
#elif defined(TEST_AXIS_TO_DDR)
    xrt::bo* sink_in_bo = new xrt::bo(device, data_sz, sink_s2u_krnl->group_id(0));
#elif defined(TEST_DDR_TO_AXIS)
    xrt::bo* sink_out_bo = new xrt::bo(device, data_sz, sink_u2s_krnl->group_id(1));
#endif

    auto cfg_buf = cfg_bo->map<uint32_t*>(); // for config
#if defined(TEST_DDR_TO_AXIS)
    auto dm_data_buf = dm_data_bo->map<uint64_t*>(); // data input for ddr2caxis
#else
    auto sink_in_buf = sink_in_bo->map<uint64_t*>(); // data input for axis2ddr/axis2cache2axis
#endif

    std::cout << "[3.0] allocating memory for patterns..." << std::endl;

/* num of patterns for 4 ctrls inside bi-DM */
#if defined(TEST_AXIS_CACHE_AXIS)
    const int num_of_pattern[4] = {5, 0, 4, 0};
#elif defined(TEST_AXIS_TO_DDR)
    const int num_of_pattern[4] = {3, 2, 0, 0};
#elif defined(TEST_DDR_TO_AXIS)
    const int num_of_pattern[4] = {0, 0, 3, 5};
#endif
    /* allocate memory for ptn */
    int* pattern[4] = {
        (int*)malloc(num_of_pattern[0] * 25 * sizeof(int)), (int*)malloc(num_of_pattern[1] * 25 * sizeof(int)),
        (int*)malloc(num_of_pattern[2] * 25 * sizeof(int)), (int*)malloc(num_of_pattern[3] * 25 * sizeof(int))};

    /* unflattened data of pattern-2 */
    int* pattern_bak = (int*)malloc(num_of_pattern[2] * 25 * sizeof(int));

/* URAM access addr of data described in each pattern */
#if defined(TEST_AXIS_CACHE_AXIS)
    int URAM_offsets[4][5] = {0x0,    0x0C00, 0x1800, 0x2400, 0x3000, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0C00, 0x1800, 0x2400, 0x3000, 0x0,    0x0, 0x0, 0x0, 0x0, 0x0};
#elif defined(TEST_AXIS_TO_DDR)
    int URAM_offsets[4][5] = {0x0, 0x0C00, 0x1800, 0x0, 0x0, 0x0C00, 0x1800, 0x0, 0x0, 0x0,
                              0x0, 0x0,    0x0,    0x0, 0x0, 0x0,    0x0,    0x0, 0x0, 0x0};
#elif defined(TEST_DDR_TO_AXIS)
    int URAM_offsets[4][5] = {0x3C00, 0x4800, 0x5400, 0x6000, 0x6C00, 0x3C00, 0x4800, 0x5400, 0x6000, 0x6C00,
                              0x0C00, 0x1800, 0x2400, 0x3000, 0x0,    0x0,    0x0C00, 0x1800, 0x2400, 0x3000};
#endif

    std::cout << "[4.0] generating patterns and fake data..." << std::endl;

    /* generate patterns for dm-ctrls */
    int sum_elem_of_pattern[4] = {0, 0, 0, 0};
    int sum_elem_of_all_patterns = 0;
#if defined(TEST_DDR_TO_AXIS)
    uint64_t* p_data_in = dm_data_buf;
#else
    uint64_t* p_data_in = sink_in_buf;
#endif
    uint64_t* pp_data_in = p_data_in;
    int* elem_in_pattern[4] = {0, 0, 0, 0};

#if defined(TEST_AXIS_CACHE_AXIS)
    for (int i = 0; i < 4; ++i) {
        int* p_ptn = pattern[i];
        fout << std::endl << "************Pattern-" << i << "************" << std::endl;
        std::cout << "[4.1] generating patterns of ctrl-" << i << std::endl;
        if (i == 0) {
            elem_in_pattern[i] = pattern_gen(num_of_pattern[i], p_ptn, URAM_offsets[i]);
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                int* _p_ptn = pattern[i] + j * 25;
                populate_data_gen(_p_ptn, pp_data_in, true, false, fout);
                std::cout << std::endl
                          << "[4.2] elem_in_pattern[" << i << "][" << j << "]=" << elem_in_pattern[i][j] << std::endl;
                pp_data_in += elem_in_pattern[i][j];
                sum_elem_of_pattern[i] += elem_in_pattern[i][j];
            }
        } else if (i == 2) {
            std::memcpy(p_ptn, pattern[0] + 25, num_of_pattern[i] * 25 * sizeof(int));
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                p_ptn[j * 25] = URAM_offsets[i][j];
                print_ptn(p_ptn + j * 25);
            }
#elif defined(TEST_AXIS_TO_DDR)
    for (int i = 0; i < 4; ++i) {
        int* p_ptn = pattern[i];
        fout << std::endl << "************Pattern-" << i << "************" << std::endl;
        std::cout << "[4.1] generating patterns of ctrl-" << i << std::endl;
        if (i == 0) {
            elem_in_pattern[i] = pattern_gen(num_of_pattern[i], p_ptn, URAM_offsets[i]);
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                int* _p_ptn = pattern[i] + j * 25;
                populate_data_gen(_p_ptn, pp_data_in, true, false, fout);
                std::cout << std::endl
                          << "[4.2] elem_in_pattern[" << i << "][" << j << "]=" << elem_in_pattern[i][j] << std::endl;
                pp_data_in += elem_in_pattern[i][j];
                sum_elem_of_pattern[i] += elem_in_pattern[i][j];
            }
        } else if (i == 1) {
            std::memcpy(p_ptn, pattern[0] + 25, num_of_pattern[i] * 25 * sizeof(int));
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                p_ptn[j * 25] = URAM_offsets[i][j];
                print_ptn(p_ptn + j * 25);
            }
            // flatten patterns of ctrl-0 from 4d to 1d
            for (int k = 0; k < num_of_pattern[0]; ++k) {
                flatten_4D_pattern(elem_in_pattern[0][k], pattern[0] + k * 25, pattern[0] + k * 25);
            }
#elif defined(TEST_DDR_TO_AXIS)
    for (int i = 3; i >= 0; --i) {
        int* p_ptn = pattern[i];
        fout << std::endl << "************Pattern-" << i << "************" << std::endl;
        std::cout << "[4.1] generating patterns of ctrl-" << i << std::endl;
        if (i == 3) {
            elem_in_pattern[i] = pattern_gen(num_of_pattern[i], p_ptn, URAM_offsets[i]);
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                int* _p_ptn = pattern[i] + j * 25;
                populate_data_gen(_p_ptn, pp_data_in, false, false, fout);
                std::cout << std::endl
                          << "[4.2] elem_in_pattern[" << i << "][" << j << "]=" << elem_in_pattern[i][j] << std::endl;
                sum_elem_of_pattern[i] += elem_in_pattern[i][j];
            }
        } else if (i == 2) {
            std::memcpy(p_ptn, pattern[3] + 25, num_of_pattern[i] * 25 * sizeof(int));
            for (int j = 0; j < num_of_pattern[i]; ++j) {
                p_ptn[j * 25] = URAM_offsets[i][j];
                print_ptn(p_ptn + j * 25);
            }
            // flatten patterns of ctrl-0/1 from 4d to 1d
            for (int k = 0; k < num_of_pattern[i]; ++k) {
                flatten_4D_pattern(elem_in_pattern[3][k + 1], pattern[i] + k * 25, pattern[i] + k * 25);
            }
#endif
        } else {
            elem_in_pattern[i] = pattern_gen(num_of_pattern[i], p_ptn, URAM_offsets[i]);
        }

        std::cout << "enter test...[4.2]" << std::endl;
        sum_elem_of_all_patterns += sum_elem_of_pattern[i];

        std::cout << "enter test...[4.3]" << std::endl;
        fout << "sum_elem_of_all_patterns: " << std::dec << sum_elem_of_all_patterns << std::endl;
        fout << "Total element count[" << std::dec << i << "]: " << std::dec << sum_elem_of_pattern[i] << std::endl;
    }

#if defined(TEST_AXIS_CACHE_AXIS)
    /* backup patterns of ctrl-2 */
    std::memcpy(pattern_bak, pattern[2], num_of_pattern[2] * 25 * sizeof(int));

    /* flatten patterns of ctrl-0,2 from 4d to 1d */
    for (int k = 0; k < num_of_pattern[0]; ++k) {
        flatten_4D_pattern(elem_in_pattern[0][k], pattern[0] + k * 25, pattern[0] + k * 25);
    }
    for (int k = 0; k < num_of_pattern[2]; ++k) {
        flatten_4D_pattern(elem_in_pattern[0][k + 1], pattern[2] + k * 25, pattern[2] + k * 25);
    }
#endif
    std::cout << "[5.0] generating program memory..." << std::endl;

    /* generate program memory for ALUs */
    uint32_t pm_sz[4] = {0, 0, 0, 0};
    uint32_t* pm[4] = {(uint32_t*)malloc(1024 * sizeof(uint32_t)), (uint32_t*)malloc(1024 * sizeof(uint32_t)),
                       (uint32_t*)malloc(1024 * sizeof(uint32_t)), (uint32_t*)malloc(1024 * sizeof(uint32_t))};
#if defined(TEST_AXIS_CACHE_AXIS)
    pm_gen_case_1(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_AXIS_TO_DDR)
    pm_gen_case_2(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#elif defined(TEST_DDR_TO_AXIS)
    pm_gen_case_3(num_of_pattern, pm[0], pm[1], pm[2], pm[3], pm_sz[0], pm_sz[1], pm_sz[2], pm_sz[3]);
#endif
    std::cout << "[5.1] dumping program memory..." << std::endl;
    fout << std::endl << "**********DUMP PM BEGIN**********" << std::endl;
    for (uint32_t i = 0; i < 4; ++i) {
        fout << std::endl << "pm-of-ctrl-" << i << ", sz: " << std::dec << pm_sz[i] << std::endl;
        for (uint32_t j = 0; j < pm_sz[i]; ++j) {
            fout << std::hex << "0x" << *(pm[i] + j) << std::endl;
        }
    }
    fout << std::endl << "**********DUMP PM END**********" << std::endl;

    std::cout << "[6.0] writing ptn&pm into cfg..." << std::endl;
    /* copy ptn&pm into ROM */
    int wr_ptr = 0;

    for (int i = 0; i < 8; ++i) {
        if (i < 4) {
            // write pattern into ROM
            int ptn_sz = num_of_pattern[i] * 25;
            cfg_buf[wr_ptr] = ptn_sz;
            wr_ptr += 2;
            if (num_of_pattern[i]) {
                std::memcpy(cfg_buf + wr_ptr, pattern[i], ptn_sz * sizeof(uint32_t));
                wr_ptr += ptn_sz;
                if (ptn_sz % 2 != 0) wr_ptr++;
            }
        } else {
            // write pm into ROM
            if (pm_sz[i - 4] % 2 != 0) pm_sz[i - 4] += 1;
            cfg_buf[wr_ptr] = pm_sz[i - 4];
            wr_ptr += 2;
            std::memcpy(cfg_buf + wr_ptr, pm[i - 4], pm_sz[i - 4] * sizeof(uint32_t));
            wr_ptr += pm_sz[i - 4];
        }
    }

    std::cout << "[7.0] launching kernels..." << std::endl;

    cfg_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, cfg_sz, /*OFFSET=*/0);
#if defined(TEST_DDR_TO_AXIS)
    dm_data_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, data_sz, /*OFFSET=*/0);
#else
    sink_in_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, data_sz, /*OFFSET=*/0);
#endif

#if defined(TEST_AXIS_CACHE_AXIS)
    for (int i = 0; i < num_of_pattern[2]; ++i) {
        sum_elem_of_pattern[2] += elem_in_pattern[0][i + 1];
    }
#elif defined(TEST_DDR_TO_AXIS)
    for (int i = 0; i < num_of_pattern[2]; ++i) {
        sum_elem_of_pattern[2] += elem_in_pattern[3][i + 1];
    }
#endif

    uint64_t cfg_bo_addr = cfg_bo->address();
    uint64_t dm_data_bo_addr = dm_data_bo->address();

    // bidm
    dm_krnl->write_register(0x10, cfg_bo_addr);
    dm_krnl->write_register(0x14, (cfg_bo_addr >> 32));
    dm_krnl->write_register(0x1C, dm_data_bo_addr);
    dm_krnl->write_register(0x20, (dm_data_bo_addr >> 32));
    dm_krnl->write_register(0x28, dm_data_bo_addr);
    dm_krnl->write_register(0x2C, (dm_data_bo_addr >> 32));

#if defined(TEST_AXIS_CACHE_AXIS)
    uint64_t sink_in_bo_addr = sink_in_bo->address();
    uint64_t sink_out_bo_addr = sink_out_bo->address();
    // mm2s
    sink_s2u_krnl->write_register(0x10, sink_in_bo_addr);
    sink_s2u_krnl->write_register(0x14, (sink_in_bo_addr >> 32));
    sink_s2u_krnl->write_register(0x1C, sum_elem_of_pattern[0] * sizeof(uint64_t));       // num of data to produce
    sink_s2u_krnl->write_register(0x20, sum_elem_of_pattern[0] * sizeof(uint64_t) >> 32); // num of data to produce

    // s2mm
    sink_u2s_krnl->write_register(0x10, sink_out_bo_addr);
    sink_u2s_krnl->write_register(0x14, (sink_out_bo_addr >> 32));
    sink_u2s_krnl->write_register(0x1C, sum_elem_of_pattern[2] * sizeof(uint64_t));       // num of data to consume
    sink_u2s_krnl->write_register(0x20, sum_elem_of_pattern[2] * sizeof(uint64_t) >> 32); // num of data to consume

    // start kernel
    dm_krnl->write_register(0x0, 0x1);       // ap_start
    sink_s2u_krnl->write_register(0x0, 0x1); // ap_start
    sink_u2s_krnl->write_register(0x0, 0x1); // ap_start
    while (true) {
        uint32_t status = dm_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink_s2u_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink_u2s_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    dm_krnl->write_register(0x10, 0x1);
    sink_s2u_krnl->write_register(0x10, 0x1);
    sink_u2s_krnl->write_register(0x10, 0x1);
#elif defined(TEST_AXIS_TO_DDR)
    uint64_t sink_in_bo_addr = sink_in_bo->address();
    sink_s2u_krnl->write_register(0x10, sink_in_bo_addr);
    sink_s2u_krnl->write_register(0x14, (sink_in_bo_addr >> 32));
    sink_s2u_krnl->write_register(0x1C, sum_elem_of_pattern[0] * sizeof(uint64_t));
    sink_s2u_krnl->write_register(0x20, sum_elem_of_pattern[0] * sizeof(uint64_t) >> 32);

    // start kernel
    dm_krnl->write_register(0x0, 0x1);       // ap_start
    sink_s2u_krnl->write_register(0x0, 0x1); // ap_start
    while (true) {
        uint32_t status = dm_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink_s2u_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    dm_krnl->write_register(0x10, 0x1);
    sink_s2u_krnl->write_register(0x10, 0x1);
#elif defined(TEST_DDR_TO_AXIS)
    uint64_t sink_out_bo_addr = sink_out_bo->address();
    // s2mm
    sink_u2s_krnl->write_register(0x10, sink_out_bo_addr);
    sink_u2s_krnl->write_register(0x14, (sink_out_bo_addr >> 32));
    sink_u2s_krnl->write_register(0x1C, sum_elem_of_pattern[2] * sizeof(uint64_t));
    sink_u2s_krnl->write_register(0x20, sum_elem_of_pattern[2] * sizeof(uint64_t) >> 32);

    // start kernel
    dm_krnl->write_register(0x0, 0x1);       // ap_start
    sink_u2s_krnl->write_register(0x0, 0x1); // ap_start
    while (true) {
        uint32_t status = dm_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = sink_u2s_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    dm_krnl->write_register(0x10, 0x1);
    sink_u2s_krnl->write_register(0x10, 0x1);
#endif

    std::cout << "[8.0] kernels exit" << std::endl;

    // check result
    std::cout << "[9.0] running data check..." << std::endl;

    int total_nerr = 0;
    bool dump_right = false;
#if defined(TEST_AXIS_CACHE_AXIS)
    int hw_ptr = 0;
    sink_out_bo->sync(XCL_BO_SYNC_BO_FROM_DEVICE, data_sz, 0);
    auto chk_data_buf = sink_out_bo->map<uint64_t*>();
    for (int p = 0; p < num_of_pattern[2]; p++) {
        int* current_ptn = pattern_bak + 25 * p + 1;
#elif defined(TEST_AXIS_TO_DDR)
    dm_data_bo->sync(XCL_BO_SYNC_BO_FROM_DEVICE, data_sz, 0);
    auto chk_data_buf = dm_data_bo->map<uint64_t*>();
    for (int p = 0; p < num_of_pattern[1]; p++) {
        int* current_ptn = pattern[1] + 25 * p + 1;
#elif defined(TEST_DDR_TO_AXIS)
    int hw_ptr = 0;
    sink_out_bo->sync(XCL_BO_SYNC_BO_FROM_DEVICE, data_sz, 0);
    auto chk_data_buf = sink_out_bo->map<uint64_t*>();
    for (int p = 0; p < num_of_pattern[2]; p++) {
        int* current_ptn = pattern[3] + 25 * (p + 1) + 1;
#endif
        int* buf_dim = current_ptn;
        int* offset = current_ptn + 4;
        int* tiling = current_ptn + 8;
        int* dim_idx = current_ptn + 12;
        int* stride = current_ptn + 16;
        int* wrap = current_ptn + 20;
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
#if defined(TEST_AXIS_TO_DDR)
                                        uint64_t hw_result = chk_data_buf[golden];
#else
                                        uint64_t hw_result = chk_data_buf[hw_ptr++];
#endif
                                        if (golden != hw_result) {
                                            nerr++;
                                            fout << "[error]golden=" << golden << " | hw_result=" << hw_result
                                                 << std::endl;
                                        } else if (dump_right) {
                                            fout << "[right]golden=" << golden << " | hw_result=" << hw_result
                                                 << std::endl;
                                        }
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
            std::cout << "[Test] Check passed for patterns of pattern-" << p << std::endl;
        else
            std::cout << "[Test] nerr:" << std::dec << nerr << std::endl;
    }

    /* free host memory */
    for (int i = 0; i < 4; ++i) {
        free(pm[i]);
        free(pattern[i]);
        free(elem_in_pattern[i]);
    }

    if (fout && fout.is_open()) {
        fout.close();
    }

#if defined(TEST_AXIS_CACHE_AXIS)
    delete sink_s2u_krnl;
    delete sink_u2s_krnl;
    delete sink_in_bo;
    delete sink_out_bo;
#elif defined(TEST_AXIS_TO_DDR)
    delete sink_s2u_krnl;
    delete sink_in_bo;
#elif defined(TEST_DDR_TO_AXIS)
    delete sink_u2s_krnl;
    delete sink_out_bo;
#endif
    delete dm_krnl;
    delete cfg_bo;
    delete dm_data_bo;
    return total_nerr;
}