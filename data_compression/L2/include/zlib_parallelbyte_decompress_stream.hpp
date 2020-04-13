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

#ifndef _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_HPP_
#define _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_HPP_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "huffman_decoder.hpp"
#include "zlib_config.hpp"
#include "lz_decompress.hpp"
#include "inflate.hpp"
#include "stream_downsizer.hpp"
#include "ap_axi_sdata.h"

#ifndef HIGH_FMAX_II
#define HIGH_FMAX_II 1
#endif

#define LZ_MAX_OFFSET_LIMIT 32768
#define LOW_OFFSET 10

extern "C" {
void xilDecompressStream(uint32_t input_size,
                         hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& inaxistreamd,
                         hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& outaxistreamd,
                         hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& sizestreamd);
}
#endif // _XFCOMPRESSION_ZLIB_DECOMPRESS_STREAM_HPP_
