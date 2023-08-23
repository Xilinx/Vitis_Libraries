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
#ifndef __KERNEL_DELAY_HPP__
#define __KERNEL_DELAY_HPP__

#include <adf.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

namespace us {
namespace L1 {

template <class T>
struct para_delay_t {
    T tx_ref_point_x;
    T tx_ref_point_z;
    T tileVApo_x;
    T tileVApo_z;
    T focal_point_x;
    T focal_point_z;
    T t_start;
    T tx_delay_distance;
    T tx_delay_distance_;
    T inverse_speed_of_sound;
    int32 iter_line;
    int32 iter_element;
    int32 iter_seg;
    int32 num_line;
    int32 num_element;
    int32 num_seg;
    int32 num_dep_seg; // = num_depth / num_seg = 512
    void print() {
        printf("delay: <%d/%d lines, %d/%d elements, %d/%d segments>\n", iter_line, num_line, iter_element, num_element,
               iter_seg, num_seg);
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

template <class T, int NUM_LINE_t>
void load_delay_rtp(para_delay_t<T>* p_const, const T (&lp)[NUM_LINE_t]) {
    p_const->t_start = lp[p_const->iter_line];
}

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
void __attribute__((noinline))
kfun_UpdatingDelay_line_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
                                adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
                                adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z,
                                const T (&para_const)[LEN32b_PARA_delay_t],
                                const T (&para_t_start)[NUM_LINE_t]);

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
void __attribute__((noinline))
kfun_UpdatingDelay_line_wrapper_shell(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
                                      adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
                                      adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z,
                                      const T (&para_const)[LEN32b_PARA_delay_t],
                                      const T (&para_t_start)[NUM_LINE_t]);
}
}

#endif // __DELAY_PARAMS_H__