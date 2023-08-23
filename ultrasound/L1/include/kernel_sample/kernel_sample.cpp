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
#include <adf.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/operators.hpp"
#include "kernel_sample.hpp"

using namespace aie::operators;

namespace us {
namespace L1 {
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
                           const T (&para_elem)[NUM_ELEMENT_t * 4]) {
    para_sample_t<T>* p_const = (para_sample_t<T>*)para_const;
    load_sample_rtp<T, NUM_LINE_t, NUM_ELEMENT_t>(p_const, para_rfdim, para_elem);

    int* p_out_inside = out_inside.data();
    int* p_out_sample = out_sample.data();
    T* p_in_img_x = in_img_x.data();
    T* p_in_img_z = in_img_z.data();
    T* p_in_delay = in_delay.data();

// scalar version
#if 0
    T xdc_x = p_const->xdc_x;
    T xdc_z = p_const->xdc_z;
    T inv_speed_of_sound = p_const->inv_speed_of_sound;
    T freq_sampling = p_const->freq_sampling;
    int rf_dim = (int)(p_const->rf_dim);

    for(int n = 0; n < p_const->num_dep_seg; n++) chess_prepare_for_pipelining{
        T x = p_in_img_x[n] - xdc_x;
        T z = p_in_img_z[n] - xdc_z;
        T rec_delay = aie::sqrt(x*x + z*z) * inv_speed_of_sound;
        T total_delay = rec_delay + p_in_delay[n];

        int t_sample = (int)(total_delay * freq_sampling) + 1;

        p_out_inside[n] = (t_sample >= 1) && (t_sample < rf_dim);
        p_out_sample[n] = t_sample * p_out_inside[n] + 1 - p_out_inside[n];
    }
#else
    // vectorization version
    auto v_xdc_x = aie::broadcast<T, VECDIM_sample_t>(p_const->xdc_x);
    auto v_xdc_z = aie::broadcast<T, VECDIM_sample_t>(p_const->xdc_z);
    auto v_inv_speed_of_sound = aie::broadcast<T, VECDIM_sample_t>(p_const->inv_speed_of_sound);
    auto v_freq_sampling = aie::broadcast<T, VECDIM_sample_t>(p_const->freq_sampling);
    auto v_rf_dim = aie::broadcast<T, VECDIM_sample_t>(p_const->rf_dim);

    auto v_ones = aie::broadcast<T, VECDIM_sample_t>(1.0);
    auto v_zeros = aie::broadcast<T, VECDIM_sample_t>(0.0);

    auto v_ones_int = aie::broadcast<int, VECDIM_sample_t>(1);
    auto v_zeros_int = aie::broadcast<int, VECDIM_sample_t>(0);
    aie::vector<int, VECDIM_sample_t> v_sample;

    aie::vector<T, VECDIM_sample_t> v_x, v_z;
    aie::vector<T, VECDIM_sample_t> v_sqrt, v_total_delay, t_sample;

    aie::accum<accfloat, VECDIM_sample_t> acc_zeros;
    acc_zeros.from_vector(v_zeros);

    aie::accum<accfloat, VECDIM_sample_t> acc_ones;
    acc_ones.from_vector(v_ones);

    for (int n = 0; n < p_const->num_dep_seg; n += VECDIM_sample_t) chess_prepare_for_pipelining {
            auto v_img_x = aie::load_v<VECDIM_sample_t>(p_in_img_x);
            auto v_img_z = aie::load_v<VECDIM_sample_t>(p_in_img_z);
            auto v_delay = aie::load_v<VECDIM_sample_t>(p_in_delay);
            p_in_img_x += VECDIM_sample_t;
            p_in_img_z += VECDIM_sample_t;
            p_in_delay += VECDIM_sample_t;

            v_x = v_img_x - v_xdc_x;
            v_z = v_img_z - v_xdc_z;

            auto acc_x = aie::mac(acc_zeros, v_x, v_x);
            v_z = aie::mac(acc_x, v_z, v_z);

            v_sqrt = aie::sqrt(v_z);
            v_total_delay = aie::mul(v_sqrt, v_inv_speed_of_sound) + v_delay;
            t_sample = aie::mac(acc_ones, v_total_delay, v_freq_sampling);

            v_sample = aie::to_fixed<int>(t_sample);

            auto msk_ge = t_sample >= v_ones;
            auto msk_lt = t_sample < v_rf_dim;
            auto msk_and = msk_ge & msk_lt;
            auto v_inside = aie::select(v_zeros_int, v_ones_int, msk_and);
            v_sample = aie::select(v_ones_int, v_sample, msk_and);

            aie::store_v(p_out_inside, v_inside);
            aie::store_v(p_out_sample, v_sample);
            p_out_inside += VECDIM_sample_t;
            p_out_sample += VECDIM_sample_t;
        }
#endif
    p_const->print();
    p_const->update();
}

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
    const T (&para_elem)[NUM_ELEMENT_t * 4]) {
    para_sample_t<T>* p_const = (para_sample_t<T>*)para_const;
    p_const->print();
    p_const->update();
}
}
}
