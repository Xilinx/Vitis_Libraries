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

class sign : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_sign;
    adf::output_plio output_sign;

    sign() {
        // input & output plio
        input_sign = adf::input_plio::create("input_sign", adf::plio_32_bits, "data/input.txt");
        output_sign = adf::output_plio::create("output_sign", adf::plio_32_bits, "data/output.txt");

        // kernel definition
        signKernel = adf::kernel::create(L1::sign<float, LENGTH, INCREMENT_VECTOR, SIMD_DEPTH>);
        adf::source(signKernel) = "sign.cpp";

        // connections
        adf::connect(input_sign.out[0], signKernel.in[0]);
        adf::connect(signKernel.out[0], output_sign.in[0]);

        // config
        adf::runtime<adf::ratio>(signKernel) = KERNEL_RATIO;
        adf::dimensions(signKernel.in[0]) = {FRAME_LENGTH};
        adf::dimensions(signKernel.out[0]) = {FRAME_LENGTH};
    }

   private:
    adf::kernel signKernel;
};
} // namespace L2
} // namespace us
