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

#define FRAME_LENGTH 128

namespace us {
namespace L2 {

class outer : public adf::graph {
   public:
    // input and output ports
    adf::input_plio input1_outer;
    adf::input_plio input2_outer;
    adf::output_plio output_outer;

    outer() {
        // Input and output port
        input1_outer = adf::input_plio::create("input1_outer", adf::plio_32_bits, "data/input1.txt");
        input2_outer = adf::input_plio::create("input2_outer", adf::plio_32_bits, "data/input2.txt");
        output_outer = adf::output_plio::create("output_outer", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        outerKernel = adf::kernel::create(L1::outer<float, LENGTH, SPACE_DIMENSION, SPACE_DIMENSION>);

        // Source kernel
        adf::source(outerKernel) = "outer.cpp";

        // Connections
        adf::connect(input1_outer.out[0], outerKernel.in[0]);
        adf::connect(input2_outer.out[0], outerKernel.in[1]);
        adf::connect(outerKernel.out[0], output_outer.in[0]);

        // Setting kernel ratio
        adf::runtime<adf::ratio>(outerKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::dimensions(outerKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(outerKernel.in[1]) = {FRAME_LENGTH};
        adf::dimensions(outerKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    // Kernel declaration
    adf::kernel outerKernel;
};
} // namespace L2
} // namespace us
