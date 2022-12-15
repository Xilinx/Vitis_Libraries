/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _FOCUSING_HPP_
#define _FOCUSING_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class focusing : public adf::graph {
   public:
    // input and output port
    adf::port<input> apo_ref_0;
    adf::port<input> xdc_def_0;
    adf::port<input> apo_ref_1;
    adf::port<input> xdc_def_1;
    adf::port<output> focusing_output;

    focusing() {
        //	kernel
        focusing_diffSVKernel1 = adf::kernel::create(us::L1::diffSV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        focusing_diffSVKernel2 = adf::kernel::create(us::L1::diffSV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        focusing_squareVKernel1 = adf::kernel::create(us::L1::squareV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        focusing_squareVKernel2 = adf::kernel::create(us::L1::squareV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        focusing_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        focusing_sqrtVKernel = adf::kernel::create(us::L1::sqrtV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);

        // connections
        adf::connect<adf::stream> focusing_apo_ref_in_0(apo_ref_0, focusing_diffSVKernel1.in[0]);
        adf::connect<adf::stream> focusing_xdc_def_in_0(xdc_def_0, focusing_diffSVKernel1.in[1]);
        adf::connect<adf::stream> focusing_apo_ref_in_1(apo_ref_1, focusing_diffSVKernel2.in[0]);
        adf::connect<adf::stream> focusing_xdc_def_in_1(xdc_def_1, focusing_diffSVKernel2.in[1]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(focusing_diffSVKernel1.out[0], focusing_squareVKernel1.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(focusing_diffSVKernel2.out[0], focusing_squareVKernel2.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(focusing_squareVKernel1.out[0], focusing_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(focusing_squareVKernel2.out[0], focusing_sumVVKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(focusing_sumVVKernel.out[0], focusing_sqrtVKernel.in[0]);
        adf::connect<adf::stream> focusing_res(focusing_sqrtVKernel.out[0], focusing_output);

        // source kernel
        adf::source(focusing_diffSVKernel1) = "diffSV/diffSV.cpp";
        adf::source(focusing_diffSVKernel2) = "diffSV/diffSV.cpp";
        adf::source(focusing_squareVKernel1) = "squareV/squareV.cpp";
        adf::source(focusing_squareVKernel2) = "squareV/squareV.cpp";
        adf::source(focusing_sumVVKernel) = "sumVV/sumVV.cpp";
        adf::source(focusing_sqrtVKernel) = "sqrtV/sqrtV.cpp";

        // runtime ratio
        adf::runtime<adf::ratio>(focusing_diffSVKernel1) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(focusing_diffSVKernel2) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(focusing_squareVKernel1) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(focusing_squareVKernel2) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(focusing_sumVVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(focusing_sqrtVKernel) = KERNEL_RATIO;

        // fifo depth
        adf::fifo_depth(focusing_apo_ref_in_0) = FIFO_DEPTH_;
        adf::fifo_depth(focusing_apo_ref_in_1) = FIFO_DEPTH_;
        adf::fifo_depth(focusing_xdc_def_in_0) = FIFO_DEPTH_;
        adf::fifo_depth(focusing_xdc_def_in_1) = FIFO_DEPTH_;
        adf::fifo_depth(focusing_res) = FIFO_DEPTH_;
    }

   private:
    adf::kernel focusing_diffSVKernel1;
    adf::kernel focusing_diffSVKernel2;
    adf::kernel focusing_squareVKernel1;
    adf::kernel focusing_squareVKernel2;
    adf::kernel focusing_sumVVKernel;
    adf::kernel focusing_sqrtVKernel;
};

} // namespace L2
} // namespace us

#endif