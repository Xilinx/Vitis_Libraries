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

class squareV : public adf::graph {
   public:
    squareV() {
        // Kernel definition
        squareVKernel = adf::kernel::create(L1::squareV<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_squareV = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_squareV = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_squareV.out[0], squareVKernel.in[0]);
        adf::connect(squareVKernel.out[0], output_squareV.in[0]);

        // source kernel
        adf::source(squareVKernel) = "squareV.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(squareVKernel) = KERNEL_RATIO;

        // setting FIFO depth
        adf::dimensions(squareVKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(squareVKernel.out[0]) = {FRAME_LENGTH};
    }

    adf::input_plio input1_squareV;
    adf::output_plio output_squareV;

   private:
    // Kernel declaration
    adf::kernel squareVKernel;
};
} // namespace L2
} // namespace us
