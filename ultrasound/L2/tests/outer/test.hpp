/*
 * Copyright 2021 Xilinx, Inc.
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

class outer : public adf::graph {
   public:
    outer() {
        // Kernel definition
        outerKernel = adf::kernel::create(L1::outer<float, LENGTH, SPACE_DIMENSION, SPACE_DIMENSION>);

        // Input and output port
        input1_outer = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_outer = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_outer = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // Connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_outer_q(input1_outer.out[0], outerKernel.in[0]);
        adf::connect<adf::stream> input2_outer_q(input2_outer.out[0], outerKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_outer_q(outerKernel.out[0], output_outer.in[0]);

        // Source kernel
        adf::source(outerKernel) = "outer/outer.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(outerKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_outer_q) = FIFO_DEPTH;
        adf::fifo_depth(input2_outer_q) = FIFO_DEPTH;
        adf::fifo_depth(output_outer_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_outer;
    adf::input_plio input2_outer;
    adf::output_plio output_outer;

   private:
    // Kernel declaration
    adf::kernel outerKernel;
};
} // namespace L2
} // namespace us
