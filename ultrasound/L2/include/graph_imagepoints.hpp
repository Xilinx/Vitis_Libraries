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
#ifndef _IMAGE_POINTS_HPP_
#define _IMAGE_POINTS_HPP_

#include "kernels.hpp"
#include "kernel_imagepoints.hpp"

namespace us {
namespace L2 {

using namespace adf;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LEN_OUT_img_t,
          int LEN32b_PARA_img_t>
class graph_img_wrapper : public graph {
   public:
    port<input> p_para_const;
    port<input> p_para_start;
    port<output> dataout1;

    kernel k_img;

    graph_img_wrapper() {
// Initialize the kernels

#ifdef _USING_SHELL_
        k_img = kernel::create(us::L1::kfun_img_1d_shell<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t,
                                                         VECDIM_img_t, LEN_OUT_img_t, LEN32b_PARA_img_t>);
#else
        k_img = kernel::create(us::L1::kfun_img_1d<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_img_t,
                                                   LEN_OUT_img_t, LEN32b_PARA_img_t>);
#endif

        runtime<ratio>(k_img) = 0.5;
        source(k_img) = "kernel_imagepoints/kernel_imagepoints.cpp";

        connect<parameter>(p_para_const, async(k_img.in[0]));
        connect<parameter>(p_para_start, async(k_img.in[1]));
        connect<>(k_img.out[0], dataout1);
        // repetition_count(k_img) = 2;
    }
};

} // namespace L2
} // namespace us

#endif