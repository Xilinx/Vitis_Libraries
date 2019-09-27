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

#ifndef _XFCOMPRESSION_TREEGEN_KERNEL_HPP_
#define _XFCOMPRESSION_TREEGEN_KERNEL_HPP_

/**
 * @file treegen_kernel.hpp
 * @brief Header for tree generator kernel used in zlib compression.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "zlib_config.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>
#include "deflate_trees.hpp"

extern "C" {
/**
 * @brief Treegen kernel top function.
 *
 * @param dyn_ltree_freq description needed...
 * @param dyn_dtree_freq description needed...
 * @param dyn_bltree_freq description needed...
 * @param dyn_ltree_codes description needed...
 * @param dyn_dtree_codes description needed...
 * @param dyn_bltree_codes description needed...
 * @param dyn_ltree_blen description needed...
 * @param dyn_dtree_blen description needed...
 * @param dyn_bltree_blen description needed...
 * @param max_codes description needed...
 * @param block_size_in_kb description needed...
 * @param input_size input size
 * @param blocks_per_chunk description needed...
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
