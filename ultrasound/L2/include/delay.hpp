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
#ifndef _DELAY_HPP_
#define _DELAY_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int WIN_SIZE_VECTOR_ = WIN_SIZE_VECTOR,
          unsigned int WIN_SIZE_MATRIX_ = WIN_SIZE_MATRIX,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class delay : public adf::graph {
   public:
    // input and output port
    adf::port<input> image_points_from_PL;
    adf::port<input> image_points_from_PL_;
    adf::port<input> tx_def_reference_point;
    adf::port<input> tx_def_delay_distance;
    adf::port<input> tx_def_delay_distance_;
    adf::port<input> tx_def_focal_point;
    adf::port<input> t_start;
    adf::port<output> delay_to_PL;

    delay() {
        // kernel
        tileVKernel = adf::kernel::create(us::L1::tileVApo<T, LENGTH_, 1, SPACE_DIMENSION_>);
        diffMVKernel1 = adf::kernel::create(us::L1::diffMV<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        mulMMKernel = adf::kernel::create(us::L1::mulMM<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        sumAxis1Kernel = adf::kernel::create(us::L1::sum_axis_1<T, LENGTH_, 1, SPACE_DIMENSION_>);
        absVKernel = adf::kernel::create(us::L1::absV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        diffVSKernel = adf::kernel::create(us::L1::diffVS<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        signKernel = adf::kernel::create(us::L1::sign<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        diffMVKernel2 = adf::kernel::create(us::L1::diffMV<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        normAxis1Kernel = adf::kernel::create(us::L1::norm_axis_1<T, LENGTH_, 1, SPACE_DIMENSION_>);
        mulVVKernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        divVSKernel = adf::kernel::create(us::L1::divVSSpeedOfSound<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        sumVSKernel = adf::kernel::create(us::L1::sumVSStream<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        diffVSKernel2 = adf::kernel::create(us::L1::diffVSStreamOut<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);

        // connections
        adf::connect<adf::stream> image_points_stream(image_points_from_PL, diffMVKernel1.in[0]);
        adf::connect<adf::stream> tx_def_reference_point_stream(tx_def_reference_point, diffMVKernel1.in[1]);

        // mulMM
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(tileVKernel.out[0], mulMMKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(diffMVKernel1.out[0], mulMMKernel.in[1]);

        // sum_axis_1
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(mulMMKernel.out[0], sumAxis1Kernel.in[0]);

        // absV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(sumAxis1Kernel.out[0], absVKernel.in[0]);

        // diffVS
        adf::connect<adf::stream> absV_out_stream(absVKernel.out[0], diffVSKernel.in[0]);
        adf::connect<adf::stream> tx_def_delay_distance_stream(tx_def_delay_distance, diffVSKernel.in[1]);

        // sign
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(diffVSKernel.out[0], signKernel.in[0]);

        // diffMV2
        adf::connect<adf::stream> image_points_stream2(image_points_from_PL_, diffMVKernel2.in[0]);
        adf::connect<adf::stream> tx_def_focal_point_stream(tx_def_focal_point, diffMVKernel2.in[1]);

        // normAxis1
        adf::connect<adf::window<WIN_SIZE_MATRIX_> >(diffMVKernel2.out[0], normAxis1Kernel.in[0]);

        // mulVV
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(signKernel.out[0], mulVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(normAxis1Kernel.out[0], mulVVKernel.in[1]);

        // sumVS
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(mulVVKernel.out[0], sumVSKernel.in[0]);
        adf::connect<adf::stream> tx_def_delay_distance_stream2(tx_def_delay_distance_, sumVSKernel.in[1]);

        // divVS
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(sumVSKernel.out[0], divVSKernel.in[0]);

        // diffVS2
        adf::connect<adf::window<WIN_SIZE_VECTOR_> >(divVSKernel.out[0], diffVSKernel2.in[0]);
        adf::connect<adf::stream> t_start_stream(t_start, diffVSKernel2.in[1]);

        // result
        adf::connect<adf::stream> result_output(diffVSKernel2.out[0], delay_to_PL);

        // source kernel
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

        // runtime ratio setting
        adf::runtime<adf::ratio>(tileVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(diffMVKernel1) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulMMKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumAxis1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(absVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(diffVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(signKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(diffMVKernel2) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(normAxis1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(mulVVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(divVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(sumVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(diffVSKernel2) = KERNEL_RATIO;

        // fifo_depth setting
        adf::fifo_depth(image_points_stream) = FIFO_DEPTH_;
        adf::fifo_depth(tx_def_reference_point_stream) = FIFO_DEPTH_;
        adf::fifo_depth(absV_out_stream) = FIFO_DEPTH_;
        adf::fifo_depth(image_points_stream2) = FIFO_DEPTH_;
        adf::fifo_depth(tx_def_focal_point_stream) = FIFO_DEPTH_;
        adf::fifo_depth(tx_def_delay_distance_stream) = FIFO_DEPTH_;
        adf::fifo_depth(tx_def_delay_distance_stream2) = FIFO_DEPTH_;
        adf::fifo_depth(t_start_stream) = FIFO_DEPTH_;
        adf::fifo_depth(result_output) = FIFO_DEPTH_;
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