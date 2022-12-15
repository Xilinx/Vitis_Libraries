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

class sumVV : public adf::graph {
   public:
    sumVV() {
        // Kernel definition
        sumVVKernel = adf::kernel::create(L1::sumVV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_sumVV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_sumVV = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_sumVV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_sumVV_q(input1_sumVV.out[0], sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input2_sumVV_q(input2_sumVV.out[0], sumVVKernel.in[1]);

        // result
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_sumVV_q(sumVVKernel.out[0], output_sumVV.in[0]);

        // source kernel
        adf::source(sumVVKernel) = "sumVV/sumVV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumVVKernel) = KERNEL_RATIO;

        // setting FIFO depth
        adf::fifo_depth(input1_sumVV_q) = FIFO_DEPTH;
        adf::fifo_depth(input2_sumVV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_sumVV_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_sumVV;
    adf::input_plio input2_sumVV;
    adf::output_plio output_sumVV;

   private:
    // Kernel declaration
    adf::kernel sumVVKernel;
};
} // namespace L2
} // namespace us
