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

class diffSV : public adf::graph {
   public:
    diffSV() {
        // Kernel definition
        diffSVKernel = adf::kernel::create(L1::diffSV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_diffSV = adf::input_plio::create("input1_diffSV", adf::plio_32_bits, "data/input1.txt");
        input2_diffSV = adf::input_plio::create("input2_diffSV", adf::plio_32_bits, "data/input1.txt");
        output_diffSV = adf::output_plio::create("output_diffSV", adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_diffSV.out[0], diffSVKernel.in[0]);
        adf::connect(input2_diffSV.out[0], diffSVKernel.in[1]);
        adf::connect(diffSVKernel.out[0], output_diffSV.in[0]);

        // source kernel
        adf::source(diffSVKernel) = "diffSV.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(diffSVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::dimensions(diffSVKernel.in[0]) = {256};
        adf::dimensions(diffSVKernel.in[1]) = {256};
        adf::dimensions(diffSVKernel.out[0]) = {128};
    }

    adf::input_plio input1_diffSV;
    adf::input_plio input2_diffSV;
    adf::output_plio output_diffSV;

   private:
    //		Kernel declaration
    adf::kernel diffSVKernel;
};
} // namespace L2
} // namespace us
