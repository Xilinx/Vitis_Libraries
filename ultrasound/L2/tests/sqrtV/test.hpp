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

class sqrtV : public adf::graph {
   public:
    sqrtV() {
        // Kernel definition
        sqrtVKernel = adf::kernel::create(L1::sqrtV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_sqrtV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_sqrtV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > input1_sqrtV_q(input1_sqrtV.out[0], sqrtVKernel.in[0]);
        adf::connect<adf::stream> output_sqrtV_q(sqrtVKernel.out[0], output_sqrtV.in[0]);

        // source kernel
        adf::source(sqrtVKernel) = "sqrtV/sqrtV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(sqrtVKernel) = KERNEL_RATIO;

        // setting FIFO depth
        adf::fifo_depth(input1_sqrtV_q) = FIFO_DEPTH;
        adf::fifo_depth(output_sqrtV_q) = FIFO_DEPTH;
    }

    adf::input_plio input1_sqrtV;
    adf::output_plio output_sqrtV;

   private:
    // kernel declaration
    adf::kernel sqrtVKernel;
};
} // namespace L2
} // namespace us
