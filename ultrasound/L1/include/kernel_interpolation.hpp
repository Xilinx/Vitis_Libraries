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

#ifndef __KERNEL_INTERPOLATION_H__
#define __KERNEL_INTERPOLATION_H__

#include <adf.h>
#include <kernels.hpp>

namespace us {
namespace L1 {

// declare param
template <class T>
struct para_Interpolation {
    int iter_line;
    int iter_element;
    int iter_seg;
    int num_line;
    int num_element;
    int num_seg;
    int num_depth;
    int num_dep_seg; // = num_depth / num_seg = 512
    int num_upSamp;
    // int last_addr0;
    // int last_addr1;
    // int last_addr2;
    // int last_ind0;
    // int last_ind1;
    // int last_ind2;
    void print() {
        printf("inp: <%d/%d lines, %d/%d elements, %d/%d segments>\n", iter_line, num_line, iter_element, num_element,
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

template <class T, int NUM_ELEMENT_t>
struct para_interp_local_t {
    T local[NUM_ELEMENT_t];
};

//----------------------------------------------------
// brief
// interpolation
//---1.top function scaler version
//---2.top function to test top graph connection only, no calculation
//---3.top function vecter version
//----------------------------------------------------

//---1.top function scaler version
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void mfun_interpolation_scaler(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,
                               adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                               adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                               adf::input_buffer<T, adf::extents<LEN_RF_IN> >& __restrict p_rfdata_in,
                               const int (&para_const)[INTERP_LEN32b_PARA]);

//---2.top function to test top graph connection only, no calculation
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void kfun_interpolation_shell(
    // output_stream<T>* p_interpolation,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,
    adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
    adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
    adf::input_buffer<T, adf::extents<LEN_RF_IN> >& __restrict p_rfdata_in,
    const int (&para_const)[INTERP_LEN32b_PARA]);

//---3.top function vecter version
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline)) kfun_rfbuf_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_rfbuf_out,

                                                  input_stream<T>* strm_rfdata,
                                                  const T (&local_data)[LEN_OUT],
                                                  const int (&para_const)[INTERP_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline))
kfun_genwin_wrapper2(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_out,
                     adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

                     adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                     const int (&para_const)[INTERP_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline))
kfun_resamp_wrapper2(adf::output_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_out,
                     adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

                     adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_rfbuf_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                     const int (&para_const)[INTERP_LEN32b_PARA]);

template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline))
kfun_interpolation_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,

                           adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_in,
                           adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                           const int (&para_const)[INTERP_LEN32b_PARA]);

} // namespace L1
} // namespace us

#endif // __KERNEL_INTERPOLATION_H__