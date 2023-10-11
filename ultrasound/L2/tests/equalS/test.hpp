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

#include <adf.h>
#include "kernels.hpp"

namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH>
class equalS : public adf::graph {
   public:
    // Input and output ports
    adf::input_plio input_equalS;
    adf::output_plio output_equalS;

    equalS() {
        // Kernel definition
        equalSKernel = adf::kernel::create(L1::equalS<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH, 1>);

        // input and output port
        input_equalS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_equalS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input_equalS.out[0], equalSKernel.in[0]);
        adf::connect(equalSKernel.out[0], output_equalS.in[0]);

        // source kernel
        adf::source(equalSKernel) = "equalS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(equalSKernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(equalSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(equalSKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    //		Kernel declaration
    adf::kernel equalSKernel;
};
} // namespace L2
} // namespace us
