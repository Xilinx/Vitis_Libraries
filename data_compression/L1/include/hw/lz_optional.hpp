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
#ifndef _XFCOMPRESSION_LZ_OPTIONAL_HPP_
#define _XFCOMPRESSION_LZ_OPTIONAL_HPP_

/**
 * @file lz_optional.hpp
 * @brief Header for modules used in LZ4 and snappy compression kernels.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// Dynamic Huffman Frequency counts
// Based on GZIP/ZLIB spec
#include "zlib_tables.hpp"

#define d_code(dist, dist_code) ((dist) < 256 ? dist_code[dist] : dist_code[256 + ((dist) >> 7)])

namespace xf {
namespace compression {

typedef ap_uint<32> compressd_dt;

static void lz77Divide(hls::stream<compressd_dt>& inStream,
                       hls::stream<ap_uint<32> >& outStream,
                       hls::stream<bool>& endOfStream,
                       hls::stream<uint32_t>& outStreamTree,
                       hls::stream<uint32_t>& compressedSize,
                       uint32_t input_size) {
    if (input_size == 0) {
        compressedSize << 0;
        outStream << 0;
        endOfStream << 1;
        outStreamTree << 9999;
        return;
    }

    const uint16_t c_ltree_size = 1024;
    const uint16_t c_dtree_size = 64;

    uint32_t lcl_dyn_ltree[c_ltree_size];
    uint32_t lcl_dyn_dtree[c_dtree_size];

ltree_init:
    for (uint32_t i = 0; i < c_ltree_size; i++) lcl_dyn_ltree[i] = 0;

dtree_init:
    for (uint32_t i = 0; i < c_dtree_size; i++) lcl_dyn_dtree[i] = 0;

    int length = 0;
    int code = 0;
    int n = 0;

    uint32_t out_cntr = 0;
    uint8_t match_len = 0;
    uint32_t loc_idx = 0;

    compressd_dt nextEncodedValue = inStream.read();
    uint32_t cSizeCntr = 0;
lz77_divide:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = lcl_dyn_ltree inter false
#pragma HLS dependence variable = lcl_dyn_dtree inter false
        compressd_dt tmpEncodedValue = nextEncodedValue;
        if (i < (input_size - 1)) nextEncodedValue = inStream.read();

        uint8_t tCh = tmpEncodedValue.range(7, 0);
        uint8_t tLen = tmpEncodedValue.range(15, 8);
        uint16_t tOffset = tmpEncodedValue.range(31, 16);
        uint32_t ltreeIdx, dtreeIdx;
        if (tLen > 0) {
            ltreeIdx = length_code[tLen - 3] + 256 + 1;
            dtreeIdx = d_code(tOffset, dist_code);

            i += (tLen - 1);
            tmpEncodedValue.range(15, 8) = tLen - 3;
        } else {
            ltreeIdx = tCh;
            dtreeIdx = 63;
        }
        lcl_dyn_ltree[ltreeIdx]++;
        lcl_dyn_dtree[dtreeIdx]++;

        outStream << tmpEncodedValue;
        endOfStream << 0;
        cSizeCntr++;
    }

    compressedSize << (cSizeCntr * 4);
    outStream << 0;
    endOfStream << 1;

    for (uint32_t i = 0; i < c_ltree_size; i++) outStreamTree << lcl_dyn_ltree[i];

    for (uint32_t j = 0; j < c_dtree_size; j++) outStreamTree << lcl_dyn_dtree[j];
}

/**
 * @brief Objective of this module is to pick character with
 * higher match length in the offset window range.
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param input_size input stream size
 */
template <int MATCH_LEN, int OFFSET_WINDOW>
void lzBestMatchFilter(hls::stream<compressd_dt>& inStream, hls::stream<compressd_dt>& outStream, uint32_t input_size) {
    const int c_max_match_length = MATCH_LEN;
    if (input_size == 0) return;

    compressd_dt compare_window[MATCH_LEN];
#pragma HLS array_partition variable = compare_window

    // Initializing shift registers
    for (uint32_t i = 0; i < c_max_match_length; i++) {
#pragma HLS UNROLL
        compare_window[i] = inStream.read();
    }

lz_bestMatchFilter:
    for (uint32_t i = c_max_match_length; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        // shift register logic
        compressd_dt outValue = compare_window[0];
        for (uint32_t j = 0; j < c_max_match_length - 1; j++) {
#pragma HLS UNROLL
            compare_window[j] = compare_window[j + 1];
        }
        compare_window[c_max_match_length - 1] = inStream.read();

        uint8_t match_length = outValue.range(15, 8);
        bool best_match = 1;
        // Find Best match
        for (uint32_t j = 0; j < c_max_match_length; j++) {
            compressd_dt compareValue = compare_window[j];
            uint8_t compareLen = compareValue.range(15, 8);
            if (match_length + j < compareLen) {
                best_match = 0;
            }
        }
        if (best_match == 0) {
            outValue.range(15, 8) = 0;
            outValue.range(31, 16) = 0;
        }
        outStream << outValue;
    }

lz_bestMatchFilter_left_over:
    for (uint32_t i = 0; i < c_max_match_length; i++) {
        outStream << compare_window[i];
    }
}

/**
 * @brief This module helps in improving the compression ratio.
 * Finds a better match length by performing more character matches
 * with supported max match, while maintaining an offset window.
 * Booster offset Window template argument (default value is 16K)
 * internally consume BRAM memory to implement history window.
 * Higher the booster value can give better compression ratio but
 * will consume more BRAM resources.
 *
 * @tparam MAX_MATCH_LEN maximum length allowed for character match
 * @tparam BOOSTER_OFFSET_WINDOW offset window to store/match the character
 *
 * @param inStream input stream 32bit per read
 * @param outStream output stream 32bit per write
 * @param input_size input size
 * @param left_bytes last 64 left over bytes
 *
*/
template <int MAX_MATCH_LEN, int BOOSTER_OFFSET_WINDOW = 16 * 1024, int LEFT_BYTES = 64>
void lzBooster(hls::stream<compressd_dt>& inStream, hls::stream<compressd_dt>& outStream, uint32_t input_size) {
    if (input_size == 0) return;
    uint8_t local_mem[BOOSTER_OFFSET_WINDOW];
    uint32_t match_loc = 0;
    uint32_t match_len = 0;
    compressd_dt outValue;
    compressd_dt outStreamValue;
    bool matchFlag = false;
    bool outFlag = false;
    bool boostFlag = false;
    uint16_t skip_len = 0;
lz_booster:
    for (uint32_t i = 0; i < (input_size - LEFT_BYTES); i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = local_mem inter false
        compressd_dt inValue = inStream.read();
        uint8_t tCh = inValue.range(7, 0);
        uint8_t tLen = inValue.range(15, 8);
        uint16_t tOffset = inValue.range(31, 16);
        if (tOffset < BOOSTER_OFFSET_WINDOW) {
            boostFlag = true;
        } else {
            boostFlag = false;
        }
        uint8_t match_ch = local_mem[match_loc % BOOSTER_OFFSET_WINDOW];
        local_mem[i % BOOSTER_OFFSET_WINDOW] = tCh;
        outFlag = false;

        if (skip_len) {
            skip_len--;
        } else if (matchFlag && (match_len < MAX_MATCH_LEN) && (tCh == match_ch)) {
            match_len++;
            match_loc++;
            outValue.range(15, 8) = match_len;
        } else {
            match_len = 1;
            match_loc = i - tOffset;
            if (i) outFlag = true;
            outStreamValue = outValue;
            outValue = inValue;
            if (tLen) {
                if (boostFlag) {
                    matchFlag = true;
                    skip_len = 0;
                } else {
                    matchFlag = false;
                    skip_len = tLen - 1;
                }
            } else {
                matchFlag = false;
            }
        }
        if (outFlag) outStream << outStreamValue;
    }
    outStream << outValue;
lz_booster_left_bytes:
    for (uint32_t i = 0; i < LEFT_BYTES; i++) {
        outStream << inStream.read();
    }
}

/**
 * @brief This module checks if match length exists, and if
 * match length exists it filters the match length -1 characters
 * writing to output stream.
 *
 * @tparam MATCH_LEN length of matched segment
 * @tparam OFFSET_WINDOW output window
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param input_size input stream size
 * @param left_bytes bytes left in block
 */
template <int LEFT_BYTES = 64>
static void lzFilter(hls::stream<compressd_dt>& inStream, hls::stream<compressd_dt>& outStream, uint32_t input_size) {
    if (input_size == 0) return;
    uint32_t skip_len = 0;
lz_filter:
    for (uint32_t i = 0; i < input_size - LEFT_BYTES; i++) {
#pragma HLS PIPELINE II = 1
        compressd_dt inValue = inStream.read();
        uint8_t tLen = inValue.range(15, 8);
        if (skip_len) {
            skip_len--;
        } else {
            outStream << inValue;
            if (tLen) skip_len = tLen - 1;
        }
    }
lz_filter_left_bytes:
    for (uint32_t i = 0; i < LEFT_BYTES; i++) {
        outStream << inStream.read();
    }
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ_OPTIONAL_HPP_
