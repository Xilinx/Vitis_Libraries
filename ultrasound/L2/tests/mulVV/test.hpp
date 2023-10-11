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

namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH>
class mulVV : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input1_mulVV;
    adf::input_plio input2_mulVV;
    adf::output_plio output_mulVV;

    mulVV() {
        // Kernel definition
        mulVVKernel = adf::kernel::create(L1::mulVV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_mulVV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_mulVV = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_mulVV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_mulVV.out[0], mulVVKernel.in[0]);
        adf::connect(input2_mulVV.out[0], mulVVKernel.in[1]);
        adf::connect(mulVVKernel.out[0], output_mulVV.in[0]);

        // source kernel
        adf::source(mulVVKernel) = "mulVV.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulVVKernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(mulVVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulVVKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel mulVVKernel;
};
} // namespace L2
} // namespace us
