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
#ifndef _APODIZATION_HPP_
#define _APODIZATION_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int WIN_SIZE_VECTOR_ = WIN_SIZE_VECTOR,
          unsigned int WIN_SIZE_MATRIX_ = WIN_SIZE_MATRIX,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class apodization : public adf::graph {
   public:
    // input and output port
    adf::port<input> image_points;
    adf::port<input> apodization_reference_i;
    adf::port<input> apo_distance_k;
    adf::port<input> F_number;
    adf::port<output> apodization_output;

    apodization() {
        tileVKernel = adf::kernel::create(us::L1::tileVApo<T, LENGTH_, 1, SPACE_DIMENSION_>);
        diffMVKernel = adf::kernel::create(us::L1::diffMV<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        mulMMKernel = adf::kernel::create(us::L1::mulMM<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        sumAxis1Kernel = adf::kernel::create(us::L1::sum_axis_1<T, LENGTH_, 1, SPACE_DIMENSION_>);
        absVKernel = adf::kernel::create(us::L1::absVWS<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        equalZeroKernel = adf::kernel::create(us::L1::equalS<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_, 0>);
        mulVS1Kernel = adf::kernel::create(us::L1::mulV1e_16<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        sumVVStreamInKernel = adf::kernel::create(us::L1::sumVVStreamIn1<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        reciprocalVKernel = adf::kernel::create(us::L1::reciprocalV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2Kernel = adf::kernel::create(us::L1::mulVS<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2FNumberKernel = adf::kernel::create(us::L1::mulVSWS<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        sumVOneKernel = adf::kernel::create(us::L1::sumVOneSW<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        mulVS2Pi2Kernel = adf::kernel::create(us::L1::mulVPi<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        cosVKernel = adf::kernel::create(us::L1::cosV<T, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);
        diffOneVKernel = adf::kernel::create(us::L1::diffOneVWW<T, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);
        mulVHalfKernel = adf::kernel::create(us::L1::mulVHalf<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        absVStreamInKernel = adf::kernel::create(us::L1::absVSW<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        lessThanOneKernel = adf::kernel::create(us::L1::lessOrEqualThanS<T, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_, 1>);
        mulVVKernel = adf::kernel::create(us::L1::mulVVStreamOut<T, LENGTH_, SIMD_DEPTH_, SIMD_DEPTH_>);

        //			tileV

        //			diffMV
        adf::connect<adf::stream> image_points_stream(image_points, diffMVKernel.in[0]);
        adf::connect<adf::stream> apo_ref_stream(apodization_reference_i, diffMVKernel.in[1]);

        //			mulMM
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(tileVKernel.out[0], mulMMKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(diffMVKernel.out[0], mulMMKernel.in[1]);

        //			sumAxis1
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(mulMMKernel.out[0], sumAxis1Kernel.in[0]);

        //			absV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(sumAxis1Kernel.out[0], absVKernel.in[0]);

        //			equalZero
        adf::connect<adf::stream> equalZero_stream_in(absVKernel.out[0], equalZeroKernel.in[0]);

        //			mulVS (1e-16 * apo_depth_zero)
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(equalZeroKernel.out[0], mulVS1Kernel.in[0]);

        //			sumVV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(mulVS1Kernel.out[0], sumVVStreamInKernel.in[1]);
        adf::connect<adf::stream> sumVV_stream_in(absVKernel.out[0], sumVVStreamInKernel.in[0]);

        //			reciprocalV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(sumVVStreamInKernel.out[0], reciprocalVKernel.in[0]);

        //			mulVS (apo_distance)
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(reciprocalVKernel.out[0], mulVS2Kernel.in[0]);
        adf::connect<adf::stream> mulVS_stream_in(apo_distance_k, mulVS2Kernel.in[1]);

        //			mulV2FNumber
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(mulVS2Kernel.out[0], mulVS2FNumberKernel.in[0]);
        adf::connect<adf::stream> f_number_stream_in(F_number, mulVS2FNumberKernel.in[1]);

        //			sumVOne
        adf::connect<adf::stream> sumVOne_stream_in(mulVS2FNumberKernel.out[0], sumVOneKernel.in[0]);

        //			mulV2Pi2
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(sumVOneKernel.out[0], mulVS2Pi2Kernel.in[0]);

        //			cosV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(mulVS2Pi2Kernel.out[0], cosVKernel.in[0]);

        //			diffOneV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(cosVKernel.out[0], diffOneVKernel.in[0]);

        //			mulVHalf
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(diffOneVKernel.out[0], mulVHalfKernel.in[0]);

        //			absVStreamIn
        adf::connect<adf::stream> absV_stream_in(mulVS2FNumberKernel.out[0], absVStreamInKernel.in[0]);

        //			lessThanOne
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(absVStreamInKernel.out[0], lessThanOneKernel.in[0]);

        //			mulVV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(mulVHalfKernel.out[0], mulVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(lessThanOneKernel.out[0], mulVVKernel.in[1]);

        //			result
        adf::connect<adf::stream> apodization_stream_out(mulVVKernel.out[0], apodization_output);

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

        adf::runtime<adf::ratio>(tileVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(diffMVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulMMKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumAxis1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(absVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVS1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumVVStreamInKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(reciprocalVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVS2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVS2FNumberKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumVOneKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVS2Pi2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(cosVKernel) = 1.0;
        adf::runtime<adf::ratio>(diffOneVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVHalfKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(absVStreamInKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(lessThanOneKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(equalZeroKernel) = KERNEL_RATIO;

        adf::fifo_depth(image_points_stream) = FIFO_DEPTH_;
        adf::fifo_depth(apo_ref_stream) = FIFO_DEPTH_;
        adf::fifo_depth(equalZero_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(sumVV_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(mulVS_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(f_number_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(sumVOne_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(absV_stream_in) = FIFO_DEPTH_;
        adf::fifo_depth(apodization_stream_out) = FIFO_DEPTH_;
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