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

#define SCALAR_ 1

class lessOrEqualThanS : public adf::graph {
   public:
    lessOrEqualThanS() {
        // Kernel definition
        lessOrEqualThanSKernel =
            adf::kernel::create(L1::lessOrEqualThanS<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH, SCALAR_>);

        // input and output port
        input_lessOrEqualThanS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_lessOrEqualThanS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input_lessOrEqualThanS_q(input_lessOrEqualThanS.out[0],
                                                                             lessOrEqualThanSKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_lessOrEqualThanS_q(lessOrEqualThanSKernel.out[0],
                                                                              output_lessOrEqualThanS.in[0]);

        // source kernels
        adf::source(lessOrEqualThanSKernel) = "lessOrEqualThanS/lessOrEqualThanS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(lessOrEqualThanSKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input_lessOrEqualThanS_q) = FIFO_DEPTH;
        adf::fifo_depth(output_lessOrEqualThanS_q) = FIFO_DEPTH;
    }

    // Input and output ports
    adf::input_plio input_lessOrEqualThanS;
    adf::output_plio output_lessOrEqualThanS;

   private:
    // Kernel declaration
    adf::kernel lessOrEqualThanSKernel;
};
} // namespace L2
} // namespace us