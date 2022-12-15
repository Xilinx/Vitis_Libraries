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
#ifndef _SAMPLES_HPP_
#define _SAMPLES_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class samples : public adf::graph {
   public:
    // input and output port
    adf::port<input> image_points_from_PL_2;
    adf::port<input> xdc_def_positions;
    adf::port<input> samplingFrequency;
    adf::port<input> delay_from_PL;
    adf::port<output> samples_to_PL;

    samples() {
        // kernels
        samples_diffMVKernel = adf::kernel::create(us::L1::diffMV<T, LENGTH_, INCREMENT_MATRIX_, SIMD_DEPTH_>);
        samples_divVSKernel =
            adf::kernel::create(us::L1::divVSSpeedOfSound<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        samples_mulVSKernel = adf::kernel::create(
            us::L1::mulVSStreamIn<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>); // sampling frequency
        samples_normAxis1Kernel = adf::kernel::create(us::L1::norm_axis_1<T, LENGTH_, 1, SIMD_DEPTH_>);
        samples_sumVSKernel = adf::kernel::create(us::L1::sumVOne<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);
        samples_sumVVKernel = adf::kernel::create(us::L1::sumVVStreamIn1<T, LENGTH_, INCREMENT_VECTOR_, SIMD_DEPTH_>);

        // connections
        adf::connect<adf::stream> samples_img_points_in(image_points_from_PL_2, samples_diffMVKernel.in[0]);
        adf::connect<adf::stream> samples_xdc_def_pos_in(xdc_def_positions, samples_diffMVKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> >(samples_diffMVKernel.out[0], samples_normAxis1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(samples_normAxis1Kernel.out[0], samples_divVSKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(samples_divVSKernel.out[0], samples_sumVVKernel.in[1]);
        adf::connect<adf::stream> samples_delay_in(delay_from_PL, samples_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(samples_sumVVKernel.out[0], samples_mulVSKernel.in[0]);
        adf::connect<adf::stream> samples_sampling_freq_in(samplingFrequency, samples_mulVSKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_VECTOR> >(samples_mulVSKernel.out[0], samples_sumVSKernel.in[0]);
        adf::connect<adf::stream> samples_res_stream(samples_sumVSKernel.out[0], samples_to_PL);

        // source kernel
        adf::source(samples_diffMVKernel) = "diffMV/diffMV.cpp";
        adf::source(samples_divVSKernel) = "divVS/divVS.cpp";
        adf::source(samples_mulVSKernel) = "mulVS/mulVS.cpp";
        adf::source(samples_normAxis1Kernel) = "norm_axis_1/norm_axis_1.cpp";
        adf::source(samples_sumVSKernel) = "sumVS/sumVS.cpp";
        adf::source(samples_sumVVKernel) = "sumVV/sumVV.cpp";

        // kernel runtime
        adf::runtime<adf::ratio>(samples_diffMVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(samples_divVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(samples_mulVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(samples_normAxis1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(samples_sumVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(samples_sumVVKernel) = KERNEL_RATIO;

        // fifo depth
        adf::fifo_depth(samples_img_points_in) = FIFO_DEPTH_;
        adf::fifo_depth(samples_xdc_def_pos_in) = FIFO_DEPTH_;
        adf::fifo_depth(samples_sampling_freq_in) = FIFO_DEPTH_;
        adf::fifo_depth(samples_delay_in) = FIFO_DEPTH_;
        adf::fifo_depth(samples_res_stream) = FIFO_DEPTH_;
    }

   private:
    adf::kernel samples_diffMVKernel;
    adf::kernel samples_divVSKernel;
    adf::kernel samples_mulVSKernel;
    adf::kernel samples_normAxis1Kernel;
    adf::kernel samples_sumVSKernel;
    adf::kernel samples_sumVVKernel;
};

} // namespace L2
} // namespace us

#endif