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

#ifndef _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_KERNEL_HPP_
#define _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_KERNEL_HPP_

/**
 * @file zlib_decompress_stream_kernel.hpp
 * @brief Header for zlib decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "xil_inflate.h"
#include "zlib_config.h"
#include "lz_decompress.hpp"
#include "axi_stream_utils.hpp"

extern "C" {
/**
 * @brief Zlib decompression host stream kernel top function.
 *
 * @param inaxistream 16-bit input host axi stream
 * @param outaxistream 8-bit output host axi stream
 * @param encodedsize compressed size output axi stream
 * @param input_size input size
 *
 */
void xilZlibDecompressStream(hls::stream<xf::compression::hStream16b_t>& inaxistream,
                             hls::stream<xf::compression::hStream8b_t>& outaxistream,
                             hls::stream<xf::compression::hStream32b_t>& encodedsize,
                             uint32_t input_size);
}

#endif // _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_KERNEL_HPP_
