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
#ifndef _IMAGE_POINTS_HPP_
#define _IMAGE_POINTS_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class imagePoints : public adf::graph {
   public:
    // input and output port
    adf::port<input> start_positions;
    adf::port<input> samples_arange;
    adf::port<input> directions;
    adf::port<output> image_points;

    imagePoints() {
        onesKernel = adf::kernel::create(L1::ones<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        outer1Kernel = adf::kernel::create(L1::outer<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        outer2Kernel = adf::kernel::create(L1::outerStream<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        sumMMKernel = adf::kernel::create(L1::sumMM<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);

        // outer1
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(onesKernel.out[0], outer1Kernel.in[0]);
        adf::connect<adf::stream> outer1_in(start_positions, outer1Kernel.in[1]);

        // outer2
        adf::connect<adf::stream> outer2_in1(samples_arange, outer2Kernel.in[0]);
        adf::connect<adf::stream> outer2_in2(directions, outer2Kernel.in[1]);

        // sumMM
        adf::connect<adf::window<WIN_SIZE_MATRIX> >(outer1Kernel.out[0], sumMMKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> >(outer2Kernel.out[0], sumMMKernel.in[1]);

        // result
        adf::connect<adf::stream> res_stream(sumMMKernel.out[0], image_points);

        adf::source(onesKernel) = "ones/ones.cpp";
        adf::source(outer1Kernel) = "outer/outer.cpp";
        adf::source(outer2Kernel) = "outer/outer.cpp";
        adf::source(sumMMKernel) = "sumMM/sumMM.cpp";

        adf::runtime<adf::ratio>(onesKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(outer1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(outer2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumMMKernel) = KERNEL_RATIO;

        adf::fifo_depth(outer1_in) = FIFO_DEPTH_;
        adf::fifo_depth(outer2_in1) = FIFO_DEPTH_;
        adf::fifo_depth(outer2_in2) = FIFO_DEPTH_;
        adf::fifo_depth(res_stream) = FIFO_DEPTH_;
    }

   private:
    adf::kernel onesKernel;
    adf::kernel outer1Kernel;
    adf::kernel outer2Kernel;
    adf::kernel sumMMKernel;
};
} // namespace L2
} // namespace us

#endif