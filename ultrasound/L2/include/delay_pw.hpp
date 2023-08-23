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
#ifndef _DELAY_PW_HPP_
#define _DELAY_PW_HPP_

#include "kernels.hpp"
#include <adf.h>

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int DIM_VECTOR_ = LENGTH,
          unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)>
class delay_pw_graph : public adf::graph {
   public:
    // input and output ports
    adf::port<input> image_points_from_PL;
    adf::port<input> tx_def_reference_point;
    adf::port<input> t_start;
    adf::port<output> delay_to_PL;

    delay_pw_graph(double kernel_ratio = KERNEL_RATIO) {
        // kernel
        tileVKernel = adf::kernel::create(us::L1::tileVApo<T, LENGTH_, INCREMENT_MATRIX_, SPACE_DIMENSION_>);
        diffMVKernel1 = adf::kernel::create(us::L1::diffMV<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        mulMMKernel = adf::kernel::create(us::L1::mulMM<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        sumAxis1Kernel = adf::kernel::create(us::L1::sum_axis_1<T, LENGTH_, INCREMENT_MATRIX_, SPACE_DIMENSION_>);
        divVSKernel = adf::kernel::create(us::L1::divVSSpeedOfSound<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        diffVSKernel2 = adf::kernel::create(us::L1::diffVSStreamOut<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);

        adf::source(tileVKernel) = "tileV/tileV.cpp";
        adf::source(diffMVKernel1) = "diffMV/diffMV.cpp";
        adf::source(mulMMKernel) = "mulMM/mulMM.cpp";
        adf::source(sumAxis1Kernel) = "sum_axis_1/sum_axis_1.cpp";
        adf::source(divVSKernel) = "divVS/divVS.cpp";
        adf::source(diffVSKernel2) = "diffVS/diffVS.cpp";

        // diffMV1
        adf::connect<>(image_points_from_PL, diffMVKernel1.in[0]);
        adf::connect<>(tx_def_reference_point, diffMVKernel1.in[1]);

        adf::dimensions(diffMVKernel1.in[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel1.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffMVKernel1.out[0]) = {DIM_MATRIX_};

        // tileV & mulMM
        adf::connect<>(tileVKernel.out[0], mulMMKernel.in[0]);
        adf::connect<>(diffMVKernel1.out[0], mulMMKernel.in[1]);

        adf::dimensions(tileVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.out[0]) = {DIM_MATRIX_};

        // sum_axis_1
        adf::connect<>(mulMMKernel.out[0], sumAxis1Kernel.in[0]);

        adf::dimensions(sumAxis1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(sumAxis1Kernel.out[0]) = {DIM_VECTOR_};

        // divVS
        adf::connect<>(sumAxis1Kernel.out[0], divVSKernel.in[0]);

        adf::dimensions(divVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(divVSKernel.out[0]) = {DIM_VECTOR_};

        // diffVS2
        adf::connect<>(divVSKernel.out[0], diffVSKernel2.in[0]);
        adf::connect<>(t_start, diffVSKernel2.in[1]);
        adf::connect<>(diffVSKernel2.out[0], delay_to_PL);

        adf::dimensions(diffVSKernel2.in[0]) = {DIM_VECTOR_};
        adf::dimensions(diffVSKernel2.in[1]) = {DIM_VECTOR_};
        adf::dimensions(diffVSKernel2.out[0]) = {DIM_VECTOR_};

        // config
        adf::runtime<adf::ratio>(tileVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffMVKernel1) = kernel_ratio;
        adf::runtime<adf::ratio>(mulMMKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumAxis1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(divVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffVSKernel2) = kernel_ratio;
    }

   private:
    // DELAY
    adf::kernel tileVKernel;
    adf::kernel diffMVKernel1;
    adf::kernel mulMMKernel;
    adf::kernel sumAxis1Kernel;
    adf::kernel divVSKernel;
    adf::kernel diffVSKernel2;
};

} // namespace L2
} // namespace us

#endif
