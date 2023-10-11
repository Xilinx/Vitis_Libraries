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
#define sumMM_DIM 32

namespace us {
namespace L2 {

class sumMM : public adf::graph {
   public:
    adf::input_plio input1_sumMM;
    adf::input_plio input2_sumMM;
    adf::output_plio output_sumMM;

    sumMM() {
        // input and output port
        input1_sumMM = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        input2_sumMM = adf::input_plio::create(adf::plio_32_bits, "data/input2.txt");
        output_sumMM = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        sumMMKernel = adf::kernel::create(L1::sumMM<float, LENGTH, SPACE_DIMENSION, SIMD_DEPTH>);

        // source kernel
        adf::source(sumMMKernel) = "sumMM.cpp";

        // connections
        adf::connect(input1_sumMM.out[0], sumMMKernel.in[0]);
        adf::connect(input2_sumMM.out[0], sumMMKernel.in[1]);
        adf::connect(sumMMKernel.out[0], output_sumMM.in[0]);

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumMMKernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(sumMMKernel.in[0]) = {sumMM_DIM};
        adf::dimensions(sumMMKernel.in[1]) = {sumMM_DIM};
        adf::dimensions(sumMMKernel.out[0]) = {sumMM_DIM};
    }

   private:
    // Kernel declaration
    adf::kernel sumMMKernel;
};
} // namespace L2
} // namespace us
