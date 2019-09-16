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
#ifndef _XFCOMPRESSION_AXI_STREAM_TYPES_HPP_
#define _XFCOMPRESSION_AXI_STREAM_TYPES_HPP_

#include "ap_axi_sdata.h"
#include "common.h"

namespace xf {
namespace compression {

/**
 *  typedef hStream<bit_width>_t
 *  User defined types for data streaming between host and kernel.
 */
typedef qdma_axis<8, 0, 0, 0> hStream8b_t;
typedef qdma_axis<16, 0, 0, 0> hStream16b_t;
typedef qdma_axis<32, 0, 0, 0> hStream32b_t;
typedef qdma_axis<64, 0, 0, 0> hStream64b_t;
typedef qdma_axis<128, 0, 0, 0> hStream128b_t;
typedef qdma_axis<256, 0, 0, 0> hStream256b_t;
typedef qdma_axis<512, 0, 0, 0> hStream512b_t;

/**
 *  typedef kStream<bit_width>_t
 *  User defined types for data streaming between kernels.
 */
typedef ap_axiu<8, 0, 0, 0> kStream8b_t;
typedef ap_axiu<16, 0, 0, 0> kStream16b_t;
typedef ap_axiu<32, 0, 0, 0> kStream32b_t;
typedef ap_axiu<64, 0, 0, 0> kStream64b_t;
typedef ap_axiu<128, 0, 0, 0> kStream128b_t;
typedef ap_axiu<256, 0, 0, 0> kStream256b_t;
typedef ap_axiu<512, 0, 0, 0> kStream512b_t;

} // end compression
} // end xf

#endif // _XFCOMPRESSION_AXI_STREAM_TYPES_HPP_
