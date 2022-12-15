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

#include <adf.h>
#include "kernels.hpp"

namespace us {
namespace L2 {

class absV : public adf::graph {
   public:
    absV() {
        // Kernel definition
        absVKernel = adf::kernel::create(L1::absV<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);

        // input & output port
        input_absV = adf::input_plio::create(adf::plio_32_bits, "data/input.txt");
        output_absV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input_absV_q(input_absV.out[0], absVKernel.in[0]);
        adf::connect<adf::stream> output_absV_q(absVKernel.out[0], output_absV.in[0]);

        // kernel source
        adf::source(absVKernel) = "absV/absV.cpp";

        // setting runtime ratio
        adf::runtime<adf::ratio>(absVKernel) = KERNEL_RATIO;

        // Setting FIFO depth
        adf::fifo_depth(input_absV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_absV_q) = FIFO_DEPTH;
    }

    // Input and output ports
    adf::input_plio input_absV;
    adf::output_plio output_absV;

   private:
    adf::kernel absVKernel;
};
} // namespace L2
} // namespace us
