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

class normAxis1 : public adf::graph {
   public:
    normAxis1() {
        // Kernel definition
        normAxis1Kernel = adf::kernel::create(L1::norm_axis_1<float, LENGTH, 1, SPACE_DIMENSION>);

        // input and output ports
        input1_normAxis1 = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_normAxis1 = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_normAxis1.out[0], normAxis1Kernel.in[0]);
        adf::connect(normAxis1Kernel.out[0], output_normAxis1.in[0]);

        // source kernels
        adf::source(normAxis1Kernel) = "norm_axis_1.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(normAxis1Kernel) = KERNEL_RATIO;

        // config
        adf::dimensions(normAxis1Kernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(normAxis1Kernel.out[0]) = {FRAME_LENGTH};
    }

    adf::input_plio input1_normAxis1;
    adf::output_plio output_normAxis1;

   private:
    // Kernel declaration
    adf::kernel normAxis1Kernel;
};
} // namespace L2
} // namespace us
