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
#ifndef _XFCOMPRESSION_ZLIB_CONFIG_H_
#define _XFCOMPRESSION_ZLIB_CONFIG_H_

/**
 * @file zlib_config.h
 * @brief Header for configuration for zlib compression and decompression kernels.
 *
 * This file is part of XF Compression Library.
 */

//#define MAX_LIT_COUNT 4096

const int gz_max_literal_count = 4096;

// Dynamic Huffman Related Content

// Literals
#define LITERALS 256

// Length codes
#define LENGTH_CODES 29

// literal and length codes including end of block
#define L_CODES (LITERALS + 1 + LENGTH_CODES)

// distance codes
#define D_CODES 30

// bit length codes
#define BL_CODES 19

// Literal Tree size - 573
#define HEAP_SIZE (2 * L_CODES + 1)

// Bit length codes must not exceed MAX_BL_BITS bits
#define MAX_BL_BITS 7

#define REP_3_6 16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10 17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138 18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

//#define GZ_MAX_MATCH 258

//#define GZ_MIN_MATCH 3

// LTREE, DTREE and BLTREE sizes
#define LTREE_SIZE 1024
#define DTREE_SIZE 64
#define BLTREE_SIZE 64
#define EXTRA_LCODES 32
#define EXTRA_DCODES 32
#define EXTRA_BLCODES 32
#define MAXCODE_SIZE 16

#endif // _XFCOMPRESSION_ZLIB_CONFIG_H_
