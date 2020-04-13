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
 * @file zlib_treegen_mm.cpp
 * @brief Source for treegen kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_treegen_mm.hpp"

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
    const uint16_t c_ltree_size = xf::compression::details::c_ltree_size;
    const uint16_t c_dtree_size = xf::compression::details::c_dtree_size;
    const uint16_t c_bltree_size = xf::compression::details::c_bltree_size;
    const uint16_t c_extra_blcodes = 32;

    // Literal & Match Length content
    uint32_t lcl_ltree_freq[c_ltree_size];
    uint32_t lcl_ltree_codes[c_ltree_size];
    uint32_t lcl_ltree_blen[c_ltree_size];
    uint32_t lcl_ltree_root[c_ltree_size];

    // Distances content
    uint32_t lcl_dtree_freq[c_dtree_size];
    uint32_t lcl_dtree_codes[c_dtree_size];
    uint32_t lcl_dtree_blen[c_dtree_size];
    uint32_t lcl_dtree_root[c_dtree_size];

    // BL tree content
    uint32_t lcl_bltree_freq[c_bltree_size];
    uint32_t lcl_bltree_codes[c_bltree_size];
    uint32_t lcl_bltree_blen[c_bltree_size];
    uint32_t lcl_bltree_root[c_bltree_size];

    for (uint8_t core_idx = 0; core_idx < blocks_per_chunk; core_idx++) {
    // Copy Frequencies of Literal, Match Length and Distances to local buffers
    copy_ltree_freq:
        for (uint32_t ci = 0; ci < c_ltree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_ltree_freq[ci] = dyn_ltree_freq[core_idx * c_ltree_size + ci];
        }
    copy_dtree_freq:
        for (uint32_t ci = 0; ci < c_dtree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_dtree_freq[ci] = dyn_dtree_freq[core_idx * c_dtree_size + ci];
        }

        // Build Literal and Match length tree codes & bit lenghts
        uint32_t lit_max_code = xf::compression::huffConstructTree<LITERAL_CODES, MAX_BITS>(
            lcl_ltree_freq, lcl_ltree_codes, lcl_ltree_blen, lcl_ltree_root);

        // Build distances codes & bit lengths
        uint32_t dst_max_code = xf::compression::huffConstructTree<DISTANCE_CODES, MAX_BITS>(
            lcl_dtree_freq, lcl_dtree_codes, lcl_dtree_blen, lcl_dtree_root);

        uint8_t bitlen_vals[c_extra_blcodes] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    bltree_init:
        for (int i = 0; i < c_bltree_size; i++) {
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

            int prevlen = -1;
            int curlen = 0;
            int count = 0;
            int max_count = 7;
            int min_count = 4;
            int nextlen = tree_len[0];

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            tree_len[max_code + 1] = (uint16_t)0xffff;

        parse_tdata:
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
                    lcl_bltree_freq[REUSE_PREV_BLEN]++;
                } else if (count <= 10) {
                    lcl_bltree_freq[REUSE_ZERO_BLEN]++;
                } else {
                    lcl_bltree_freq[REUSE_ZERO_BLEN_7]++;
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
        uint32_t max_blindex = xf::compression::huffConstructTree<BL_CODES, MAX_BL_BITS>(
            lcl_bltree_freq, lcl_bltree_codes, lcl_bltree_blen, lcl_bltree_root);

    bltree_blen:
        for (max_blindex = BL_CODES - 1; max_blindex >= 3; max_blindex--) {
            if (lcl_bltree_blen[bitlen_vals[max_blindex]] != 0) break;
        }

        max_codes[core_idx * 3 + 0] = lit_max_code;
        max_codes[core_idx * 3 + 1] = dst_max_code;
        max_codes[core_idx * 3 + 2] = max_blindex;

    // Copy data back to ddr  -- Literals / MLs
    copy2ddr_ltree_codes:
        for (uint32_t ci = 0; ci < c_ltree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_ltree_codes[core_idx * c_ltree_size + ci] = lcl_ltree_codes[ci];
        }
    copy2ddr_ltree_blen:
        for (uint32_t ci = 0; ci < c_ltree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_ltree_blen[core_idx * c_ltree_size + ci] = lcl_ltree_blen[ci];
        }

    // Copy data back to ddr -- Distances
    copy2ddr_dtree_codes:
        for (uint32_t ci = 0; ci < c_dtree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_dtree_codes[core_idx * c_dtree_size + ci] = lcl_dtree_codes[ci];
        }
    copy2ddr_dtree_blen:
        for (uint32_t ci = 0; ci < c_dtree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_dtree_blen[core_idx * c_dtree_size + ci] = lcl_dtree_blen[ci];
        }

    // Copy data back to ddr -- Bit Lengths
    copy2ddr_bltree_codes:
        for (uint32_t ci = 0; ci < c_bltree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_bltree_codes[core_idx * c_bltree_size + ci] = lcl_bltree_codes[ci];
        }
    copy2ddr_bltree_blen:
        for (uint32_t ci = 0; ci < c_bltree_size; ++ci) {
#pragma HLS PIPELINE II = 1
            dyn_bltree_blen[core_idx * c_bltree_size + ci] = lcl_bltree_blen[ci];
        }
    }
}
}
