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

class divVS : public adf::graph {
   public:
    divVS() {
        // Kernel definition
        divVSKernel = adf::kernel::create(L1::divVSSpeedOfSound<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_divVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_divVS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_divVS_q(input1_divVS.out[0], divVSKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_divVS_q(divVSKernel.out[0], output_divVS.in[0]);

        // source kernel
        adf::source(divVSKernel) = "divVS/divVS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(divVSKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_divVS_q) = FIFO_DEPTH;
        adf::fifo_depth(output_divVS_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_divVS;
    adf::output_plio output_divVS;

   private:
    // Kernel declaration
    adf::kernel divVSKernel;
};
} // namespace L2
} // namespace us
