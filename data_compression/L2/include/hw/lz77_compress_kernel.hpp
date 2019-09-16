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
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "zlib_config.h"
#include "lz_compress.hpp"
#include "lz_optional.hpp"
#include "stream_downsizer.hpp"
#include "zlib_stream_utils.hpp"

#define VEC 8
#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16

#define MIN_BLOCK_SIZE 128

// LZ4 Compress STATES
#define WRITE_TOKEN 0
#define WRITE_LIT_LEN 1
#define WRITE_MATCH_LEN 2
#define WRITE_LITERAL 3
#define WRITE_OFFSET0 4
#define WRITE_OFFSET1 5

#define GET_DIFF_IF_BIG(x, y) (x > y) ? (x - y) : 0

// LZ specific Defines
#define BIT 8
#define MIN_OFFSET 10
#define MIN_MATCH 4
#define LZ_MAX_OFFSET_LIMIT 32768
#define LZ_HASH_BIT 12
#define LZ_DICT_SIZE (1 << LZ_HASH_BIT)
#define MAX_MATCH_LEN 255
#define OFFSET_WINDOW 32768
#define MATCH_LEN 6
#define MIN_MATCH 4
typedef ap_uint<MATCH_LEN * BIT> uintMatchV_t;
#define MATCH_LEVEL 6
#define DICT_ELE_WIDTH (MATCH_LEN * BIT + 24)

typedef ap_uint<DICT_ELE_WIDTH> uintDict_t;
typedef ap_uint<MATCH_LEVEL * DICT_ELE_WIDTH> uintDictV_t;

#define OUT_BYTES (4)
typedef ap_uint<OUT_BYTES * BIT> uintOut_t;
typedef ap_uint<2 * OUT_BYTES * BIT> uintDoubleOut_t;
typedef ap_uint<GMEM_DWIDTH> uint512_t;
typedef ap_uint<32> compressd_dt;
typedef ap_uint<VEC * 32> compressdV_dt;
typedef ap_uint<64> lz77_compressd_dt;

#define MAX_MATCH 258
#define MIN_MATCH 3
#define LENGTH_CODES 29

#define d_code(dist, dist_code) ((dist) < 256 ? dist_code[dist] : dist_code[256 + ((dist) >> 7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. dist_code[256] and dist_code[257] are never
 * used.
 */

extern "C" {
#if (C_COMPUTE_UNIT == 1)
void xilLz77_cu1
#elif (C_COMPUTE_UNIT == 2)
void xilLz77_cu2
#endif
    (const uint512_t* in,
     uint512_t* out,
     uint32_t* compressd_size,
     uint32_t* in_block_size,
     uint32_t* dyn_ltree_freq,
     uint32_t* dyn_dtree_freq,
     uint32_t block_size_in_kb,
     uint32_t input_size);
}
