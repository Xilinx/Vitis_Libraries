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
#ifndef _XFCOMPRESSION_LZ_COMPRESS_HPP_
#define _XFCOMPRESSION_LZ_COMPRESS_HPP_

/**
 * @file lz_compress.hpp
 * @brief Header for modules used in LZ4 and snappy compression kernels.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace xf {
namespace compression {

/**
 * @brief This module reads input literals from stream and updates
 * match length and offset of each literal.
 *
 * @tparam MATCH_LEN match length
 * @tparam MIN_MATCH minimum match
 * @tparam LZ_MAX_OFFSET_LIMIT maximum offset limit
 * @tparam MATCH_LEVEL match level
 * @tparam MIN_OFFSET minimum offset
 * @tparam LZ_DICT_SIZE dictionary size
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param input_size input size
 */
template <int MATCH_LEN,
          int MIN_MATCH,
          int LZ_MAX_OFFSET_LIMIT,
          int MATCH_LEVEL = 6,
          int MIN_OFFSET = 1,
          int LZ_DICT_SIZE = 1 << 12,
          int LEFT_BYTES = 64>
void lzCompress(hls::stream<ap_uint<8> >& inStream, hls::stream<ap_uint<32> >& outStream, uint32_t input_size) {
    const int c_dictEleWidth = (MATCH_LEN * 8 + 24);
    typedef ap_uint<MATCH_LEVEL * c_dictEleWidth> uintDictV_t;
    typedef ap_uint<c_dictEleWidth> uintDict_t;

    if (input_size == 0) return;
    // Dictionary
    uintDictV_t dict[LZ_DICT_SIZE];
#pragma HLS BIND_STORAGE variable = dict type = RAM_T2P impl = URAM
    uintDictV_t resetValue = 0;
    for (int i = 0; i < MATCH_LEVEL; i++) {
#pragma HLS UNROLL
        resetValue.range((i + 1) * c_dictEleWidth - 1, i * c_dictEleWidth + MATCH_LEN * 8) = -1;
    }
// Initialization of Dictionary
dict_flush:
    for (int i = 0; i < LZ_DICT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
        dict[i] = resetValue;
    }

    uint8_t present_window[MATCH_LEN];
#pragma HLS ARRAY_PARTITION variable = present_window complete
    for (uint8_t i = 1; i < MATCH_LEN; i++) {
        present_window[i] = inStream.read();
    }
lz_compress:
    for (uint32_t i = MATCH_LEN - 1; i < input_size - LEFT_BYTES; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = dict inter false
        uint32_t currIdx = i - MATCH_LEN + 1;
        // shift present window and load next value
        for (int m = 0; m < MATCH_LEN - 1; m++) {
#pragma HLS UNROLL
            present_window[m] = present_window[m + 1];
        }
        present_window[MATCH_LEN - 1] = inStream.read();

        // Calculate Hash Value
        uint32_t hash =
            (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 3) ^ (present_window[3]);

        // Dictionary Lookup
        uintDictV_t dictReadValue = dict[hash];
        uintDictV_t dictWriteValue = dictReadValue << c_dictEleWidth;
        for (int m = 0; m < MATCH_LEN; m++) {
#pragma HLS UNROLL
            dictWriteValue.range((m + 1) * 8 - 1, m * 8) = present_window[m];
        }
        dictWriteValue.range(c_dictEleWidth - 1, MATCH_LEN * 8) = currIdx;
        // Dictionary Update
        dict[hash] = dictWriteValue;

        // Match search and Filtering
        // Comp dict pick
        uint8_t match_length = 0;
        uint32_t match_offset = 0;
        for (int l = 0; l < MATCH_LEVEL; l++) {
            uint8_t len = 0;
            bool done = 0;
            uintDict_t compareWith = dictReadValue.range((l + 1) * c_dictEleWidth - 1, l * c_dictEleWidth);
            uint32_t compareIdx = compareWith.range(c_dictEleWidth - 1, MATCH_LEN * 8);
            for (int m = 0; m < MATCH_LEN; m++) {
                if (present_window[m] == compareWith.range((m + 1) * 8 - 1, m * 8) && !done) {
                    len++;
                } else {
                    done = 1;
                }
            }
            if ((len >= MIN_MATCH) && (currIdx > compareIdx) && ((currIdx - compareIdx) < LZ_MAX_OFFSET_LIMIT) &&
                ((currIdx - compareIdx - 1) >= MIN_OFFSET)) {
                len = len;
            } else {
                len = 0;
            }
            if (len > match_length) {
                match_length = len;
                match_offset = currIdx - compareIdx - 1;
            }
        }
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = present_window[0];
        outValue.range(15, 8) = match_length;
        outValue.range(31, 16) = match_offset;
        outStream << outValue;
    }
lz_compress_leftover:
    for (int m = 1; m < MATCH_LEN; m++) {
#pragma HLS PIPELINE
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = present_window[m];
        outStream << outValue;
    }
lz_left_bytes:
    for (int l = 0; l < LEFT_BYTES; l++) {
#pragma HLS PIPELINE
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = inStream.read();
        outStream << outValue;
    }
}
/**
 * @brief This module reads input literals from stream and updates
 * match length and offset of each literal.
 *
 * @tparam MAX_INPUT_SIZE input size
 * @tparam MATCH_LEN match length
 * @tparam MIN_MATCH minimum match
 * @tparam LZ_MAX_OFFSET_LIMIT maximum offset limit
 * @tparam MATCH_LEVEL match level
 * @tparam MIN_OFFSET minimum offset
 * @tparam LZ_DICT_SIZE dictionary size
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param inSize input size stream
 * @param outSize output size stream
 */
template <int MAX_INPUT_SIZE = 64 * 1024,
          class SIZE_DT = uint32_t,
          int MATCH_LEN,
          int MIN_MATCH,
          int LZ_MAX_OFFSET_LIMIT,
          int MATCH_LEVEL = 6,
          int MIN_OFFSET = 1,
          int LZ_DICT_SIZE = 1 << 12,
          int LEFT_BYTES = 64>
void lzCompress(hls::stream<ap_uint<8> >& inStream,
                hls::stream<ap_uint<32> >& outStream,
                hls::stream<SIZE_DT>& inSize) {
    const uint32_t c_indxBitCnts = 24;
    const int c_dictEleWidth = (MATCH_LEN * 8 + c_indxBitCnts);
    typedef ap_uint<MATCH_LEVEL * c_dictEleWidth> uintDictV_t;
    typedef ap_uint<c_dictEleWidth> uintDict_t;
    const uint32_t totalDictSize = (1 << c_indxBitCnts); // 16MB based on index 3 bytes
    uint32_t relativeInSize = 0;
    bool resetDictFlag = true;
    uint32_t relativeNumBlocks = 0;

    uintDictV_t dict[LZ_DICT_SIZE];
#pragma HLS RESOURCE variable = dict core = XPM_MEMORY uram

    // loop over blocks
    for (uint32_t input_size = inSize.read(); input_size != 0; input_size = inSize.read()) { // Dictionary
        assert(input_size <= MAX_INPUT_SIZE);

        // once 16MB data is processed reset dictionary
        // 16MB based on index 3 bytes
        if (resetDictFlag) {
            ap_uint<MATCH_LEVEL* c_dictEleWidth> resetValue = 0;
            for (int i = 0; i < MATCH_LEVEL; i++) {
#pragma HLS UNROLL
                resetValue.range((i + 1) * c_dictEleWidth - 1, i * c_dictEleWidth + MATCH_LEN * 8) = -1;
            }
        // Initialization of Dictionary
        dict_flush:
            for (int i = 0; i < LZ_DICT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
                dict[i] = resetValue;
            }
            resetDictFlag = false;
            relativeInSize = 0;
            relativeNumBlocks = 0;
        } else {
            relativeNumBlocks++;
        }
        relativeInSize += input_size;
        uint8_t present_window[MATCH_LEN];
#pragma HLS ARRAY_PARTITION variable = present_window complete
        for (uint8_t i = 1; i < MATCH_LEN; i++) {
            present_window[i] = inStream.read();
        }
    lz_compress:
        for (uint32_t i = MATCH_LEN - 1; i < input_size - LEFT_BYTES; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = dict inter false
            uint32_t currIdx = (i + (relativeNumBlocks * MAX_INPUT_SIZE)) - MATCH_LEN + 1;
            // shift present window and load next value
            for (int m = 0; m < MATCH_LEN - 1; m++) {
#pragma HLS UNROLL
                present_window[m] = present_window[m + 1];
            }
            present_window[MATCH_LEN - 1] = inStream.read();

            // Calculate Hash Value
            uint32_t hash =
                (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 3) ^ (present_window[3]);

            // Dictionary Lookup
            uintDictV_t dictReadValue = dict[hash];
            uintDictV_t dictWriteValue = dictReadValue << c_dictEleWidth;
            for (int m = 0; m < MATCH_LEN; m++) {
#pragma HLS UNROLL
                dictWriteValue.range((m + 1) * 8 - 1, m * 8) = present_window[m];
            }
            dictWriteValue.range(c_dictEleWidth - 1, MATCH_LEN * 8) = currIdx;
            // Dictionary Update
            dict[hash] = dictWriteValue;

            // Match search and Filtering
            // Comp dict pick
            uint8_t match_length = 0;
            uint32_t match_offset = 0;
            for (int l = 0; l < MATCH_LEVEL; l++) {
                uint8_t len = 0;
                bool done = 0;
                uintDict_t compareWith = dictReadValue.range((l + 1) * c_dictEleWidth - 1, l * c_dictEleWidth);
                uint32_t compareIdx = compareWith.range(c_dictEleWidth - 1, MATCH_LEN * 8);
                for (int m = 0; m < MATCH_LEN; m++) {
                    if (present_window[m] == compareWith.range((m + 1) * 8 - 1, m * 8) && !done) {
                        len++;
                    } else {
                        done = 1;
                    }
                }
                if ((len >= MIN_MATCH) && (currIdx > compareIdx) && ((currIdx - compareIdx) < LZ_MAX_OFFSET_LIMIT) &&
                    ((currIdx - compareIdx - 1) >= MIN_OFFSET) &&
                    (compareIdx >= (relativeNumBlocks * MAX_INPUT_SIZE))) {
                    len = len;
                } else {
                    len = 0;
                }
                if (len > match_length) {
                    match_length = len;
                    match_offset = currIdx - compareIdx - 1;
                }
            }
            ap_uint<32> outValue = 0;
            outValue.range(7, 0) = present_window[0];
            outValue.range(15, 8) = match_length;
            outValue.range(31, 16) = match_offset;
            outStream << outValue;
        }
    lz_compress_leftover:
        for (int m = 1; m < MATCH_LEN; m++) {
#pragma HLS PIPELINE
            ap_uint<32> outValue = 0;
            outValue.range(7, 0) = present_window[m];
            outStream << outValue;
        }
    lz_left_bytes:
        for (int l = 0; l < LEFT_BYTES; l++) {
#pragma HLS PIPELINE
            ap_uint<32> outValue = 0;
            outValue.range(7, 0) = inStream.read();
            outStream << outValue;
        }

        // once relativeInSize becomes 16MB set the flag to true
        resetDictFlag = (relativeInSize >= (totalDictSize)) ? true : false;
    }
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ_COMPRESS_HPP_
