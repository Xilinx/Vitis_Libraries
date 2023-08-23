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
#include "kernel_interpolation.hpp"
#include <aie_api/operators.hpp>
#include <aie_api/utils.hpp>

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
//------2.1.1 function rf_data_buf
//------2.1.2 function resample
//------2.1.3 function gen_window
//------2.1.4 function interpolation
// 3.top function (shell) to test top graph connection
// 4.backup
//----------------------------------------------------

// 1.scaler version
//---1.1 model function
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM>
void mfun_genLineInterpolation(T p_interpolation[LEN_OUT],
                               int p_sample_in[LEN_IN],
                               int p_inside_in[LEN_IN],
                               T p_rf_in[LEN_RF_IN],
                               para_Interpolation<T>* param) {
    int e = param->iter_element;
    int num_element = param->num_element;
    int num_depth = param->num_depth;
    int num_upSample = param->num_upSamp;

    // after num_depth(num_sample) there need 3 more address with 0.
    int sample_pad = 0;

    for (int n = 0; n < num_depth; n++) {
        T s[6];
        int k_sample_base = n;
        s[0] = p_rf_in[(int)p_sample_in[k_sample_base] + e];
        s[1] = p_rf_in[(k_sample_base + 1 >= num_depth) ? sample_pad : p_sample_in[k_sample_base + 1] + e];
        s[2] = p_rf_in[(k_sample_base + 1 >= num_depth) ? sample_pad : p_sample_in[k_sample_base + 1] + e];
        s[3] = p_rf_in[(k_sample_base + 2 >= num_depth) ? sample_pad : p_sample_in[k_sample_base + 2] + e];
        s[4] = p_rf_in[(k_sample_base + 2 >= num_depth) ? sample_pad : p_sample_in[k_sample_base + 2] + e];
        s[5] = p_rf_in[(k_sample_base + 3 >= num_depth) ? sample_pad : p_sample_in[k_sample_base + 3] + e];

        // printf(" %d ,", (int)p_sample[k_sample_base]);
        T vec[4] = {1, 1.33333333, 1.66666667, 2};
        for (int i = 0; i < num_upSample; i++) {
            T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
            T A2 = (2 - vec[i]) * s[2] + (vec[i] - 1) * s[3];
            T A3 = (3 - vec[i]) * s[4] + (vec[i] - 2) * s[5];

            T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
            T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;

            T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;

            p_interpolation[k_sample_base * num_upSample + i] = C * p_inside_in[k_sample_base];
        }
    }
};

//---1.2 model function wapper
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void mfun_interpolation_scaler(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,
                               adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                               adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                               adf::input_buffer<T, adf::extents<LEN_RF_IN> >& __restrict p_rfdata_in,
                               const int (&para_const)[INTERP_LEN32b_PARA]) {
    // 1. load once in the first iter
    // if(param.iter==0){
    //     load_interp_rtp(param, lp);
    //     //p_invD_out.acquire();
    // }

    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;

    // 2. run iter times func
    T* p_out = p_interpolation.data();
    int* p_sample = p_sample_in.data();
    int* p_inside = p_inside_in.data();
    T* p_rfdata = p_rfdata_in.data();
    mfun_genLineInterpolation<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM>(p_out, p_sample, p_inside, p_rfdata, param);

    // 3. iter
    // printf("DEBUG warpper : iter_line = %d, iter_ele = %d, iter_seg = %d\n", param->iter_line, param->iter_element,
    // param->iter_seg);
    param->print();
    param->update();
    // if(depth_iter == GEMM_NUM_DEPTH_ITERS){
    //     p_invD_out.release();
    // }
}

// 2.vector version
//---2.1 vector function
//------2.1.1 function rf_data_buf
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN /*no use*/, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline)) kfun_rfbuf_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_rfbuf_out,

                                                  input_stream<T>* strm_rfdata,
                                                  const T (&local_data)[LEN_OUT],
                                                  const int (&para_const)[INTERP_LEN32b_PARA]) {
    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;
    para_interp_local_t<T, LEN_OUT>* p_local = (para_interp_local_t<T, LEN_OUT>*)local_data;

    T* p_out = p_rfbuf_out.data();
#if 1

    int iter_seg = param->iter_seg;
    // read rf_data while compare index
    if (iter_seg == 0) {
        for (int d = 0; d < LEN_OUT; d++) chess_prepare_for_pipelining {
                T rf_in = readincr(strm_rfdata);
                p_local->local[d] = rf_in;
                p_out[d] = rf_in;
            }
    } else {
        for (int d = 0; d < LEN_OUT; d++) chess_prepare_for_pipelining {
                T rf_in = p_local->local[d];
                p_out[d] = rf_in;
            }
    }

#else
#endif
    param->update();
}

//------2.1.2 function resample
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline))
kfun_resamp_wrapper2(adf::output_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_out,
                     adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

                     adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_rfbuf_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                     const int (&para_const)[INTERP_LEN32b_PARA]) {
    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;

    T* p_out = p_resamp_out.data();
    int* p_ind_out = p_inside_out.data();

    T* p_rf_in = p_rfbuf_in.data();
    int* p_samp_in = p_sample_in.data();
    int* p_ind_in = p_inside_in.data();

    int num_dep_seg = param->num_dep_seg;
    int num_seg = param->num_seg;
    int iter_seg = param->iter_seg;

#if 1
    const int last_sample = p_samp_in[num_dep_seg - 1];
    // read rf_data while compare index
    for (int d = 0; d < num_dep_seg; d++) chess_prepare_for_pipelining {
            p_out[d] = p_rf_in[p_samp_in[d]];
            // printf("%d, %f\n", p_samp_in[d], p_out[d]);
            p_ind_out[d] = p_ind_in[d];
        }

// if(iter_seg != num_seg - 1){
//     for(int d = 0; d < 8; d++) chess_prepare_for_pipelining { // because all 1/0 in the last
//         p_out[num_dep_seg + d] = p_rf_in[last_sample+d];  // p_out[num_dep_seg + d] =
//         p_rf_in[p_samp_in[num_dep_seg-1]+d];
//     }
// }else{
//     for(int d = 0; d < 8; d++) chess_prepare_for_pipelining { // because all 1/0 in the last
//         p_out[num_dep_seg + d] = 0;
//     }
// }

#else
#endif

    // param->print();
    param->update();
}

//------2.1.3 function gen_window
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline))
kfun_genwin_wrapper2(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_out,
                     adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

                     adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                     const int (&para_const)[INTERP_LEN32b_PARA]) {
    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;

    int* p_ind_out = p_inside_out.data();
    T* p_in = p_resamp_in.data();
    int* p_ind_in = p_inside_in.data();

    // int num_upSamp = param->num_upSamp;
    int num_dep_seg = param->num_dep_seg;

    aie::vector<T, 4> vec_out, vec0, vec1, vec2, vec3;
    auto p_out = aie::begin_vector<4>(p_vec_out);
#if 1

    vec0 = aie::load_v<4>(p_in);
    *p_out++ = vec0;
    // vec0 = aie::broadcast<4>(p_in[0]);
    // read rf_data while compare index
    for (int d = 0; d < num_dep_seg - 4; d++) chess_prepare_for_pipelining {
            // vec0 = aie::load_v<4>(p_in + d);
            p_ind_out[d] = p_ind_in[d];
            vec1[0] = p_in[d + 4];
            vec2 = vec0;
            vec0 = aie::shuffle_down_fill(vec2, vec1, 1);
            *p_out++ = vec0;
        }
    T last = p_in[num_dep_seg - 1];
#if defined(__X86SIM__)
    printf("last : %f\n", last);
#endif
    for (int d = num_dep_seg - 4; d < num_dep_seg - 1; d++) chess_prepare_for_pipelining {
            // vec0 = aie::load_v<4>(p_in + d);
            p_ind_out[d] = p_ind_in[d];
            vec1[0] = last;
            vec2 = vec0;
            vec0 = aie::shuffle_down_fill(vec2, vec1, 1);
            *p_out++ = vec0;
        }
    p_ind_out[num_dep_seg - 1] = p_ind_in[num_dep_seg - 1];

#else
    vec0 = aie::load_v<4>(p_in);
    if (iter_seg == 0) {
        // read rf_data while compare index
        for (int d = 0; d < num_dep_seg - 3; d++) chess_prepare_for_pipelining { // 0~509
                *p_out++ = vec0;
                p_ind_out[d] = p_ind_in[d];
                vec1[0] = p_in[d + 4];
                vec2 = vec0;
                vec0 = aie::shuffle_down_fill(vec2, vec1, 1);
            }
        param->last_addr0 = p_in[num_dep_seg - 3];
        param->last_addr1 = p_in[num_dep_seg - 2];
        param->last_addr2 = p_in[num_dep_seg - 1];
        param->last_ind0 = p_ind_in[num_dep_seg - 3];
        param->last_ind1 = p_ind_in[num_dep_seg - 2];
        param->last_ind2 = p_ind_in[num_dep_seg - 1];
    } else {
        vec0[0] = param->last_addr0;
        vec0[1] = param->last_addr1;
        vec0[2] = param->last_addr2;
        vec0[3] = p_in[0];
        int last_ind0 = param->last_ind0;
        int last_ind1 = param->last_ind1;
        int last_ind2 = param->last_ind2;
        for (int d = 0; d < num_dep_seg; d++) chess_prepare_for_pipelining {
                *p_out++ = vec0;
                p_ind_out[d] = p_ind_in[d];
                vec1[0] = p_in[d + 1];
                vec2 = vec0;
                vec0 = aie::shuffle_down_fill(vec2, vec1, 1);
            }
        param->last_addr0 = p_in[num_dep_seg - 3];
        param->last_addr1 = p_in[num_dep_seg - 2];
        param->last_addr2 = p_in[num_dep_seg - 1];
        param->last_ind0 = p_ind_in[num_dep_seg - 3];
        param->last_ind1 = p_ind_in[num_dep_seg - 2];
        param->last_ind2 = p_ind_in[num_dep_seg - 1];
    }
#endif

    // param->print();
    param->update();
}

//------2.1.4 function interpolation
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline)) kfun_interpolation_wrapper(
    // output_stream<T>* p_interpolation,
    adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,

    adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_in,
    adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
    const int (&para_const)[INTERP_LEN32b_PARA]) {
    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;

    int* p_ind_in = p_inside_in.data();

    // int num_upSamp = param->num_upSamp;
    int num_dep_seg = param->num_dep_seg;

    aie::vector<T, 4> s, vec_out, vec0, vec1, vec2, vec3;
    auto p_in = aie::begin_vector<4>(p_vec_in);
    auto p_out = aie::begin_vector<4>(p_interpolation);
#if 1

    T vec_1t[4] = {-0.074074074, 0.777777778, 0.333333333, -0.037037036};
    T vec_2t[4] = {-0.037037036, 0.333333333, 0.777777778, -0.074074074};

    for (int d = 0; d < param->num_dep_seg; d++) chess_prepare_for_pipelining {
            s = *p_in++;

            T v1_0 = vec_1t[0] * s[0];
            T v1_1 = vec_1t[1] * s[1];
            T v1_3 = vec_1t[2] * s[2];
            T v1_5 = vec_1t[3] * s[3];
            T C1 = v1_0 + v1_1 + v1_3 + v1_5;

            T v2_0 = vec_2t[0] * s[0];
            T v2_1 = vec_2t[1] * s[1];
            T v2_3 = vec_2t[2] * s[2];
            T v2_5 = vec_2t[3] * s[3];
            T C2 = v2_0 + v2_1 + v2_3 + v2_5;

            vec_out[0] = s[1] * p_ind_in[d];
            vec_out[1] = C1 * p_ind_in[d];
            vec_out[2] = C2 * p_ind_in[d];
            vec_out[3] = s[2] * p_ind_in[d];

            *p_out++ = vec_out;

#if defined(__X86SIM__)
            if (param->iter_element == 0 && param->iter_seg == 0 && d >= 507) {
                T* ptr = (T*)&s;
                printf("s[0] = %f, C=%f\n", ptr[0], ptr[1]);
                printf("s[1] = %f, C=%f\n", ptr[1], C1);
                printf("s[2] = %f, C=%f\n", ptr[2], C2);
                printf("s[3] = %f, C=%f\n", ptr[3], ptr[2]);
            }
#endif
        }

#else

    T coeff[8] = {-0.074074074, 0.777777778, 0.333333333, -0.037037036,
                  -0.037037036, 0.333333333, 0.777777778, -0.074074074};
    T vec_1t[4] = {-0.074074074, 0.777777778, 0.333333333, -0.037037036};
    T vec_2t[4] = {-0.037037036, 0.333333333, 0.777777778, -0.074074074};

    for (int d = 0; d < param->num_dep_seg; d++) chess_prepare_for_pipelining {
            s = *p_in++;

            T v1_0 = vec_1t[0] * s[0];
            T v1_1 = vec_1t[1] * s[1];
            T v1_3 = vec_1t[2] * s[2];
            T v1_5 = vec_1t[3] * s[3];
            T C1 = v1_0 + v1_1 + v1_3 + v1_5;
            aie::accum<acc48, 8> acc = aie::sliding_mul_ops<8, 8, 1, 1, 1, int16, int16, acc48>(va, 0, vb0, 0);

            T v2_0 = vec_2t[0] * s[0];
            T v2_1 = vec_2t[1] * s[1];
            T v2_3 = vec_2t[2] * s[2];
            T v2_5 = vec_2t[3] * s[3];
            T C2 = v2_0 + v2_1 + v2_3 + v2_5;

            vec_out[0] = s[1] * p_ind_in[d];
            vec_out[1] = C1 * p_ind_in[d];
            vec_out[2] = C2 * p_ind_in[d];
            vec_out[3] = s[2] * p_ind_in[d];

            *p_out++ = vec_out;
        }
#endif

    param->print();
    param->update();
}

// 3.top function (shell) to test top graph connection
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void kfun_interpolation_shell(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,
                              adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                              adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                              adf::input_buffer<T, adf::extents<LEN_RF_IN> >& __restrict p_rfdata_in,
                              const int (&para_const)[INTERP_LEN32b_PARA]) {
    para_Interpolation<T>* param = (para_Interpolation<T>*)para_const;
    param->print();
    param->update();
}

// 4. backup
#if 0
template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
void __attribute__((noinline)) kfun_rfbuf_wrapper(
    adf::output_buffer<T, adf::extents<LEN_IN> >& __restrict p_rfbuf_out,
    adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_out,
    adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

    adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
    adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
    input_stream<T>* strm_rfdata,
    const int (&para_const)[INTERP_LEN32b_PARA]
){
    para_Interpolation<T>* param = (para_Interpolation<T>*) para_const;

    T* p_out = p_rfbuf_out.data();
    int* p_samp_out = p_sample_out.data();
    int* p_ind_out = p_inside_out.data();

    int* p_samp_in = p_sample_in.data();
    int* p_ind_in = p_inside_in.data();

    int num_dep_seg = param->num_dep_seg;
    int num_seg = param->num_seg;
    int num_depth = num_seg * num_dep_seg;

    aie::vector<int, VECDIM> vec_out, vec0, vec1, vec2, vec3;
    auto p_in0 = aie::begin_vector<VECDIM>(p_sample_in);
    auto p_in1 = aie::begin_vector<VECDIM>(p_inside_in);
    auto p_out0 = aie::begin_vector<VECDIM>(p_sample_out);
    auto p_out1 = aie::begin_vector<VECDIM>(p_inside_out);
#if 1
    
    static int seg_end, seg_start;
    static T seg_end_value;

    int start, end;
    
    int rf_in0, rf_in1, max;
    //max = end;
    //printf("DEBUG: interp checke line END 1 : end %d\n", end);
    // read front unused number
    if(param->iter_seg == 0){//int idx_start = param->iter_seg * num_dep_seg;
        start = (p_ind_in[0]==1) ? p_samp_in[0] : p_samp_in[1];
        end = p_samp_in[num_dep_seg - 1];
        printf("DEBUG: interp check element : start %d, end %d, ind %d\n", start, end, p_ind_in[0]);
        for(int i = 0; i< start; i++) chess_prepare_for_pipelining{
            rf_in0 = readincr(strm_rfdata); 
        }
    }else{
        start = (p_samp_in[0]>seg_start) ? p_samp_in[0] : seg_start;
        end = seg_end;  
    }

    // read rf_data while compare index
    for(int d = 0; d < num_dep_seg; d++) chess_prepare_for_pipelining {
        p_samp_out[d] = p_samp_in[d];
        p_ind_out[d] = p_ind_in[d];
        end = (p_samp_in[d] > end) ? p_samp_in[d] : end; //SEG 3
    }
    p_samp_out[num_dep_seg - 1] = end;
    //printf("DEBUG: interp checke line END 2 : end %d\n", end);

    int num_rf;
    if(param->iter_seg == 0){
        num_rf = (end - start + 1);
    }else{
        num_rf =  p_ind_in[0] ? (end - start + 1) : 0;
    }
    int start_addr = 0;
    if(param->iter_seg != 0 && p_ind_in[0] == 1){//when end == start in the inside
        num_rf = num_rf - (seg_end == start);
        start_addr = (seg_end == start);
        p_out[0] = seg_end_value;
    }

    T readin;
    for(int d = 0; d < num_rf; d++) chess_prepare_for_pipelining {
      
        readin = readincr(strm_rfdata); 
        p_out[start_addr + d] = readin;
        if( d<=1){
        //DEBUG
        printf("%d, p_out[d] = %f, num_rf=%d\n",param->iter_seg, p_out[d], num_rf);
        }
    }
    //p_out[num_dep_seg - 1] = end;//to be clean
    seg_start = start;
    seg_end = end;
    seg_end_value = readin;

    // read end unused number
    if(param->iter_seg == num_seg - 1){
        int idx_end = num_depth - 1 - end;
        printf("DEBUG: interp check element : last %d, num_rf %d\n", idx_end, num_rf);
        for(int i = 0; i < idx_end; i++) chess_prepare_for_pipelining {
            rf_in0 = readincr(strm_rfdata);
        }

    }

#else
//Vectorization 

    // int start = p_samp_in[0];
    // int end = p_samp_in[LEN_IN-1];
    // static int seg_end;
    // static T seg_end_value;
    
    // int rf_in0, rf_in1, max;
    //max = end;
    //printf("DEBUG: interp checke line END 1 : end %d\n", end);
    // read front unused number
    if(param->iter_seg == 0){//int idx_start = param->iter_seg * num_dep_seg;
        // for(int i = 0; i< start/2; i+=2) chess_prepare_for_pipelining{
        //     rf_in0 = readincr(strm_rfdata); 
        //     rf_in1 = readincr(strm_rfdata); 
        // }
        // if(start%2) rf_in0 = readincr(strm_rfdata); 
    }

    // read rf_data while compare index

    //printf("DEBUG: interp checke line END 2 : end %d\n", end);
    // for(int d = 0; d < 8; d++) chess_prepare_for_pipelining {
    //     p_samp_out[num_dep_seg+d] = p_samp_in[num_dep_seg-1];
    // }
    // for(int d = 0; d < param->num_dep_seg; d+=VECDIM) chess_prepare_for_pipelining {
    //     vec0 = *p_in0++;
    //     *p_out0++ = vec0;
    //     vec1 = *p_in1++;
    //     *p_out1++ = vec1;
    //     //if(d < num_rf) p_out[d] = readincr(strm_rfdata); 
    // }
    // vec0 = *p_in0++;
    // *p_out0++ = vec0;

    // int num_rf = end - start + 1;
    // int start_addr = 0;
    // if(param->iter_seg != 0){
    //     num_rf = num_rf - (seg_end == start);
    //     start_addr = (seg_end == start);
    //     p_out[0] = seg_end_value;
    // }

    // T readin;
    // seg_end = end;
    // seg_end_value = readin;

    // for(int d = 0; d < 8; d++) chess_prepare_for_pipelining {
    //     p_out[num_dep_seg+d] = p_out[num_dep_seg-1];
    // }

    // for(int d = 0; d < num_rf/2; d+=2) chess_prepare_for_pipelining {
    //     p_out[d] = readincr(strm_rfdata);
    //     p_out[d+1] = readincr(strm_rfdata);  
    // }
    // if(num_rf%2) p_out[num_rf-1] = readincr(strm_rfdata); 

    // read end unused number
    // if(param->iter_seg == num_seg - 1){
    //     int idx_end = num_depth - 1 - end;
    //     printf("DEBUG: interp checke line : last %d\n", idx_end);
    //     for(int i = 0; i < idx_end/2; i+=2) chess_prepare_for_pipelining {
    //         rf_in0 = readincr(strm_rfdata);
    //         rf_in1 = readincr(strm_rfdata);  
    //     }
    //     if(idx_end%2) rf_in0 = readincr(strm_rfdata); 
    // }
#endif

    //param->print();
    param->update();
}

#endif
//////////////////////////////////////////////////////////////////

} // namespace L1
} // namespace us
