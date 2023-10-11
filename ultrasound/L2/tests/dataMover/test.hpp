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

class dataMover : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_dataMover;
    adf::output_plio output_dataMover;

    dataMover() {
        // input & output plio
        input_dataMover = adf::input_plio::create("input_dataMover", adf::plio_32_bits, "data/input.txt");
        output_dataMover = adf::output_plio::create("output_dataMover", adf::plio_32_bits, "data/output.txt");

        // kernel definition
        dataMoverKernel = adf::kernel::create(L1::dataMover<float, LENGTH, SIMD_DEPTH>);
        adf::source(dataMoverKernel) = "dataMover.cpp";

        // connections
        adf::connect(input_dataMover.out[0], dataMoverKernel.in[0]);
        adf::connect(dataMoverKernel.out[0], output_dataMover.in[0]);

        // config
        adf::runtime<adf::ratio>(dataMoverKernel) = KERNEL_RATIO;
        adf::dimensions(dataMoverKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(dataMoverKernel.out[0]) = {FRAME_LENGTH * SIMD_DEPTH};
    }

   private:
    adf::kernel dataMoverKernel;
};
} // namespace L2
} // namespace us
