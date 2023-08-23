/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH>
class imagePoints_graph : public adf::graph {
   public:
    // input and output port
    adf::port<input> start_positions;
    adf::port<input> directions;
    adf::port<input> samples_arange;
    adf::port<output> image_points;

    imagePoints_graph(double kernel_ratio = KERNEL_RATIO) {
        // kernel definition
        onesKernel = adf::kernel::create(L1::ones<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        outer1Kernel = adf::kernel::create(L1::outer<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        outer2Kernel = adf::kernel::create(L1::outer<T, LENGTH_, SPACE_DIMENSION_, SPACE_DIMENSION_>);
        sumMMKernel = adf::kernel::create(L1::sumMM<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);

        adf::source(onesKernel) = "ones/ones.cpp";
        adf::source(outer1Kernel) = "outer/outer.cpp";
        adf::source(outer2Kernel) = "outer/outer.cpp";
        adf::source(sumMMKernel) = "sumMM/sumMM.cpp";

        // outer1
        adf::connect<>(onesKernel.out[0], outer1Kernel.in[0]);
        adf::connect<>(start_positions, outer1Kernel.in[1]);

        adf::dimensions(onesKernel.out[0]) = {LENGTH_};
        adf::dimensions(outer1Kernel.in[0]) = {LENGTH_};
        adf::dimensions(outer1Kernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(outer1Kernel.out[0]) = {LENGTH_ * SPACE_DIMENSION_};

        // outer2
        adf::connect<>(samples_arange, outer2Kernel.in[0]);
        adf::connect<>(directions, outer2Kernel.in[1]);

        adf::dimensions(outer2Kernel.in[0]) = {LENGTH_};
        adf::dimensions(outer2Kernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(outer2Kernel.out[0]) = {LENGTH_ * SPACE_DIMENSION_};

        // sumMM
        adf::connect<>(outer1Kernel.out[0], sumMMKernel.in[0]);
        adf::connect<>(outer2Kernel.out[0], sumMMKernel.in[1]);
        adf::connect<>(sumMMKernel.out[0], image_points);

        adf::dimensions(sumMMKernel.in[0]) = {LENGTH_ * SPACE_DIMENSION_};
        adf::dimensions(sumMMKernel.in[1]) = {LENGTH_ * SPACE_DIMENSION_};
        adf::dimensions(sumMMKernel.out[0]) = {LENGTH_ * SPACE_DIMENSION_};

        // config
        adf::runtime<adf::ratio>(onesKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(outer1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(outer2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumMMKernel) = kernel_ratio;
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