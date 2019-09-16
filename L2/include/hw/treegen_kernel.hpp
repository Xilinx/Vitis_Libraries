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
#include "zlib_stream_utils.hpp"

#define LTREE_SIZE 1024
#define DTREE_SIZE 64
#define BLTREE_SIZE 64

// LZ specific Defines
#define BIT 8

#define MAX_BITS 15

// Find the minimum among the two subtrees
#define findmin(tree_freq, n, m, depth) \
    (tree_freq[n] < tree_freq[m] || (tree_freq[n] == tree_freq[m] && depth[n] <= depth[m]))

extern "C" {
#if (T_COMPUTE_UNIT == 1)
void xilTreegen_cu1
#elif (T_COMPUTE_UNIT == 2)
void xilTreegen_cu2
#endif
    (uint32_t* dyn_ltree_freq,
     uint32_t* dyn_dtree_freq,
     uint32_t* dyn_bltree_freq,
     uint32_t* dyn_ltree_codes,
     uint32_t* dyn_dtree_codes,
     uint32_t* dyn_bltree_codes,
     uint32_t* dyn_ltree_blen,
     uint32_t* dyn_dtree_blen,
     uint32_t* dyn_bltree_blen,
     uint32_t* max_codes,
     uint32_t block_size_in_kb,
     uint32_t input_size,
     uint32_t blocks_per_chunk);
}
