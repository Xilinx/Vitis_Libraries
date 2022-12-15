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

class mulMM : public adf::graph {
   public:
    mulMM() {
        // Kernel definition
        mulMMKernel = adf::kernel::create(L1::mulMM<float, LENGTH, SPACE_DIMENSION, SIMD_DEPTH>);

        // input and output ports
        input1_mulMM = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_mulMM = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_mulMM = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_mulMM_q(input1_mulMM.out[0], mulMMKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input2_mulMM_q(input2_mulMM.out[0], mulMMKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_mulMM_q(mulMMKernel.out[0], output_mulMM.in[0]);

        // source kernel
        adf::source(mulMMKernel) = "mulMM/mulMM.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulMMKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_mulMM_q) = FIFO_DEPTH;
        adf::fifo_depth(input2_mulMM_q) = FIFO_DEPTH;
        adf::fifo_depth(output_mulMM_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_mulMM;
    adf::input_plio input2_mulMM;
    adf::output_plio output_mulMM;

   private:
    // Kernel declaration
    adf::kernel mulMMKernel;
};
} // namespace L2
} // namespace us
