/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */

/**
 * @file xil_treegen_kernel.cpp
 * @brief Source for treegen kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "treegen_kernel.hpp"

extern "C" {
void xilTreegenKernel(uint32_t* dyn_ltree_freq,
                      uint32_t* dyn_dtree_freq,
                      uint32_t* dyn_bltree_freq,
                      uint32_t* dyn_ltree_codes,
                      uint32_t* dyn_dtree_codes,
                      uint32_t* dyn_bltree_codes,
                      uint32_t* dyn_ltree_blen,
                      uint32_t* dyn_dtree_blen,
                      uint32_t* dyn_bltree_blen,
                      uint32_t* max_codes,
                      uint32_t block_size_in_kb,
                      uint32_t input_size,
                      uint32_t blocks_per_chunk) {
#pragma HLS INTERFACE m_axi port = dyn_ltree_freq offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_dtree_freq offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_bltree_freq offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_ltree_codes offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_dtree_codes offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_bltree_codes offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_ltree_blen offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_dtree_blen offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_bltree_blen offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = max_codes offset = slave bundle = gmem0

#pragma HLS INTERFACE s_axilite port = dyn_ltree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bltree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_ltree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bltree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_ltree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bltree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = max_codes bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = blocks_per_chunk bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Literal & Match Length content
    uint32_t lcl_ltree_freq[LTREE_SIZE];
    uint32_t lcl_ltree_codes[LTREE_SIZE];
    uint32_t lcl_ltree_blen[LTREE_SIZE];
    uint32_t lcl_ltree_root[LTREE_SIZE];

    // Distances content
    uint32_t lcl_dtree_freq[DTREE_SIZE];
    uint32_t lcl_dtree_codes[DTREE_SIZE];
    uint32_t lcl_dtree_blen[DTREE_SIZE];
    uint32_t lcl_dtree_root[DTREE_SIZE];

    // BL tree content
    uint32_t lcl_bltree_freq[BLTREE_SIZE];
    uint32_t lcl_bltree_codes[BLTREE_SIZE];
    uint32_t lcl_bltree_blen[BLTREE_SIZE];
    uint32_t lcl_bltree_root[BLTREE_SIZE];

    for (uint8_t core_idx = 0; core_idx < blocks_per_chunk; core_idx++) {
        // Copy Frequencies of Literal, Match Length and Distances to local buffers
        memcpy(lcl_ltree_freq, &dyn_ltree_freq[core_idx * LTREE_SIZE], LTREE_SIZE * sizeof(uint32_t));
        memcpy(lcl_dtree_freq, &dyn_dtree_freq[core_idx * DTREE_SIZE], DTREE_SIZE * sizeof(uint32_t));

        // printf("Before literal tree gen \n");
        // Build Literal and Match length tree codes & bit lenghts
        uint32_t lit_max_code = xf::compression::constructTree<L_CODES, MAX_BITS>(
            lcl_ltree_freq, lcl_ltree_codes, lcl_ltree_blen, lcl_ltree_root, (uint32_t)LITERALS + 1);

        // printf("Before distance tree gen \n");
        // Build distances codes & bit lengths
        uint32_t dst_max_code = xf::compression::constructTree<D_CODES, MAX_BITS>(lcl_dtree_freq, lcl_dtree_codes,
                                                                                  lcl_dtree_blen, lcl_dtree_root, 0);

        uint8_t bl_order[EXTRA_BLCODES] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    /* The lengths of the bit length codes are sent in order of decreasing
     *  * probability, to avoid transmitting the lengths for unused bit length
     *  codes.
     */
    bltree_init:
        for (int i = 0; i < BLTREE_SIZE; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
#pragma HLS PIPELINE II = 1
            lcl_bltree_freq[i] = 0;
        }

        uint32_t* tree_len = NULL;
        uint32_t max_code = 0;

        for (uint32_t pt = 0; pt < 2; pt++) {
            if (pt == 0) {
                tree_len = lcl_ltree_blen;
                max_code = lit_max_code;
            } else {
                tree_len = lcl_dtree_blen;
                max_code = dst_max_code;
            }

            int prevlen = -1;          // Last emitted length
            int curlen = 0;            // Length of current code
            int count = 0;             // Repeat count of the current code
            int max_count = 7;         // Max repeat count
            int min_count = 4;         // Min repeat count
            int nextlen = tree_len[0]; // Length of next code

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            tree_len[max_code + 1] = (uint16_t)0xffff;

        scan_tree:
            for (uint32_t n = 0; n <= max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 286 max = 286
#pragma HLS PIPELINE II = 1
                curlen = nextlen;
                nextlen = tree_len[n + 1];

                if (++count < max_count && curlen == nextlen)
                    continue;
                else if (count < min_count)
                    lcl_bltree_freq[curlen] += count;
                else if (curlen != 0) {
                    if (curlen != prevlen) lcl_bltree_freq[curlen]++;
                    lcl_bltree_freq[REP_3_6]++;
                } else if (count <= 10) {
                    lcl_bltree_freq[REPZ_3_10]++;
                } else {
                    lcl_bltree_freq[REPZ_11_138]++;
                }
                count = 0;
                prevlen = curlen;
                if (nextlen == 0) {
                    max_count = 138, min_count = 3;
                } else if (curlen == nextlen) {
                    max_count = 6, min_count = 3;
                } else {
                    max_count = 7, min_count = 4;
                }
            }
        }

        /* Build the bit length tree */
        uint32_t max_blindex = xf::compression::constructTree<BL_CODES, MAX_BL_BITS>(
            lcl_bltree_freq, lcl_bltree_codes, lcl_bltree_blen, lcl_bltree_root, (uint32_t)0);

    bltree_blen:
        for (max_blindex = BL_CODES - 1; max_blindex >= 3; max_blindex--) {
            if (lcl_bltree_blen[bl_order[max_blindex]] != 0) break;
        }

        max_codes[core_idx * 3 + 0] = lit_max_code;
        max_codes[core_idx * 3 + 1] = dst_max_code;
        max_codes[core_idx * 3 + 2] = max_blindex;

        // Copy data back to ddr  -- Literals / MLs
        memcpy(&dyn_ltree_codes[core_idx * LTREE_SIZE], &lcl_ltree_codes[0], LTREE_SIZE * sizeof(uint32_t));
        memcpy(&dyn_ltree_blen[core_idx * LTREE_SIZE], &lcl_ltree_blen[0], LTREE_SIZE * sizeof(uint32_t));

        // Copy data back to ddr -- Distances
        memcpy(&dyn_dtree_codes[core_idx * DTREE_SIZE], &lcl_dtree_codes[0], DTREE_SIZE * sizeof(uint32_t));
        memcpy(&dyn_dtree_blen[core_idx * DTREE_SIZE], &lcl_dtree_blen[0], DTREE_SIZE * sizeof(uint32_t));

        // Copy data back to ddr -- Bit Lengths
        memcpy(&dyn_bltree_codes[core_idx * BLTREE_SIZE], &lcl_bltree_codes[0], BLTREE_SIZE * sizeof(uint32_t));
        memcpy(&dyn_bltree_blen[core_idx * BLTREE_SIZE], &lcl_bltree_blen[0], BLTREE_SIZE * sizeof(uint32_t));
    }
}
}
