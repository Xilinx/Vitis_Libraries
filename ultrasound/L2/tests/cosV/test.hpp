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

#define FRAME_LENGTH 32

namespace us {
namespace L2 {

class cosV : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_cosV;
    adf::output_plio output_cosV;

    cosV() {
        // Input and output plio port
        input_cosV = adf::input_plio::create("input_cosV", adf::plio_32_bits, "data/input.txt");
        output_cosV = adf::output_plio::create("output_cosV", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        cosVKernel = adf::kernel::create(L1::cosV<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        adf::source(cosVKernel) = "cosV.cpp";

        // connections
        adf::connect(input_cosV.out[0], cosVKernel.in[0]);
        adf::connect(cosVKernel.out[0], output_cosV.in[0]);

        // config
        adf::runtime<adf::ratio>(cosVKernel) = KERNEL_RATIO;
        adf::dimensions(cosVKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(cosVKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    adf::kernel cosVKernel;
};
} // namespace L2
} // namespace us
