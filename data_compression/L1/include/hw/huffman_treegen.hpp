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
 * @file huffman_treegen.hpp
 * @brief Header for modules used in Tree Generation kernel.
 *
 * This file is part of XF Compression Library.
 */
#include "zlib_specs.hpp"

namespace xf {
namespace compression {

const static uint8_t c_tgnSymbolBits = 10;
const static uint8_t c_tgnBitlengthBits = 5;

const static uint8_t c_lengthHistogram = 32;

const static uint16_t c_tgnSymbolSize = c_litCodeCount;
const static uint16_t c_tgnTreeDepth = c_litCodeCount;
const static uint16_t c_tgnMaxBits = c_maxCodeBits;

// typedef ap_uint<c_frequency_bits> Frequency;
typedef uint32_t Frequency;

typedef ap_uint<12> Histogram;

struct Symbol {
    ap_uint<c_tgnSymbolBits> value;
    Frequency frequency;
};

struct Codeword {
    ap_uint<c_maxBits> codeword;
    ap_uint<c_tgnBitlengthBits> bitlength;
};

namespace details {

// local types and constants
const static uint8_t RADIX = 16;
const static uint8_t BITS_PER_LOOP = 4;

const static ap_uint<c_tgnSymbolBits> INTERNAL_NODE = -1;
typedef ap_uint<BITS_PER_LOOP> Digit;

void filter(hls::stream<Frequency>& inFreq,
            Symbol* heap,
            uint16_t* heapLength,
            hls::stream<ap_uint<c_tgnSymbolBits> >& symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    uint16_t smLen = 0;
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        Frequency cf = inFreq.read();
        if (n == 256) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = 1;
            ++hpLen;
        } else if (cf != 0) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = cf;
            ++hpLen;
        }
    }

    heapLength[0] = hpLen;
    symLength << smLen;
}

void filter(Frequency* inFreq, Symbol* heap, uint16_t* heapLength, uint16_t* symLength, uint16_t i_symSize) {
    uint16_t hpLen = 0;
    uint16_t smLen = 0;
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        Frequency cf = inFreq[n];
        if (n == 256) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = 1;
            ++hpLen;
        } else if (cf != 0) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = cf;
            ++hpLen;
        }
    }

    heapLength[0] = hpLen;
    symLength[0] = smLen;
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void radixSort_1(Symbol* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (int shift = 0; shift < 32; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 8 avg = 8
    init_histogram:
        for (int i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

    compute_histogram:
        for (int j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Symbol val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (int i = 0; i < RADIX - 1; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (int j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void radixSort(Symbol* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (int shift = 0; shift < 32; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 8 avg = 8
    init_histogram:
        for (int i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

    compute_histogram:
        for (int j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
            Symbol val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (int i = 0; i < RADIX - 1; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (int j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void createTree(Symbol* heap,
                int num_symbols,
                ap_uint<SYMBOL_BITS>* parent,
                ap_uint<SYMBOL_SIZE>& left,
                ap_uint<SYMBOL_SIZE>& right,
                Frequency* frequency) {
    ap_uint<SYMBOL_BITS> tree_count = 0; // Number of intermediate node assigned a parent
    ap_uint<SYMBOL_BITS> in_count = 0;   // Number of inputs consumed
    ap_uint<SYMBOL_SIZE> tmp;
    left = 0;
    right = 0;

    // for case with less number of symbols
    if (num_symbols < 3) num_symbols++;
// this loop needs to run at least twice
create_heap:
    for (uint16_t i = 0; i < num_symbols; ++i) {
#pragma HLS PIPELINE II = 3
#pragma HLS LOOP_TRIPCOUNT min = 19 avg = 286 max = 286
        Frequency node_freq = 0;
        Frequency intermediate_freq = frequency[tree_count];
        Symbol s = heap[in_count];
        tmp = 1;
        tmp <<= i;

        if ((in_count < num_symbols && s.frequency <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // left[i] = s.value;       // set input symbol value as left node
            node_freq = s.frequency; // Add symbol frequency to total node frequency
            // move to the next input symbol
            ++in_count;
        } else {
            // pick internal node without a parent
            // left[i] = INTERNAL_NODE;           // Set symbol to indicate an internal node
            left |= tmp;
            node_freq = intermediate_freq; // Add child node frequency
            parent[tree_count] = i;        // Set this node as child's parent
            // Go to next parent-less internal node
            ++tree_count;
        }

        intermediate_freq = frequency[tree_count];
        s = heap[in_count];
        if ((in_count < num_symbols && s.frequency <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // right[i] = s.value;
            frequency[i] = node_freq + s.frequency;
            ++in_count;
        } else {
            // Pick internal node without a parent
            // right[i] = INTERNAL_NODE;
            right |= tmp;
            frequency[i] = node_freq + intermediate_freq;
            parent[tree_count] = i;
            ++tree_count;
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void computeBitLength(ap_uint<SYMBOL_BITS>* parent,
                      ap_uint<SYMBOL_SIZE>& left,
                      ap_uint<SYMBOL_SIZE>& right,
                      int num_symbols,
                      Histogram* length_histogram,
                      Frequency* child_depth) {
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
// this loop needs to run at least once
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS pipeline II = 2
        tmp = 1;
        tmp <<= i;
        uint32_t length = child_depth[parent[i]] + 1;
        child_depth[i] = length;
        bool is_left_internal = ((left & tmp) == 0);
        bool is_right_internal = ((right & tmp) == 0);

        if ((is_left_internal || is_right_internal)) {
            uint32_t children = 1; // One child of the original node was a symbol
            if (is_left_internal && is_right_internal) {
                children = 2; // Both the children of the original node were symbols
            }
            length_histogram[length] += children;
        }
    }
}

void truncateTree(Histogram* length_histogram, uint16_t c_tree_depth, int max_bit_len) {
    int j = max_bit_len;
move_nodes:
    for (uint16_t i = c_tree_depth - 1; i > max_bit_len; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 572 max = 572 avg = 572
#pragma HLS PIPELINE II = 1
        // Look to see if there are any nodes at lengths greater than target depth
        int cnt = 0;
    reorder:
        for (; length_histogram[i] != 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 3 avg = 3
            if (j == max_bit_len) {
                // find the deepest leaf with codeword length < target depth
                --j;
                while (length_histogram[j] == 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
                    --j;
                }
            }
            // Move leaf with depth i to depth j + 1
            length_histogram[j] -= 1;     // The node at level j is no longer a leaf
            length_histogram[j + 1] += 2; // Two new leaf nodes are attached at level j+1
            length_histogram[i - 1] += 1; // The leaf node at level i+1 gets attached here
            length_histogram[i] -= 2;     // Two leaf nodes have been lost from  level i

            // now deepest leaf with codeword length < target length
            // is at level (j+1) unless (j+1) == target length
            ++j;
        }
    }
}

template <int SYMBOL_BITS>
void canonizeTree(
    Symbol* sorted, uint32_t num_symbols, Histogram* length_histogram, ap_uint<4>* symbol_bits, uint16_t i_treeDepth) {
    int16_t length = i_treeDepth;
    ap_uint<SYMBOL_BITS> count = 0;
// Iterate across the symbols from lowest frequency to highest
// Assign them largest bit length to smallest
process_symbols:
    for (int k = 0; k < num_symbols; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 286 avg = 286
        if (count == 0) {
            // find the next non-zero bit length k
            count = length_histogram[--length];
            while (count == 0 && length >= 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 avg = 1 max = 2
#pragma HLS PIPELINE II = 1
                // n  is the number of symbols with encoded length k
                count = length_histogram[--length];
            }
        }
        if (length < 0) break;
        symbol_bits[sorted[k].value] = length; // assign symbol k to have length bits
        --count;                               // keep assigning i bits until we have counted off n symbols
    }
}

template <int MAX_LEN>
void createCodeword(ap_uint<4>* symbol_bits,
                    Histogram* length_histogram,
                    Codeword* huffCodes,
                    uint16_t cur_symSize,
                    uint16_t cur_maxBits,
                    uint16_t symCnt) {
    //#pragma HLS inline
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
first_codewords:
    for (int i = 1; i <= cur_maxBits; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 7 max = 15
#pragma HLS PIPELINE II = 1
        first_codeword[i] = (first_codeword[i - 1] + length_histogram[i - 1]) << 1;
    }

assign_codewords_mm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 286 avg = 286
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // if symbol has 0 bits, it doesn't need to be encoded
        LCL_Code_t out_reversed = first_codeword[length];
        out_reversed.reverse();
        out_reversed = out_reversed >> (MAX_LEN - length);

        huffCodes[k].codeword = (length == 0) ? (uint16_t)0 : (uint16_t)out_reversed;
        huffCodes[k].bitlength = length;
        first_codeword[length]++;
    }
    if (symCnt == 0) {
        huffCodes[0].bitlength = 1;
    }
}

template <int MAX_LEN>
void createCodeword(ap_uint<4>* symbol_bits,
                    Histogram* length_histogram,
                    hls::stream<Codeword>& huffCodes,
                    uint16_t cur_symSize,
                    uint16_t cur_maxBits,
                    uint16_t symCnt) {
    //#pragma HLS inline
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
first_codewords:
    for (int i = 1; i <= cur_maxBits; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 7 max = 15
#pragma HLS PIPELINE II = 1
        first_codeword[i] = (first_codeword[i - 1] + length_histogram[i - 1]) << 1;
    }
    Codeword code;
assign_codewords_sm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 286 avg = 286
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // if symbol has 0 bits, it doesn't need to be encoded
        LCL_Code_t out_reversed = first_codeword[length];
        out_reversed.reverse();
        out_reversed = out_reversed >> (MAX_LEN - length);

        code.codeword = (length == 0 || symCnt == 0) ? (uint16_t)0 : (uint16_t)out_reversed;
        code.bitlength = (symCnt == 0) ? 0 : length;
        code.bitlength = (symCnt == 0 && k == 0) ? 1 : length;
        first_codeword[length]++;
        huffCodes << code;
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void huffConstructTreeStream_2(hls::stream<Symbol>& heapStream,
                               hls::stream<uint16_t>& heapLenStream,
                               hls::stream<bool>& eos,
                               hls::stream<Codeword>& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];

    Frequency temp_array[SYMBOL_SIZE];
    Symbol heap[SYMBOL_SIZE];
    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    const int hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                               {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                               {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};

    do {
        if (eos.read() == 1) break;
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            uint16_t i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
            uint16_t i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
            uint16_t i_maxBits = hctMeta[metaIdx][2];    // current max bits

            uint16_t heapLength = heapLenStream.read();

        init_buffers:
            for (int i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
                parent[i] = 0;
                if (i < c_lengthHistogram) length_histogram[i] = 0;
                temp_array[i] = 0;
                if (i < heapLength) heap[i] = heapStream.read();
                symbol_bits[i] = 0;
            }

            // create tree
            createTree<SYMBOL_SIZE, SYMBOL_BITS>(heap, heapLength, parent, left, right, temp_array);

            // get bit-lengths from tree
            computeBitLength<SYMBOL_SIZE, SYMBOL_BITS>(parent, left, right, heapLength, length_histogram, temp_array);

            // truncate tree for any bigger bit-lengths
            truncateTree(length_histogram, c_lengthHistogram, i_maxBits);

            // canonize the tree
            canonizeTree<SYMBOL_BITS>(heap, heapLength, length_histogram, symbol_bits, c_lengthHistogram);

            // generate huffman codewords
            createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
        }
    } while (1);
}

template <int SYMBOL_SIZE, int SYMBOL_BITS>
void huffConstructTreeStream_1(hls::stream<Frequency>& inFreq,
                               hls::stream<Symbol>& heapStream,
                               hls::stream<uint16_t>& heapLenStream,
                               //                               hls::stream<Symbol>& heapStream_1,
                               //                               hls::stream<uint16_t>& heapLenStream_1,
                               hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                               hls::stream<bool>& eos) {
    // internal buffers
    Symbol heap[SYMBOL_SIZE];
    do {
        if (eos.read() == 1) break;
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};

            uint16_t i_symbolSize = hctMeta[metaIdx]; // current symbol size

            uint16_t heapLength = 0;

            //#pragma HLS DATAFLOW
            // filter the input
            filter(inFreq, heap, &heapLength, maxCodes, i_symbolSize);

            // sort the input
            radixSort<SYMBOL_SIZE, SYMBOL_BITS>(heap, heapLength);

            heapLenStream << heapLength;
            for (uint16_t i = 0; i < heapLength; i++) {
                heapStream << heap[i];
            }
        }
    } while (1);
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int LENGTH_SIZE>
void huffConstructTreeStream(hls::stream<Frequency>& inFreq,
                             hls::stream<Codeword>& outCodes,
                             hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                             uint8_t metaIdx) {
    //#pragma HLS inline
    // construct huffman tree and generate codes and bit-lengths
    const int hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                               {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                               {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};

    uint16_t i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
    uint16_t i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
    uint16_t i_maxBits = hctMeta[metaIdx][2];    // current max bits

    // internal buffers
    Symbol heap[SYMBOL_SIZE];

    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
    Histogram length_histogram[LENGTH_SIZE];

    Frequency temp_array[SYMBOL_SIZE];
    //#pragma HLS resource variable=temp_array core=RAM_2P_BRAM

    ap_uint<4> symbol_bits[SYMBOL_SIZE];

    uint16_t heapLength = 0;

init_buffers:
    for (int i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
        parent[i] = 0;
        if (i < LENGTH_SIZE) length_histogram[i] = 0;
        temp_array[i] = 0;
        symbol_bits[i] = 0;
        heap[i].value = 0;
        heap[i].frequency = 0;
    }

    //#pragma HLS DATAFLOW
    // filter the input
    filter(inFreq, heap, &heapLength, maxCodes, i_symbolSize);

    // sort the input
    radixSort_1<SYMBOL_SIZE, SYMBOL_BITS>(heap, heapLength);

    // create tree
    createTree<SYMBOL_SIZE, SYMBOL_BITS>(heap, heapLength, parent, left, right, temp_array);

    // get bit-lengths from tree
    computeBitLength<SYMBOL_SIZE, SYMBOL_BITS>(parent, left, right, heapLength, length_histogram, temp_array);

    // truncate tree for any bigger bit-lengths
    truncateTree(length_histogram, LENGTH_SIZE, i_maxBits);

    // canonize the tree
    canonizeTree<SYMBOL_BITS>(heap, heapLength, length_histogram, symbol_bits, LENGTH_SIZE);

    // generate huffman codewords
    createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
}

void huffConstructTree(Frequency* inFreq, Codeword* outCodes, uint16_t* maxCodes, uint8_t metaIdx) {
    //#pragma HLS inline
    // construct huffman tree and generate codes and bit-lengths
    const int hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                               {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                               {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};

    uint16_t i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
    uint16_t i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
    uint16_t i_maxBits = hctMeta[metaIdx][2];    // current max bits

    // internal buffers
    Symbol heap[c_tgnSymbolSize];

    ap_uint<c_tgnSymbolSize> left = 0;
    ap_uint<c_tgnSymbolSize> right = 0;
    ap_uint<c_tgnSymbolBits> parent[c_tgnSymbolSize];
    Histogram length_histogram[c_tgnSymbolSize];

    Frequency temp_array[c_tgnSymbolSize];
    //#pragma HLS resource variable=temp_array core=RAM_2P_BRAM

    ap_uint<4> symbol_bits[c_tgnSymbolSize];

    uint16_t heapLength = 0;

init_buffers:
    for (int i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
        parent[i] = 0;
        length_histogram[i] = 0;
        temp_array[i] = 0;
        symbol_bits[i] = 0;
        heap[i].value = 0;
        heap[i].frequency = 0;
    }

    //#pragma HLS DATAFLOW
    // filter the input
    filter(inFreq, heap, &heapLength, maxCodes, i_symbolSize);

    // sort the input
    radixSort<c_tgnSymbolSize, c_tgnSymbolBits>(heap, heapLength);

    // create tree
    createTree<c_tgnSymbolSize, c_tgnSymbolBits>(heap, heapLength, parent, left, right, temp_array);

    // get bit-lengths from tree
    computeBitLength<c_tgnSymbolSize, c_tgnSymbolBits>(parent, left, right, heapLength, length_histogram, temp_array);

    // truncate tree for any bigger bit-lengths
    truncateTree(length_histogram, i_treeDepth, i_maxBits);

    // canonize the tree
    canonizeTree<c_tgnSymbolBits>(heap, heapLength, length_histogram, symbol_bits, i_treeDepth);

    // generate huffman codewords
    createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
}
void genBitLenFreq(hls::stream<Codeword>& outCodes,
                   hls::stream<Frequency>& freq,
                   hls::stream<ap_uint<c_tgnSymbolBits> >& maxCode) {
    Frequency blFreq[19];
    const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};

    for (uint8_t i = 0; i < 19; i++) {
        blFreq[i] = 0;
    }

    for (uint8_t itr = 0; itr < 2; itr++) {
        int16_t prevlen = -1;
        int16_t curlen = 0;
        int16_t count = 0;
        int16_t max_count = 7;
        int16_t min_count = 4;
        int16_t nextlen = outCodes.read().bitlength;

        if (nextlen == 0) {
            max_count = 138;
            min_count = 3;
        }

        ap_uint<c_tgnSymbolBits> maximumCodeLength = maxCode.read();
    parse_tdata:
        for (uint32_t n = 0; n <= maximumCodeLength; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 1
            curlen = nextlen;
            if (n == maximumCodeLength) {
                nextlen = 0xF;
            } else {
                nextlen = outCodes.read().bitlength;
            }

            if (++count < max_count && curlen == nextlen) {
                continue;
            } else if (count < min_count) {
                blFreq[curlen] += count;
            } else if (curlen != 0) {
                if (curlen != prevlen) {
                    blFreq[curlen]++;
                }
                blFreq[c_reusePrevBlen]++;
            } else if (count <= 10) {
                blFreq[c_reuseZeroBlen]++;
            } else {
                blFreq[c_reuseZeroBlen7]++;
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
        for (int i = maximumCodeLength + 1; i < hctMeta[itr]; i++) {
            outCodes.read();
        }
    }

    for (uint8_t i = 0; i < 19; i++) {
        freq << blFreq[i];
    }
}

void genBitLenFreq(Codeword* outCodes, Frequency* blFreq, uint16_t maxCode) {
    //#pragma HLS inline
    // generate bit-length frequencies using literal and distance bit-lengths
    ap_uint<4> tree_len[c_litCodeCount];

copy_blens:
    for (int i = 0; i <= maxCode; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = (uint8_t)outCodes[i].bitlength;
    }
clear_rem_blens:
    for (int i = maxCode + 2; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = 0;
    }
    tree_len[maxCode + 1] = (uint8_t)0xff;

    int16_t prevlen = -1;
    int16_t curlen = 0;
    int16_t count = 0;
    int16_t max_count = 7;
    int16_t min_count = 4;
    int16_t nextlen = tree_len[0];

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

parse_tdata:
    for (uint32_t n = 0; n <= maxCode; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 3
        curlen = nextlen;
        nextlen = tree_len[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            blFreq[curlen] += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) {
                blFreq[curlen]++;
            }
            blFreq[c_reusePrevBlen]++;
        } else if (count <= 10) {
            blFreq[c_reuseZeroBlen]++;
        } else {
            blFreq[c_reuseZeroBlen7]++;
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

void sendTrees(hls::stream<ap_uint<c_tgnSymbolBits> >& maxLdCodes,
               hls::stream<ap_uint<c_tgnSymbolBits> >& maxBlCodes,
               hls::stream<Codeword>& Ldcodes,
               hls::stream<Codeword>& Blcodes,
               hls::stream<bool>& isEOBlock,
               hls::stream<uint16_t>& codeStream,
               hls::stream<uint8_t>& codeSize) {
    while (isEOBlock.read() == 0) {
        Codeword zeroValue;
        zeroValue.bitlength = 0;
        zeroValue.codeword = 0;
        Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
        uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        ap_uint<c_tgnSymbolBits> maxCodesReg[3];

        uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
        ap_uint<20> codePacked;

        maxCodesReg[0] = maxLdCodes.read();
        // initialize all the memory
        for (int i = 0; i < c_litCodeCount; ++i) {
            outCodes[i] = Ldcodes.read();
        }
        outCodes[c_litCodeCount] = zeroValue;

        maxCodesReg[1] = maxLdCodes.read();

        for (int i = 0; i < c_dstCodeCount; i++) {
            outCodes[offsets[1] + i] = Ldcodes.read();
        }

        outCodes[c_litCodeCount + c_dstCodeCount + 1] = zeroValue;

    //********************************************//
    send_ltrees:
        for (int i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            Codeword code = outCodes[i];
            // prepare packet as <<bitlen>..<code>>
            codeStream << code.codeword;
            codeSize << code.bitlength;
        }

    send_dtrees:
        for (int i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            Codeword code = outCodes[offsets[1] + i];
            // prepare packet as <<bitlen>..<code>>
            codeStream << code.codeword;
            codeSize << code.bitlength;
        }

        maxCodesReg[2] = maxBlCodes.read();

        for (int i = offsets[2]; i < offsets[2] + c_blnCodeCount; ++i) {
            outCodes[i] = Blcodes.read();
        }
        uint16_t bl_mxc;

    bltree_blen:
        for (bl_mxc = c_blnCodeCount - 1; bl_mxc >= 3; --bl_mxc) {
#pragma HLS PIPELINE II = 1
            if ((uint8_t)(outCodes[offsets[2] + bitlen_vals[bl_mxc]].bitlength) != 0) break;
        }

        maxCodesReg[2] = bl_mxc;

        // Code from Huffman Encoder
        //********************************************//
        // Start of block = 4 and len = 3
        // For dynamic tree
        codeStream << 4;
        codeSize << 3;
        // lcodes
        codeStream << ((maxCodesReg[0] + 1) - 257);
        codeSize << 5;

        // dcodes
        codeStream << ((maxCodesReg[1] + 1) - 1);
        codeSize << 5;

        // blcodes
        codeStream << ((maxCodesReg[2] + 1) - 4);
        codeSize << 4;

        uint16_t bitIndex = offsets[2];
    // Send BL length data
    send_bltree:
        for (int rank = 0; rank < maxCodesReg[2] + 1; rank++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
#pragma HLS PIPELINE II = 1
            codeStream << outCodes[bitIndex + bitlen_vals[rank]].bitlength;
            codeSize << 3;
        } // BL data copy loop

        // Send Bitlengths for Literal and Distance Tree
        for (int tree = 0; tree < 2; tree++) {
            uint8_t prevlen = 0; // Last emitted Length
            uint8_t curlen = 0;  // Length of Current Code
            uint8_t nextlen =
                (tree == 0) ? outCodes[0].bitlength : outCodes[offsets[1]].bitlength; // Length of next code
            uint8_t count = 0;
            int max_count = 7; // Max repeat count
            int min_count = 4; // Min repeat count

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            ap_uint<10> max_code = (tree == 0) ? maxCodesReg[0] : maxCodesReg[1];

            Codeword temp = outCodes[bitIndex + c_reusePrevBlen];
            uint16_t reuse_prev_code = temp.codeword;
            uint8_t reuse_prev_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen];
            uint16_t reuse_zero_code = temp.codeword;
            uint8_t reuse_zero_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen7];
            uint16_t reuse_zero7_code = temp.codeword;
            uint8_t reuse_zero7_len = temp.bitlength;

        send_ltree:
            for (int n = 0; n <= max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 286 max = 286
                curlen = nextlen;
                nextlen = (tree == 0) ? outCodes[n + 1].bitlength
                                      : outCodes[offsets[1] + n + 1].bitlength; // Length of next code

                if (++count < max_count && curlen == nextlen) {
                    continue;
                } else if (count < min_count) {
                lit_cnt:
                    temp = outCodes[bitIndex + curlen];
                    for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                        codeStream << temp.codeword;
                        codeSize << temp.bitlength;
                    }
                    count = 0;

                } else if (curlen != 0) {
                    if (curlen != prevlen) {
                        temp = outCodes[bitIndex + curlen];
                        codeStream << temp.codeword;
                        codeSize << temp.bitlength;
                        count--;
                    }
                    codeStream << reuse_prev_code;
                    codeSize << reuse_prev_len;

                    codeStream << count - 3;
                    codeSize << 2;

                } else if (count <= 10) {
                    codeStream << reuse_zero_code;
                    codeSize << reuse_zero_len;

                    codeStream << count - 3;
                    codeSize << 3;

                } else {
                    codeStream << reuse_zero7_code;
                    codeSize << reuse_zero7_len;

                    codeStream << count - 11;
                    codeSize << 7;
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
        codeSize << 0;
    }
}

void codeWordDistributor(hls::stream<Codeword>& inStreamCode,
                         hls::stream<Codeword>& outStreamCode1,
                         hls::stream<Codeword>& outStreamCode2,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& inStreamMaxCode,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode1,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode2,
                         hls::stream<bool>& isEOBlock) {
    const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    while (isEOBlock.read() == 0) {
        for (uint8_t i = 0; i < 2; i++) {
            ap_uint<c_tgnSymbolBits> maxCode = inStreamMaxCode.read();
            outStreamMaxCode1 << maxCode;
            outStreamMaxCode2 << maxCode;
            for (uint32_t j = 0; j < hctMeta[i]; j++) {
                Codeword val = inStreamCode.read();
                outStreamCode1 << val;
                outStreamCode2 << val;
            }
        }
    }
}

template <int SLAVES>
void streamDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 1) break;
    } while (1);
}

void processLiteralDistance(hls::stream<Frequency>& frequencies,
                            hls::stream<bool>& isEOBlock,
                            hls::stream<Codeword>& outCodes,
                            hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes) {
    do {
        if (isEOBlock.read() == 1) break;
        // read freqStream and generate codes for it
        for (int i = 0; i < 2; ++i) {
            // construct the huffman tree and generate huffman codes
            details::huffConstructTreeStream<c_litCodeCount, c_tgnSymbolBits, c_lengthHistogram>(frequencies, outCodes,
                                                                                                 maxCodes, i);
        }
    } while (1);
}

void getFrequencies(hls::stream<Codeword>& codes,
                    hls::stream<Frequency>& frequencies,
                    hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                    hls::stream<bool>& isEOBlock) {
    while (isEOBlock.read() == 0) {
        genBitLenFreq(codes, frequencies, maxCodes);
    }
}

void processBitLength(hls::stream<Frequency>& frequencies,
                      hls::stream<bool>& isEOBlock,
                      hls::stream<Codeword>& outCodes,
                      hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes) {
    while (isEOBlock.read() == 0) {
        // read freqStream and generate codes for it
        // construct the huffman tree and generate huffman codes
        details::huffConstructTreeStream<c_blnCodeCount, c_tgnBitlengthBits, 15>(frequencies, outCodes, maxCodes, 2);
    }
}

void zlibTreegenStream(hls::stream<Frequency>& frequencies,
                       hls::stream<bool>& isEOBlock,
                       hls::stream<uint16_t>& codeStream,
                       hls::stream<uint8_t>& codeSize) {
    hls::stream<bool> eoBlock[6];
    hls::stream<Codeword> ldCodes("ldCodes");
    hls::stream<Codeword> ldCodes1("ldCodes1");
    hls::stream<Codeword> ldCodes2("ldCodes2");
    hls::stream<Codeword> blCodes("blCodes");
    hls::stream<Frequency> ldFrequency("ldFrequency");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes("ldMaxCodes");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes1("ldMaxCodes1");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes2("ldMaxCodes2");
    hls::stream<ap_uint<c_tgnSymbolBits> > blMaxCodes("blMaxCodes");
    hls::stream<Symbol> heapStream("heapStream");
    hls::stream<uint16_t> heapLenStream("heapLenStream");
#pragma HLS STREAM variable = heapStream depth = 286
// DATAFLOW
#pragma HLS DATAFLOW
    streamDistributor<6>(isEOBlock, eoBlock);
    huffConstructTreeStream_1<c_litCodeCount, c_tgnSymbolBits>(frequencies, heapStream, heapLenStream, /*heapStream_1,
                                                               heapLenStream_1, */ ldMaxCodes,
                                                               eoBlock[0]);
    huffConstructTreeStream_2<c_litCodeCount, c_tgnSymbolBits>(heapStream, heapLenStream, eoBlock[5], ldCodes);
    codeWordDistributor(ldCodes, ldCodes1, ldCodes2, ldMaxCodes, ldMaxCodes1, ldMaxCodes2, eoBlock[1]);
    getFrequencies(ldCodes1, ldFrequency, ldMaxCodes1, eoBlock[2]);
    processBitLength(ldFrequency, eoBlock[3], blCodes, blMaxCodes);
    sendTrees(ldMaxCodes2, blMaxCodes, ldCodes2, blCodes, eoBlock[4], codeStream, codeSize);
}

} // end namespace details

void zlibTreegenCore(Frequency* inFreq, Codeword* outCodes, uint16_t* maxCodes) {
    // Core module for zlib huffman treegen
    // one value padding between codes of literals-distance-bitlengths, needed later
    uint16_t offsets[3] = {0, c_litCodeCount, (c_litCodeCount + c_dstCodeCount)};

    // initialize all the memory
    for (int i = 0; i < (c_litCodeCount + c_dstCodeCount + c_blnCodeCount); ++i) {
        outCodes[i].codeword = 0;
        outCodes[i].bitlength = 0;
    }
    // read input data from axi stream and put it in BRAM after filtering
    // read freqStream and generate codes for it
    for (int i = 0; i < 3; ++i) {
        // construct the huffman tree and generate huffman codes
        details::huffConstructTree(&(inFreq[offsets[i]]), &(outCodes[offsets[i]]), &(maxCodes[i]), i);
        // only after codes have been generated for literals and distances
        if (i < 2) {
            // generate frequency data for bitlengths
            details::genBitLenFreq(&(outCodes[offsets[i]]), &(inFreq[offsets[2]]), maxCodes[i]);
        }
    }

    // get maxCodes count for bit-length codes
    // specific to huffman tree of bit-lengths
    uint8_t bitlen_vals[32] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    uint16_t bl_mxc;
bltree_blen:
    for (bl_mxc = c_blnCodeCount - 1; bl_mxc >= 3; --bl_mxc) {
#pragma HLS PIPELINE II = 3
        if ((uint8_t)(outCodes[bitlen_vals[bl_mxc] + offsets[2]].bitlength) != 0) break;
    }
    maxCodes[2] = bl_mxc;
}

void zlibTreegenInMMOutStream(uint32_t* lit_freq,
                              uint32_t* dist_freq,
                              hls::stream<uint16_t>& codeStream,
                              hls::stream<uint8_t>& codeSize) {
    // Core module for zlib huffman treegen
    // Using single array for Frequency and Codes for literals, distances and bit-lengths
    Frequency inFreq[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
    // one value padding between codes of literals-distance-bitlengths, needed later
    Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
    uint16_t maxCodes[3] = {0, 0, 0};

    uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
    ap_uint<20> codePacked;

    // initialize all the memory
    for (int i = 0; i < (c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2); ++i) {
        outCodes[i].codeword = 0;
        outCodes[i].bitlength = 0;
    }

    int offset = 0;
    for (int i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
        inFreq[i] = (Frequency)lit_freq[i];
    }
    offset = offsets[1]; // copy distances
    for (int i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
        inFreq[i + offset] = (Frequency)dist_freq[i];
    }
    offset = offsets[2];
    for (int i = 0; i < c_blnCodeCount; ++i) {
        inFreq[i + offset] = 0; // just initialize
    }

    // read freqStream and generate codes for it
    for (int i = 0; i < 3; ++i) {
        // construct the huffman tree and generate huffman codes
        details::huffConstructTree(&(inFreq[offsets[i]]), &(outCodes[offsets[i]]), &(maxCodes[i]), i);
        // only after codes have been generated for literals and distances
        if (i < 2) {
            // generate frequency data for bitlengths
            details::genBitLenFreq(&(outCodes[offsets[i]]), &(inFreq[offsets[2]]), maxCodes[i]);
        }
    }

    // get maxCodes count for bit-length codes
    // specific to huffman tree of bit-lengths
    uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    uint16_t bl_mxc;
bltree_blen:
    for (bl_mxc = c_blnCodeCount - 1; bl_mxc >= 3; --bl_mxc) {
#pragma HLS PIPELINE II = 3
        if ((uint8_t)(outCodes[bitlen_vals[bl_mxc] + offsets[2]].bitlength) != 0) break;
    }
    maxCodes[2] = bl_mxc;

    // Code from Huffman Encoder
    //********************************************//
    // Start of block = 4 and len = 3
    uint32_t start_of_block = 0x30004;

    codeStream << 4;
    codeSize << 3;
    // lcodes
    codeStream << ((maxCodes[0] + 1) - 257);
    codeSize << 5;

    // dcodes
    codeStream << ((maxCodes[1] + 1) - 1);
    codeSize << 5;

    // blcodes
    codeStream << ((maxCodes[2] + 1) - 4);
    codeSize << 4;

    uint16_t bitIndex = offsets[2];
// Send BL length data
send_bltree:
    for (int rank = 0; rank < bl_mxc + 1; rank++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
#pragma HLS PIPELINE II = 1
        codeStream << outCodes[bitIndex + bitlen_vals[rank]].bitlength;
        codeSize << 3;
    } // BL data copy loop

    // Send Bitlengths for Literal and Distance Tree
    for (int tree = 0; tree < 2; tree++) {
        uint8_t prevlen = 0; // Last emitted Length
        uint8_t curlen = 0;  // Length of Current Code
        uint8_t nextlen = (tree == 0) ? outCodes[0].bitlength : outCodes[offsets[1]].bitlength; // Length of next code
        uint8_t count = 0;
        int max_count = 7; // Max repeat count
        int min_count = 4; // Min repeat count

        if (nextlen == 0) {
            max_count = 138;
            min_count = 3;
        }

        uint16_t max_code = (tree == 0) ? maxCodes[0] : maxCodes[1];

        Codeword temp = outCodes[bitIndex + c_reusePrevBlen];
        uint16_t reuse_prev_code = temp.codeword;
        uint8_t reuse_prev_len = temp.bitlength;
        temp = outCodes[bitIndex + c_reuseZeroBlen];
        uint16_t reuse_zero_code = temp.codeword;
        uint8_t reuse_zero_len = temp.bitlength;
        temp = outCodes[bitIndex + c_reuseZeroBlen7];
        uint16_t reuse_zero7_code = temp.codeword;
        uint8_t reuse_zero7_len = temp.bitlength;

    send_ltree:
        for (int n = 0; n <= max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 286 max = 286
            curlen = nextlen;
            nextlen =
                (tree == 0) ? outCodes[n + 1].bitlength : outCodes[offsets[1] + n + 1].bitlength; // Length of next code

            if (++count < max_count && curlen == nextlen) {
                continue;
            } else if (count < min_count) {
            lit_cnt:
                temp = outCodes[bitIndex + curlen];
                for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                    codeStream << temp.codeword;
                    codeSize << temp.bitlength;
                }
                count = 0;

            } else if (curlen != 0) {
                if (curlen != prevlen) {
                    temp = outCodes[bitIndex + curlen];
                    codeStream << temp.codeword;
                    codeSize << temp.bitlength;
                    count--;
                }
                codeStream << reuse_prev_code;
                codeSize << reuse_prev_len;

                codeStream << count - 3;
                codeSize << 2;

            } else if (count <= 10) {
                codeStream << reuse_zero_code;
                codeSize << reuse_zero_len;

                codeStream << count - 3;
                codeSize << 3;

            } else {
                codeStream << reuse_zero7_code;
                codeSize << reuse_zero7_len;

                codeStream << count - 11;
                codeSize << 7;
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
    codeSize << 0;
//********************************************//
send_ltrees:
    for (int i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
        Codeword code = outCodes[i];
        // prepare packet as <<bitlen>..<code>>
        codeStream << code.codeword;
        codeSize << code.bitlength;
    }

send_dtrees:
    for (int i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
        Codeword code = outCodes[offsets[1] + i];
        // prepare packet as <<bitlen>..<code>>
        codeStream << code.codeword;
        codeSize << code.bitlength;
    }
}

void zlibTreegenStream(hls::stream<uint32_t>& lz77Tree,
                       hls::stream<bool>& lz77TreeBlockEos,
                       hls::stream<uint16_t>& codeStream,
                       hls::stream<uint8_t>& codeSize) {
    details::zlibTreegenStream(lz77Tree, lz77TreeBlockEos, codeStream, codeSize);
}

} // End of compression
} // End of xf

#endif // _XFCOMPRESSION_DEFLATE_TREES_HPP_
