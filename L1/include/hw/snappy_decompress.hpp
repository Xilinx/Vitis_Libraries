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
#ifndef _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_
#define _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_

/**
 * @file snappy_decompress.hpp
 * @brief Header for modules used for snappy decompression kernle
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef ap_uint<8> uintV_t;
typedef ap_uint<32> encoded_dt;

namespace xf {
namespace compression {

/**
 * @brief This module decodes the compressed data based on the snappy decompression format
 *
 * @tparam READ_STATE current state
 * @tparam MATCH_STATE match the characters
 * @tparam LOW_OFFSET_STATE matching the lowest distance characters
 * @tparam READ_TOKEN read from stream
 * @tparam READ_LITERAL read literals
 * @tparam READ_LITLEN_60 standard specific literal length
 * @tparam READ_LITLEN_61 standard specific literal length
 * @tparam READ_LITLEN_60_CONT standard specific literal length
 * @tparam READ_OFFSET read offset
 * @tparam READ_OFFSET_C01 standard specific offset support
 * @tparam READ_OFFSET_C10 standard specific offset support
 * @tparam READ_LITLEN_61_CONT continue standard specific literal length
 * @tparam READ_OFFSET_C10_CONT continue standard specific offset
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param input_size input data size
 */
static void snappyDecompress(hls::stream<uintV_t>& inStream, hls::stream<encoded_dt>& outStream, uint32_t input_size) {
    // Snappy Decoder states
    enum SnappyDecompressionStates {
        READ_STATE,
        MATCH_STATE,
        LOW_OFFSET_STATE,
        READ_TOKEN,
        READ_LITERAL,
        READ_LITLEN_60,
        READ_LITLEN_61,
        READ_OFFSET,
        READ_OFFSET_C01,
        READ_OFFSET_C10,
        READ_LITLEN_61_CONT,
        READ_OFFSET_C10_CONT
    };

    if (input_size == 0) return;

    enum SnappyDecompressionStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    encoded_dt decodedOut = 0;
    ap_uint<32> lit_len = 0;
    uint32_t match_len = 0;
    ap_uint<8> inValue = 0;
    bool read_instream = true;

    uint32_t inCntr_idx = 0;
    ap_uint<32> inBlkSize = 0;

    inValue = inStream.read();
    inCntr_idx++;

    if ((inValue >> 7) == 1) {
        inBlkSize.range(6, 0) = inValue.range(6, 0);
        inValue = inStream.read();
        inCntr_idx++;
        inBlkSize.range(13, 7) = inValue.range(6, 0);
        if ((inValue >> 7) == 1) {
            inValue = inStream.read();
            inCntr_idx++;
            inBlkSize.range(20, 14) = inValue.range(6, 0);
        }

    } else
        inBlkSize = inValue;

snappy_decompress:
    for (; inCntr_idx < input_size; inCntr_idx++) {
#pragma HLS PIPELINE II = 1
        if (read_instream)
            inValue = inStream.read();
        else
            inCntr_idx--;

        read_instream = true;
        if (next_state == READ_TOKEN) {
            if (inValue.range(1, 0) != 0) {
                next_state = READ_OFFSET;
                read_instream = false;
            } else {
                lit_len = inValue.range(7, 2);

                if (lit_len < 60) {
                    lit_len++;
                    next_state = READ_LITERAL;
                } else if (lit_len == 60) {
                    next_state = READ_LITLEN_60;
                } else if (lit_len == 61) {
                    next_state = READ_LITLEN_61;
                }
            }
        } else if (next_state == READ_LITERAL) {
            encoded_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) next_state = READ_TOKEN;

        } else if (next_state == READ_OFFSET) {
            offset = 0;
            if (inValue.range(1, 0) == 1) {
                match_len = inValue.range(4, 2);
                offset.range(10, 8) = inValue.range(7, 5);
                next_state = READ_OFFSET_C01;
            } else if (inValue.range(1, 0) == 2) {
                match_len = inValue.range(7, 2);
                next_state = READ_OFFSET_C10;
            } else {
                next_state = READ_TOKEN;
                read_instream = false;
            }
        } else if (next_state == READ_OFFSET_C01) {
            offset.range(7, 0) = inValue;
            encoded_dt outValue = 0;
            outValue.range(31, 16) = match_len + 3;
            outValue.range(15, 0) = offset - 1;
            outStream << outValue;
            next_state = READ_TOKEN;
        } else if (next_state == READ_OFFSET_C10) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET_C10_CONT;
        } else if (next_state == READ_OFFSET_C10_CONT) {
            offset.range(15, 8) = inValue;
            encoded_dt outValue = 0;

            outValue.range(31, 16) = match_len;
            outValue.range(15, 0) = offset - 1;
            outStream << outValue;
            next_state = READ_TOKEN;

        } else if (next_state == READ_LITLEN_60) {
            lit_len = inValue + 1;
            next_state = READ_LITERAL;
        } else if (next_state == READ_LITLEN_61) {
            lit_len.range(7, 0) = inValue;
            next_state = READ_LITLEN_61_CONT;
        } else if (next_state == READ_LITLEN_61_CONT) {
            lit_len.range(15, 8) = inValue;
            lit_len = lit_len + 1;
            next_state = READ_LITERAL;
        }
    } // End of main snappy_decoder for-loop
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_
