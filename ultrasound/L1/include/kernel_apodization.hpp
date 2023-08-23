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

#ifndef __KERNEL_APODIZATION_H__
#define __KERNEL_APODIZATION_H__

#include <adf.h>
#include <kernels.hpp>

namespace us {
namespace L1 {

// declare param
template <class T>
struct para_Apodization {
    int iter_line;
    int iter_element;
    int iter_seg;
    int num_line;
    int num_element;
    int num_seg;
    int num_dep_seg; // = num_depth / num_seg = 512
    T f_num;
    T tileVApo_x;
    T tileVApo_z;
    T ref_point_x;
    T ref_point_z;
    void print() {
        printf("apo: <%d/%d lines, %d/%d elements, %d/%d segments>\n", iter_line, num_line, iter_element, num_element,
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

//----------------------------------------------------
// brief
// apodization_preprocess
//---1.top function to switch between vector and scaler version
//---2.top function to test top graph connection only, no calculation
// apodization_main_precess
//---3.top function to switch between vector and scaler version
//---4.top function to test top graph connection only, no calculation
//----------------------------------------------------

template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_pre(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
                          adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
                          adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
                          const int (&para_const)[APODI_PRE_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_pre_shell(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
                                adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
                                adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
                                const int (&para_const)[APODI_PRE_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void __attribute__((noinline))
kfun_apodization_main(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
                      adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
                      adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
                      const int (&para_const)[APODI_PRE_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_main_shell(
    // output_stream<T>* p_apodization_out,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
    adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
    adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
    const int (&para_const)[APODI_PRE_LEN32b_PARA]);

} // namespace L1
} // namespace us

#endif //__KERNEL_APODIZATION_H__
