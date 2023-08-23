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
#include "kernel_delay.hpp"

using namespace aie::operators;

namespace us {
namespace L1 {

/* non-vectorization version
template<class T>
void __attribute__((noinline)) fun_UpdatingDelay_line(
    adf::output_buffer<T>& __restrict out_delay,
    para_Delay<T> &param,
    adf::input_buffer<T>& __restrict in_img_x,
    adf::input_buffer<T>& __restrict in_img_z
){
    T* p_out_delay = out_delay.data();
    T* p_in_delay_img_x = in_img_x.data();
    T* p_in_delay_img_z = in_img_z.data();

    for(int n = 0; n < LEN_OUT; n++)
    {//for sample
        T acc_sample1 = 0;
        T acc_sample2 = 0;

        // DIM_X
        T diff1 = p_in_delay_img_x[n] - p_const->tx_ref_point_x;
        acc_sample1 = acc_sample1 + diff1 * p_const->tileVApo_x;
        T diff2 = p_in_delay_img_x[n] - p_const->focal_point_x;
        acc_sample2 = acc_sample2 + diff2 * diff2;
        // DIM_Z
        diff1 = p_in_delay_img_z[n] - p_const->tx_ref_point_z;
        acc_sample1 = acc_sample1 + diff1 * p_const->tileVApo_z;
        diff2 = p_in_delay_img_z[n] - p_const->focal_point_z;
        acc_sample2 = acc_sample2 + diff2 * diff2;

        acc_sample1 = aie::abs(acc_sample1) - p_const->tx_delay_distance;
        int sign = acc_sample1 >= 0 ? 1 : -1;

        acc_sample2 = sign * aie::sqrt(acc_sample2) + p_const->tx_delay_distance_;
        p_out_delay[n] = acc_sample2 * p_const->inverse_speed_of_sound - p_const->t_start;
    }

};
*/

template <class T, int LEN_IN_delay_t, int LEN_OUT_delay_t, int VECDIM_delay_t>
void __attribute__((noinline))
kfun_UpdatingDelay_line(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
                        us::L1::para_delay_t<T>* p_const,
                        adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
                        adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z) {
    T* p_out_delay = out_delay.data();
    T* p_in_delay_img_x = in_img_x.data();
    T* p_in_delay_img_z = in_img_z.data();

    // input and output
    aie::vector<T, VECDIM_delay_t> v_img_x, v_img_z;
    aie::vector<T, VECDIM_delay_t> v_delay;

    // parameter
    auto v_ref_x = aie::broadcast<T, VECDIM_delay_t>(p_const->tx_ref_point_x);
    auto v_ref_z = aie::broadcast<T, VECDIM_delay_t>(p_const->tx_ref_point_z);
    auto v_til_x = aie::broadcast<T, VECDIM_delay_t>(p_const->tileVApo_x);
    auto v_til_z = aie::broadcast<T, VECDIM_delay_t>(p_const->tileVApo_z);
    auto v_foc_x = aie::broadcast<T, VECDIM_delay_t>(p_const->focal_point_x);
    auto v_foc_z = aie::broadcast<T, VECDIM_delay_t>(p_const->focal_point_z);
    auto v_dis_0 = aie::broadcast<T, VECDIM_delay_t>(p_const->tx_delay_distance);
    auto v_dis_1 = aie::broadcast<T, VECDIM_delay_t>(p_const->tx_delay_distance_);
    auto v_speed = aie::broadcast<T, VECDIM_delay_t>(p_const->inverse_speed_of_sound);
    auto v_start = aie::broadcast<T, VECDIM_delay_t>(p_const->t_start);
    auto v_zeros = aie::broadcast<T, VECDIM_delay_t>(0.0);

    aie::vector<T, VECDIM_delay_t> v_sign;
    aie::vector<T, VECDIM_delay_t> v_sample1, v_sample2;

    aie::vector<T, VECDIM_delay_t> v_diff1, v_diff2;
    aie::vector<T, VECDIM_delay_t> acc_sample1, acc_sample2;

    for (int n = 0; n < p_const->num_dep_seg; n += VECDIM_delay_t) chess_prepare_for_pipelining {
            v_img_x = aie::load_v<VECDIM_delay_t>(p_in_delay_img_x);
            p_in_delay_img_x += VECDIM_delay_t;
            v_img_z = aie::load_v<VECDIM_delay_t>(p_in_delay_img_z);
            p_in_delay_img_z += VECDIM_delay_t;

            // DIM_X
            v_diff1 = v_img_x - v_ref_x;
            acc_sample1 = aie::mul(v_diff1, v_til_x);
            v_diff2 = v_img_x - v_foc_x;
            acc_sample2 = aie::mul(v_diff2, v_diff2);
            // DIM_Z
            v_diff1 = v_img_z - v_ref_z;
            acc_sample1 = aie::mul(v_diff1, v_til_z) + acc_sample1;
            v_diff2 = v_img_z - v_foc_z;
            acc_sample2 = aie::mul(v_diff2, v_diff2) + acc_sample2;
            acc_sample1 = aie::abs(acc_sample1) - v_dis_0;

            auto msk_lt = acc_sample1 < v_zeros;
            auto v_sqrt = aie::sqrt(acc_sample2);
            v_sign = aie::select(v_sqrt, aie::neg(v_sqrt), msk_lt);

            acc_sample2 = v_sign + v_dis_1;
            v_delay = aie::mul(acc_sample2, v_speed) - v_start;

            aie::store_v(p_out_delay, v_delay);
            p_out_delay += VECDIM_delay_t;
        }
}

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
void __attribute__((noinline))
kfun_UpdatingDelay_line_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
                                adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
                                adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z,
                                const T (&para_const)[LEN32b_PARA_delay_t],
                                const T (&para_t_start)[NUM_LINE_t]) {
    para_delay_t<T>* p_const = (para_delay_t<T>*)para_const;
    load_delay_rtp<T, NUM_LINE_t>(p_const, para_t_start);
    kfun_UpdatingDelay_line<T, LEN_IN_delay_t, LEN_OUT_delay_t, VECDIM_delay_t>(out_delay, p_const, in_img_x, in_img_z);
    p_const->print();
    p_const->update();
}

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
void __attribute__((noinline))
kfun_UpdatingDelay_line_wrapper_shell(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
                                      adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
                                      adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z,
                                      const T (&para_const)[LEN32b_PARA_delay_t],
                                      const T (&para_t_start)[NUM_LINE_t]) {
    para_delay_t<T>* p_const = (para_delay_t<T>*)para_const;
    p_const->print();
    p_const->update();
}
}
}
