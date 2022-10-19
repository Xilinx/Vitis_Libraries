/*
 * (c) Copyright 2019-2022 Xilinx, Inc. All rights reserved.
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
#include "compress_utils.hpp"
#include "zlib_specs.hpp"
#include "zstd_specs.hpp"

namespace xf {
namespace compression {

const static uint8_t c_tgnSymbolBits = 10;
const static uint8_t c_tgnBitlengthBits = 5;

const static uint8_t c_lengthHistogram = 18;

const static uint16_t c_tgnSymbolSize = c_litCodeCount;
const static uint16_t c_tgnTreeDepth = c_litCodeCount;
const static uint16_t c_tgnMaxBits = c_maxCodeBits;

typedef ap_uint<12> Histogram;

template <int MAX_FREQ_DWIDTH>
using Frequency = ap_uint<MAX_FREQ_DWIDTH>;

template <int MAX_FREQ_DWIDTH>
struct Symbol {
    ap_uint<c_tgnSymbolBits> value;
    Frequency<MAX_FREQ_DWIDTH> frequency;
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

template <int MAX_FREQ_DWIDTH = 32, int WRITE_MXC = 1>
void filter(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
            Symbol<MAX_FREQ_DWIDTH>* heap,
            uint16_t* heapLength,
            hls::stream<ap_uint<c_tgnSymbolBits> >& symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    ap_uint<c_tgnSymbolBits> smLen = 0;
    bool read_flag = false;
    auto curFreq = inFreq.read();
    if (curFreq.strobe == 0) {
        heapLength[0] = 0xFFFF;
        return;
    }
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        if (read_flag) curFreq = inFreq.read();
        auto cf = curFreq.data[0];
        read_flag = true;
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
    if (WRITE_MXC) symLength << smLen;
}

template <int MAX_FREQ_DWIDTH = 32>
void filter(Frequency<MAX_FREQ_DWIDTH>* inFreq,
            Symbol<MAX_FREQ_DWIDTH>* heap,
            uint16_t* heapLength,
            uint16_t* symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    uint16_t smLen = 0;
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        auto cf = inFreq[n];
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

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void radixSort_1(Symbol<MAX_FREQ_DWIDTH>* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];
    bool not_sorted = true;

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (uint8_t shift = 0; shift < MAX_FREQ_DWIDTH && not_sorted; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 5 avg = 4
    init_histogram:
        for (ap_uint<5> i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

        auto prev_freq = heap[0].frequency;
        not_sorted = false;
    compute_histogram:
        for (uint16_t j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Symbol<MAX_FREQ_DWIDTH> val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
            // check if not already in sorted order
            if (prev_freq > val.frequency) not_sorted = true;
            prev_freq = val.frequency;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (uint8_t i = 0; (i < RADIX - 1) && not_sorted; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (uint16_t j = 0; j < n && not_sorted; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void radixSort(Symbol<MAX_FREQ_DWIDTH>* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];
    bool not_sorted = true;

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (uint8_t shift = 0; shift < MAX_FREQ_DWIDTH && not_sorted; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 5 avg = 4
    init_histogram:
        for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

        auto prev_freq = heap[0].frequency;
        not_sorted = false;
    compute_histogram:
        for (uint16_t j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
            Symbol<MAX_FREQ_DWIDTH> val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
            // check if not already in sorted order
            if (prev_freq > val.frequency) not_sorted = true;
            prev_freq = val.frequency;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (uint8_t i = 0; (i < RADIX - 1) && not_sorted; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (uint16_t j = 0; j < n && not_sorted; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

template <int SYMBOL_BITS>
void _rdxsrtInitHistogram(ap_uint<SYMBOL_BITS> digit_histogram[RADIX]) {
#pragma HLS INLINE
init_histogram:
    for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS UNROLL
        digit_histogram[i] = 0;
    }
}

template <int SYMBOL_BITS>
inline void _rdxsrtFindDigitLocation(ap_uint<SYMBOL_BITS> digit_histogram[RADIX],
                                     ap_uint<SYMBOL_BITS> digit_location[RADIX]) {
#pragma HLS INLINE
    digit_location[0] = 0;
find_digit_location:
    for (uint8_t i = 0; (i < RADIX - 1); ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS UNROLL
        digit_location[i + 1] = digit_location[i] + digit_histogram[i];
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32, int WRITE_MXC = 1>
void filterRadixSortPart1(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                          hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                          hls::stream<ap_uint<9> >& heapLenStream1,
                          hls::stream<ap_uint<9> >& heapLenStream2,
                          hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                          hls::stream<bool>& isEOBlocks) {
    //#pragma HLS allocation function instances = _rdxsrtInitHistogram<SYMBOL_BITS> limit = 1
    // Filter input frequencies and partial radix sort (1st half)
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_2p impl = lutram
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = prev_sorting type = ram_1p impl = lutram
    Digit current_digit[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = current_digit type = ram_1p impl = lutram
    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete

    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;
    ap_uint<1> metaIdx = 0;
filter_partsort_ldblock:
    while (!last) {
        // filter-sort for literals and distances
        ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
        uint16_t heapLength = 0;

        ap_uint<c_tgnSymbolBits> smLen = 0;
        bool read_flag = false;
        auto curFreq = inFreqStream.read();
        // send oes stream once per lit-dist (2)trees
        last = (curFreq.strobe == 0);
        if (metaIdx == 0) isEOBlocks << last;
        if (last) break;

        // pre-initialize
        _rdxsrtInitHistogram<SYMBOL_BITS>(digit_histogram);

    filter_compute_histogram:
        for (uint16_t n = 0; n < i_symbolSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 30
            if (read_flag) curFreq = inFreqStream.read();
            auto cf = ((n == 256) ? (ap_uint<MAX_FREQ_DWIDTH>)1 : curFreq.data[0]);
            read_flag = true;
            // get non-zero frequencies
            if (cf > 0) {
                Symbol<MAX_FREQ_DWIDTH> cVal;
                cVal.value = smLen = n;
                cVal.frequency = cf;
                heap[heapLength] = cVal;
                prev_sorting[heapLength] = cVal;
                // create histogram
                Digit digit = (cVal.frequency & (RADIX - 1));
                current_digit[heapLength] = digit;
                ++digit_histogram[digit];
                ++heapLength;
            }
        }
        if (WRITE_MXC) maxCodes << smLen;

        _rdxsrtFindDigitLocation<SYMBOL_BITS>(digit_histogram, digit_location);

    re_sort_write_output1:
        for (uint16_t j = 0; j < heapLength; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            auto dlc = digit_location[digit];
            heap[dlc] = prev_sorting[j];
            digit_location[digit] = dlc + 1;
        }
        heapLenStream1 << heapLength;
        heapLenStream2 << heapLength;
        // send sorted frequencies
        hpVal.strobe = 1;
        for (uint16_t i = 0; i < heapLength; ++i) {
#pragma HLS PIPELINE II = 1
            hpVal.data[0] = heap[i];
            heapStream << hpVal;
        }
        // to prevent hang, send one 0 frequency value (invalid, used to detect 0 heapLength)
        if (heapLength == 0) {
            hpVal.data[0].frequency = 0;
            hpVal.data[0].value = 0;
            heapStream << hpVal;
        }
        // end of this chunk
        hpVal.strobe = 0;
        heapStream << hpVal;

        ++metaIdx;
    }
    // terminate
    hpVal.strobe = 0;
    heapStream << hpVal;
    heapLenStream1 << 0xFFF;
    heapLenStream2 << 0xFFF;
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void radixSortMidPartial(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& inHeapStream,
                         hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& outHeapStream,
                         uint8_t shiftNum) {
    // partial radix sort, 2nd half
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_2p impl = lutram
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = prev_sorting type = ram_1p impl = lutram
    Digit current_digit[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = current_digit type = ram_1p impl = lutram
    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;
    ap_uint<1> metaIdx = 0;
filter_partsort_ldblock:
    while (!last) {
        // filter-sort for literals and distances
        ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
        uint16_t heapLength = 0;

        auto inHeapV = inHeapStream.read();
        if (inHeapV.strobe == 0) break;

        // pre-initialize
        _rdxsrtInitHistogram<SYMBOL_BITS>(digit_histogram);

    read_heap_compute_histogram:
        for (; inHeapV.strobe > 0; inHeapV = inHeapStream.read()) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 30
            Symbol<MAX_FREQ_DWIDTH> cVal = inHeapV.data[0];
            prev_sorting[heapLength] = cVal;
            heap[heapLength] = cVal;
            // create histogram
            Digit digit = (cVal.frequency >> shiftNum) & (RADIX - 1);
            current_digit[heapLength] = digit;
            ++digit_histogram[digit];
            ++heapLength;
        }
        // detect 0 heapLength, 0 frequency is invalid
        if (heapLength == 1) {
            auto cVal = heap[0];
            if (cVal.frequency == 0 && cVal.value == 0) heapLength = 0;
        }

        _rdxsrtFindDigitLocation<SYMBOL_BITS>(digit_histogram, digit_location);

    re_sort_write_output:
        for (uint16_t j = 0; j < heapLength; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            auto dlc = digit_location[digit];
            heap[dlc] = prev_sorting[j];
            digit_location[digit] = dlc + 1;
        }

        // send sorted frequencies
        hpVal.strobe = 1;
    write_rdx_srtd_heap:
        for (uint16_t i = 0; i < heapLength; ++i) {
#pragma HLS PIPELINE II = 1
            hpVal.data[0] = heap[i];
            outHeapStream << hpVal;
        }
        // to prevent hang, send one 0 frequency value (invalid, used to detect 0 heapLength)
        if (heapLength == 0) {
            hpVal.data[0].frequency = 0;
            hpVal.data[0].value = 0;
            outHeapStream << hpVal;
        }
        // end of this chunk
        hpVal.strobe = 0;
        outHeapStream << hpVal;

        ++metaIdx;
    }
    // terminate
    hpVal.strobe = 0;
    outHeapStream << hpVal;
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void radixPartialFinalSort(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& inHeapStream,
                           hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& outHeapStream1,
                           hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& outHeapStream2,
                           uint8_t shiftNum) {
    //#pragma HLS allocation function instances = _rdxsrtInitHistogram<SYMBOL_BITS> limit = 1
    // partial radix sort, 2nd half
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_2p impl = lutram
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = prev_sorting type = ram_1p impl = lutram
    Digit current_digit[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = current_digit type = ram_1p impl = lutram
    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;
    ap_uint<1> metaIdx = 0;
filter_partsort_ldblock:
    while (!last) {
        // filter-sort for literals and distances
        ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
        uint16_t heapLength = 0;
        // const uint8_t c_sftLim = ((1 + (MAX_FREQ_DWIDTH - 1) / (2 * BITS_PER_LOOP)) * BITS_PER_LOOP);

        auto inHeapV = inHeapStream.read();
        if (inHeapV.strobe == 0) break;

        // pre-initialize
        _rdxsrtInitHistogram<SYMBOL_BITS>(digit_histogram);

    read_heap_compute_histogram:
        for (; inHeapV.strobe > 0; inHeapV = inHeapStream.read()) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 30
            Symbol<MAX_FREQ_DWIDTH> cVal = inHeapV.data[0];
            prev_sorting[heapLength] = cVal;
            heap[heapLength] = cVal;
            // create histogram
            Digit digit = (cVal.frequency >> shiftNum) & (RADIX - 1);
            current_digit[heapLength] = digit;
            ++digit_histogram[digit];
            ++heapLength;
        }
        // detect 0 heapLength, 0 frequency is invalid
        if (heapLength == 1) {
            auto cVal = heap[0];
            if (cVal.frequency == 0 && cVal.value == 0) heapLength = 0;
        }

        _rdxsrtFindDigitLocation<SYMBOL_BITS>(digit_histogram, digit_location);

    re_sort_write_output:
        for (uint16_t j = 0; j < heapLength; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            auto dlc = digit_location[digit];
            heap[dlc] = prev_sorting[j];
            digit_location[digit] = dlc + 1;
        }

        // send sorted frequencies
        hpVal.strobe = 1;
    write_rdx_srtd_heap:
        for (uint16_t i = 0; i < heapLength; ++i) {
#pragma HLS PIPELINE II = 1
            hpVal.data[0] = heap[i];
            outHeapStream1 << hpVal;
            outHeapStream2 << hpVal;
        }
        ++metaIdx;
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void createTree(Symbol<MAX_FREQ_DWIDTH>* heap,
                int num_symbols,
                ap_uint<SYMBOL_BITS>* parent,
                ap_uint<SYMBOL_SIZE>& left,
                ap_uint<SYMBOL_SIZE>& right,
                Frequency<MAX_FREQ_DWIDTH>* frequency) {
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
        Frequency<MAX_FREQ_DWIDTH> node_freq = 0;
        Frequency<MAX_FREQ_DWIDTH> intermediate_freq = frequency[tree_count];
        Frequency<MAX_FREQ_DWIDTH> symFreq = heap[in_count].frequency;
        tmp = 1;
        tmp <<= i;

        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // left[i] = s.value;       // set input symbol value as left node
            node_freq = symFreq; // Add symbol frequency to total node frequency
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
        symFreq = heap[in_count].frequency;
        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // right[i] = s.value;
            frequency[i] = node_freq + symFreq;
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

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
inline void createTreeStream(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                             int num_symbols,
                             hls::stream<IntVectorStream_dt<SYMBOL_BITS, 1> >& parentStream,
                             ap_uint<SYMBOL_SIZE>& left,
                             ap_uint<SYMBOL_SIZE>& right,
                             Frequency<MAX_FREQ_DWIDTH>* frequency) {
#pragma HLS INLINE
    ap_uint<SYMBOL_BITS> tree_count = 0; // Number of intermediate node assigned a parent
    ap_uint<SYMBOL_BITS> in_count = 0;   // Number of inputs consumed
    ap_uint<SYMBOL_BITS> read_count = 0; // Number of values read from input stream
    ap_uint<SYMBOL_SIZE> tmp;
    left = 0;
    right = 0;
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> heapVal1, heapVal2, zeroVal;
    zeroVal.data[0].value = 0;
    zeroVal.data[0].frequency = 0;
    frequency[0] = 0;
    frequency[1] = 0;
    frequency[2] = 0;
    frequency[3] = 0;

    // for case with less number of symbols
    uint16_t hpLen = num_symbols;
    if (num_symbols < 3) ++num_symbols;
    bool onceFlag = true;
    Frequency<MAX_FREQ_DWIDTH> wTmp = 0;
    // uint16_t pfwIdx = 0;
    IntVectorStream_dt<SYMBOL_BITS, 1> parentVal;
#pragma HLS AGGREGATE variable = parentVal
    parentVal.strobe = 1;
// this loop needs to run at least twice
create_heap:
    for (uint16_t i = 0; i < num_symbols; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 avg = 286 max = 286
#pragma HLS PIPELINE II = 3
        Frequency<MAX_FREQ_DWIDTH> node_freq = 0;
        Frequency<MAX_FREQ_DWIDTH> intermediate_freq = ((i - 1) == tree_count ? wTmp : frequency[tree_count]);
        Frequency<MAX_FREQ_DWIDTH> intermediate_freq2 =
            ((i - 1) == (tree_count + 1) ? wTmp : frequency[tree_count + 1]);
        Frequency<MAX_FREQ_DWIDTH> symFreq, symFreq2, hTmp;
        tmp = 1;
        tmp <<= i;

        if (read_count == in_count) {
            // read from heap stream
            heapVal1 = ((read_count < hpLen) ? heapStream.read() : zeroVal);
            heapVal2 = ((read_count + 1 < hpLen) ? heapStream.read() : zeroVal);
            // increment the read counter
            read_count += ((uint8_t)(read_count < hpLen) + (uint8_t)(read_count + 1 < hpLen));
        } else if (read_count == in_count + 1) {
            auto tmpHp = heapVal2;
            if (in_count != num_symbols - 1) {
                heapVal1 = tmpHp;
            } else {
                if (onceFlag) {
                    heapVal1 = tmpHp;
                    onceFlag = false;
                }
            }
            heapVal2 = ((read_count < hpLen) ? heapStream.read() : zeroVal);
            read_count += (uint8_t)(read_count < hpLen);
        }
        symFreq = heapVal1.data[0].frequency;
        symFreq2 = heapVal2.data[0].frequency;

        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            node_freq = symFreq; // Add symbol frequency to total node frequency
            // move to the next input symbol
            ++in_count;
            symFreq = symFreq2;
        } else {
            // pick internal node without a parent
            left |= tmp;
            node_freq = intermediate_freq; // Add child node frequency
            parentVal.data[0] = i;         // Set this node as child's parent
            parentStream << parentVal;
            // Go to next parent-less internal node
            ++tree_count;
            intermediate_freq = intermediate_freq2;
        }
        wTmp = node_freq + intermediate_freq;
        hTmp = node_freq + symFreq;

        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            ++in_count;
            wTmp = hTmp;
        } else {
            // Pick internal node without a parent
            right |= tmp;
            parentVal.data[0] = i;
            parentStream << parentVal;
            ++tree_count;
        }
        frequency[i] = wTmp;
    }
// reset remaining parent values, to avoid ambiguity later
reset_remaining_parent:
    for (uint16_t i = tree_count; i < num_symbols; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 avg = 2
        parentVal.data[0] = 0;
        parentStream << parentVal;
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void computeBitLengthLL(ap_uint<SYMBOL_BITS>* parent,
                        ap_uint<SYMBOL_SIZE>& left,
                        ap_uint<SYMBOL_SIZE>& right,
                        int num_symbols,
                        Histogram* length_histogram,
                        Frequency<MAX_FREQ_DWIDTH>* child_depth) {
    //#pragma HLS INLINE
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
    auto prevParent = parent[num_symbols - 2];
    auto length = child_depth[prevParent];
    ++length;
// this loop needs to run at least once
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 301
#pragma HLS pipeline II = 1
        auto pIdx = parent[i];
        if (pIdx != prevParent) {
            // use previously written value to avoid dependence
            length = 1 + (pIdx == (i + 1) ? length : child_depth[pIdx]);
            prevParent = pIdx;
        } else {
            tmp = 1;
            tmp <<= i;
            child_depth[i] = length;
            bool is_left_internal = ((left & tmp) == 0);
            bool is_right_internal = ((right & tmp) == 0);

            if ((is_left_internal || is_right_internal)) {
                length_histogram[length] += (1 + (uint8_t)(is_left_internal & is_right_internal));
            }
            --i;
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void computeBitLength(ap_uint<SYMBOL_BITS>* parent,
                      ap_uint<SYMBOL_SIZE>& left,
                      ap_uint<SYMBOL_SIZE>& right,
                      int num_symbols,
                      Histogram* length_histogram,
                      Frequency<MAX_FREQ_DWIDTH>* child_depth) {
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
// this loop needs to run at least once
// II is 1 or 2 depending on configuration of memory
// used for arrays "child_depth" and "length_histogram"
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE
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
            trctr_mv:
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

void truncateTreeStream(hls::stream<DSVectorStream_dt<Histogram, 1> >& inLengthHistogramStream,
                        hls::stream<DSVectorStream_dt<Histogram, 1> >& outLengthHistogramStream) {
    const uint16_t c_maxBitLen[2] = {c_blnCodeCount, c_blnCodeCount};
    Histogram length_histogram[c_lengthHistogram];
    //#pragma HLS ARRAY_PARTITION variable = length_histogram type = complete
    ap_uint<1> metaIdx = 0;
    DSVectorStream_dt<Histogram, 1> histVal;
trunc_tree_outer:
    while (true) {
        int max_bit_len = c_maxBitLen[metaIdx++];
        histVal = inLengthHistogramStream.read();
        length_histogram[0] = histVal.data[0];
        if (histVal.strobe == 0) {
            // exit
            outLengthHistogramStream << histVal;
            break;
        }
    read_blen_hist:
        for (uint8_t i = 1; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 17 max = 17
#pragma HLS PIPELINE II = 1
            histVal = inLengthHistogramStream.read();
            length_histogram[i] = histVal.data[0];
        }
        int j = max_bit_len;
    move_nodes:
        for (uint16_t i = c_lengthHistogram - 1; i > max_bit_len; --i) {
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
                trctr_mv:
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
        histVal.strobe = 1;
    write_trc_blen_hist:
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 18
#pragma HLS PIPELINE II = 1
            histVal.data[0] = length_histogram[i];
            outLengthHistogramStream << histVal;
        }
    }
}

template <int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void canonizeTree(Symbol<MAX_FREQ_DWIDTH>* sorted,
                  uint32_t num_symbols,
                  Histogram* length_histogram,
                  ap_uint<4>* symbol_bits,
                  uint16_t i_treeDepth) {
    int16_t length = i_treeDepth;
    ap_uint<SYMBOL_BITS> count = 0;
// Iterate across the symbols from lowest frequency to highest
// Assign them largest bit length to smallest
process_symbols:
    for (uint32_t k = 0; k < num_symbols; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 256 avg = 286
        if (count == 0) {
            // find the next non-zero bit length k
            count = length_histogram[--length];
        canonize_inner:
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

template <int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 16>
inline void canonizeTreeStream(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& sortedStream,
                               uint32_t num_symbols,
                               Histogram* length_histogram,
                               ap_uint<4>* symbol_bits,
                               uint16_t i_treeDepth) {
#pragma HLS INLINE
    int16_t length = i_treeDepth;
    ap_uint<SYMBOL_BITS> count = 0;
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> srtdVal;
    // Iterate across the symbols from lowest frequency to highest
    // Assign them largest bit length to smallest
    uint32_t k = 0;
    bool decLenHistFlag = false;
    bool updSymBitsFlag = true;
process_symbols:
    for (k = 0; k < num_symbols;) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 301 min = 19 avg = 286
        if (decLenHistFlag) {
            if (count == 0 && length >= 0) {
                count = length_histogram[--length];
            } else {
                decLenHistFlag = false;
                updSymBitsFlag = true;
            }
        } else {
            srtdVal = sortedStream.read();
            if (count == 0 && length >= 0) {
                // find the next non-zero bit length k
                count = length_histogram[--length];
                decLenHistFlag = true;
                updSymBitsFlag = false;
            } else {
                updSymBitsFlag = true;
            }
        }
        if (updSymBitsFlag) {
            if (length >= 0) {
                symbol_bits[srtdVal.data[0].value] = length; // assign symbol k to have length bits
                --count; // keep assigning i bits until we have counted off n symbols
            }
            ++k;
        }
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
    for (uint16_t i = 1; i <= cur_maxBits; ++i) {
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
                    hls::stream<DSVectorStream_dt<Codeword, 1> >& huffCodes,
                    uint16_t cur_symSize,
                    uint16_t cur_maxBits,
                    uint16_t symCnt) {
    //#pragma HLS inline
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    DSVectorStream_dt<Codeword, 1> hfc;
    hfc.strobe = 1;

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
first_codewords:
    for (uint16_t i = 1; i <= cur_maxBits; ++i) {
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

        hfc.data[0].codeword = (length == 0 || symCnt == 0) ? (uint16_t)0 : (uint16_t)out_reversed;
        length = (symCnt == 0) ? 0 : length;
        hfc.data[0].bitlength = (symCnt == 0 && k == 0) ? 1 : length;
        first_codeword[length]++;
        huffCodes << hfc;
        // reset symbol bits
        symbol_bits[k] = 0;
    }
}

template <int MAX_LEN>
void createZstdCodeword(ap_uint<4>* symbol_bits,
                        Histogram* length_histogram,
                        hls::stream<DSVectorStream_dt<Codeword, 1> >& huffCodes,
                        uint16_t cur_symSize,
                        uint16_t cur_maxBits,
                        uint16_t symCnt) {
    //#pragma HLS inline
    bool allSameBlen = true;
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    DSVectorStream_dt<Codeword, 1> hfc;
    hfc.strobe = 1;

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
    uint8_t uniq_bl_idx = 0;
find_uniq_blen_count:
    for (uint8_t i = 0; i < cur_maxBits + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 12
        if (length_histogram[i] == cur_symSize) uniq_bl_idx = i;
    }
    // If only 1 uniq_blc for all symbols divide into 3 bitlens
    // Means, if all the bitlens are same(mainly bitlen-8) then use an alternate tree
    // Fix the current bitlength_histogram and symbol_bits so that it generates codes-bitlens for alternate tree
    if (uniq_bl_idx > 0) {
        length_histogram[7] = 1;
        length_histogram[9] = 2;
        length_histogram[8] -= 3;

        symbol_bits[0] = 7;
        symbol_bits[1] = 9;
        symbol_bits[2] = 9;
    }

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = cur_maxBits; i > 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 11
        uint16_t cur = nextCode;
        nextCode += (length_histogram[i]);
        nextCode >>= 1;
        first_codeword[i] = cur;
    }
    Codeword code;
assign_codewords_sm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 256 min = 256 avg = 256
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // length = (uniq_bl_idx > 0 && k > 2 && length > 8) ? 8 : length;	// not needed if treegen is optimal
        length = (symCnt == 0) ? 0 : length;
        code.codeword = (uint16_t)first_codeword[length];
        // get bitlength for code
        length = (symCnt == 0 && k == 0) ? 1 : length;
        code.bitlength = length;
        // write out codes
        hfc.data[0] = code;
        first_codeword[length]++;
        huffCodes << hfc;
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void huffConstructTreeStream_1(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
                               hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                               hls::stream<ap_uint<9> >& heapLenStream,
                               hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                               hls::stream<bool>& isEOBlocks) {
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    bool last = false;
filter_sort_ldblock:
    while (!last) {
    // filter-sort for literals and distances
    filter_sort_litdist:
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
            uint16_t heapLength = 0;

            // filter the input, write 0 heapLength at end of block
            filter<MAX_FREQ_DWIDTH>(inFreq, heap, &heapLength, maxCodes, i_symbolSize);

            // check for end of block
            last = (heapLength == 0xFFFF);
            if (metaIdx == 0) isEOBlocks << last;
            if (last) break;

            // sort the input
            radixSort<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

            // send sorted frequencies
            heapLenStream << heapLength;
            hpVal.strobe = 1;
            for (uint16_t i = 0; i < heapLength; i++) {
                hpVal.data[0] = heap[i];
                heapStream << hpVal;
            }
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 18>
void zstdFreqFilterSort(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                        hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                        hls::stream<ap_uint<9> >& heapLenStream,
                        hls::stream<bool>& eobStream) {
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;

    hls::stream<ap_uint<SYMBOL_BITS> > maxCodes("maxCodes");
#pragma HLS STREAM variable = maxCodes depth = 2

filter_sort_ldblock:
    while (!last) {
        // filter-sort for literals
        ap_uint<9> i_symbolSize = SYMBOL_SIZE; // current symbol size
        uint16_t heapLength = 0;

        // filter the input, write 0 heapLength at end of block
        filter<MAX_FREQ_DWIDTH, 0>(inFreqStream, heap, &heapLength, maxCodes, i_symbolSize);
        // dump maxcode
        // maxCodes.read();
        // check for end of block
        last = (heapLength == 0xFFFF);
        eobStream << last;
        if (last) break;

        // sort the input
        radixSort<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

        // send sorted frequencies
        heapLenStream << heapLength;
        hpVal.strobe = 1;
        for (uint16_t i = 0; i < heapLength; i++) {
            hpVal.data[0] = heap[i];
            heapStream << hpVal;
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32, int WRITE_MXC = 1>
void huffFilterRadixSort(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                         hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapTreeStream,
                         hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapCanzStream,
                         hls::stream<ap_uint<9> >& heapLenStream1,
                         hls::stream<ap_uint<9> >& heapLenStream2,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                         hls::stream<bool>& isEOBlocks) {
#pragma HLS INLINE
    constexpr uint8_t c_maxSftLim = 1 + ((MAX_FREQ_DWIDTH - 1) / BITS_PER_LOOP);
    constexpr uint8_t c_maxSft = BITS_PER_LOOP * (c_maxSftLim - 1);
    // internal streams
    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > intlHeapStream[c_maxSftLim - 1];
#pragma HLS STREAM variable = intlHeapStream depth = 32

#pragma HLS DATAFLOW disable_start_propagation
    filterRadixSortPart1<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH, WRITE_MXC>(
        inFreqStream, intlHeapStream[0], heapLenStream1, heapLenStream2, maxCodes, isEOBlocks);

    // mid loops for radix sort
    radixSortMidPartial<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(intlHeapStream[0], intlHeapStream[1], BITS_PER_LOOP);

    if (c_maxSftLim >= 4) { // not needed if blockSize < 8KB or c_maxSftLim < 4
        radixSortMidPartial<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(intlHeapStream[1], intlHeapStream[2],
                                                                       BITS_PER_LOOP * 2);
    }

    radixPartialFinalSort<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(intlHeapStream[c_maxSftLim - 2], heapTreeStream,
                                                                     heapCanzStream, c_maxSft);
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void createTreeWrapper(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                       hls::stream<ap_uint<9> >& heapLenStream,
                       hls::stream<IntVectorStream_dt<SYMBOL_BITS, 1> >& parentStream) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = temp_array type = ram_t2p impl = bram
    bool done = false;
    IntVectorStream_dt<SYMBOL_BITS, 1> outPrtVal;
    outPrtVal.strobe = 1;

create_tree_comp_bitlengths:
    while (true) {
        uint16_t heapLength = heapLenStream.read();
        done = (heapLength == (((uint16_t)1 << 9) - 1));
        // termination condition
        if (done) {
            outPrtVal.strobe = 0;
            outPrtVal.data[0] = 0;
            parentStream << outPrtVal;
            break;
        }
        // write heapLength
        outPrtVal.data[0] = heapLength;
        parentStream << outPrtVal;
        // create tree
        createTreeStream<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heapStream, heapLength, parentStream, left, right,
                                                                    temp_array);
        // write the left & right to output stream
        constexpr uint8_t itrCnt = 1 + ((SYMBOL_SIZE - 1) / SYMBOL_BITS);
    write_left_right:
        for (ap_uint<2> k = 0; k < 2; ++k) {
            if (k == 1) left = right;
        write_word:
            for (uint8_t i = 0; i < itrCnt; ++i) {
#pragma HLS PIPELINE II = 1
                outPrtVal.data[0] = left.range(SYMBOL_BITS - 1, 0);
                left >>= SYMBOL_BITS;
                parentStream << outPrtVal;
            }
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void getHuffBitLengths(hls::stream<IntVectorStream_dt<SYMBOL_BITS, 1> >& parentStream,
                       hls::stream<DSVectorStream_dt<Histogram, 1> >& lengthHistogramStream) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    ap_uint<1> metaIdx = 0;
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = parent type = ram_2p impl = lutram
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
    //#pragma HLS BIND_STORAGE variable = temp_array type = ram_t2p impl = bram
    Histogram length_histogram[c_lengthHistogram];
#pragma HLS ARRAY_PARTITION variable = length_histogram complete

create_tree_comp_bitlengths:
    while (true) {
        DSVectorStream_dt<Histogram, 1> outHistVal;
        uint16_t i_symbolSize = hctMeta[metaIdx++]; // current symbol size
        auto inPrtVal = parentStream.read();
        uint16_t heapLength = inPrtVal.data[0];
        // termination condition
        if (inPrtVal.strobe == 0) {
            outHistVal.strobe = 0;
            outHistVal.data[0] = 0;
            lengthHistogramStream << outHistVal;
            break;
        }
        auto rhpLen = heapLength + (uint16_t)(heapLength < 3);

    init_lenHist_parent_stream:
        for (uint16_t i = 0; i < i_symbolSize; ++i) {
#pragma HLS PIPELINE II = 1
            temp_array[i] = 0;
            if (i < rhpLen) {
                inPrtVal = parentStream.read();
                parent[i] = inPrtVal.data[0];
            } else {
                parent[i] = 0;
            }
        }
        // init histogram
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS UNROLL
            length_histogram[i] = 0;
        }
        // read the left & right to output stream
        constexpr uint8_t lrItr = 1 + ((SYMBOL_SIZE - 1) / SYMBOL_BITS);
        constexpr uint8_t bitRem = SYMBOL_SIZE - ((lrItr - 1) * SYMBOL_BITS);
    read_left_right:
        for (ap_uint<2> k = 0; k < 2; ++k) {
            inPrtVal = parentStream.read();
        read_word:
            for (uint8_t i = 0; i < lrItr - 1; ++i) {
#pragma HLS PIPELINE II = 1
                right >>= SYMBOL_BITS;
                right.range(SYMBOL_SIZE - 1, SYMBOL_SIZE - SYMBOL_BITS) = inPrtVal.data[0];
                // read next part
                inPrtVal = parentStream.read();
            }
            right >>= bitRem;
            right.range(SYMBOL_SIZE - 1, SYMBOL_SIZE - bitRem) = (ap_uint<bitRem>)inPrtVal.data[0];
            if (k == 0) left = right;
        }

        // get bit-lengths from tree
        computeBitLengthLL<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                      temp_array);
        // write the output
        outHistVal.strobe = 1;
    write_blen_hist:
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 18
#pragma HLS PIPELINE II = 1
            outHistVal.data[0] = length_histogram[i];
            lengthHistogramStream << outHistVal;
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void canonizeGetCodes(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                      hls::stream<ap_uint<9> >& heapLenStream,
                      hls::stream<DSVectorStream_dt<Histogram, 1> >& lengthHistogramStream,
                      hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    static bool initBitsMem = true;
    // internal buffers
    static ap_uint<4> symbol_bits[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];
    ap_uint<1> metaIdx = 0;
    DSVectorStream_dt<Codeword, 1> cdVal;
    if (initBitsMem) { // init only once
    init_smb_buffer:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS LOOP_TRIPCOUNT max = SYMBOL_SIZE
#pragma HLS PIPELINE off
            symbol_bits[i] = 0;
        }
        initBitsMem = false;
    }
construct_tree_ldblock:
    while (true) {
        uint16_t i_symbolSize = hctMeta[metaIdx++]; // current symbol size
        uint16_t heapLength = heapLenStream.read(); // value dumped at end of file
        auto histVal = lengthHistogramStream.read();
        if (histVal.strobe == 0) break;
        // init latency will hide behind latency of previous module in dataflow

        length_histogram[0] = histVal.data[0];
    read_cnz_ln_hist_buff:
        for (uint8_t i = 1; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 17 max = 17
#pragma HLS PIPELINE II = 1
            auto histVal = lengthHistogramStream.read();
            length_histogram[i] = histVal.data[0];
        }

        // canonize the tree
        canonizeTreeStream<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heapStream, heapLength, length_histogram, symbol_bits,
                                                         c_lengthHistogram);

        // generate huffman codewords
        createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, c_maxCodeBits, heapLength);
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH = 32>
void huffConstructTreeStream_2(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                               hls::stream<ap_uint<9> >& heapLenStream,
                               hls::stream<bool>& isEOBlocks,
                               hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = parent type = ram_2p impl = lutram
    Histogram length_histogram[c_lengthHistogram];

    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = temp_array type = ram_1wnr impl = bram
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_t2p impl = bram
#pragma HLS AGGREGATE variable = heap
    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    const ap_uint<9> hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                                      {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                                      {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
construct_tree_ldblock:
    while (isEOBlocks.read() == false) {
    construct_tree_litdist:
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            uint16_t i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
            uint16_t i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
            uint16_t i_maxBits = hctMeta[metaIdx][2];    // current max bits

            uint16_t heapLength = heapLenStream.read();

        init_buffers:
            for (uint16_t i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
                parent[i] = 0;
                if (i < c_lengthHistogram) length_histogram[i] = 0;
                temp_array[i] = 0;
                if (i < heapLength) {
                    hpVal = heapStream.read();
                    heap[i] = hpVal.data[0];
                }
                symbol_bits[i] = 0;
            }

            // create tree
            createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

            // get bit-lengths from tree
            computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength,
                                                                        length_histogram, temp_array);

            // truncate tree for any bigger bit-lengths
            truncateTree(length_histogram, c_lengthHistogram, i_maxBits);

            // canonize the tree
            canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits,
                                                       c_lengthHistogram);

            // generate huffman codewords
            createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
        }
    }
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int MAX_FREQ_DWIDTH, int MAX_BITS>
void zstdGetHuffmanCodes(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                         hls::stream<ap_uint<9> >& heapLenStream,
                         hls::stream<bool>& isEOBlocks,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];
#pragma HLS ARRAY_PARTITION variable = length_histogram complete
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];

    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_t2p impl = bram
#pragma HLS AGGREGATE variable = heap

    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
construct_tree_ldblock:
    while (isEOBlocks.read() == false) {
        uint16_t heapLength = heapLenStream.read();
    init_buffers:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
#pragma HLS PIPELINE II = 1
            parent[i] = 0;
            if (i < c_lengthHistogram) length_histogram[i] = 0;
            temp_array[i] = 0;
            if (i < heapLength) {
                hpVal = heapStream.read();
                heap[i] = hpVal.data[0];
            }
            symbol_bits[i] = 0;
        }

        // create tree
        createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

        // get bit-lengths from tree
        computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                    temp_array);

        // truncate tree for any bigger bit-lengths
        truncateTree(length_histogram, c_lengthHistogram, MAX_BITS);

        // canonize the tree
        canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits, MAX_BITS + 1);

        // generate huffman codewords
        createZstdCodeword<MAX_BITS>(symbol_bits, length_histogram, outCodes, SYMBOL_SIZE, MAX_BITS, heapLength);
    }
}

template <int SYMBOL_SIZE, int MAX_FREQ_DWIDTH, int MAX_BITS, int BLEN_BITS>
void huffCodeWeightDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& hufCodeStream,
                               hls::stream<bool>& isEOBlocks,
                               hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                               hls::stream<IntVectorStream_dt<BLEN_BITS, 1> >& outWeightsStream,
                               hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
    // distribute huffman codes to multiple output streams and one separate bitlen stream
    DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> outCode;
    IntVectorStream_dt<BLEN_BITS, 1> outWeight;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outFreq;
    int blk_n = 0;
distribute_code_block:
    while (isEOBlocks.read() == false) {
        ++blk_n;
        ap_uint<MAX_FREQ_DWIDTH> weightFreq[MAX_BITS + 1];
        ap_uint<BLEN_BITS> blenBuf[SYMBOL_SIZE];
#pragma HLS ARRAY_PARTITION variable = weightFreq complete
#pragma HLS BIND_STORAGE variable = blenBuf type = ram_1p impl = lutram
        ap_uint<BLEN_BITS> curMaxBitlen = 0;
        uint8_t maxWeight = 0;
        uint16_t maxVal = 0;
    init_freq_bl:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE off
            weightFreq[i] = 0;
        }
        outCode.strobe = 1;
        outWeight.strobe = 1;
        outFreq.strobe = 1;
    distribute_code_loop:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto inCode = hufCodeStream.read();
            uint8_t hfblen = inCode.data[0].bitlength;
            uint16_t hfcode = inCode.data[0].codeword;
            outCode.data[0].code = hfcode;
            outCode.data[0].bitlen = hfblen;
            blenBuf[i] = hfblen;
            if (hfblen > curMaxBitlen) curMaxBitlen = hfblen;
            if (hfblen > 0) {
                maxVal = (uint16_t)i;
            }
            outCodeStream << outCode;
        }
    send_weights:
        for (ap_uint<9> i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto bitlen = blenBuf[i];
            auto blenWeight = (uint8_t)((bitlen > 0) ? (uint8_t)(curMaxBitlen + 1 - bitlen) : 0);
            outWeight.data[0] = blenWeight;
            weightFreq[blenWeight] += (uint8_t)(i < maxVal + 1); // conditional increment
            outWeightsStream << outWeight;
        }
        // write maxVal as first value
        outFreq.data[0] = maxVal;
        weightFreqStream << outFreq;
    // send weight frequencies
    send_weight_freq:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE II = 1
            outFreq.data[0] = weightFreq[i];
            weightFreqStream << outFreq;
            if (outFreq.data[0] > 0) maxWeight = i; // to be deduced by module reading this stream
        }
        // end of block
        outCode.strobe = 0;
        outWeight.strobe = 0;
        outFreq.strobe = 0;
        outCodeStream << outCode;
        outWeightsStream << outWeight;
        weightFreqStream << outFreq;
    }
    // end of all data
    outCode.strobe = 0;
    outWeight.strobe = 0;
    outFreq.strobe = 0;
    outCodeStream << outCode;
    outWeightsStream << outWeight;
    weightFreqStream << outFreq;
}

template <int SYMBOL_SIZE, int SYMBOL_BITS, int LENGTH_SIZE, int MAX_FREQ_DWIDTH = 32>
void huffConstructTreeStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
                             hls::stream<bool>& isEOBlocks,
                             hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                             hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                             uint8_t metaIdx) {
#pragma HLS inline
    // construct huffman tree and generate codes and bit-lengths
    const ap_uint<9> hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                                      {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                                      {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};

    const ap_uint<9> i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
    const ap_uint<9> i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
    const ap_uint<9> i_maxBits = hctMeta[metaIdx][2];    // current max bits

    while (isEOBlocks.read() == false) {
        // internal buffers
        Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];

        ap_uint<SYMBOL_SIZE> left = 0;
        ap_uint<SYMBOL_SIZE> right = 0;
        ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
        Histogram length_histogram[LENGTH_SIZE];
        Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
        ap_uint<4> symbol_bits[SYMBOL_SIZE];
        IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> curFreq;
        uint16_t heapLength = 0;

        {
#pragma HLS dataflow
        init_buffers:
            for (ap_uint<9> i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE off
                parent[i] = 0;
                if (i < LENGTH_SIZE) length_histogram[i] = 0;
                temp_array[i] = 0;
                symbol_bits[i] = 0;
                /*heap[i].value = 0;
                heap[i].frequency = 0;*/
            }
            // filter the input, write 0 heapLength at end of block
            filter<MAX_FREQ_DWIDTH>(inFreq, heap, &heapLength, maxCodes, i_symbolSize);
        }

        // sort the input
        radixSort_1<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

        // create tree
        createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

        // get bit-lengths from tree
        computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                    temp_array);

        // truncate tree for any bigger bit-lengths
        truncateTree(length_histogram, LENGTH_SIZE, i_maxBits);

        // canonize the tree
        canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits, LENGTH_SIZE);

        // generate huffman codewords
        createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
    }
}

template <int MAX_FREQ_DWIDTH = 32>
void genBitLenFreq(hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                   hls::stream<bool>& isEOBlocks,
                   hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& freq,
                   hls::stream<ap_uint<c_tgnSymbolBits> >& maxCode) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    ap_uint<MAX_FREQ_DWIDTH> blFreq[19];
    // iterate over blocks
    while (isEOBlocks.read() == false) {
    init_bitlen_freq:
        for (uint8_t i = 0; i < 19; ++i) {
#pragma HLS PIPELINE off
            blFreq[i] = 0;
        }
        ap_uint<MAX_FREQ_DWIDTH> repPrevBlen = 0;
        ap_uint<MAX_FREQ_DWIDTH> repZeroBlen = 0;
        ap_uint<MAX_FREQ_DWIDTH> repZeroBlen7 = 0;

        for (uint8_t itr = 0; itr < 2; ++itr) {
            int16_t prevlen = -1;
            int16_t curlen = 0;
            int16_t count = 0;
            int16_t max_count = 7;
            int16_t min_count = 4;

            int16_t nextlen = outCodes.read().data[0].bitlength;
            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            ap_uint<c_tgnSymbolBits> maximumCodeLength = maxCode.read();
            ap_uint<MAX_FREQ_DWIDTH> inc = 0;
        parse_tdata:
            for (ap_uint<c_tgnSymbolBits> n = 0; n <= maximumCodeLength; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 2
                curlen = nextlen;
                if (n == maximumCodeLength) {
                    nextlen = 0xF;
                } else {
                    nextlen = outCodes.read().data[0].bitlength;
                }
                if (++count < max_count && curlen == nextlen) {
                    continue;
                } else if (count < min_count) {
                    blFreq[curlen] += count;
                } else if (curlen != 0) {
                    if (curlen != prevlen) {
                        blFreq[curlen]++;
                    }
                    ++repPrevBlen;
                } else if (count <= 10) {
                    ++repZeroBlen;
                } else {
                    ++repZeroBlen7;
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
        read_spare_codes:
            for (auto i = maximumCodeLength + 1; i < hctMeta[itr]; ++i) {
#pragma HLS PIPELINE II = 1
                auto tmp = outCodes.read();
            }
        }
        // repeat freqs
        blFreq[c_reusePrevBlen] = repPrevBlen;
        blFreq[c_reuseZeroBlen] = repZeroBlen;
        blFreq[c_reuseZeroBlen7] = repZeroBlen7;
        // write output
        IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> blf;
        blf.strobe = 1;
    output_bitlen_frequencies:
        for (uint8_t i = 0; i < 19; ++i) {
#pragma HLS PIPELINE II = 1
            blf.data[0] = blFreq[i];
            freq << blf;
        }
    }
}

template <int MAX_FREQ_DWIDTH = 32>
void genBitLenFreq(Codeword* outCodes, Frequency<MAX_FREQ_DWIDTH>* blFreq, uint16_t maxCode) {
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

template <uint8_t SEND_EOS = 1>
void sendTrees(hls::stream<ap_uint<c_tgnSymbolBits> >& maxLdCodes,
               hls::stream<ap_uint<c_tgnSymbolBits> >& maxBlCodes,
               hls::stream<DSVectorStream_dt<Codeword, 1> >& Ldcodes,
               hls::stream<DSVectorStream_dt<Codeword, 1> >& Blcodes,
               hls::stream<bool>& isEOBlocks,
               hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hfcodeStream) {
    const uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
#pragma HLS ARRAY_PARTITION variable = bitlen_vals complete dim = 1

    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outHufCode;
#pragma HLS AGGREGATE variable = outHufCode
send_tree_outer:
    while (isEOBlocks.read() == false) {
        Codeword zeroValue;
        zeroValue.bitlength = 0;
        zeroValue.codeword = 0;
        Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
#pragma HLS AGGREGATE variable = outCodes

        ap_uint<c_tgnSymbolBits> maxCodesReg[3];

        const uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
        // initialize all the memory
        maxCodesReg[0] = maxLdCodes.read();
        outHufCode.strobe = 1;
    read_litcodes_send:
        for (uint16_t i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }
        // last value write
        outCodes[c_litCodeCount] = zeroValue;
        maxCodesReg[1] = maxLdCodes.read();

    read_dstcodes_send:
        for (uint16_t i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[offsets[1] + i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }

        outCodes[c_litCodeCount + c_dstCodeCount + 1] = zeroValue;
        maxCodesReg[2] = maxBlCodes.read();

    read_blcodes:
        for (uint16_t i = offsets[2]; i < offsets[2] + c_blnCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Blcodes.read();
            outCodes[i] = ldc.data[0];
        }

        ap_uint<c_tgnSymbolBits> bl_mxc;
        bool mxb_continue = true;
    bltree_blen:
        for (bl_mxc = c_blnCodeCount - 1; (bl_mxc >= 3) && mxb_continue; --bl_mxc) {
#pragma HLS PIPELINE II = 1
            auto cIdx = offsets[2] + bitlen_vals[bl_mxc];
            mxb_continue = (outCodes[cIdx].bitlength == 0);
        }

        maxCodesReg[2] = bl_mxc + 1;

        // Code from Huffman Encoder
        //********************************************//
        // Start of block = 4 and len = 3
        // For dynamic tree
        outHufCode.strobe = 1;
        outHufCode.data[0].code = 4;
        outHufCode.data[0].bitlen = 3;
        hfcodeStream << outHufCode;
        // lcodes
        outHufCode.data[0].code = ((maxCodesReg[0] + 1) - 257);
        outHufCode.data[0].bitlen = 5;
        hfcodeStream << outHufCode;
        // dcodes
        outHufCode.data[0].code = ((maxCodesReg[1] + 1) - 1);
        outHufCode.data[0].bitlen = 5;
        hfcodeStream << outHufCode;
        // blcodes
        outHufCode.data[0].code = ((maxCodesReg[2] + 1) - 4);
        outHufCode.data[0].bitlen = 4;
        hfcodeStream << outHufCode;

        ap_uint<c_tgnSymbolBits> bitIndex = offsets[2];
        outHufCode.strobe = 1;
    // Send BL length data
    send_bltree:
        for (ap_uint<c_tgnSymbolBits> rank = 0; rank < maxCodesReg[2] + 1; ++rank) {
#pragma HLS LOOP_TRIPCOUNT avg = 19 max = 19
#pragma HLS PIPELINE II = 1
            outHufCode.data[0].code = outCodes[bitIndex + bitlen_vals[rank]].bitlength;
            outHufCode.data[0].bitlen = 3;
            hfcodeStream << outHufCode;
        } // BL data copy loop

        Codeword temp;
#pragma HLS AGGREGATE variable = temp
    // Send Bitlengths for Literal and Distance Tree
    send_ld_trees_outer:
        for (int tree = 0; tree < 2; ++tree) {
            uint8_t prevlen = 0;                                 // Last emitted Length
            uint8_t curlen = 0;                                  // Length of Current Code
            uint8_t nextlen = outCodes[offsets[tree]].bitlength; // Length of next code
            uint8_t count = 0;
            int max_count = 7; // Max repeat count
            int min_count = 4; // Min repeat count

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            uint16_t max_code = maxCodesReg[tree];
            temp = outCodes[bitIndex + c_reusePrevBlen];
            uint16_t reuse_prev_code = temp.codeword;
            uint8_t reuse_prev_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen];
            uint16_t reuse_zero_code = temp.codeword;
            uint8_t reuse_zero_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen7];
            uint16_t reuse_zero7_code = temp.codeword;
            uint8_t reuse_zero7_len = temp.bitlength;

            uint8_t repCnt = 0;
            bool refCntFlag = true;
        send_ldtree_main:
            for (uint16_t n = 0; n <= max_code || repCnt > 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 3
                if (repCnt != 0) {
                    outHufCode.data[0].code = temp.codeword;
                    outHufCode.data[0].bitlen = temp.bitlength;
                write_rep_codes_blens:
                    for (uint8_t k = 0; k < 3; ++k) {
#pragma HLS UNROLL
                        if (repCnt > 0) {
                            --repCnt;
                            hfcodeStream << outHufCode;
                        }
                    }
                    refCntFlag = (repCnt == 0);
                } else {
                    refCntFlag = true;
                    curlen = nextlen;
                    // Length of next code
                    nextlen = outCodes[offsets[tree] + n + 1].bitlength;

                    if (++count < max_count && curlen == nextlen) {
                        refCntFlag = false;
                    } else if (count < min_count) {
                        temp = outCodes[bitIndex + curlen];
                        repCnt = count;
                        refCntFlag = false;
                    } else if (curlen != 0) {
                        if (curlen != prevlen) {
                            temp = outCodes[bitIndex + curlen];
                            outHufCode.data[0].code = temp.codeword;
                            outHufCode.data[0].bitlen = temp.bitlength;
                            hfcodeStream << outHufCode;
                            count--;
                        }
                        outHufCode.data[0].code = reuse_prev_code;
                        outHufCode.data[0].bitlen = reuse_prev_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 3;
                        outHufCode.data[0].bitlen = 2;
                        hfcodeStream << outHufCode;

                    } else if (count <= 10) {
                        outHufCode.data[0].code = reuse_zero_code;
                        outHufCode.data[0].bitlen = reuse_zero_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 3;
                        outHufCode.data[0].bitlen = 3;
                        hfcodeStream << outHufCode;
                    } else {
                        outHufCode.data[0].code = reuse_zero7_code;
                        outHufCode.data[0].bitlen = reuse_zero7_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 11;
                        outHufCode.data[0].bitlen = 7;
                        hfcodeStream << outHufCode;
                    }
                    ++n;
                }
                if (refCntFlag) {
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
        }
        // ends huffman stream for each block, strobe eos not needed from this module
        outHufCode.data[0].bitlen = 0;
        hfcodeStream << outHufCode;
    }
    if (SEND_EOS > 0) {
        // end of huffman tree data for all blocks
        outHufCode.strobe = 0;
        hfcodeStream << outHufCode;
    }
}

void sendProcTrees(hls::stream<ap_uint<c_tgnSymbolBits> >& maxLdCodes,
                   hls::stream<ap_uint<c_tgnSymbolBits> >& maxBlCodes,
                   hls::stream<DSVectorStream_dt<Codeword, 1> >& Ldcodes,
                   hls::stream<DSVectorStream_dt<Codeword, 1> >& Blcodes,
                   hls::stream<bool>& isEOBlocks,
                   hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> >& hfcodeStream) {
    const uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
#pragma HLS ARRAY_PARTITION variable = bitlen_vals complete dim = 1

    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> outHufCode;
#pragma HLS AGGREGATE variable = outHufCode
send_tree_outer:
    while (isEOBlocks.read() == false) {
        Codeword zeroValue;
        zeroValue.bitlength = 0;
        zeroValue.codeword = 0;
        Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
#pragma HLS AGGREGATE variable = outCodes

        ap_uint<c_tgnSymbolBits> maxCodesReg[3];

        const uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
        // initialize all the memory
        maxCodesReg[0] = maxLdCodes.read();
        outHufCode.strobe = 1;
    read_litcodes_send:
        for (uint16_t i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }
        // last value write
        outCodes[c_litCodeCount] = zeroValue;
        maxCodesReg[1] = maxLdCodes.read();

    read_dstcodes_send:
        for (uint16_t i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[offsets[1] + i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }

        outCodes[c_litCodeCount + c_dstCodeCount + 1] = zeroValue;
        maxCodesReg[2] = maxBlCodes.read();

    read_blcodes:
        for (uint16_t i = offsets[2]; i < offsets[2] + c_blnCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Blcodes.read();
            outCodes[i] = ldc.data[0];
        }

        ap_uint<c_tgnSymbolBits> bl_mxc;
        bool mxb_continue = true;
    bltree_blen:
        for (bl_mxc = c_blnCodeCount - 1; (bl_mxc >= 3) && mxb_continue; --bl_mxc) {
#pragma HLS PIPELINE II = 1
            auto cIdx = offsets[2] + bitlen_vals[bl_mxc];
            mxb_continue = (outCodes[cIdx].bitlength == 0);
        }

        maxCodesReg[2] = bl_mxc + 1;

        // Code from Huffman Encoder
        //********************************************//
        // Start of block = 4 and len = 3
        // For dynamic tree
        outHufCode.strobe = 1;
        outHufCode.data[0].code = 4;
        outHufCode.data[0].bitlen = 3;
        hfcodeStream << outHufCode;
        // lcodes
        outHufCode.data[0].code = ((maxCodesReg[0] + 1) - 257);
        outHufCode.data[0].bitlen = 5;
        // dcodes
        outHufCode.data[1].code = ((maxCodesReg[1] + 1) - 1);
        outHufCode.data[1].bitlen = 5;
        // blcodes
        outHufCode.data[2].code = ((maxCodesReg[2] + 1) - 4);
        outHufCode.data[2].bitlen = 4;
        outHufCode.strobe = 3;
        hfcodeStream << outHufCode;

        ap_uint<c_tgnSymbolBits> bitIndex = offsets[2];
        outHufCode.strobe = 1;
    // Send BL length data
    send_bltree:
        for (ap_uint<c_tgnSymbolBits> rank = 0; rank < maxCodesReg[2] + 1; ++rank) {
#pragma HLS LOOP_TRIPCOUNT avg = 19 max = 19
#pragma HLS PIPELINE II = 1
            outHufCode.data[0].code = outCodes[bitIndex + bitlen_vals[rank]].bitlength;
            outHufCode.data[0].bitlen = 3;
            hfcodeStream << outHufCode;
        } // BL data copy loop

        Codeword temp;
#pragma HLS AGGREGATE variable = temp
    // Send Bitlengths for Literal and Distance Tree
    send_ld_trees_outer:
        for (int tree = 0; tree < 2; ++tree) {
            uint8_t prevlen = 0;                                 // Last emitted Length
            uint8_t curlen = 0;                                  // Length of Current Code
            uint8_t nextlen = outCodes[offsets[tree]].bitlength; // Length of next code
            uint8_t count = 0;
            int max_count = 7; // Max repeat count
            int min_count = 4; // Min repeat count

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            uint16_t max_code = maxCodesReg[tree];
            temp = outCodes[bitIndex + c_reusePrevBlen];
            uint16_t reuse_prev_code = temp.codeword;
            uint8_t reuse_prev_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen];
            uint16_t reuse_zero_code = temp.codeword;
            uint8_t reuse_zero_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen7];
            uint16_t reuse_zero7_code = temp.codeword;
            uint8_t reuse_zero7_len = temp.bitlength;

            uint8_t repCnt = 0;
            bool refCntFlag = true;
        send_ldtree_main:
            for (uint16_t n = 0; n <= max_code || repCnt > 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 1
                if (repCnt != 0) {
                    uint8_t lclCnt = 0;
                    uint8_t tmp = repCnt;
                    bool cond = (repCnt <= 3);
                    repCnt = cond ? 0 : repCnt - 3;
                write_rep_codes_blens:
                    for (uint8_t k = 0; k < 3; ++k) {
#pragma HLS UNROLL
                        if (tmp > 0) {
                            outHufCode.data[k].code = temp.codeword;
                            outHufCode.data[k].bitlen = temp.bitlength;
                            --tmp;
                            ++lclCnt;
                        }
                    }
                    outHufCode.strobe = lclCnt;
                    hfcodeStream << outHufCode;
                    refCntFlag = (cond);
                } else {
                    refCntFlag = true;
                    curlen = nextlen;
                    // Length of next code
                    nextlen = outCodes[offsets[tree] + n + 1].bitlength;

                    if (++count < max_count && curlen == nextlen) {
                        refCntFlag = false;
                    } else if (count < min_count) {
                        temp = outCodes[bitIndex + curlen];
                        repCnt = count;
                        refCntFlag = false;
                    } else if (curlen != 0) {
                        uint8_t idx = 0;
                        if (curlen != prevlen) {
                            temp = outCodes[bitIndex + curlen];
                            outHufCode.data[0].code = temp.codeword;
                            outHufCode.data[0].bitlen = temp.bitlength;
                            idx = 1;
                            count--;
                        }
                        outHufCode.data[idx].code = reuse_prev_code;
                        outHufCode.data[idx].bitlen = reuse_prev_len;

                        outHufCode.data[idx + 1].code = count - 3;
                        outHufCode.data[idx + 1].bitlen = 2;

                        outHufCode.strobe = idx + 2;
                        hfcodeStream << outHufCode;
                    } else if (count <= 10) {
                        outHufCode.data[0].code = reuse_zero_code;
                        outHufCode.data[0].bitlen = reuse_zero_len;

                        outHufCode.data[1].code = count - 3;
                        outHufCode.data[1].bitlen = 3;
                        outHufCode.strobe = 2;
                        hfcodeStream << outHufCode;
                    } else {
                        outHufCode.data[0].code = reuse_zero7_code;
                        outHufCode.data[0].bitlen = reuse_zero7_len;

                        outHufCode.data[1].code = count - 11;
                        outHufCode.data[1].bitlen = 7;
                        outHufCode.strobe = 2;
                        hfcodeStream << outHufCode;
                    }
                    ++n;
                }
                if (refCntFlag) {
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
        }
        // ends huffman stream for each block, strobe eos not needed from this module
        outHufCode.strobe = 1;
        outHufCode.data[0].bitlen = 0;
        hfcodeStream << outHufCode;
    }
    // end of huffman tree data for all blocks
    outHufCode.strobe = 0;
    hfcodeStream << outHufCode;
}

template <uint8_t SEND_EOS = 1>
void sendHuffData(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> >& hfcodeInStream,
                  hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hfcodeOutStream) {
    // run until strobe 0
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> inHfVal;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outHufVal;
    ap_uint<3> cnt = 0, idx = 0;

    outHufVal.strobe = 1;
    inHfVal.strobe = 1;
send_all_hf_data:
    while (inHfVal.strobe > 0) {
#pragma HLS PIPELINE II = 1
        if (cnt > 0) {
            outHufVal.data[0] = inHfVal.data[idx++];
            --cnt;
        } else {
            inHfVal = hfcodeInStream.read();
            cnt = inHfVal.strobe - 1;
            idx = 1;
            outHufVal.data[0] = inHfVal.data[0];
        }
        if (inHfVal.strobe > 0) hfcodeOutStream << outHufVal;
    }
    if (SEND_EOS > 0) {
        // end of huffman tree data for all blocks
        outHufVal.strobe = 0;
        hfcodeOutStream << outHufVal;
    }
}

void codeWordDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& inStreamCode,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode1,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode2,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& inStreamMaxCode,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode1,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode2,
                         hls::stream<bool>& isEOBlocks) {
    const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    while (isEOBlocks.read() == false) {
    distribute_litdist_codes:
        for (uint8_t i = 0; i < 2; i++) {
            auto maxCode = inStreamMaxCode.read();
            outStreamMaxCode1 << maxCode;
            outStreamMaxCode2 << maxCode;
        distribute_hufcodes_main:
            for (uint16_t j = 0; j < hctMeta[i]; j++) {
#pragma HLS PIPELINE II = 1
                auto inVal = inStreamCode.read();
                outStreamCode1 << inVal;
                outStreamCode2 << inVal;
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

template <int MAX_FREQ_DWIDTH = 32>
void processBitLength(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& frequencies,
                      hls::stream<bool>& isEOBlocks,
                      hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                      hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes) {
    // read freqStream and generate codes for it
    // construct the huffman tree and generate huffman codes
    details::huffConstructTreeStream<c_blnCodeCount, c_tgnBitlengthBits, 15, MAX_FREQ_DWIDTH>(frequencies, isEOBlocks,
                                                                                              outCodes, maxCodes, 2);
}

template <int MAX_FREQ_DWIDTH = 24, uint8_t SEND_EOS = 1>
void zlibTreegenStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& lz77TreeStream,
                       hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeStream) {
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes("ldCodes");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes1("ldCodes1");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes2("ldCodes2");
    hls::stream<DSVectorStream_dt<Codeword, 1> > blCodes("blCodes");
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > ldFrequency("ldFrequency");

    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes("ldMaxCodes");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes1("ldMaxCodes1");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes2("ldMaxCodes2");
    hls::stream<ap_uint<c_tgnSymbolBits> > blMaxCodes("blMaxCodes");

    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapStream("heapStream");
    hls::stream<ap_uint<9> > heapLenStream("heapLenStream");
    hls::stream<bool> isEOBlocks("eob_source");
    hls::stream<bool> eoBlocks[5];

#pragma HLS STREAM variable = heapStream depth = 320
#pragma HLS STREAM variable = ldCodes2 depth = 320
#pragma HLS STREAM variable = ldMaxCodes2 depth = 4
#pragma HLS STREAM variable = eoBlocks depth = 8

// DATAFLOW
#pragma HLS DATAFLOW
    huffConstructTreeStream_1<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(lz77TreeStream, heapStream,
                                                                                heapLenStream, ldMaxCodes, isEOBlocks);
    streamDistributor<5>(isEOBlocks, eoBlocks);
    huffConstructTreeStream_2<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(heapStream, heapLenStream, eoBlocks[0],
                                                                                ldCodes);
    codeWordDistributor(ldCodes, ldCodes1, ldCodes2, ldMaxCodes, ldMaxCodes1, ldMaxCodes2, eoBlocks[1]);
    genBitLenFreq<MAX_FREQ_DWIDTH>(ldCodes1, eoBlocks[2], ldFrequency, ldMaxCodes1);
    processBitLength<MAX_FREQ_DWIDTH>(ldFrequency, eoBlocks[3], blCodes, blMaxCodes);
    sendTrees<SEND_EOS>(ldMaxCodes2, blMaxCodes, ldCodes2, blCodes, eoBlocks[4], hufCodeStream);
}

template <int MAX_FREQ_DWIDTH = 24, uint8_t SEND_EOS = 1>
void zlibTreegenStreamLL(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& lz77TreeStream,
                         hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeStream) {
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes("ldCodes");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes1("ldCodes1");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes2("ldCodes2");
    hls::stream<DSVectorStream_dt<Codeword, 1> > blCodes("blCodes");
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > ldFrequency("ldFrequency");

    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes("ldMaxCodes");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes1("ldMaxCodes1");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes2("ldMaxCodes2");
    hls::stream<ap_uint<c_tgnSymbolBits> > blMaxCodes("blMaxCodes");

    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapTreeStream("heapTreeStream");
    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapCanzStream("heapCanzStream");
    hls::stream<ap_uint<9> > heapLenStream1("heapLenStream1");
    hls::stream<ap_uint<9> > heapLenStream2("heapLenStream2");
    hls::stream<IntVectorStream_dt<c_tgnSymbolBits, 1> > parentStream("parentStream");
    hls::stream<DSVectorStream_dt<Histogram, 1> > lengthHistogramStream("lengthHistogramStream");
    hls::stream<DSVectorStream_dt<Histogram, 1> > truncLengthHistStream("truncLengthHistStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> > intlHufCodeStream("intlHufCodeStream");
    hls::stream<bool> isEOBlocks("eob_source");
    hls::stream<bool> eoBlocks[4];

#pragma HLS AGGREGATE variable = parentStream
#pragma HLS AGGREGATE variable = heapTreeStream
#pragma HLS AGGREGATE variable = heapCanzStream
#pragma HLS AGGREGATE variable = intlHufCodeStream
#pragma HLS AGGREGATE variable = lengthHistogramStream
#pragma HLS AGGREGATE variable = truncLengthHistStream
#pragma HLS AGGREGATE variable = ldFrequency

#pragma HLS STREAM variable = heapTreeStream depth = 320
#pragma HLS STREAM variable = heapCanzStream depth = 1280
#pragma HLS STREAM variable = heapLenStream1 depth = 8
#pragma HLS STREAM variable = heapLenStream2 depth = 16
#pragma HLS STREAM variable = parentStream depth = 640
#pragma HLS STREAM variable = lengthHistogramStream depth = 36
#pragma HLS STREAM variable = truncLengthHistStream depth = 72
#pragma HLS STREAM variable = ldCodes depth = 16
#pragma HLS STREAM variable = ldCodes1 depth = 320
#pragma HLS STREAM variable = ldCodes2 depth = 640
#pragma HLS STREAM variable = ldMaxCodes depth = 16
#pragma HLS STREAM variable = ldMaxCodes1 depth = 4
#pragma HLS STREAM variable = ldMaxCodes2 depth = 8
#pragma HLS STREAM variable = ldFrequency depth = 32
#pragma HLS STREAM variable = blCodes depth = 32
#pragma HLS STREAM variable = blMaxCodes depth = 4
#pragma HLS STREAM variable = intlHufCodeStream depth = 320
#pragma HLS STREAM variable = isEOBlocks depth = 8
#pragma HLS STREAM variable = eoBlocks depth = 8

// DATAFLOW
#pragma HLS DATAFLOW
    // Runs 2 times per block
    huffFilterRadixSort<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH, 1>(
        lz77TreeStream, heapTreeStream, heapCanzStream, heapLenStream1, heapLenStream2, ldMaxCodes, isEOBlocks);
    createTreeWrapper<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(heapTreeStream, heapLenStream1, parentStream);
    getHuffBitLengths<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(parentStream, lengthHistogramStream);
    truncateTreeStream(lengthHistogramStream, truncLengthHistStream);
    canonizeGetCodes<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(heapCanzStream, heapLenStream2,
                                                                       truncLengthHistStream, ldCodes);
    // Runs 1 time per block
    streamDistributor<4>(isEOBlocks, eoBlocks);
    codeWordDistributor(ldCodes, ldCodes1, ldCodes2, ldMaxCodes, ldMaxCodes1, ldMaxCodes2, eoBlocks[0]);
    genBitLenFreq<MAX_FREQ_DWIDTH>(ldCodes1, eoBlocks[1], ldFrequency, ldMaxCodes1);
    processBitLength<MAX_FREQ_DWIDTH>(ldFrequency, eoBlocks[2], blCodes, blMaxCodes);
    sendProcTrees(ldMaxCodes2, blMaxCodes, ldCodes2, blCodes, eoBlocks[3], intlHufCodeStream);
    sendHuffData<SEND_EOS>(intlHufCodeStream, hufCodeStream);
}

} // end namespace details

template <int MAX_FREQ_DWIDTH, int MAX_BITS>
void zstdTreegenStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                       hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                       hls::stream<IntVectorStream_dt<4, 1> >& outWeightStream,
                       hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
#pragma HLS DATAFLOW
    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapStream("heapStream");
    hls::stream<ap_uint<9> > heapLenStream("heapLenStream");
    hls::stream<DSVectorStream_dt<Codeword, 1> > hufCodeStream("hufCodeStream");
    hls::stream<bool> eobStream("eobStream");
    hls::stream<bool> eoBlocks[2];

    details::zstdFreqFilterSort<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(inFreqStream, heapStream,
                                                                                          heapLenStream, eobStream);
    details::streamDistributor<2>(eobStream, eoBlocks);
    details::zstdGetHuffmanCodes<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH, MAX_BITS>(
        heapStream, heapLenStream, eoBlocks[0], hufCodeStream);
    details::huffCodeWeightDistributor<details::c_maxLitV + 1, MAX_FREQ_DWIDTH, MAX_BITS, 4>(
        hufCodeStream, eoBlocks[1], outCodeStream, outWeightStream, weightFreqStream);
}

} // End of compression
} // End of xf

#endif // _XFCOMPRESSION_HUFFMAN_TREEGEN_HPP_
