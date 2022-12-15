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

class sumVS : public adf::graph {
   public:
    sumVS() {
        // Kernel definition
        sumVSKernel = adf::kernel::create(L1::sumVSStream<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_sumVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_sumVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_sumVS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_sumVS_q(input1_sumVS.out[0], sumVSKernel.in[0]);
        adf::connect<adf::stream> input2_sumVS_q(input2_sumVS.out[0], sumVSKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_sumVS_q(sumVSKernel.out[0], output_sumVS.in[0]);

        // source kernel
        adf::source(sumVSKernel) = "sumVS/sumVS.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumVSKernel) = KERNEL_RATIO;

        // setting FIFO depth
        adf::fifo_depth(input1_sumVS_q) = FIFO_DEPTH;
        adf::fifo_depth(output_sumVS_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_sumVS;
    adf::input_plio input2_sumVS;
    adf::output_plio output_sumVS;

   private:
    // Kernel declaration
    adf::kernel sumVSKernel;
};
} // namespace L2
} // namespace us
