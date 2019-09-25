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

#ifndef _XFCOMPRESSION_ZLIB_DECOMPRESS_KERNEL_HPP_
#define _XFCOMPRESSION_ZLIB_DECOMPRESS_KERNEL_HPP_

/**
 * @file zlib_decompress_kernel.hpp
 * @brief Header for zlib decompression kernel.
 *
 * This file is part of XF Compression Library.
 */
#include "zlib_config.h"
#include "lz_decompress.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_upsizer.hpp"
#include "stream_downsizer.hpp"
#include "inflate_trees.hpp"
#include "inflate_huffman.hpp"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

extern "C" {
/**
 * @brief Zlib decompression kernel top function.
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
