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
#ifndef _SAMPLES_HPP_
#define _SAMPLES_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
          unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
          unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int DIM_VECTOR_ = LENGTH,
          unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)>
class samples_graph : public adf::graph {
   public:
    // input and output port
    adf::port<input> image_points_from_PL;
    adf::port<input> delay_from_PL;
    adf::port<input> xdc_def_positions;
    adf::port<input> sampling_frequency;
    adf::port<output> samples_to_PL;

    samples_graph(double kernel_ratio = KERNEL_RATIO) {
        // kernels
        samples_diffMVKernel = adf::kernel::create(us::L1::diffMV<float, LENGTH, INCREMENT_MATRIX, SIMD_DEPTH>);
        samples_divVSKernel =
            adf::kernel::create(us::L1::divVSSpeedOfSound<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        samples_mulVSKernel =
            adf::kernel::create(us::L1::mulVS<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>); // sampling frequency
        samples_normAxis1Kernel = adf::kernel::create(us::L1::norm_axis_1<float, LENGTH, 1, SIMD_DEPTH>);
        samples_sumVSKernel = adf::kernel::create(us::L1::sumVOne<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        samples_sumVVKernel = adf::kernel::create(us::L1::sumVVStreamIn1<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);

        // source kernel
        adf::source(samples_diffMVKernel) = "diffMV/diffMV.cpp";
        adf::source(samples_divVSKernel) = "divVS/divVS.cpp";
        adf::source(samples_mulVSKernel) = "mulVS/mulVS.cpp";
        adf::source(samples_normAxis1Kernel) = "norm_axis_1/norm_axis_1.cpp";
        adf::source(samples_sumVSKernel) = "sumVS/sumVS.cpp";
        adf::source(samples_sumVVKernel) = "sumVV/sumVV.cpp";

        // diffMV
        adf::connect<>(image_points_from_PL, samples_diffMVKernel.in[0]);
        adf::connect<>(xdc_def_positions, samples_diffMVKernel.in[1]);

        adf::dimensions(samples_diffMVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(samples_diffMVKernel.in[1]) = {SPACE_DIMENSION_};
        adf::dimensions(samples_diffMVKernel.out[0]) = {DIM_MATRIX_};

        // normAxis1
        adf::connect<>(samples_diffMVKernel.out[0], samples_normAxis1Kernel.in[0]);

        adf::dimensions(samples_normAxis1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(samples_normAxis1Kernel.out[0]) = {DIM_VECTOR_};

        /// divVS
        adf::connect<>(samples_normAxis1Kernel.out[0], samples_divVSKernel.in[0]);

        adf::dimensions(samples_divVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(samples_divVSKernel.out[0]) = {DIM_VECTOR_};

        // sumVV
        adf::connect<>(samples_divVSKernel.out[0], samples_sumVVKernel.in[1]);
        adf::connect<>(delay_from_PL, samples_sumVVKernel.in[0]);

        adf::dimensions(samples_sumVVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(samples_sumVVKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(samples_sumVVKernel.out[0]) = {DIM_VECTOR_};

        // mulVS
        adf::connect<>(samples_sumVVKernel.out[0], samples_mulVSKernel.in[0]);
        adf::connect<>(sampling_frequency, samples_mulVSKernel.in[1]);

        adf::dimensions(samples_mulVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(samples_mulVSKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(samples_mulVSKernel.out[0]) = {DIM_VECTOR_};

        // sumVS
        adf::connect<>(samples_mulVSKernel.out[0], samples_sumVSKernel.in[0]);
        adf::connect<>(samples_sumVSKernel.out[0], samples_to_PL);

        adf::dimensions(samples_sumVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(samples_sumVSKernel.out[0]) = {DIM_VECTOR_};

        // kernel runtime
        adf::runtime<adf::ratio>(samples_diffMVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(samples_divVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(samples_mulVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(samples_normAxis1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(samples_sumVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(samples_sumVVKernel) = kernel_ratio;
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