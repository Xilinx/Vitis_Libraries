/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_ZSTD_SPECS_HPP_
#define _XFCOMPRESSION_ZSTD_SPECS_HPP_

/**
 * @file zstd_specs.hpp
 * @brief Custom data types, constants and stored tables for use in ZSTD decompress kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
namespace xf {
namespace compression {
namespace details {

typedef struct {
    uint8_t symbol;     // code to get length/offset basevalue
    uint8_t bitCount;   // bits to be read to generate next state
    uint16_t nextState; // basevalue for next state
} FseBSState;

typedef struct {
    ap_uint<4> bitlen; // code bit-length
    ap_uint<8> symbol; // decoded symbol
} HuffmanTable;

enum xfBlockType_t { RAW_BLOCK = 0, RLE_BLOCK, CMP_BLOCK, INVALID_BLOCK };
enum xfLitBlockType_t { RAW_LBLOCK = 0, RLE_LBLOCK, CMP_LBLOCK, TREELESS_LBLOCK };
enum xfSymbolCompMode_t { PREDEFINED_MODE = 0, RLE_MODE, FSE_COMPRESSED_MODE, REPEAT_MODE };

const uint32_t kMagicNumber = 0xFD2FB528;
const uint32_t kSkipFrameMagicNumber = 0x184D2A50; // till +15 values
const uint32_t kSkippableFrameMask = 0xFFFFFFF0;

const uint16_t kMaxCharLit = 35;
const uint16_t kMaxCharDefOffset = 28;
const uint16_t kMaxCharOffset = 31;
const uint16_t kMaxCharMatchlen = 52;
const uint16_t kMaxCharHuffman = 255;

const uint32_t kLLBase[kMaxCharLit + 1] = {0,  1,  2,   3,   4,   5,    6,    7,    8,    9,     10,    11,
                                           12, 13, 14,  15,  16,  18,   20,   22,   24,   28,    32,    40,
                                           48, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
const uint32_t kMLBase[kMaxCharMatchlen + 1] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  12,  13,   14,   15,   16,   17,    18,    19,   20,
    21, 22, 23, 24, 25, 26, 27, 28,  29,  30,  31,   32,   33,   34,   35,    37,    39,   41,
    43, 47, 51, 59, 67, 83, 99, 131, 259, 515, 1027, 2051, 4099, 8195, 16387, 32771, 65539};

const uint8_t kLLExtraBits[kMaxCharLit + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  1,
                                               1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const uint8_t kMLExtraBits[kMaxCharMatchlen + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0,
                                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  1,  1, 1,
                                                    2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// [litlen, offset, matlen]
const int16_t defaultDistribution[kMaxCharLit + kMaxCharDefOffset + kMaxCharMatchlen + 3] = { // litlen
    4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1, -1, -1, -1, -1,
    // offsets
    1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
    // matlen
    1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1};

} // details
} // compression
} // xf
#endif
