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
class sumVS : public adf::graph {
   public:
    adf::input_plio input1_sumVS;
    adf::input_plio input2_sumVS;
    adf::output_plio output_sumVS;

    sumVS() {
        // input & output plio
        input1_sumVS = adf::input_plio::create("input1_sumVS", adf::plio_32_bits, "data/input1.txt");
        input2_sumVS = adf::input_plio::create("input2_sumVS", adf::plio_32_bits, "data/input1.txt");
        output_sumVS = adf::output_plio::create("output_sumVS", adf::plio_32_bits, "data/output.txt");

        // kernel definition
        sumVSKernel = adf::kernel::create(L1::sumVSStream<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // source kernel
        adf::source(sumVSKernel) = "sumVS.cpp";

        // connections
        adf::connect(input1_sumVS.out[0], sumVSKernel.in[0]);
        adf::connect(input2_sumVS.out[0], sumVSKernel.in[1]);
        adf::connect(sumVSKernel.out[0], output_sumVS.in[0]);

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumVSKernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(sumVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumVSKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(sumVSKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel sumVSKernel;
};
} // namespace L2
} // namespace us
