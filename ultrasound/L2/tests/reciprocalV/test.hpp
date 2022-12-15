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

class reciprocalV : public adf::graph {
   public:
    reciprocalV() {
        // Kernel definition
        reciprocalVKernel = adf::kernel::create(L1::reciprocalV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input_reciprocalV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_reciprocalV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input_reciprocalV_q(input_reciprocalV.out[0],
                                                                        reciprocalVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_reciprocalV_q(reciprocalVKernel.out[0],
                                                                         output_reciprocalV.in[0]);

        // source kernel
        adf::source(reciprocalVKernel) = "reciprocalV/reciprocalV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(reciprocalVKernel) = KERNEL_RATIO;

        // setting FIFO depth
        adf::fifo_depth(input_reciprocalV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_reciprocalV_q) = FIFO_DEPTH;
    }

    // Input and output ports
    adf::input_plio input_reciprocalV;
    adf::output_plio output_reciprocalV;

   private:
    // Kernel declaration
    adf::kernel reciprocalVKernel;
};
} // namespace L2
} // namespace us
