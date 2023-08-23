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
#include "kernel_focusing.hpp"
#include "aie_api/operators.hpp"
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

namespace us {
namespace L1 {
using namespace adf;
using namespace aie;
using namespace aie::operators;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
void mfun_foc(const T (&para_const)[LEN32b_PARA_foc_t],
              const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
              T p_foc_line[NUM_ELEMENT_t]) {
    para_foc_t* p_const = (para_foc_t*)para_const; // convert array to structure for easy use

    {
        for (int e = 0; e < NUM_ELEMENT_t; e++) {
            T x = para_xdc_def_pos_4d[e];
            T y = para_xdc_def_pos_4d[e + NUM_ELEMENT_t];
            x *= x;
            y *= y;
            T sum = x + y;
            p_foc_line[e] = sqrt(sum);
        }
    }
    p_const->print();
    p_const->update();
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
inline void kfun_foc_scalar(const T (&para_const)[LEN32b_PARA_foc_t],
                            const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
                            output_buffer<T, adf::extents<NUM_ELEMENT_t> >& out) {
    T* __restrict p_foc_line = out.data();
    para_foc_t* p_const = (para_foc_t*)para_const; // convert array to structure for easy use

    mfun_foc<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t>(
        para_const, para_xdc_def_pos_4d, p_foc_line);

    /*
    //if(p_const->allZero())
    {

            for(int e = 0; e < NUM_ELEMENT_t; e++){
                    T x = para_xdc_def_pos_4d[e ];
                    T y = para_xdc_def_pos_4d[e + NUM_ELEMENT_t];
                    x *=x;
                    y *=y;
                    T sum = x + y;
                    p_foc_line[e] = aie::sqrt(sum);
            }
    }
    p_const->print();
    p_const->update();
    */
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
inline void kfun_foc_vector(const T (&para_const)[LEN32b_PARA_foc_t],
                            const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
                            output_buffer<T, adf::extents<NUM_ELEMENT_t> >& out) {
    para_foc_t* p_const = (para_foc_t*)para_const; // convert array to structure for easy use

    // if(p_const->allZero())
    {
        T* __restrict p_foc_line = out.data();
        auto p_foc_line_v = aie::begin_vector<VECDIM_foc_t>(out);
        auto p_x_v = aie::begin_vector<VECDIM_foc_t>(para_xdc_def_pos_4d);
        auto p_z_v = aie::begin_vector<VECDIM_foc_t>(para_xdc_def_pos_4d + NUM_ELEMENT_t);
        aie::vector<T, VECDIM_foc_t> v_x;
        aie::vector<T, VECDIM_foc_t> v_y;
        for (int e = 0; e < NUM_ELEMENT_t / VECDIM_foc_t; e++) {
            v_x = *p_x_v++;
            v_y = *p_z_v++;
            // aie::print(v_x,true,"v_x=");
            // aie::print(v_y,true,"v_y=");
            v_x = aie::mul_square(v_x);
            v_y = aie::mul_square(v_y);
            // aie::print(v_x,true,"v_x^2=");
            // aie::print(v_y,true,"v_y^2=");
            // v_x = v_x * v_x;
            // v_y = v_y * v_y;

            aie::vector<T, VECDIM_foc_t> sum = v_x + v_y;
            // aie::print(sum ,true,"sum=");
            p_foc_line_v[e] = aie::sqrt(sum);
            // aie::print(aie::sqrt(sum),true,"sqrt(sum)=");
        }
    }
    p_const->print();
    p_const->update();
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
void kfun_foc_shell(const T (&para_const)[LEN32b_PARA_foc_t],
                    const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
                    output_buffer<T, adf::extents<NUM_ELEMENT_t> >& out) {
    para_foc_t* p_const = (para_foc_t*)para_const; // convert array to structure for easy use
    p_const->print();
    p_const->update();
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
void kfun_foc(const T (&para_const)[LEN32b_PARA_foc_t],
              const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
              output_buffer<T, adf::extents<NUM_ELEMENT_t> >& out) {
    // kfun_foc_scalar<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t ,
    // LEN32b_PARA_foc_t>(para_const, para_xdc_def_pos_4d, out);
    kfun_foc_vector<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t>(
        para_const, para_xdc_def_pos_4d, out);
};

} // L1
} // su
