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

#ifndef _XFCOMPRESSION_LZ77_COMPRESS_KERNEL_HPP_
#define _XFCOMPRESSION_LZ77_COMPRESS_KERNEL_HPP_

/**
 * @file lz77_compress_kernel.hpp
 * @brief Header for lz77 compression kernel used in zlib compression.
 *
 * This file is part of XF Compression Library.
 */

#include "zlib_config.h"
#include "lz_compress.hpp"
#include "lz_optional.hpp"
#include "stream_downsizer.hpp"
#include "zlib_stream_utils.hpp"

#include "hls_stream.h"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>

extern "C" {
/**
 * @brief LZ77 compression kernel top function.
 *
 * @param in input stream
 * @param out output stream
 * @param compressd_size compressed size output
 * @param in_block_size description needed...
 * @param dyn_ltree_freq description needed...
 * @param dyn_dtree_freq description needed...
 * @param block_size_in_kb description needed...
 * @param input_size input size
 *
 */
void xilLz77Compress(const xf::compression::uintMemWidth_t* in,
                     xf::compression::uintMemWidth_t* out,
                     uint32_t* compressd_size,
                     uint32_t* in_block_size,
                     uint32_t* dyn_ltree_freq,
                     uint32_t* dyn_dtree_freq,
                     uint32_t block_size_in_kb,
                     uint32_t input_size);
}

#endif // _XFCOMPRESSION_LZ77_COMPRESS_KERNEL_HPP_
