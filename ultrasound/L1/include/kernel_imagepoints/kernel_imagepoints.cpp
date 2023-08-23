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

#include "kernels.hpp"
#include "kernel_imagepoints.hpp"
#include "aie_api/operators.hpp"

namespace us {
namespace L1 {

using namespace aie;
using namespace adf;
using namespace aie::operators;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LEN_OUT_img_t,
          int LEN32b_PARA_img_t>
void kfun_img_1d_shell(const T (&para_const)[LEN32b_PARA_img_t],
                       const T (&para_start)[NUM_LINE_t],
                       output_buffer<T, adf::extents<LEN_OUT_img_t> >& out) {
    para_img_t<T>* p_const = (para_img_t<T>*)para_const;
    p_const->print();
    p_const->update();
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LEN_OUT_img_t,
          int LEN32b_PARA_img_t>
void kfun_img_1d(const T (&para_const)[LEN32b_PARA_img_t],
                 const T (&para_start)[NUM_LINE_t],
                 output_buffer<T, adf::extents<LEN_OUT_img_t> >& out) {
    para_img_t<T>* p_const = (para_img_t<T>*)para_const; // convert array to structure for easy use
    float step = p_const->step;                          // dz or dx
    int32 iter_line = p_const->iter_line;                // current line number

    // vectorizizing dx or dz and xi or zi
    aie::vector<T, VECDIM_img_t> v_step = aie::broadcast<T, VECDIM_img_t>(p_const->step * VECDIM_img_t);
    aie::vector<T, VECDIM_img_t> v_base;
    int iter_seg = p_const->iter_seg;
    int base_i = LEN_OUT_img_t * iter_seg;
    for (int i = 0; i < VECDIM_img_t; i++) v_base[i] = para_start[iter_line] + step * (i + base_i);

    auto pout = aie::begin_vector<VECDIM_img_t>(out);
    for (int i = 0; i < LEN_OUT_img_t / VECDIM_img_t; i++) chess_prepare_for_pipelining {
            *pout++ = v_base;
            v_base = v_base + v_step; // aie::add(v_base, v_step);
        }
    p_const->print();
    p_const->update();
};

} // namespace L1
} // namespace us
