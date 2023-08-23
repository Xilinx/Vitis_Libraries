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
#ifndef _APODIZATION_SA_HPP_
#define _APODIZATION_SA_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int DIM_VECTOR_ = LENGTH,
          unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)>
class apodization_sa_graph : public adf::graph {
   public:
    // input and output port
    adf::port<input> image_points;
    adf::port<input> apodization_reference;
    adf::port<input> apo_distance_k;
    adf::port<input> F_number;
    adf::port<output> apodization_output;

    apodization_sa_graph(double kernel_ratio = KERNEL_RATIO) {
        tileVKernel = adf::kernel::create(us::L1::tileVApo<float, LENGTH_, 1, SPACE_DIMENSION_>);
        diffMVKernel = adf::kernel::create(us::L1::diffMV<float, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        mulMMKernel = adf::kernel::create(us::L1::mulMM<float, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        sumAxis1Kernel = adf::kernel::create(us::L1::sum_axis_1<float, LENGTH_, 1, SPACE_DIMENSION_>);
        absVKernel = adf::kernel::create(us::L1::absVWS<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        equalZeroKernel = adf::kernel::create(us::L1::equalS<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_, 0>);
        mulVS1Kernel = adf::kernel::create(us::L1::mulV1e_16<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        sumVVStreamInKernel =
            adf::kernel::create(us::L1::sumVVStreamIn1<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        reciprocalVKernel = adf::kernel::create(us::L1::reciprocalV<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2Kernel = adf::kernel::create(us::L1::mulVS<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2FNumberKernel = adf::kernel::create(us::L1::mulVSWS<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        sumVOneKernel = adf::kernel::create(us::L1::sumVOneSW<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2Pi2Kernel = adf::kernel::create(us::L1::mulVPi<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        cosVKernel = adf::kernel::create(us::L1::cosV<float, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);
        diffOneVKernel = adf::kernel::create(us::L1::diffOneVWW<float, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);
        mulVHalfKernel = adf::kernel::create(us::L1::mulVHalf<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        absVStreamInKernel = adf::kernel::create(us::L1::absVSW<float, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        lessThanOneKernel = adf::kernel::create(us::L1::lessOrEqualThanS<float, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_, 1>);
        mulVVKernel = adf::kernel::create(us::L1::mulVVStreamOut<float, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);

        adf::source(tileVKernel) = "tileV/tileV.cpp";
        adf::source(diffMVKernel) = "diffMV/diffMV.cpp";
        adf::source(mulMMKernel) = "mulMM/mulMM.cpp";
        adf::source(sumAxis1Kernel) = "sum_axis_1/sum_axis_1.cpp";
        adf::source(absVKernel) = "absV/absV.cpp";
        adf::source(equalZeroKernel) = "equalS/equalS.cpp";
        adf::source(mulVS1Kernel) = "mulVS/mulVS.cpp";
        adf::source(sumVVStreamInKernel) = "sumVV/sumVV.cpp";
        adf::source(reciprocalVKernel) = "reciprocalV/reciprocalV.cpp";
        adf::source(mulVS2Kernel) = "mulVS/mulVS.cpp";
        adf::source(mulVS2FNumberKernel) = "mulVS/mulVS.cpp";
        adf::source(mulVS2Pi2Kernel) = "mulVS/mulVS.cpp";
        adf::source(cosVKernel) = "cosV/cosV.cpp";
        adf::source(diffOneVKernel) = "diffSV/diffSV.cpp";
        adf::source(mulVHalfKernel) = "mulVS/mulVS.cpp";
        adf::source(absVStreamInKernel) = "absV/absV.cpp";
        adf::source(lessThanOneKernel) = "lessOrEqualThanS/lessOrEqualThanS.cpp";
        adf::source(mulVVKernel) = "mulVV/mulVV.cpp";
        adf::source(sumVOneKernel) = "sumVS/sumVS.cpp";

        // diffMV & tileV
        adf::connect<>(image_points, diffMVKernel.in[0]);
        adf::connect<>(apodization_reference, diffMVKernel.in[1]);

        adf::dimensions(tileVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(diffMVKernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(diffMVKernel.out[0]) = {DIM_MATRIX_};

        // mulMM
        adf::connect<>(tileVKernel.out[0], mulMMKernel.in[0]);
        adf::connect<>(diffMVKernel.out[0], mulMMKernel.in[1]);
        adf::dimensions(mulMMKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(mulMMKernel.out[0]) = {DIM_MATRIX_};

        // sumAxis1
        adf::connect<>(mulMMKernel.out[0], sumAxis1Kernel.in[0]);
        adf::dimensions(sumAxis1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(sumAxis1Kernel.out[0]) = {DIM_VECTOR_};

        // absV
        adf::connect<>(sumAxis1Kernel.out[0], absVKernel.in[0]);
        adf::dimensions(absVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(absVKernel.out[0]) = {DIM_VECTOR_};

        // equalZero
        adf::connect<>(absVKernel.out[0], equalZeroKernel.in[0]);
        adf::dimensions(equalZeroKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(equalZeroKernel.out[0]) = {DIM_VECTOR_};

        // mulVS (1e-16 * apo_depth_zero)
        adf::connect<>(equalZeroKernel.out[0], mulVS1Kernel.in[0]);
        adf::dimensions(mulVS1Kernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVS1Kernel.out[0]) = {DIM_VECTOR_};

        // sumVV
        adf::connect<>(mulVS1Kernel.out[0], sumVVStreamInKernel.in[1]);
        adf::connect<>(absVKernel.out[0], sumVVStreamInKernel.in[0]);
        adf::dimensions(sumVVStreamInKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumVVStreamInKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(sumVVStreamInKernel.out[0]) = {DIM_VECTOR_};

        // reciprocalV
        adf::connect<>(sumVVStreamInKernel.out[0], reciprocalVKernel.in[0]);
        adf::dimensions(reciprocalVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(reciprocalVKernel.out[0]) = {DIM_VECTOR_};

        // mulVS (apo_distance)
        adf::connect<>(reciprocalVKernel.out[0], mulVS2Kernel.in[0]);
        adf::connect<>(apo_distance_k, mulVS2Kernel.in[1]);
        adf::dimensions(mulVS2Kernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVS2Kernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulVS2Kernel.out[0]) = {DIM_VECTOR_};

        // mulV2FNumber
        adf::connect<>(mulVS2Kernel.out[0], mulVS2FNumberKernel.in[0]);
        adf::connect<>(F_number, mulVS2FNumberKernel.in[1]);
        adf::dimensions(mulVS2FNumberKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVS2FNumberKernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(mulVS2FNumberKernel.out[0]) = {DIM_VECTOR_};

        // sumVOne
        adf::connect<>(mulVS2FNumberKernel.out[0], sumVOneKernel.in[0]);
        adf::dimensions(sumVOneKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumVOneKernel.out[0]) = {DIM_VECTOR_};

        // mulV2Pi2
        adf::connect<>(sumVOneKernel.out[0], mulVS2Pi2Kernel.in[0]);
        adf::dimensions(mulVS2Pi2Kernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVS2Pi2Kernel.out[0]) = {DIM_VECTOR_};

        // cosV
        adf::connect<>(mulVS2Pi2Kernel.out[0], cosVKernel.in[0]);
        adf::dimensions(cosVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(cosVKernel.out[0]) = {DIM_VECTOR_};

        // diffOneV
        adf::connect<>(cosVKernel.out[0], diffOneVKernel.in[0]);
        adf::dimensions(diffOneVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(diffOneVKernel.out[0]) = {DIM_VECTOR_};

        // mulVHalf
        adf::connect<>(diffOneVKernel.out[0], mulVHalfKernel.in[0]);
        adf::dimensions(mulVHalfKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVHalfKernel.out[0]) = {DIM_VECTOR_};

        // absVStreamIn
        adf::connect<>(mulVS2FNumberKernel.out[0], absVStreamInKernel.in[0]);
        adf::dimensions(absVStreamInKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(absVStreamInKernel.out[0]) = {DIM_VECTOR_};

        // lessThanOne
        adf::connect<>(absVStreamInKernel.out[0], lessThanOneKernel.in[0]);

        adf::dimensions(lessThanOneKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(lessThanOneKernel.out[0]) = {DIM_VECTOR_};

        // mulVV
        adf::connect<>(mulVHalfKernel.out[0], mulVVKernel.in[0]);
        adf::connect<>(lessThanOneKernel.out[0], mulVVKernel.in[1]);

        adf::dimensions(mulVVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.out[0]) = {DIM_VECTOR_};

        // result
        adf::connect<>(mulVVKernel.out[0], apodization_output);

        // config
        adf::runtime<adf::ratio>(tileVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffMVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulMMKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumAxis1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(absVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVS1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumVVStreamInKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(reciprocalVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVS2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVS2FNumberKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(sumVOneKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVS2Pi2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(cosVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(diffOneVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVHalfKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(absVStreamInKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(lessThanOneKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(mulVVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(equalZeroKernel) = kernel_ratio;
    }

   private:
    adf::kernel tileVKernel;
    adf::kernel diffMVKernel;
    adf::kernel mulMMKernel;
    adf::kernel sumAxis1Kernel;
    adf::kernel absVKernel;
    adf::kernel mulVS1Kernel;
    adf::kernel sumVVStreamInKernel;
    adf::kernel reciprocalVKernel;
    adf::kernel equalZeroKernel;
    adf::kernel mulVS2Kernel;
    adf::kernel mulVS2FNumberKernel;
    adf::kernel sumVOneKernel;
    adf::kernel mulVS2Pi2Kernel;
    adf::kernel cosVKernel;
    adf::kernel diffOneVKernel;
    adf::kernel mulVHalfKernel;
    adf::kernel absVStreamInKernel;
    adf::kernel lessThanOneKernel;
    adf::kernel mulVVKernel;
};

} // namespace L2
} // namespace us

#endif