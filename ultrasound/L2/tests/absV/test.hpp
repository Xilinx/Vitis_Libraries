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

#define FRAME_LENGTH 32

namespace us {
namespace L2 {

class absV : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_absV;
    adf::output_plio output_absV;

    absV() {
        // input & output plio
        input_absV = adf::input_plio::create("input_absV", adf::plio_32_bits, "data/input.txt");
        output_absV = adf::output_plio::create("output_absV", adf::plio_32_bits, "data/output.txt");

        // kernel definition
        absVKernel = adf::kernel::create(L1::absV<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        adf::source(absVKernel) = "absV.cpp";

        // connections
        adf::connect(input_absV.out[0], absVKernel.in[0]);
        adf::connect(absVKernel.out[0], output_absV.in[0]);

        // config
        adf::runtime<adf::ratio>(absVKernel) = KERNEL_RATIO;
        adf::dimensions(absVKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(absVKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    adf::kernel absVKernel;
};
} // namespace L2
} // namespace us
