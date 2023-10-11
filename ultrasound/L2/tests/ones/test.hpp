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

#define FRAME_LENGTH 128

namespace us {
namespace L2 {

class ones : public adf::graph {
   public:
    // output ports
    adf::output_plio output_ones;

    ones() {
        // input and output port
        output_ones = adf::output_plio::create("output_ones", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        onesKernel = adf::kernel::create(L1::ones<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);

        // source kernel
        adf::source(onesKernel) = "ones.cpp";

        // result
        adf::connect(onesKernel.out[0], output_ones.in[0]);

        // Setting kernel ratio
        adf::runtime<adf::ratio>(onesKernel) = KERNEL_RATIO;
        adf::dimensions(onesKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    // Kernel declaration
    adf::kernel onesKernel;
};
} // namespace L2
} // namespace us
