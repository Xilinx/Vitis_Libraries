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
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// config

namespace xf {
namespace compression {

const int kGMemDWidth = 512;
const int kGMemBurstSize = 16;

typedef ap_uint<kGMemDWidth> uintMemWidth_t;
typedef ap_uint<32> compressd_dt;
typedef ap_uint<8> streamDt;
typedef ap_uint<64> lz4_compressd_dt;
typedef ap_uint<64> snappy_compressd_dt;

const int c_gmemBurstSize = (2 * kGMemBurstSize);
const int c_sizeStreamDepth = 8;
} // end namespace compression
} // end namespace xf

#endif // _XFCOMPRESSION_COMMON_H_
