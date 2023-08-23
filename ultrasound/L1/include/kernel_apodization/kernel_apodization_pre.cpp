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
#include "kernel_apodization.hpp"
#include <aie_api/operators.hpp>

using namespace aie::operators;

namespace us {
namespace L1 {

template <typename T, int LEN_OUT, int LEN_IN, int VECDIM>
void fun_genLineApodization_preprocess(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
                                       adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
                                       adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
                                       para_Apodization<T>* param) {
    int num_dep_seg = param->num_dep_seg;
// printf("DEBUG kernel : param init = %f, iter = %d, all iter = %d\n", param->tileVApo_z, param->iter_seg,
// num_dep_seg);

#if 0 
//no Vectorization 

    T* p_out = p_invD_out.data();
    T* p_inx = p_points_x_in.data();
    T* p_inz = p_points_z_in.data();

    for(int d = 0; d < num_dep_seg; ++d)
    {
        //  m_p_points_x[d] = xi + n * dx; // todo:merge with imagepoint? //  m_p_points_z[d] = zi + n * dz; 

        T diff_x = (p_inx[d] - param->ref_point_x) * param->tileVApo_x;
        T diff_z = (p_inz[d] - param->ref_point_z) * param->tileVApo_z;

        T D = std::abs(diff_x + diff_z); assert(D!=0);
        bool equalZero = D == 0;//todo
        D = D + equalZero * 1e-16;
        p_out[d] = D; // 0c72c33e744d21bc6e12f3927643daeb //p_invD[d] = 1/D; //aie::inv(D) // 210578f184c9966e78ed30cd4cb2ffa8
    }

#else // Vectorization

    T xz = -param->ref_point_x * param->tileVApo_x - param->ref_point_z * param->tileVApo_z;
    aie::vector<T, VECDIM> vec_xz = aie::broadcast<T, VECDIM>(xz);

    aie::accum<accfloat, VECDIM> diff_x, diff_z, acc_init;
    acc_init.from_vector(vec_xz);

    auto vec_x = aie::begin_vector<VECDIM>(p_points_x_in);
    auto vec_z = aie::begin_vector<VECDIM>(p_points_z_in);
    auto vec_out = aie::begin_vector<VECDIM>(p_invD_out);
    for (int d = 0; d < num_dep_seg; d += VECDIM) chess_prepare_for_pipelining { // chess_loop_range(8, )
            diff_x = aie::mac(acc_init, *vec_x++, param->tileVApo_x);
            diff_z =
                aie::mac(diff_x, *vec_z++, param->tileVApo_z); // D = p_inx * tileVApo_x + p_inz*tileVApo_z + vec_xz;
            // static_assert(diff_z == 0, "Warning: waveform seems start at (0,0)!");

            *vec_out++ = diff_z.template to_vector<T>();
        }
#endif
};

template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_pre(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
                          adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
                          adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
                          const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    // 1. load once in the first iter
    // if(param.iter==0){
    //     load_apodi_rtp(param, para_const);
    //     //p_invD_out.acquire();
    // }

    para_Apodization<T>* param = (para_Apodization<T>*)para_const;

    // 2. run iter times func
    fun_genLineApodization_preprocess<T, LEN_OUT, LEN_IN, VECDIM>(p_invD_out, p_points_x_in, p_points_z_in, param);

    // 3. iter
    param->print();
    param->update();

    // if(depth_iter == GEMM_NUM_DEPTH_ITERS){
    //     p_invD_out.release();
    // }
}

template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
void kfun_apodization_pre_shell(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
                                adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
                                adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
                                const int (&para_const)[APODI_PRE_LEN32b_PARA]) {
    para_Apodization<T>* param = (para_Apodization<T>*)para_const;
    param->print();
    param->update();
}

} // namespace L1
} // namespace us
