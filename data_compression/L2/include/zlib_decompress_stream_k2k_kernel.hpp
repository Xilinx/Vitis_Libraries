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

#ifndef _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_K2K_KERNEL_HPP_
#define _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_K2K_KERNEL_HPP_

/**
 * @file zlib_decompress_stream_k2k_kernel.hpp
 * @brief Header for zlib decompression streaming(kernel to kernel) kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "inflate_trees.hpp"
#include "inflate_huffman.hpp"
#include "zlib_config.h"
#include "lz_decompress.hpp"
#include "axi_stream_types.hpp"

extern "C" {
/**
 * @brief Zlib decompression stream kernel top function.
 *
 * @param input_size input size
 * @param inaxistreamd input kernel axi stream
 * @param outaxistreamd output kernel axi stream
 *
 */
void xilDecompressStreamk2k(uint32_t input_size,
                            hls::stream<xf::compression::kStream16b_t>& inaxistreamd,
                            hls::stream<xf::compression::kStream8b_t>& outaxistreamd);
}
#endif // _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_K2K_KERNEL_HPP_
