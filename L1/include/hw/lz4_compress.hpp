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
#ifndef _XFCOMPRESSION_LZ4_COMPRESS_HPP_
#define _XFCOMPRESSION_LZ4_COMPRESS_HPP_

/**
 * @file lz4_compress.hpp
 * @brief Header for modules used in LZ4 compression kernel.
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

typedef ap_uint<64> lz4_compressd_dt;
typedef ap_uint<32> compressd_dt;

/**
 * @brief This module does the lz4 encoding by starting from WRITE_TOKEN state
 * and based on the input read from stream the processing state changes.
 *
 * @param in_lit_inStream Read Literals
 * @param in_lenOffset_Stream Read Offset-length
 * @param outStream Output data stream
 * @param endOfStream Stream indicating that all data is processed or not
 * @param compressdSizeStream Gives the compressed size for each 64K block
 * @param input_size Size of input
 */
static void lz4Compress(hls::stream<uint8_t>& in_lit_inStream,
                        hls::stream<lz4_compressd_dt>& in_lenOffset_Stream,
                        hls::stream<ap_uint<8> >& outStream,
                        hls::stream<bool>& endOfStream,
                        hls::stream<uint32_t>& compressdSizeStream,
                        uint32_t input_size) {
    // LZ4 Compress STATES
    enum lz4CompressStates { WRITE_TOKEN, WRITE_LIT_LEN, WRITE_MATCH_LEN, WRITE_LITERAL, WRITE_OFFSET0, WRITE_OFFSET1 };
    uint32_t lit_len = 0;
    uint16_t outCntr = 0;
    uint32_t compressedSize = 0;
    enum lz4CompressStates next_state = WRITE_TOKEN;
    uint16_t lit_length = 0;
    uint16_t match_length = 0;
    uint16_t write_lit_length = 0;
    ap_uint<16> match_offset = 0;
    bool lit_ending = false;
    bool extra_match_len = false;

lz4_compress:
    for (uint32_t inIdx = 0; (inIdx < input_size) || (next_state != WRITE_TOKEN);) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> outValue;
        if (next_state == WRITE_TOKEN) {
            lz4_compressd_dt tmpValue = in_lenOffset_Stream.read();
            lit_length = tmpValue.range(63, 32);
            match_length = tmpValue.range(15, 0);
            match_offset = tmpValue.range(31, 16);
            inIdx += match_length + lit_length + 4;

            if (match_length == 777 && match_offset == 777) {
                inIdx = input_size;
                lit_ending = true;
            }

            lit_len = lit_length;
            write_lit_length = lit_length;
            if (match_offset == 0 && match_length == 0) {
                lit_ending = true;
            }
            if (lit_length >= 15) {
                outValue.range(7, 4) = 15;
                lit_length -= 15;
                next_state = WRITE_LIT_LEN;
            } else if (lit_length) {
                outValue.range(7, 4) = lit_length;
                lit_length = 0;
                next_state = WRITE_LITERAL;
            } else {
                outValue.range(7, 4) = 0;
                next_state = WRITE_OFFSET0;
            }
            if (match_length >= 15) {
                outValue.range(3, 0) = 15;
                match_length -= 15;
                extra_match_len = true;
            } else {
                outValue.range(3, 0) = match_length;
                match_length = 0;
                extra_match_len = false;
            }
        } else if (next_state == WRITE_LIT_LEN) {
            if (lit_length >= 255) {
                outValue = 255;
                lit_length -= 255;
            } else {
                outValue = lit_length;
                next_state = WRITE_LITERAL;
            }
        } else if (next_state == WRITE_LITERAL) {
            outValue = in_lit_inStream.read();
            write_lit_length--;
            if (write_lit_length == 0) {
                if (lit_ending) {
                    next_state = WRITE_TOKEN;
                } else {
                    next_state = WRITE_OFFSET0;
                }
            }
        } else if (next_state == WRITE_OFFSET0) {
            match_offset++; // LZ4 standard
            outValue = match_offset.range(7, 0);
            next_state = WRITE_OFFSET1;
        } else if (next_state == WRITE_OFFSET1) {
            outValue = match_offset.range(15, 8);
            if (extra_match_len) {
                next_state = WRITE_MATCH_LEN;
            } else {
                next_state = WRITE_TOKEN;
            }
        } else if (next_state == WRITE_MATCH_LEN) {
            if (match_length >= 255) {
                outValue = 255;
                match_length -= 255;
            } else {
                outValue = match_length;
                next_state = WRITE_TOKEN;
            }
        }
        if (compressedSize < input_size) {
            // Limiting compression size not more than input size.
            // Host code should ignore such blocks
            outStream << outValue;
            endOfStream << 0;
            compressedSize++;
        }
    }

    compressdSizeStream << compressedSize;
    outStream << 0;
    endOfStream << 1;
}

/**
 * @brief This is an intermediate module that seperates the input stream
 * into two output streams, one literal stream and the other matchlen and
 * offset stream.
 *
 * @tparam MAX_LIT_COUNT maximum literal count
 * @tparam PARALLEL_UNITS determined based on number of parallel engines
 *
 * @param inStream reference of input literals stream
 * @param lit_outStream Offset-length stream for literals in input stream
 * @param lenOffset_Stream output data stream
 * @param input_size end flag for stream
 * @param max_lit_limit Size for compressed stream
 * @param index size of input
 */
template <int MAX_LIT_COUNT, int PARALLEL_UNITS>
static void lz4Divide(hls::stream<compressd_dt>& inStream,
                      hls::stream<uint8_t>& lit_outStream,
                      hls::stream<lz4_compressd_dt>& lenOffset_Stream,
                      uint32_t input_size,
                      uint32_t max_lit_limit[PARALLEL_UNITS],
                      uint32_t index) {
    if (input_size == 0) return;

    uint8_t match_len = 0;
    uint32_t lit_count = 0;
    uint32_t lit_count_flag = 0;

    compressd_dt nextEncodedValue = inStream.read();
lz4_divide:
    for (uint32_t i = 0; i < input_size;) {
#pragma HLS PIPELINE II = 1
        compressd_dt tmpEncodedValue = nextEncodedValue;
        if (i < (input_size - 1)) nextEncodedValue = inStream.read();
        uint8_t tCh = tmpEncodedValue.range(7, 0);
        uint8_t tLen = tmpEncodedValue.range(15, 8);
        uint16_t tOffset = tmpEncodedValue.range(31, 16);
        uint32_t match_offset = tOffset;

        if (lit_count >= MAX_LIT_COUNT) {
            lit_count_flag = 1;
        } else if (tLen) {
            uint8_t match_len = tLen - 4; // LZ4 standard
            lz4_compressd_dt tmpValue;
            tmpValue.range(63, 32) = lit_count;
            tmpValue.range(15, 0) = match_len;
            tmpValue.range(31, 16) = match_offset;
            lenOffset_Stream << tmpValue;
            match_len = tLen - 1;
            lit_count = 0;
        } else {
            lit_outStream << tCh;
            lit_count++;
        }
        if (tLen)
            i += tLen;
        else
            i += 1;
    }
    if (lit_count) {
        lz4_compressd_dt tmpValue;
        tmpValue.range(63, 32) = lit_count;
        if (lit_count == MAX_LIT_COUNT) {
            lit_count_flag = 1;
            tmpValue.range(15, 0) = 777;
            tmpValue.range(31, 16) = 777;
        } else {
            tmpValue.range(15, 0) = 0;
            tmpValue.range(31, 16) = 0;
        }
        lenOffset_Stream << tmpValue;
    }
    max_lit_limit[index] = lit_count_flag;
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ4_COMPRESS_HPP_
