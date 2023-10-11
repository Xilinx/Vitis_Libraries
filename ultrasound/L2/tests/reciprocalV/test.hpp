/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include <adf.h>

#define FRAME_LENGTH 128

namespace us {
namespace L2 {

class reciprocalV : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_reciprocalV;
    adf::output_plio output_reciprocalV;

    reciprocalV() {
        // input and output port
        input_reciprocalV = adf::input_plio::create("input_reciprocalV", adf::plio_32_bits, "data/input1.txt");
        output_reciprocalV = adf::output_plio::create("output_reciprocalV", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        reciprocalVKernel = adf::kernel::create(L1::reciprocalV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);
        adf::source(reciprocalVKernel) = "reciprocalV.cpp";

        // connections
        adf::connect(input_reciprocalV.out[0], reciprocalVKernel.in[0]);
        adf::connect(reciprocalVKernel.out[0], output_reciprocalV.in[0]);

        // setting kernel ratio
        adf::runtime<adf::ratio>(reciprocalVKernel) = KERNEL_RATIO;
        adf::dimensions(reciprocalVKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(reciprocalVKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    adf::kernel reciprocalVKernel;
};
} // namespace L2
} // namespace us
