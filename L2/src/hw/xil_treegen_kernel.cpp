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
#include "treegen_kernel.hpp"

// Balance the tree
inline void reduceHeap(
    uint32_t* tree_freq, uint32_t* heap, uint8_t* depth, uint16_t heapLength, uint16_t curr_val, uint16_t startIdx) {
#pragma HLS INLINE
reduceheap:
    for (uint32_t idx = startIdx << 1; idx <= heapLength; idx <<= 1) {
#pragma HLS LOOP_TRIPCOUNT min = 512 max = 512
#pragma HLS PIPELINE II = 1
        // Set idx to least of two child nodes
        if (idx < heapLength && findmin(tree_freq, heap[idx + 1], heap[idx], depth)) idx++;

        // Terminate if curr_val is smaller than leave nodes
        if (findmin(tree_freq, curr_val, heap[idx], depth)) break;

        // Replace curr_val with least child
        heap[startIdx] = heap[idx];
        startIdx = idx;
    }

    heap[startIdx] = curr_val;
}

#define LOW 1
#define MAX(a, b) (a >= b ? a : b)

inline uint16_t reverseBits(uint16_t code, int len) {
#pragma HLS INLINE
    uint16_t res = 0;
bitreverse:
    for (uint32_t i = len; i > 0; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
        res |= code & 1;
        code >>= 1;
        res <<= 1;
    }

    return res >> 1;
}

/* the arguments must not have side effects */
template <uint32_t ELEMS, uint32_t MAX_LENGTH>
inline uint32_t constructTree(
    uint32_t* tree_freq, uint32_t* tree_codes, uint32_t* tree_blen, uint32_t* tree_root, uint32_t base_extended) {
#pragma HLS INLINE
    const uint32_t elems = ELEMS;
    const uint32_t max_length = MAX_LENGTH;

    // Upper boundary of non-zero value
    // for a given tree
    int lcl_max_code = -1;

    uint32_t node = elems;
    uint8_t depth[LTREE_SIZE];
    uint32_t heap[LTREE_SIZE];

    uint32_t heapLength = 0;
    uint32_t max_heap = HEAP_SIZE;
    tree_freq[256] = 1;

// Initial heap built wrt least frequent in
// heap location smallest i.e., 1 and leafs
// @ 2*n and 2*n+1 locations
freq_cnt:
    for (uint32_t n = 0; n < elems; n++) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
#pragma HLS PIPELINE II = 1
        if (tree_freq[n] != 0) {
            heap[++heapLength] = lcl_max_code = n;
            depth[n] = 0;
        } else {
            tree_blen[n] = 0;
        }
    }

    const uint32_t s_trip = elems / 2;
// Create subheap in increasing order
subheap:
    for (uint32_t node_lcl = heapLength / 2; node_lcl >= 1; node_lcl--) {
#pragma HLS LOOP_TRIPCOUNT min = s_trip max = s_trip
#pragma HLS PIPELINE II = 1
        reduceHeap(tree_freq, heap, depth, heapLength, heap[node_lcl], node_lcl);
    }

// Construct the huffman tree by repeatedly combining the least two frequent
// nodes
huffman_tree:
    for (; heapLength >= 2;) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
#pragma HLS PIPELINE II = 1
        uint32_t node_trav = heap[LOW];
        heap[LOW] = heap[heapLength--];
        reduceHeap(tree_freq, heap, depth, heapLength, heap[LOW], LOW);
        uint32_t m = heap[LOW]; // m = node of least frequency

        heap[--max_heap] = node_trav; // keep the nodes sorted by frequency
        heap[--max_heap] = m;

        // Create a new node father of node_lcl and m
        tree_freq[node] = tree_freq[node_trav] + tree_freq[m];
        depth[node] = (uint8_t)(MAX(depth[node_trav], depth[m]) + 1);
        tree_root[node_trav] = tree_root[m] = (uint16_t)node;

        // and insert the new node in the heap
        heap[LOW] = node++;
        reduceHeap(tree_freq, heap, depth, heapLength, heap[LOW], LOW);
    }

    heap[--max_heap] = heap[LOW];

    // ***********************************************************
    //              Generate Bit Lengths
    // ***********************************************************

    int base = base_extended;
    int overflow = 0;
    uint16_t bitlength_cntr[MAX_BITS + 1];

// Initialize bitlength_cntr array
genbitlen:
    for (uint32_t bits = 0; bits <= MAX_BITS; bits++) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
        bitlength_cntr[bits] = 0;
    }

    // Calculate bitlength
    // overflow may occur for bltree
    // Below is the top most element on heap
    tree_blen[heap[max_heap]] = 0;

    uint32_t temp_h = 0;
blen_update:
    for (uint32_t h = max_heap + 1; h < HEAP_SIZE; h++) {
#pragma HLS LOOP_TRIPCOUNT min = 573 max = 573
#pragma HLS PIPELINE II = 1
        uint32_t n_bgen = heap[h];
        uint32_t bits = tree_blen[tree_root[n_bgen]] + 1;

        if (bits > max_length) {
            bits = max_length;
            overflow++;
        }
        tree_blen[n_bgen] = bits;

        // Overwrite dad tree
        if (n_bgen > lcl_max_code) continue; // not a leaf node

        bitlength_cntr[bits]++;
        temp_h = h + 1;
    } // for loop bit length gen ends
    // printf("after a loop \n");

    if (overflow != 0) {
    // printf("Inside overflow \n");
    // Find the first bit length which could increase
    overflow_1:
        for (uint32_t i_over = overflow; i_over > 0; i_over -= 2) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
            uint32_t bits = max_length - 1;

            while (bitlength_cntr[bits] == 0)
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
                bits--;

            bitlength_cntr[bits]--;        // move one leaf down the tree
            bitlength_cntr[bits + 1] += 2; // move one overflow item as its brother
            bitlength_cntr[max_length]--;
        }

    // printf("after bitlength do-while \n");
    // Now recompute all bit lengths scanning in increasing frequency
    // h is still equal to heapsize
    // It is easier to reconstruct all the lengths instead of fixing only
    // the wrong ones.
    overflow_2:
        for (uint32_t bits = max_length; bits != 0; bits--) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
            // printf("berore bitlength_cntr \n");
            uint32_t n_bgen = bitlength_cntr[bits];

            while (n_bgen != 0) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
                // printf("berore heap\n");
                uint32_t m_bgen = heap[--temp_h];
                if (m_bgen > lcl_max_code) continue;

                if (tree_blen[m_bgen] != bits) {
                    // printf("berore tree_blen\n");
                    tree_blen[m_bgen] = bits;
                    // printf("after tree_blen\n");
                }
                n_bgen--;
            }
        }
        // printf("after bits maxlength loop \n");
    }

    // printf("Before Generate Codes \n");
    // ***********************************************************
    //              Generate Codes
    // ***********************************************************
    uint16_t next_code[MAX_BITS + 1]; // Next code value for each bit length
    uint16_t code = 0;                // Running code value

// Use distribution counts to generate the code values
// without bit reversal
gencode:
    for (uint32_t bits = 1; bits <= MAX_BITS; bits++) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
        code = (code + bitlength_cntr[bits - 1]) << 1;
        next_code[bits] = code;
    }

code_update:
    for (uint32_t n_cgen = 0; n_cgen <= lcl_max_code; n_cgen++) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
#pragma HLS PIPELINE II = 1
        uint32_t len = tree_blen[n_cgen];
        if (len == 0) continue;
        // Now reverse the bits
        tree_codes[n_cgen] = reverseBits(next_code[len]++, len);
        // printf("n_cgen %d code %d tree_blen %d\n", n_cgen, tree_codes[n_cgen], tree_blen[n_cgen]);
    }

    return lcl_max_code;
}

extern "C" {
#if (T_COMPUTE_UNIT == 1)
void xilTreegen_cu1
#elif (T_COMPUTE_UNIT == 2)
void xilTreegen_cu2
#endif
    (uint32_t* dyn_ltree_freq,
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

    // printf("In treegen kernel %d\n", (uint32_t)T_COMPUTE_UNIT);

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
        uint32_t lit_max_code = constructTree<L_CODES, MAX_BITS>(lcl_ltree_freq, lcl_ltree_codes, lcl_ltree_blen,
                                                                  lcl_ltree_root, (uint32_t)LITERALS + 1);

        // printf("Before distance tree gen \n");
        // Build distances codes & bit lengths
        uint32_t dst_max_code =
            constructTree<D_CODES, MAX_BITS>(lcl_dtree_freq, lcl_dtree_codes, lcl_dtree_blen, lcl_dtree_root, 0);

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
        uint32_t max_blindex = constructTree<BL_CODES, MAX_BL_BITS>(lcl_bltree_freq, lcl_bltree_codes, lcl_bltree_blen,
                                                                     lcl_bltree_root, (uint32_t)0);

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
