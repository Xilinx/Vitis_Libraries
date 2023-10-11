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

namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH>
class divVS : public adf::graph {
   public:
    adf::input_plio input1_divVS;
    adf::output_plio output_divVS;

    divVS() {
        // input and output port
        input1_divVS = adf::input_plio::create("input1_divVS", adf::plio_32_bits, "data/input1.txt");
        output_divVS = adf::output_plio::create("output_divVS", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        divVSKernel = adf::kernel::create(L1::divVSSpeedOfSound<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);
        adf::source(divVSKernel) = "divVS.cpp";

        // connections
        adf::connect(input1_divVS.out[0], divVSKernel.in[0]);
        adf::connect(divVSKernel.out[0], output_divVS.in[0]);

        // Setting kernel ratio
        adf::runtime<adf::ratio>(divVSKernel) = KERNEL_RATIO;
        adf::dimensions(divVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(divVSKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    adf::kernel divVSKernel;
};
} // namespace L2
} // namespace us
