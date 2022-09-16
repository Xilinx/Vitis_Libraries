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

#ifndef _XF_CODEC_KERNEL_RESIZE_SC_HPP_
#define _XF_CODEC_KERNEL_RESIZE_SC_HPP_

#pragma once

#include "vpp_acc.hpp"

#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>

/* The fixed width of interpolation */
#define W 32 // 36 //23 24
#define I 13 // 8K

/* The pixel width */
#define WBIT 8              // the input image width
#define NPPC 8              // 1-pixel/8-pixel implementation
#define WDATA (WBIT * NPPC) // axi data width
// 1 - 1-pixel / clock Interpolation
// 8 - 8-pixel / clock Interpolation

#define MAX_SRC (8192 * 8192 / NPPC) // 64M
#define MAX_DST (8192 * 8192 / NPPC) // 64M

/* define the type of data for resize */
typedef ap_fixed<W, I> fixed_t;

template <typename T>
T DivCeil(T a, int b) {
    return (a + b - 1) / b; // 8
}

#if NPPC == 1

class resize_acc : public VPP_ACC<resize_acc, 1> {
    // port bindings
    ZERO_COPY(configs);
    ZERO_COPY(axi_src);
    ZERO_COPY(axi_dst);

    SYS_PORT(configs, HBM[0]);
    SYS_PORT(axi_src, HBM[1]);
    SYS_PORT(axi_dst, HBM[2]);

   public:
    static void compute(ap_uint<32>* configs, ap_uint<8>* axi_src, ap_uint<8>* axi_dst);
    static void kernel_resize(ap_uint<32>* configs, ap_uint<8>* axi_src, ap_uint<8>* axi_dst);
};

#else

class resize_acc : public VPP_ACC<resize_acc, 1> {
    // port bindings
    ZERO_COPY(configs);
    ZERO_COPY(axi_src);
    ZERO_COPY(axi_dst);

    SYS_PORT(configs, HBM[0]);
    SYS_PORT(axi_src, HBM[1]);
    SYS_PORT(axi_dst, HBM[2]);

   public:
    static void compute(ap_uint<32>* configs, ap_uint<64>* axi_src, ap_uint<64>* axi_dst);
    static void kernel_resize(ap_uint<32>* configs, ap_uint<64>* axi_src, ap_uint<64>* axi_dst);
};

#endif

#endif // _XF_CODEC_KERNEL_RESIZE_SC_HPP_
