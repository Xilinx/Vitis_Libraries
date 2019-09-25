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

#ifndef _XFCOMPRESSION_HUFFMAN_KERNEL_HPP_
#define _XFCOMPRESSION_HUFFMAN_KERNEL_HPP_

/**
 * @file huffman_kernel.hpp
 * @brief Header for huffman kernel used in zlib compression.
 *
 * This file is part of XF Compression Library.
 */
// L1 modules
#include "lz_optional.hpp"
#include "stream_downsizer.hpp"
#include "zlib_stream_utils.hpp"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>
#include "zlib_config.h"

extern "C" {
/**
 * @brief Huffman kernel top function.
 *
 * @param in input stream
 * @param out output stream
 * @param in_block_size input block size
 * @param compressd_size compressed size output
 * @param dyn_litmtree_codes description needed...
 * @param dyn_distree_codes description needed...
 * @param dyn_bitlentree_codes description needed...
 * @param dyn_litmtree_blen description needed...
 * @param dyn_dtree_blen description needed...
 * @param dyn_bitlentree_blen description needed...
 * @param dyn_max_codes description needed...
 * @param block_size_in_kb description needed...
 * @param input_size input size
 *
 */
void xilHuffmanKernel(xf::compression::uintMemWidth_t* in,
                      xf::compression::uintMemWidth_t* out,
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

#endif // _XFCOMPRESSION_HUFFMAN_KERNEL_HPP_
