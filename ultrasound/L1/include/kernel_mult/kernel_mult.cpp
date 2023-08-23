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
#include "kernel_mult.hpp"

using namespace aie::operators;

namespace us {
namespace L1 {

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
void __attribute__((noinline))
kfun_mult_pre(const T (&para_const)[LEN32b_PARA_mult_t],
              adf::input_buffer<T, adf::extents<LEN_IN_mult_t> >& __restrict in_interp, // for cascade can not be plio
              adf::input_buffer<T, adf::extents<LEN_IN_mult_t / 4> >& __restrict in_apod,
              output_stream<accfloat>* p_out_cascade) {
    T* __restrict p_in_apod = in_apod.data();
    T* __restrict p_in_interp = in_interp.data();

    para_mult_t<T>* p_const = (para_mult_t<T>*)para_const;
    int iter_seg = p_const->iter_seg;
    aie::vector<T, VECDIM_mult_t> v_apo;
    aie::vector<T, VECDIM_mult_t> v_interp;
    aie::accum<accfloat, VECDIM_mult_t> v_out;

    for (int n = 0; n < p_const->num_dep_seg; n += 2 * VECDIM_mult_t) chess_prepare_for_pipelining {
            auto v_interp_0 = aie::load_v<VECDIM_mult_t>(p_in_interp);
            p_in_interp += VECDIM_mult_t;
            auto v_interp_1 = aie::load_v<VECDIM_mult_t>(p_in_interp);
            p_in_interp += VECDIM_mult_t;

            auto v_in_apo = aie::load_v<4>(p_in_apod);
            p_in_apod += 4;

            auto v_apo_0 = aie::broadcast<T, 4>(v_in_apo[0]);
            auto v_apo_1 = aie::broadcast<T, 4>(v_in_apo[1]);
            auto v_apo_2 = aie::broadcast<T, 4>(v_in_apo[2]);
            auto v_apo_3 = aie::broadcast<T, 4>(v_in_apo[3]);

            v_apo = aie::concat(v_apo_0, v_apo_1);

            v_out = aie::mul(v_apo, v_interp_0);

            writeincr(p_out_cascade, v_out);

            v_apo = aie::concat(v_apo_2, v_apo_3);

            v_out = aie::mul(v_apo, v_interp_1);

            writeincr(p_out_cascade, v_out);
        }
    p_const->update();
}

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
                                                 input_stream<accfloat>* p_in_cascade,
                                                 output_stream<accfloat>* p_out_cascade,
                                                 output_stream<T>* p_out_mult) {
    para_mult_t<T>* p_const = (para_mult_t<T>*)para_const;
    para_mult_local_t<T, NUM_DEP_SEG_t>* p_local_0 = (para_mult_local_t<T, NUM_DEP_SEG_t>*)local_data_0;
    para_mult_local_t<T, NUM_DEP_SEG_t>* p_local_1 = (para_mult_local_t<T, NUM_DEP_SEG_t>*)local_data_1;

    int iter_element = p_const->iter_element;
    int iter_seg = p_const->iter_seg;
    int num_element = p_const->num_element;
    int num_seg = p_const->num_seg;

    aie::accum<accfloat, VECDIM_mult_t> acc;

    if (iter_seg == MULT_ID_t) {
        if (iter_element % 2 == 0) {
            for (int n = 0; n < p_const->num_dep_seg; n += VECDIM_mult_t) chess_prepare_for_pipelining {
                    auto v_last = aie::load_v<VECDIM_mult_t>(&(p_local_0->local[n]));
                    acc = readincr_v<VECDIM_mult_t>(p_in_cascade);
                    v_last = v_last + acc.to_vector();
                    aie::store_v(&(p_local_1->local[n]), v_last);
                }
        } else {
            for (int n = 0; n < p_const->num_dep_seg; n += VECDIM_mult_t) chess_prepare_for_pipelining {
                    auto v_last = aie::load_v<VECDIM_mult_t>(&(p_local_1->local[n]));
                    acc = readincr_v<VECDIM_mult_t>(p_in_cascade);
                    v_last = v_last + acc.to_vector();
                    aie::store_v(&(p_local_0->local[n]), v_last);
                }
        }

    } else if (iter_seg > MULT_ID_t) {
        for (int n = 0; n < p_const->num_dep_seg; n += VECDIM_mult_t) chess_prepare_for_pipelining {
                acc = readincr_v<VECDIM_mult_t>(p_in_cascade);
                writeincr(p_out_cascade, acc);
            }
    }

    p_const->print();
    p_const->update();

    if (iter_element == num_element - 1 && iter_seg == num_seg - 1) {
        if (num_element % 2 == 0) {
            for (int n = 0; n < p_const->num_dep_seg; n++) chess_prepare_for_pipelining {
                    writeincr(p_out_mult, p_local_0->local[n]);
                    p_local_0->local[n] = 0;
                }
        } else {
            for (int n = 0; n < p_const->num_dep_seg; n++) chess_prepare_for_pipelining {
                    writeincr(p_out_mult, p_local_1->local[n]);
                    p_local_1->local[n] = 0;
                }
        }
    }
}

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
                                                 input_stream<accfloat>* p_in_cascade,
                                                 output_stream<T>* p_out_mult) {
    output_stream<accfloat>* p_out_cascade;
    kfun_mult_cascade<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t, VECDIM_mult_t,
                      LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, MULT_ID_t>(
        para_const, local_data_0, local_data_1, p_in_cascade, p_out_cascade, p_out_mult);
}

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
                     adf::output_buffer<T, adf::extents<LEN_OUT_mult_t> >& __restrict out_mult) {
    para_mult_t<T>* p_const = (para_mult_t<T>*)para_const;
    printf("MULT_ID_t=%d, IS_END_t=%d\t ", MULT_ID_t, IS_END_t);
    p_const->print();
    p_const->update();
}
}
}
