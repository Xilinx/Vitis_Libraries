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

#ifndef _XFCOMPRESSION_ZLIB_TREEGEN_MM_HPP_
#define _XFCOMPRESSION_ZLIB_TREEGEN_MM_HPP_

/**
 * @file treegen_kernel.hpp
 * @brief Header for tree generator kernel used in zlib compression.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "zlib_config.hpp"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>
#include "huffman_treegen.hpp"

extern "C" {
/**
 * @brief This is an initial version of dynamic huffman codes & bit
 * length generation kernel which takes literal and distance frequency data as input
 * and generates dynamic huffman codes and bit length data. This version of the
 * kernel performs better for larger data sets in synchronization with LZ77
 * and Huffman Kernels. It need to be optimized further to achieve better
 * results for smaller block sizes (<1MB) so that it improves for smaller file
 * usecase.
 *
 * @param dyn_ltree_freq input literal frequency data
 * @param dyn_dtree_freq input distance frequency data
 * @param dyn_bltree_freq output bit-length frequency data
 * @param dyn_ltree_codes output literal codes
 * @param dyn_dtree_codes output distance codes
 * @param dyn_bltree_codes output bit-length codes
 * @param dyn_ltree_blen output literal bit length data
 * @param dyn_dtree_blen output distance bit length data
 * @param dyn_bltree_blen output bit-length of bitlengths data
 * @param max_codes output upper limit of codes for literal, distances,
 * bitlengths
 * @param block_size_in_kb input block size in bytes
 * @param input_size input data size
 * @param blocks_per_chunk number of blocks persent in current input data
 *
 */
void xilTreegenKernel(uint32_t* dyn_ltree_freq,
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

#endif // _XFCOMPRESSION_TREEGEN_KERNEL_HPP_
