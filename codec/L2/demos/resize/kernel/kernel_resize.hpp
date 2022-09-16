/*
 * Copyright 2019 Xilinx, Inc.
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
 */
/**
 * @file kernel_resize.hpp
 *
 * @brief This file contains top function of test case.
 */

#ifndef _XF_CODEC_KERNEL_RESIZE_HPP_
#define _XF_CODEC_KERNEL_RESIZE_HPP_

#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>

/* The fixed width of interpolation */
#define W 33 // 36 //23 24
#define I 14 // 8K

/* The pixel width */
#define WBIT 8              // the input image width
#define NPPC 8              // 1-pixel/8-pixel implementation
#define WDATA (WBIT * NPPC) // axi data width
// 1 - 1-pixel / clock Interpolation
// 8 - 8-pixel / clock Interpolation

#define MAX_SRC (8192 * 8192 / NPPC) // 64M
#define MAX_DST (8192 * 8192 / NPPC) // 64M

/* define the type of data for resize */
typedef hls::stream<ap_uint<WDATA> > data_t;
typedef hls::stream<ap_uint<72> > data_8x_t;
typedef ap_fixed<W, I> fixed_t;

template <typename T>
T DivCeil(T a, int b) {
    return (a + b - 1) / b; // 8
}

extern "C" void kernel_resize(ap_uint<32>* configs, ap_uint<WDATA>* axi_src, ap_uint<WDATA>* axi_dst);

#endif // _XF_CODEC_KERNEL_RESIZE_HPP_
