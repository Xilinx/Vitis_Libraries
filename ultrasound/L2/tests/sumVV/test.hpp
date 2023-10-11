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

#include "kernels.hpp"
namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH>
class sumVV : public adf::graph {
   public:
    adf::input_plio input1_sumVV;
    adf::input_plio input2_sumVV;
    adf::output_plio output_sumVV;

    sumVV() {
        // input & output plio
        input1_sumVV = adf::input_plio::create("input1_sumVV", adf::plio_32_bits, "data/input1.txt");
        input2_sumVV = adf::input_plio::create("input2_sumVV", adf::plio_32_bits, "data/input2.txt");
        output_sumVV = adf::output_plio::create("output_sumVV", adf::plio_32_bits, "data/output.txt");

        // kernel definition
        sumVVKernel = adf::kernel::create(L1::sumVV<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        adf::source(sumVVKernel) = "sumVV.cpp";

        // connections
        adf::connect(input1_sumVV.out[0], sumVVKernel.in[0]);
        adf::connect(input2_sumVV.out[0], sumVVKernel.in[1]);
        adf::connect(sumVVKernel.out[0], output_sumVV.in[0]);

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumVVKernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(sumVVKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumVVKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(sumVVKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel sumVVKernel;
};
} // namespace L2
} // namespace us
