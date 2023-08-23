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
#include "kernels.hpp"
#include "kernel_apodization.hpp"
#include "aie_api/operators.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

#ifndef MINUS_M_PI
#define MINUS_M_PI -3.14159265358979323846 /* pi */
#endif

namespace us {
namespace L1 {

using namespace aie;
using namespace adf;
using namespace aie::operators;

//----------------------------------------------------
// brief
// 1.scaler version
//---1.1 model function
//---1.2 model function wapper
// 2.vector version
//---2.1 vector function
//---2.2 vector function wapper
// 3.top function to switch between vector and scaler version
// 4.top function to test top graph connection only, no calculation
//----------------------------------------------------

// scaler version
template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM>
void mfun_apodization_main(T p_apodization_out[LEN_OUT],
                           T p_focal_in[LEN_IN_F],
                           T p_invD_in[LEN_IN_D],
                           para_Apodization<T>* param) {
    int e = param->iter_element;
    float m_f_number = param->f_num;
    int num_dep_seg = param->num_dep_seg;

    for (int d = 0; d < num_dep_seg; d++) chess_prepare_for_pipelining {
            T x_ele = m_f_number * p_focal_in[e] / p_invD_in[d]; //*p_invD[n]
            bool in_range = std::abs(x_ele) <= 1;
            T cos_res = std::cos(M_PI * (1 + x_ele)); // aie::cos input is [-pi, pi]
            p_apodization_out[d] = 0.5 * (1.0 - cos_res) * in_range;
        }
};

template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void mfun_apodization_main_scaler(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
                                  adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
                                  adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
                                  const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    T* __restrict p_out = p_apodization_out.data();
    T* __restrict p_focal = p_focal_in.data();
    T* __restrict p_invD = p_invD_in.data();
    para_Apodization<T>* param = (para_Apodization<T>*)para_const;

    // 2. run iter times func
    mfun_apodization_main<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM>(p_out, p_focal, p_invD, param);

    // 3. iter
    param->print();
    param->update();
}

// vector version
template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM>
void fun_genLineApodization_main(
    // output_stream<T>* p_apodization_strm_out,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
    adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
    adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
    para_Apodization<T>* param) {
    T* __restrict p_out = p_apodization_out.data();
    T* __restrict p_focal = p_focal_in.data();
    T* __restrict p_invD = p_invD_in.data();

    int e = param->iter_element;

#ifdef DEBUG
    printf("DEBUG interp kernel : param.num_dep_seg = %d, iter_ele = %d, all iter = %d\n", param->num_dep_seg, e,
           param->num_dep_seg / VECDIM);
#endif

#if 0 
//no Vectorization
    T init_f = param->f_num * p_focal[e]; 
    for(int d = 0; d < param->num_dep_seg; d++)chess_prepare_for_pipelining {
        T x_ele = init_f * aie::inv(p_invD[d]);
        bool in_range = x_ele <= 1;
        // when x_ele in range, x always 1<x<2
        
        float cos_in = M_PI * (x_ele - 1);
        float cos_res = aie::cos(cos_in); //Note, aie::cos input must in [-M_PI, M_PI]

        //T p_out = 0.5 * (1.0f - cos_res) * in_range;
        p_out[d] = 0.5 * (1.0 - (float)cos_res) * in_range;

        //for debug
        //T p_out = 0.5 * (1.0f - cos_res) * in_range;
        //writeincr(p_apodization_strm_out, p_out);
    }

#else
    // Vectorization
    T init_f = param->f_num * p_focal[e];
    aie::vector<T, VECDIM> v_f = aie::broadcast<T, VECDIM>(init_f);
    aie::vector<T, VECDIM> v_pi = aie::broadcast<T, VECDIM>(M_PI);
    aie::vector<T, VECDIM> v_minus_pi = aie::broadcast<T, VECDIM>(MINUS_M_PI);
    // aie::vector<T, VECDIM> v_half_pi = aie::broadcast<T, VECDIM>(M_PI*0.5);
    aie::vector<T, VECDIM> ones = aie::broadcast<T, VECDIM>(1.0);
    aie::vector<T, VECDIM> half = aie::broadcast<T, VECDIM>(0.5);
    aie::vector<T, VECDIM> zero = aie::broadcast<T, VECDIM>(0);

    aie::vector<T, VECDIM> v_invD, v_x, v_abs_x, v_res, v_coeff;
    aie::accum<accfloat, VECDIM> acc_cos_in, acc_out;

    auto vec_D = aie::begin_vector<VECDIM>(p_invD_in);
    auto vec_out = aie::begin_vector<VECDIM>(p_apodization_out);
    for (int d = 0; d < param->num_dep_seg; d += VECDIM) chess_prepare_for_pipelining chess_loop_range(8, ) {
            v_invD = aie::inv(*vec_D++);
            v_x = aie::mul(v_f, v_invD); // vec_f * vec_invD;

            acc_cos_in.from_vector(v_minus_pi);           //- M_PI;
            acc_cos_in = aie::mac(acc_cos_in, v_x, v_pi); // M_PI * (x_ele - 1)
            auto cos_res = aie::cos(acc_cos_in.template to_vector<T>());
            v_res = cos_res; // another sin^2(A/2) = (1 - cos(A)) / 2

            // p_out[n] = 0.5 * (1.0 - cos_res) * in_range = coeff*(1.0 - cos_res);
            v_abs_x = aie::abs(v_x);
            auto range_msk = v_abs_x <= ones;
            auto in_range = aie::select(zero, ones, range_msk);
            v_coeff = aie::mul(half, in_range);
            acc_out.from_vector(v_coeff);
            acc_out = aie::msc(acc_out, v_res, v_coeff);

            *vec_out++ = acc_out.template to_vector<T>();

            // for debug
            // writeincr(p_apodization_strm_out, acc_out.template to_vector<T>() );
        }
#endif // Vectorization
};

template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void __attribute__((noinline)) kfun_apodization_main_vector(
    // output_stream<T>* p_apodization_strm_out,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
    adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
    adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
    const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    // 1. load once in the first iter
    // if(param.iter==0){
    //     load_apodi_rtp(param, para_const);
    //     //p_invD_out.acquire();
    // }

    para_Apodization<T>* param = (para_Apodization<T>*)para_const;

    // 2. run iter times func
    fun_genLineApodization_main<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM>(p_apodization_out, p_focal_in, p_invD_in,
                                                                        param);

    // 3. iter
    param->print();
    param->update();
    // if(depth_iter == GEMM_NUM_DEPTH_ITERS){
    //     p_invD_out.release();
    // }
}

// top function to switch between vector and scaler version
template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void __attribute__((noinline)) kfun_apodization_main(
    // output_stream<T>* p_apodization_strm_out,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
    adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
    adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
    const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    kfun_apodization_main_vector<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM, APODI_PRE_LEN32b_PARA>(
        p_apodization_out, p_focal_in, p_invD_in, para_const);
    // mfun_apodization_main_scaler<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM, APODI_PRE_LEN32b_PARA>(p_apodization_out,
    // p_focal_in, p_invD_in, para_const);
}

// top function to test top graph connection
template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_main_shell(
    // output_stream<T>* p_apodization_strm_out,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
    adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
    adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
    const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    para_Apodization<T>* param = (para_Apodization<T>*)para_const;
    param->print();
    param->update();
}

} // namespace L1
} // namespace us
