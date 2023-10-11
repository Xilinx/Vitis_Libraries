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

class sqrtV : public adf::graph {
   public:
    sqrtV() {
        // Kernel definition
        sqrtVKernel = adf::kernel::create(L1::sqrtV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_sqrtV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_sqrtV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_sqrtV.out[0], sqrtVKernel.in[0]);
        adf::connect(sqrtVKernel.out[0], output_sqrtV.in[0]);

        // source kernel
        adf::source(sqrtVKernel) = "sqrtV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(sqrtVKernel) = KERNEL_RATIO;

        // config
        adf::dimensions(sqrtVKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(sqrtVKernel.out[0]) = {FRAME_LENGTH};
    }

    adf::input_plio input1_sqrtV;
    adf::output_plio output_sqrtV;

   private:
    // kernel declaration
    adf::kernel sqrtVKernel;
};
} // namespace L2
} // namespace us
