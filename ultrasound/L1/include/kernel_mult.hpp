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
#ifndef __KERNEL_MULT_HPP__
#define __KERNEL_MULT_HPP__

#include <adf.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

namespace us {
namespace L1 {

template <class T, int NUM_DEP_SEG_t>
struct para_mult_local_t {
    T local[NUM_DEP_SEG_t];
};

template <class T>
struct para_mult_t {
    int32 iter_line;
    int32 iter_element;
    int32 iter_seg;
    int32 num_line;
    int32 num_element;
    int32 num_seg;
    int32 num_dep_seg;
    int32 mult_id;

    void print() {
        printf("mult-%d: <%d/%d lines, %d/%d elements, %d/%d segments>\n", mult_id, iter_line, num_line, iter_element,
               num_element, iter_seg, num_seg);
    }
    void update() {
        if (iter_seg != num_seg - 1)
            iter_seg++;
        else {
            iter_seg = 0;
            if (iter_element != num_element - 1)
                iter_element++;
            else {
                iter_element = 0;
                iter_line++;
            }
        }
    }
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t,
          int IS_END_t>
void kfun_mult_shell(const T (&para_const)[LEN32b_PARA_mult_t],
                     const T (&local_data)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
                     input_stream<T>* p_in_interp,
                     input_stream<T>* p_in_apod,
                     adf::output_buffer<T, adf::extents<LEN_OUT_mult_t> >& __restrict out_mult);

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t>
void __attribute__((noinline)) kfun_mult_pre(const T (&para_const)[LEN32b_PARA_mult_t],
                                             adf::input_buffer<T, adf::extents<LEN_IN_mult_t> >& __restrict in_interp,
                                             adf::input_buffer<T, adf::extents<LEN_IN_mult_t / 4> >& __restrict in_apod,
                                             output_cascade<accfloat>* p_out_cascade);

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t>
void __attribute__((noinline)) kfun_mult_cascade(const T (&para_const)[LEN32b_PARA_mult_t],
                                                 const T (&local_data_0)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
                                                 const T (&local_data_1)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
                                                 input_cascade<accfloat>* p_in_cascade,
                                                 output_cascade<accfloat>* p_out_cascade,
                                                 output_stream<T>* p_out_mult);

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t,
          int IS_END_t>
void __attribute__((noinline)) kfun_mult_cascade(const T (&para_const)[LEN32b_PARA_mult_t],
                                                 const T (&local_data_0)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
                                                 const T (&local_data_1)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
                                                 input_cascade<accfloat>* p_in_cascade,
                                                 output_stream<T>* p_out_mult);
}
}

#endif // __KERNEL_MULT_H__