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
class mulVS : public adf::graph {
   public:
    // input and output ports
    adf::input_plio input1_mulVS;
    adf::input_plio input2_mulVS;
    adf::output_plio output_mulVS;

    mulVS() {
        // Kernel definition
        mulVSKernel = adf::kernel::create(L1::mulVS<float, LENGTH, SIMD_DEPTH, SIMD_DEPTH>);

        // input and output port
        input1_mulVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_mulVS = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_mulVS = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect(input1_mulVS.out[0], mulVSKernel.in[0]);
        adf::connect(input2_mulVS.out[0], mulVSKernel.in[1]);
        adf::connect(mulVSKernel.out[0], output_mulVS.in[0]);

        // source kernel
        adf::source(mulVSKernel) = "mulVS.cpp";

        // Setting kernel ratio
        adf::runtime<adf::ratio>(mulVSKernel) = KERNEL_RATIO;

        // config
        adf::dimensions(mulVSKernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(mulVSKernel.in[1]) = {DIM_VECTOR_};
        adf::dimensions(mulVSKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel mulVSKernel;
};
} // namespace L2
} // namespace us
