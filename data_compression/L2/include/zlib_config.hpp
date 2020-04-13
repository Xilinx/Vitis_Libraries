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
#ifndef _XFCOMPRESSION_ZLIB_CONFIG_HPP_
#define _XFCOMPRESSION_ZLIB_CONFIG_HPP_

/**
 * @file zlib_config.h
 * @brief Header for configuration for zlib compression and decompression kernels.
 *
 * This file is part of Vitis Data Compression Library.
 */

const int gz_max_literal_count = 4096;

// Dynamic Huffman Related Content
#define MAX_BITS 15
// Literals
#define LITERALS 256

// Length codes
#define LENGTH_CODES 29

// Literal Codes
#define LITERAL_CODES (LITERALS + 1 + LENGTH_CODES)

// Distance Codes
#define DISTANCE_CODES 30

// bit length codes
#define BL_CODES 19

// Literal Tree size - 573
#define HEAP_SIZE (2 * LITERAL_CODES + 1)

// Bit length codes must not exceed MAX_BL_BITS bits
#define MAX_BL_BITS 7

#define REUSE_PREV_BLEN 16

#define REUSE_ZERO_BLEN 17

#define REUSE_ZERO_BLEN_7 18

#endif // _XFCOMPRESSION_ZLIB_CONFIG_HPP_
