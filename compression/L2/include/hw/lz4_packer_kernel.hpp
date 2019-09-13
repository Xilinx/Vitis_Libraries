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
#ifndef _XFCOMPRESSION_XIL_LZ4_PACKER_KERNEL_HPP_
#define _XFCOMPRESSION_XIL_LZ4_PACKER_KERNEL_HPP_

/**
 * @file xil_lz4_packer_kernel.hpp
 * @brief Header for LZ4 packer kernel.
 *
 * This file is part of XF Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "packer_modules.hpp"

#define PACK_WIDTH 64
typedef ap_uint<PACK_WIDTH> uintV_t; // 64bit input stream (for packer kernel)

// Kernel top functions
extern "C" {
/**
 * @brief LZ4 packer kernel
 *
 * @param in input stream width
 * @param out output stream width
 * @param head_prev_blk header of previous block
 * @param compressd_size output size
 * @param in_block_size intput size
 * @param encoded_size output size
 * @param orig_input_data output size
 * @param head_res_size output size
 * @param offset output size
 * @param block_size_in_kb intput size
 * @param no_blocks input size
 * @param tail_bytes input size
 */
void xilLz4Packer(const xf::compression::uintMemWidth_t* in,
                  xf::compression::uintMemWidth_t* out,
                  xf::compression::uintMemWidth_t* head_prev_blk,
                  uint32_t* compressd_size,
                  uint32_t* in_block_size,
                  uint32_t* encoded_size,
                  xf::compression::uintMemWidth_t* orig_input_data,
                  uint32_t head_res_size,
                  uint32_t offset,
                  uint32_t block_size_in_kb,
                  uint32_t no_blocks,
                  uint32_t tail_bytes);
}

#endif // _XFCOMPRESSION_XIL_LZ4_PACKER_KERNEL_HPP_
