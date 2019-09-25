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
#ifndef _XFCOMPRESSION_DEFLATE_TREES_HPP_
#define _XFCOMPRESSION_DEFLATE_TREES_HPP_

// Find the minimum among the two subtrees
#define findmin(tree_freq, n, m, depth) \
    (tree_freq[n] < tree_freq[m] || (tree_freq[n] == tree_freq[m] && depth[n] <= depth[m]))

#define MAX_BITS 15

#define LOW 1
#define MAX(a, b) (a >= b ? a : b)

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

// Balance the tree
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
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[node_lcl], node_lcl);
    }

// Construct the huffman tree by repeatedly combining the least two frequent
// nodes
huffman_tree:
    for (; heapLength >= 2;) {
#pragma HLS LOOP_TRIPCOUNT min = elems max = elems
#pragma HLS PIPELINE II = 1
        uint32_t node_trav = heap[LOW];
        heap[LOW] = heap[heapLength--];
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[LOW], LOW);
        uint32_t m = heap[LOW]; // m = node of least frequency

        heap[--max_heap] = node_trav; // keep the nodes sorted by frequency
        heap[--max_heap] = m;

        // Create a new node father of node_lcl and m
        tree_freq[node] = tree_freq[node_trav] + tree_freq[m];
        depth[node] = (uint8_t)(MAX(depth[node_trav], depth[m]) + 1);
        tree_root[node_trav] = tree_root[m] = (uint16_t)node;

        // and insert the new node in the heap
        heap[LOW] = node++;
        xf::compression::reduceHeap(tree_freq, heap, depth, heapLength, heap[LOW], LOW);
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

} // Compression
} // End of xf

#endif // _XFCOMPRESSION_DEFLATE_TREES_HPP_
