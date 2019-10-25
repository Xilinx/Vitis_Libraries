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

#ifndef _XFCOMPRESSION_ZLIB_DECOMPRESS_MM_HPP_
#define _XFCOMPRESSION_ZLIB_DECOMPRESS_MM_HPP_

/**
 * @file zlib_decompress_kernel.hpp
 * @brief Header for zlib decompression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_config.hpp"
#include "lz_decompress.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_upsizer.hpp"
#include "stream_downsizer.hpp"
#include "huffman_decoder.hpp"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#define BIT 8
#define MIN_OFFSET 1
#define MIN_MATCH 4
#define LZ_MAX_OFFSET_LIMIT 32768
#define HISTORY_SIZE LZ_MAX_OFFSET_LIMIT
#define LOW_OFFSET 10

extern "C" {
/**
 * @brief Zlib decompression kernel top function.
 * This is an initial version of zlib decompression process.
 * It does huffman bit upacking and lz77 decompression. This can be further
 * optimized to achieve better throughput in case of less compression ratio
 * case.
 *
 * @param in input stream
 * @param out output stream
 * @param encoded_size decompressed size output
 * @param input_size input size
 */
void xilDecompressZlib(xf::compression::uintMemWidth_t* in,
                       xf::compression::uintMemWidth_t* out,
                       uint32_t* encoded_size,
                       uint32_t input_size);
}

#endif // _XFCOMPRESSION_ZLIB_DECOMPRESS_KERNEL_HPP_
