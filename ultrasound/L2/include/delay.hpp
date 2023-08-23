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
#ifndef _DELAY_HPP_
#define _DELAY_HPP_

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
class delay_graph : public adf::graph {
   public:
    // DELAY
    adf::port<input> image_points_from_PL;
    adf::port<input> image_points_from_PL_;
    adf::port<input> tx_def_ref_point;
    adf::port<input> tx_def_delay_distance;
    adf::port<input> tx_def_delay_distance2;
    adf::port<input> tx_def_focal_point;
    adf::port<input> t_start;
    adf::port<output> delay_to_PL;

    delay_graph(double kernel_ratio = KERNEL_RATIO) {
        // kernel
        tileVKernel = adf::kernel::create(us::L1::tileVApo<float, LENGTH, 1, SPACE_DIMENSION>);
        diffMVKernel1 = adf::kernel::create(us::L1::diffMV<float, LENGTH, INCREMENT_MATRIX_, SIMD_DEPTH>);
        mulMMKernel = adf::kernel::create(us::L1::mulMM<float, LENGTH, INCREMENT_MATRIX_, SIMD_DEPTH>);
        sumAxis1Kernel = adf::kernel::create(us::L1::sum_axis_1<float, LENGTH, 1, SPACE_DIMENSION>);
        absVKernel = adf::kernel::create(us::L1::absV<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        diffVSKernel = adf::kernel::create(us::L1::diffVS<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        signKernel = adf::kernel::create(us::L1::sign<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        diffMVKernel2 = adf::kernel::create(us::L1::diffMV<float, LENGTH, INCREMENT_MATRIX_, SIMD_DEPTH>);
        normAxis1Kernel = adf::kernel::create(us::L1::norm_axis_1<float, LENGTH, 1, SPACE_DIMENSION>);
        mulVVKernel = adf::kernel::create(us::L1::mulVV<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        divVSKernel = adf::kernel::create(us::L1::divVSSpeedOfSound<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        sumVSKernel = adf::kernel::create(us::L1::sumVSStream<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);
        diffVSKernel2 = adf::kernel::create(us::L1::diffVSStreamOut<float, LENGTH, INCREMENT_VECTOR_, SIMD_DEPTH>);

        adf::source(tileVKernel) = "tileV/tileV.cpp";
        adf::source(diffMVKernel1) = "diffMV/diffMV.cpp";
        adf::source(mulMMKernel) = "mulMM/mulMM.cpp";
        adf::source(sumAxis1Kernel) = "sum_axis_1/sum_axis_1.cpp";
        adf::source(absVKernel) = "absV/absV.cpp";
        adf::source(diffVSKernel) = "diffVS/diffVS.cpp";
        adf::source(signKernel) = "sign/sign.cpp";
        adf::source(diffMVKernel2) = "diffMV/diffMV.cpp";
        adf::source(normAxis1Kernel) = "norm_axis_1/norm_axis_1.cpp";
        adf::source(mulVVKernel) = "mulVV/mulVV.cpp";
        adf::source(divVSKernel) = "divVS/divVS.cpp";
        adf::source(sumVSKernel) = "sumVS/sumVS.cpp";
        adf::source(diffVSKernel2) = "diffVS/diffVS.cpp";

        // diffMVKernel1
        adf::connect<>(image_points_from_PL, diffMVKernel1.in[0]);
        adf::connect<>(tx_def_ref_point, diffMVKernel1.in[1]);

        adf::dimensions(tileVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel1.in[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel1.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffMVKernel1.out[0]) = {DIM_MATRIX_};

        // mulMM
        adf::connect<>(tileVKernel.out[0], mulMMKernel.in[0]);
        adf::connect<>(diffMVKernel1.out[0], mulMMKernel.in[1]);

        adf::dimensions(mulMMKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.out[0]) = {DIM_MATRIX_};

        // sum_axis_1
        adf::connect<>(mulMMKernel.out[0], sumAxis1Kernel.in[0]);

        adf::dimensions(sumAxis1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(sumAxis1Kernel.out[0]) = {DIM_VECTOR_};

        // absV
        adf::connect<>(sumAxis1Kernel.out[0], absVKernel.in[0]);

        adf::dimensions(absVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(absVKernel.out[0]) = {DIM_VECTOR_};

        // diffVS
        adf::connect<>(absVKernel.out[0], diffVSKernel.in[0]);
        adf::connect<>(tx_def_delay_distance, diffVSKernel.in[1]);

        adf::dimensions(diffVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(diffVSKernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffVSKernel.out[0]) = {DIM_VECTOR_};

        // sign
        adf::connect<>(diffVSKernel.out[0], signKernel.in[0]);

        adf::dimensions(signKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(signKernel.out[0]) = {DIM_VECTOR_};

        // diffMV2
        adf::connect<>(image_points_from_PL_, diffMVKernel2.in[0]);
        adf::connect<>(tx_def_focal_point, diffMVKernel2.in[1]);

        adf::dimensions(diffMVKernel2.in[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel2.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffMVKernel2.out[0]) = {DIM_MATRIX_};

        // normAxis1
        adf::connect<>(diffMVKernel2.out[0], normAxis1Kernel.in[0]);

        adf::dimensions(normAxis1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(normAxis1Kernel.out[0]) = {DIM_VECTOR_};

        // mulVV
        adf::connect<>(signKernel.out[0], mulVVKernel.in[0]);
        adf::connect<>(normAxis1Kernel.out[0], mulVVKernel.in[1]);

        adf::dimensions(mulVVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.out[0]) = {DIM_VECTOR_};

        // sumVS
        adf::connect<>(mulVVKernel.out[0], sumVSKernel.in[0]);
        adf::connect<>(tx_def_delay_distance2, sumVSKernel.in[1]);

        adf::dimensions(sumVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumVSKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(sumVSKernel.out[0]) = {DIM_VECTOR_};

        // divVS
        adf::connect<>(sumVSKernel.out[0], divVSKernel.in[0]);

        adf::dimensions(divVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(divVSKernel.out[0]) = {DIM_VECTOR_};

        // diffVS2
        adf::connect<>(divVSKernel.out[0], diffVSKernel2.in[0]);
        adf::connect<>(t_start, diffVSKernel2.in[1]);

        adf::dimensions(diffVSKernel2.in[0]) = {DIM_VECTOR_};
        adf::dimensions(diffVSKernel2.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffVSKernel2.out[0]) = {DIM_VECTOR_};

        // result
        adf::connect<>(diffVSKernel2.out[0], delay_to_PL);

        // runtime ratio setting
        adf::runtime<adf::ratio>(tileVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffMVKernel1) = kernel_ratio;
        adf::runtime<adf::ratio>(mulMMKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumAxis1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(absVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(signKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffMVKernel2) = kernel_ratio;
        adf::runtime<adf::ratio>(normAxis1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(divVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffVSKernel2) = kernel_ratio;
    }

   private:
    // DELAY
    adf::kernel tileVKernel;
    adf::kernel diffMVKernel1;
    adf::kernel mulMMKernel;
    adf::kernel sumAxis1Kernel;
    adf::kernel absVKernel;
    adf::kernel diffVSKernel;
    adf::kernel signKernel;
    adf::kernel diffMVKernel2;
    adf::kernel normAxis1Kernel;
    adf::kernel mulVVKernel;
    adf::kernel divVSKernel;
    adf::kernel sumVSKernel;
    adf::kernel diffVSKernel2;
};
} // namespace L2
} // namespace us

#endif
