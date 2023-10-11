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

template <unsigned int DIM_VECTOR_ = LENGTH>
class tileV : public adf::graph {
   public:
    adf::output_plio output_tileV;

    tileV() {
        // input and output port
        output_tileV = adf::output_plio::create("output_tileV", adf::plio_32_bits, "data/output.txt");

        // Kernel definition
        tileVKernel = adf::kernel::create(L1::tileVApo<float, LENGTH, INCREMENT_VECTOR, SPACE_DIMENSION>);

        // source kernel
        adf::source(tileVKernel) = "tileV.cpp";

        // connections
        adf::connect(tileVKernel.out[0], output_tileV.in[0]);

        // setting kernel ratio
        adf::runtime<adf::ratio>(tileVKernel) = KERNEL_RATIO;

        //	Setting frame depth
        adf::dimensions(tileVKernel.out[0]) = {DIM_VECTOR_};
    }

   private:
    // Kernel declaration
    adf::kernel tileVKernel;
};
} // namespace L2
} // namespace us
