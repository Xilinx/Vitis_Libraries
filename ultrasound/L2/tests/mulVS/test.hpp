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

class mulVS : public adf::graph {
   public:
    mulVS() {
        // Kernel definition
        mulVSKernel = adf::kernel::create(L1::mulVS<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_mulVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_mulVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_mulVS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_mulVS_q(input1_mulVS.out[0], mulVSKernel.in[0]);
        adf::connect<adf::stream> input2_mulVS_q(input2_mulVS.out[0], mulVSKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_mulVS_q(mulVSKernel.out[0], output_mulVS.in[0]);

        // source kernel
        adf::source(mulVSKernel) = "mulVS/mulVS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulVSKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_mulVS_q) = FIFO_DEPTH;
        adf::fifo_depth(output_mulVS_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_mulVS;
    adf::input_plio input2_mulVS;
    adf::output_plio output_mulVS;

   private:
    // Kernel declaration
    adf::kernel mulVSKernel;
};
} // namespace L2
} // namespace us
