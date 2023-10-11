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

class diffVS : public adf::graph {
   public:
    diffVS() {
        // Kernel definition
        diffVSKernel = adf::kernel::create(L1::diffVS<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_diffVS = adf::input_plio::create("input1_diffVS", adf::plio_32_bits, "data/input1.txt");
        input2_diffVS = adf::input_plio::create("input2_diffVS", adf::plio_32_bits, "data/input2.txt");
        output_diffVS = adf::output_plio::create("output_diffVS", adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_diffVS.out[0], diffVSKernel.in[0]);
        adf::connect(input2_diffVS.out[0], diffVSKernel.in[1]);
        adf::connect(diffVSKernel.out[0], output_diffVS.in[0]);

        // source kernel
        adf::source(diffVSKernel) = "diffVS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(diffVSKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::dimensions(diffVSKernel.in[0]) = {256};
        adf::dimensions(diffVSKernel.in[1]) = {4};
        adf::dimensions(diffVSKernel.out[0]) = {128};
    }

    adf::input_plio input1_diffVS;
    adf::input_plio input2_diffVS;
    adf::output_plio output_diffVS;

   private:
    // Kernel declaration
    adf::kernel diffVSKernel;
};
} // namespace L2
} // namespace us
