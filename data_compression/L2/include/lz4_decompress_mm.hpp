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
#ifndef _XFCOMPRESSION_LZ4_DECOMPRESS_MM_HPP_
#define _XFCOMPRESSION_LZ4_DECOMPRESS_MM_HPP_
/**
 * @file lz4_decompress_mm.hpp
 * @brief Header for LZ4 decompression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "lz_decompress.hpp"
#include "lz_optional.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"

#include "lz4_decompress.hpp"

#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16
#define MAX_OFFSET 65536
#define HISTORY_SIZE MAX_OFFSET

// Kernel top functions
extern "C" {

/**
 * @brief LZ4 decompression kernel takes compressed data as input and process in
 * block based fashion and writes the raw data to global memory.
 *
 * @param in input compressed data
 * @param out output raw data
 * @param in_block_size input block size of each block
 * @param in_compress_size compress size of each block
 * @param block_size_in_kb block size in bytes
 * @param no_blocks number of blocks
 */
void xilLz4Decompress(const xf::compression::uintMemWidth_t* in,
                      xf::compression::uintMemWidth_t* out,
                      uint32_t* in_block_size,
                      uint32_t* in_compress_size,
                      uint32_t block_size_in_kb,
                      uint32_t no_blocks);
}

#endif // _XFCOMPRESSION_LZ4_DECOMPRESS_MM_HPP_
