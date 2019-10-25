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
#ifndef _XFCOMPRESSION_HUFFMAN_TREEGEN_HPP_
#define _XFCOMPRESSION_HUFFMAN_TREEGEN_HPP_

/**
 * @file deflate_trees.hpp
 * @brief Header for modules used in Tree Generation kernel.
 *
 * This file is part of XF Compression Library.
 */

bool findmin(uint32_t* tree_freq, uint32_t val1, uint32_t val2, uint8_t* tree_dist) {
#pragma HLS INLINE
    bool result = (tree_freq[val1] < tree_freq[val2] ||
                   (tree_freq[val1] == tree_freq[val2] && tree_dist[val1] <= tree_dist[val2]));
    return result;
}

#define MAX_BITS 15

#define LEAST_VAL 1

namespace xf {
namespace compression {

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

inline void reduceHeap(
    uint32_t* tree_freq, uint32_t* heap, uint8_t* depth, uint16_t heapLength, uint16_t curr_val, uint16_t startIdx) {
#pragma HLS INLINE
reduceHeap:
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


template <uint32_t ELEMS, uint32_t MAX_LENGTH>
inline uint32_t huffConstructTree(uint32_t* tree_freq, uint32_t* tree_codes, uint32_t* tree_blen, uint32_t* tree_root) {
#pragma HLS INLINE
/**
 * @brief This module generates huffman codes for either literal, distance or
 * bitlength data.
 *
 * @param tree_freq input frequency data of literals, distances or bit lengths
 * @param tree_codes output huffman codes data
 * @param tree_blen output huffan codes bit lengths
 * @param tree_root input buffer to construct codes and bit length information
 */
    const uint32_t elems = ELEMS;
    const uint32_t max_length = MAX_LENGTH;

    int lcl_max_code = -1;

    uint32_t node = elems;
    uint8_t depth[LTREE_SIZE];
    uint32_t heap[LTREE_SIZE];

    uint32_t heapLength = 0;
    uint32_t max_heap = HEAP_SIZE;
    tree_freq[256] = 1;

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
subheap:
    for (uint32_t node_lcl = heapLength / 2; node_lcl >= 1; node_lcl--) {
#pragma HLS LOOP_TRIPCOUNT min = s_trip max = s_trip
#pragma HLS PIPELINE II = 1
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[node_lcl], node_lcl);
    }

huffman_tree:
    for (; heapLength >= 2;) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
#pragma HLS PIPELINE II = 1
        uint32_t node_trav = heap[LEAST_VAL];
        heap[LEAST_VAL] = heap[heapLength--];
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[LEAST_VAL], LEAST_VAL);
        uint32_t h_val = heap[LEAST_VAL];

        heap[--max_heap] = node_trav;
        heap[--max_heap] = h_val;

        tree_freq[node] = tree_freq[node_trav] + tree_freq[h_val];
        depth[node] = (uint8_t)((depth[node_trav] >= (depth[h_val] + 1)) ? depth[node_trav] : (depth[h_val] + 1));
        tree_root[node_trav] = tree_root[h_val] = (uint16_t)node;

        heap[LEAST_VAL] = node++;
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[LEAST_VAL], LEAST_VAL);
    }

    heap[--max_heap] = heap[LEAST_VAL];

    int overflow = 0;
    uint16_t bitlength_cntr[MAX_BITS + 1];

genbitlen:
    for (uint32_t bits = 0; bits <= MAX_BITS; bits++) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
        bitlength_cntr[bits] = 0;
    }

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

        if (n_bgen > lcl_max_code) continue;

        bitlength_cntr[bits]++;
        temp_h = h + 1;
    }

    if (overflow != 0) {
    overflow_1:
        for (uint32_t i_over = overflow; i_over > 0; i_over -= 2) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
            uint32_t bits = max_length - 1;

            while (bitlength_cntr[bits] == 0)
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
                bits--;

            bitlength_cntr[bits]--;
            bitlength_cntr[bits + 1] += 2;
            bitlength_cntr[max_length]--;
        }

    overflow_2:
        for (uint32_t bits = max_length; bits != 0; bits--) {
#pragma HLS LOOP_TRIPCOUNT min = 15 max = 15
#pragma HLS PIPELINE II = 1
            uint32_t n_bgen = bitlength_cntr[bits];

            while (n_bgen != 0) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
                uint32_t m_bgen = heap[--temp_h];
                if (m_bgen > lcl_max_code) continue;

                if (tree_blen[m_bgen] != bits) {
                    tree_blen[m_bgen] = bits;
                }
                n_bgen--;
            }
        }
    }

    uint16_t next_code[MAX_BITS + 1];
    uint16_t code = 0;

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
        tree_codes[n_cgen] = reverseBits(next_code[len]++, len);
    }

    return lcl_max_code;
}

} // Compression
} // End of xf

#endif // _XFCOMPRESSION_DEFLATE_TREES_HPP_
