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

class cosV : public adf::graph {
   public:
    cosV() {
        // Kernel definition
        cosVKernel = adf::kernel::create(L1::cosV<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);

        // Input and output port
        input_cosV = adf::input_plio::create(adf::plio_32_bits, "data/input.txt");
        output_cosV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // cosV
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input_cosV_q(input_cosV.out[0], cosVKernel.in[0]);

        // cosV result
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_cosV_q(cosVKernel.out[0], output_cosV.in[0]);

        adf::source(cosVKernel) = "cosV/cosV.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(cosVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input_cosV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_cosV_q) = FIFO_DEPTH;
    }

    // Input and output ports
    adf::input_plio input_cosV;
    adf::output_plio output_cosV;

   private:
    adf::kernel cosVKernel;
};
} // namespace L2
} // namespace us
