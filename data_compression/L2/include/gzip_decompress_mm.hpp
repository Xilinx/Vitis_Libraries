/*
 * (c) Copyright 2019-2021 Xilinx, Inc. All rights reserved.
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

#ifndef _XFCOMPRESSION_GZIP_DECOMPRESS_MM_HPP_
#define _XFCOMPRESSION_GZIP_DECOMPRESS_MM_HPP_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "huffman_decoder.hpp"
#include "zlib_specs.hpp"
#include "lz_decompress.hpp"
#include "inflate.hpp"
#include "stream_downsizer.hpp"
#include "ap_axi_sdata.h"

// by default disable low latency model
#ifndef LL_MODEL
#define LL_MODEL false
#endif

#define LZ_MAX_OFFSET_LIMIT 32768
#define LOW_OFFSET 1

#define MULTIPLE_BYTES 8
#define MAX_OFFSET (32 * 1024)
#define HISTORY_SIZE MAX_OFFSET

#define HUFFMAN_TYPE xf::compression::FULL

#define GMEM_DWIDTH (MULTIPLE_BYTES * 8)
#define GMEM_BURST_SIZE 512

extern "C" {
void xilDecompress(const ap_uint<GMEM_DWIDTH>* in,
                   ap_uint<GMEM_DWIDTH>* out,
                   uint32_t* encodedSize,
                   uint32_t inputSize);
}
#endif // _XFCOMPRESSION_GZIP_DECOMPRESS_MM_HPP_
