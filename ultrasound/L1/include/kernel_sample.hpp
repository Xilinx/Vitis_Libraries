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
#ifndef __KERNEL_SAMPLE_HPP__
#define __KERNEL_SAMPLE_HPP__

#include <adf.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

namespace us {
namespace L1 {

template <class T>
struct para_sample_t {
    T xdc_x;
    T xdc_z;
    T inv_speed_of_sound;
    T freq_sampling;
    T rf_dim;
    int32 iter_line;
    int32 iter_element;
    int32 iter_seg;
    int32 num_line;
    int32 num_element;
    int32 num_seg;
    int32 num_dep_seg;
    void print() {
        printf("sample: <%d/%d lines, %d/%d elements, %d/%d segments>\n", iter_line, num_line, iter_element,
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

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t>
void load_sample_rtp(para_sample_t<T>* p_const,
                     const T (&lp_rfdim)[NUM_LINE_t],
                     const T (&lp_elem)[NUM_ELEMENT_t * 4]) {
    p_const->rf_dim = lp_rfdim[p_const->iter_line];
    p_const->xdc_x = lp_elem[p_const->iter_element * 4 + 0];
    p_const->xdc_z = lp_elem[p_const->iter_element * 4 + 2];
}

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int VECDIM_sample_t,
          int LEN_IN_sample_t,
          int LEN_OUT_sample_t,
          int LEN32b_PARA_sample_t>
void __attribute__((noinline))
kfun_genLineSample_wrapper(adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_sample, // output
                           adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_inside, // output
                           adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_x,       // input
                           adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_z,       // input
                           adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_delay,       // input
                           const T (&para_const)[LEN32b_PARA_sample_t],
                           const T (&para_rfdim)[NUM_LINE_t],
                           const T (&para_elem)[NUM_ELEMENT_t * 4]);

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int VECDIM_sample_t,
          int LEN_IN_sample_t,
          int LEN_OUT_sample_t,
          int LEN32b_PARA_sample_t>
void __attribute__((noinline)) kfun_genLineSample_wrapper_shell(
    adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_sample, // output
    adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_inside, // output
    adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_x,       // input
    adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_z,       // input
    adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_delay,       // input
    const T (&para_const)[LEN32b_PARA_sample_t],
    const T (&para_rfdim)[NUM_LINE_t],
    const T (&para_elem)[NUM_ELEMENT_t * 4]);
}
}

#endif // __SAMPLE_PARAMS_H__