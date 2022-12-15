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

class mulVV : public adf::graph {
   public:
    mulVV() {
        // Kernel definition
        mulVVKernel = adf::kernel::create(L1::mulVV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_mulVV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_mulVV = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_mulVV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_mulVV_q(input1_mulVV.out[0], mulVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input2_mulVV_q(input2_mulVV.out[0], mulVVKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_mulVV_q(mulVVKernel.out[0], output_mulVV.in[0]);

        // source kernel
        adf::source(mulVVKernel) = "mulVV/mulVV.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulVVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_mulVV_q) = FIFO_DEPTH;
        adf::fifo_depth(input2_mulVV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_mulVV_q) = FIFO_DEPTH;
    }

    // Input and output ports
    adf::input_plio input1_mulVV;
    adf::input_plio input2_mulVV;
    adf::output_plio output_mulVV;

   private:
    // Kernel declaration
    adf::kernel mulVVKernel;
};
} // namespace L2
} // namespace us
