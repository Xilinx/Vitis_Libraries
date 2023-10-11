/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
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

#pragma once

#include <adf.h>
#include "kernels.hpp"

#define FREME_LINGTH FRAME_LENGTH

namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH>
class mulMM : public adf::graph {
   public:
    // input and output ports
    adf::input_plio input1_mulMM;
    adf::input_plio input2_mulMM;
    adf::output_plio output_mulMM;

    mulMM() {
        // Kernel definition
        mulMMKernel = adf::kernel::create(L1::mulMM<float, LENGTH, SPACE_DIMENSION, SIMD_DEPTH>);

        // input and output ports
        input1_mulMM = adf::input_plio::create("input1_mulMM", adf::plio_32_bits, "data/input1.txt");
        input2_mulMM = adf::input_plio::create("input2_mulMM", adf::plio_32_bits, "data/input2.txt");
        output_mulMM = adf::output_plio::create("output_mulMM", adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_mulMM.out[0], mulMMKernel.in[0]);
        adf::connect(input2_mulMM.out[0], mulMMKernel.in[1]);
        adf::connect(mulMMKernel.out[0], output_mulMM.in[0]);

        // source kernel
        adf::source(mulMMKernel) = "mulMM.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulMMKernel) = KERNEL_RATIO;

        // config
        adf::dimensions(mulMMKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulMMKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulMMKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel mulMMKernel;
};
} // namespace L2
} // namespace us
