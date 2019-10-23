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
#ifndef _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
#define _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_

/**
 * @file lz4_decompress.hpp
 * @brief Header for modules used in LZ4 decompression kernel.
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

typedef ap_uint<32> compressd_dt;

/**
 * @brief This module reads the compressed data from input stream
 * and decodes the offset, match length and literals by processing
 * in various decompress states.
 *
 *
 * @param inStream Input stream 8bit
 * @param outStream Output stream 32bit
 * @param input_size Input size
 */
inline void lz4Decompress(hls::stream<ap_uint<8> >& inStream,
                          hls::stream<compressd_dt>& outStream,
                          uint32_t input_size) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    compressd_dt decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            compressd_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

inline void lz4DecompressSimple(hls::stream<ap_uint<8> >& inStream,
                                hls::stream<compressd_dt>& outStream,
                                uint32_t input_size,
                                bool uncomp_flag) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    compressd_dt decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
    if (uncomp_flag == 1) {
        next_state = READ_LITERAL;
        lit_len = input_size;
    }
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            compressd_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
