/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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

#ifndef _XFCOMPRESSION_ZSTD_DECOMPRESS_STREAM_HPP_
#define _XFCOMPRESSION_ZSTD_DECOMPRESS_STREAM_HPP_

/**
 * @file zstd_decompress_stream.hpp
 * @brief Header for zstd decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "zstd_decompress.hpp"
#include "ap_axi_sdata.h"

#define LZ_MAX_OFFSET_LIMIT 32768
#define HISTORY_SIZE LZ_MAX_OFFSET_LIMIT
#define ZSTD_BLOCK_SIZE_KB 32

#ifndef MULTIPLE_BYTES
#define MULTIPLE_BYTES 4
#endif

const int c_streamDWidth = 8 * MULTIPLE_BYTES;

extern "C" {
/**
 * @brief Zstd decompression stream kernel top function.
 *
 * @param input_size input size
 * @param inaxistreamd input kernel axi stream
 * @param outaxistreamd output kernel axi stream
 *
 */
void xilZstdDecompressStream(uint64_t input_size,
                             hls::stream<ap_axiu<c_streamDWidth, 0, 0, 0> >& inaxistreamd,
                             hls::stream<ap_axiu<c_streamDWidth, 0, 0, 0> >& outaxistreamd,
                             hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamd);
}
#endif // _XFCOMPRESSION_ZSTD_DECOMPRESS_STREAM_HPP_
