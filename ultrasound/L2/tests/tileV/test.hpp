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

class tileV : public adf::graph {
   public:
    tileV() {
        // Kernel definition
        tileVKernel = adf::kernel::create(L1::tileVApo<float, LENGTH, INCREMENT_VECTOR, SPACE_DIMENSION>);

        // input and output port
        output_tileV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::window<WIN_SIZE_MATRIX> > output_tileV_q(tileVKernel.out[0], output_tileV.in[0]);

        // source kernel
        adf::source(tileVKernel) = "tileV/tileV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(tileVKernel) = KERNEL_RATIO;

        //	Setting FIFO depth
        adf::fifo_depth(output_tileV_q) = FIFO_DEPTH;
    }

    adf::output_plio output_tileV;

   private:
    // Kernel declaration
    adf::kernel tileVKernel;
};
} // namespace L2
} // namespace us
