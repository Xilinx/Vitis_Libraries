/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _FOCUSING_HPP_
#define _FOCUSING_HPP_

#include "kernels.hpp"
#include "kernel_focusing.hpp"
#include <adf.h>

namespace us {
namespace L2 {

using namespace adf;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
class graph_foc_wrapper : public adf::graph {
   public:
    port<input> p_para_const;
    port<input> p_para_pos;
    port<output> dataout1;

    kernel k_foc;

    graph_foc_wrapper() {
// Initialize the kernels
#ifdef _USING_SHELL_
        k_foc = kernel::create(us::L1::kfun_foc_shell<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t,
                                                      VECDIM_foc_t, LEN32b_PARA_foc_t>);
#else
        k_foc = kernel::create(
            us::L1::kfun_foc<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t>);
#endif
        source(k_foc) = "kernel_focusing/kernel_focusing.cpp";

        runtime<ratio>(k_foc) = 0.9;

        // Declare the sources for the two kernels

        connect<parameter>(p_para_const, async(k_foc.in[0]));
        connect<parameter>(p_para_pos, async(k_foc.in[1]));
        connect<>(k_foc.out[0], dataout1);
        // repetition_count(k_foc) = 2;
    }
};

} // namespace L2
} // namespace us

#endif
