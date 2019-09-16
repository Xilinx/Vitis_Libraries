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
#include "lz_decompress.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_upsizer.hpp"
#include "stream_downsizer.hpp"
#include "zlib_stream_utils.hpp"

#define VEC 8
#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16

#define MIN_BLOCK_SIZE 128

#define Z_DEFLATED 8
#define TOP 0
#define PATH 1
#define PATHDO 2
#define STORED 3
#define COPY_ 4
#define COPY 5
#define TABLE 6
#define BITLENHUFF 7
#define CODELENS 8
#define BITLENGTH 9
#define LEN 10
#define CHECK 11
#define DONE 12
#define WRONG 13
#define MAXBITS 15
#define TOTAL_LENS 852
#define TOTAL_DISTS 592
#define TCODESIZE (TOTAL_LENS + TOTAL_DISTS)

#define GET_DIFF_IF_BIG(x, y) (x > y) ? (x - y) : 0

// LZ specific Defines
#define BIT 8
#define MIN_OFFSET 1
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

#define HISTORY_SIZE LZ_MAX_OFFSET_LIMIT
#define READ_STATE 0
#define MATCH_STATE 1
#define LOW_OFFSET_STATE 2
#define LOW_OFFSET 10

typedef ap_uint<DICT_ELE_WIDTH> uintDict_t;
typedef ap_uint<MATCH_LEVEL * DICT_ELE_WIDTH> uintDictV_t;

#define OUT_BYTES (4)
typedef ap_uint<OUT_BYTES * BIT> uintOut_t;
typedef ap_uint<2 * OUT_BYTES * BIT> uintDoubleOut_t;
typedef ap_uint<GMEM_DWIDTH> uint512_t;
typedef ap_uint<32> compressd_dt;
typedef ap_uint<VEC * 32> compressdV_dt;
typedef ap_uint<64> gzip_decompress_compressd_dt;

#define MAX_MATCH 258
#define MIN_MATCH 3
#define LENGTH_CODES 29

typedef enum { CODES, LENS, DISTS } packcodedata;

typedef struct {
    uint8_t op;   // OPeration, extrabits, table bits
    uint8_t bits; // bits in this part of code
    uint16_t val; // offset in table or code value
} code;

extern "C" {
#if (D_COMPUTE_UNIT == 1)
void xilDecompress_cu1
#elif (D_COMPUTE_UNIT == 2)
void xilDecompress_cu2
#endif
    (uint512_t* in, uint512_t* out, uint32_t* encoded_size, uint32_t input_size);
}
