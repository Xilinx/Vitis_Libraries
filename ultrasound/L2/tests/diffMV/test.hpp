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

class diffMV : public adf::graph {
   public:
    diffMV() {
        // Kernel definition
        diffMVKernel = adf::kernel::create(L1::diffMV<float, LENGTH, INCREMENT_MATRIX, SIMD_DEPTH>);

        // input and output port
        input1_diffMV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_diffMV = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_diffMV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::stream> input1_diffMV_q(input1_diffMV.out[0], diffMVKernel.in[0]);
        adf::connect<adf::stream> input2_diffMV_q(input2_diffMV.out[0], diffMVKernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_diffMV_q(diffMVKernel.out[0], output_diffMV.in[0]);

        // kernel source
        adf::source(diffMVKernel) = "diffMV/diffMV.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(diffMVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input1_diffMV_q) = FIFO_DEPTH;
        adf::fifo_depth(input2_diffMV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_diffMV_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_diffMV;
    adf::input_plio input2_diffMV;
    adf::output_plio output_diffMV;

   private:
    // Kernel declaration
    adf::kernel diffMVKernel;
};
} // namespace L2
} // namespace us