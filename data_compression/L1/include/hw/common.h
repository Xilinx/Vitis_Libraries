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
#ifndef _XFCOMPRESSION_COMMON_H_
#define _XFCOMPRESSION_COMMON_H_

/**
 * @file common.h
 * @brief Header for defining common constants, types and macros.
 *
 * This file is part of XF Compression Library.
 */

#include <ap_int.h>

// config
#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16


namespace xf {
namespace compression {

typedef ap_uint<GMEM_DWIDTH> uintMemWidth_t;
typedef ap_uint<32> compressd_dt;
typedef ap_uint<8> streamDt;
typedef ap_uint<64> lz4_compressd_dt;
typedef ap_uint<64> snappy_compressd_dt;

const int c_gmemBurstSize = (2 * GMEM_BURST_SIZE);
const int c_sizeStreamDepth = 8;

// const int kParallelBlock = 8;
// const int kGmemDwidth = GMEM_DWIDTH;
// const int kMarker = 255;

} // end namespace compression
} // end namespace xf

#undef GMEM_DWIDTH
#undef GMEM_BURST_SIZE

#endif // _XFCOMPRESSION_COMMON_H_
