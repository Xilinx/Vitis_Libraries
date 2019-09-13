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
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>
#include "zlib_config.h"
// L1 modules
#include "lz_optional.hpp"
#include "stream_downsizer.hpp"
#include "zlib_stream_utils.hpp"

#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16

#define MIN_BLOCK_SIZE 116

// DYNAMIC HUFFMAN Compress STATES
#define WRITE_TOKEN 0
#define ML_DIST_REP 1
#define LIT_REP 2
#define SEND_OUTPUT 3
#define ML_EXTRA 4
#define DIST_REP 5
#define DIST_EXTRA 6

// LTREE, DTREE and BLTREE sizes
#define LTREE_SIZE 1024
#define DTREE_SIZE 64
#define BLTREE_SIZE 64

// LZ specific Defines
#define BIT 8

typedef ap_uint<GMEM_DWIDTH> uint512_t;

#define d_code(dist, dist_code) ((dist) < 256 ? dist_code[dist] : dist_code[256 + ((dist) >> 7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 *  * must not have side effects. dist_code[256] and dist_code[257] are never
 *   * used.
 *    */

// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;

extern "C" {
#if (H_COMPUTE_UNIT == 1)
void xilHuffman_cu1
#elif (H_COMPUTE_UNIT == 2)
void xilHuffman_cu2
#endif
    (uint512_t* in,
     uint512_t* out,
     uint32_t* in_block_size,
     uint32_t* compressd_size,
     uint32_t* dyn_litmtree_codes,
     uint32_t* dyn_distree_codes,
     uint32_t* dyn_bitlentree_codes,
     uint32_t* dyn_litmtree_blen,
     uint32_t* dyn_dtree_blen,
     uint32_t* dyn_bitlentree_blen,
     uint32_t* dyn_max_codes,
     uint32_t block_size_in_kb,
     uint32_t input_size);
}
