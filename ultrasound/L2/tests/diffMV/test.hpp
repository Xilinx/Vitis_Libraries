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

class diffMV : public adf::graph {
   public:
    adf::input_plio input1_diffMV;
    adf::input_plio input2_diffMV;
    adf::output_plio output_diffMV;

    diffMV() {
        // input and output port
        input1_diffMV = adf::input_plio::create("input1_diffMV", adf::plio_32_bits, "data/input1.txt");
        input2_diffMV = adf::input_plio::create("input2_diffMV", adf::plio_32_bits, "data/input2.txt");
        output_diffMV = adf::output_plio::create("output_diffMV", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        diffMVKernel = adf::kernel::create(L1::diffMV<float, LENGTH, INCREMENT_MATRIX, SIMD_DEPTH>);
        adf::source(diffMVKernel) = "diffMV.cpp";

        // connections
        adf::connect(input1_diffMV.out[0], diffMVKernel.in[0]);
        adf::connect(input2_diffMV.out[0], diffMVKernel.in[1]);
        adf::connect(diffMVKernel.out[0], output_diffMV.in[0]);

        // Setting kernel ratio
        adf::runtime<adf::ratio>(diffMVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::dimensions(diffMVKernel.in[0]) = {1024};
        adf::dimensions(diffMVKernel.in[1]) = {1024};
        adf::dimensions(diffMVKernel.out[0]) = {128};
    }

   private:
    // Kernel declaration
    adf::kernel diffMVKernel;
};
} // namespace L2
} // namespace us