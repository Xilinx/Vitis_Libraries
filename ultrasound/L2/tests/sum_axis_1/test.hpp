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

namespace us {
namespace L2 {

template <unsigned int DIM_VECTOR_ = LENGTH, unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)>
class sumAxis1 : public adf::graph {
   public:
    adf::input_plio input1_sumAxis1;
    adf::output_plio output_sumAxis1;

    sumAxis1() {
        // input and output port
        input1_sumAxis1 = adf::input_plio::create(adf::plio_32_bits, "data/input1.txt");
        output_sumAxis1 = adf::output_plio::create(adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        sumAxis1Kernel = adf::kernel::create(L1::sum_axis_1<float, LENGTH, 1, SPACE_DIMENSION>);

        // connections
        adf::connect(input1_sumAxis1.out[0], sumAxis1Kernel.in[0]);
        adf::connect(sumAxis1Kernel.out[0], output_sumAxis1.in[0]);

        // source kernel
        adf::source(sumAxis1Kernel) = "sum_axis_1.cpp";

        // setting kernel ratio
        adf::runtime<adf::ratio>(sumAxis1Kernel) = KERNEL_RATIO;

        // Config
        adf::dimensions(sumAxis1Kernel.in[0]) = {DIM_VECTOR_};
        adf::dimensions(sumAxis1Kernel.out[0]) = {DIM_MATRIX_};
    }

   private:
    // Kernel declaration
    adf::kernel sumAxis1Kernel;
};
} // namespace L2
} // namespace us
