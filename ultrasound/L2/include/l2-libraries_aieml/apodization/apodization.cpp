/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#include "apodization.hpp"
#include "common_defines.hpp"

#include "l1-libraries_aieml/tile_v/tile_v.cpp"
#include "l1-libraries_aieml/diff/diff.cpp"
#include "l1-libraries_aieml/mul/mul.cpp"
#include "l1-libraries_aieml/sum_axis_1/sum_axis_1.cpp"
#include "l1-libraries_aieml/abs_v/abs_v.cpp"
#include "l1-libraries_aieml/equal_s/equal_s.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"
#include "l1-libraries_aieml/div/div.cpp"
#include "l1-libraries_aieml/cos_v/cos_v.cpp"
#include "l1-libraries_aieml/less_or_equal_than_s/less_or_equal_than_s.cpp"

namespace us {
namespace L2 {

template <typename T>
void Apodization1(

    adf::input_buffer<T>& image_points,
    adf::input_buffer<T>& apodization_reference,
    adf::input_buffer<T>& apodization_direction,

    adf::output_buffer<T>& apo_depth_output

    ) {
    T* image_points_buffer = (T*)image_points.data();
    T* apodization_reference_buffer = (T*)apodization_reference.data();
    T* apodization_direction_buffer = (T*)apodization_direction.data();

    T* apo_depth_output_buffer = (T*)apo_depth_output.data();

    T buffer_out_m_1[LEN * M_COLUMNS];
    T buffer_out_m_2[LEN * M_COLUMNS];
    T buffer_out_m_3[LEN * M_COLUMNS];
    T buffer_out_v_1[LEN];

    // DiffMV
    us::L1::m_Diff<T, LEN, INCREMENT_M, SIMD_DEPTH>(image_points_buffer, apodization_reference_buffer, buffer_out_m_1);
    //	//TileV
    us::L1::m_TileV<T, LEN, INCREMENT_M, SIMD_DEPTH>(apodization_direction_buffer, buffer_out_m_2);
    //	//MulMM
    us::L1::m_Mul<T, LEN, INCREMENT_M, SIMD_DEPTH>(buffer_out_m_1, buffer_out_m_2, buffer_out_m_3);
    //	//SumAxis1
    us::L1::m_SumAxis1<T, LEN, 1, M_COLUMNS>(buffer_out_m_3, buffer_out_v_1);
    //	//AbsV
    us::L1::m_AbsV<T, T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, apo_depth_output_buffer);
}

template <typename T>
void Apodization2(

    adf::input_buffer<T>& apo_depth,
    adf::input_buffer<T>& two,
    adf::input_buffer<T>& apo_distance,
    adf::input_buffer<T>& f_number,
    adf::input_buffer<T>& one,

    adf::output_buffer<T>& index_output

    ) {
    T* apo_depth_buffer = (T*)apo_depth.data();
    T* two_buffer = (T*)two.data();
    T* apo_distance_buffer = (T*)apo_distance.data();
    T* f_number_buffer = (T*)f_number.data();
    T* one_buffer = (T*)one.data();

    T* index_output_buffer = (T*)index_output.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];
    T buffer_out_v_4[LEN];
    T buffer_out_v_5[LEN];

    /*float buffer_out_v_1_f[LEN];
    float buffer_out_v_2_f[LEN];
    float buffer_out_v_3_f[LEN];*/

    // EqualS
    us::L1::m_EqualS<T, LEN, INCREMENT_V, SIMD_DEPTH, 0>(apo_depth_buffer, buffer_out_v_1);
    // MulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(two_buffer, apo_distance_buffer, buffer_out_v_2);
    // MulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(f_number_buffer, buffer_out_v_2, buffer_out_v_3);
    // MulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, one_buffer, buffer_out_v_4);
    // SumVS
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(apo_depth_buffer, buffer_out_v_4, buffer_out_v_5);
    // DivVS
    us::L1::m_Div<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_3, buffer_out_v_5, index_output_buffer);
}

template <typename T>
void ApodizationOutput(

    adf::input_buffer<T>& index_from_apo_2,
    adf::input_buffer<T>& one,
    adf::input_buffer<T>& pi,
    adf::input_buffer<T>& two,

    adf::output_buffer<T>& output_hanning

    ) {
    T* index_from_apo_2_buffer = (T*)index_from_apo_2.data();
    T* two_buffer = (T*)two.data();
    T* pi_buffer = (T*)pi.data();
    T* one_buffer = (T*)one.data();

    T* output_hanning_buffer = (T*)output_hanning.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];
    T buffer_out_v_4[LEN];
    T buffer_out_v_5[LEN];

    // SumVS
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(index_from_apo_2_buffer, one_buffer, buffer_out_v_1);
    // MulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(two_buffer, pi_buffer, buffer_out_v_2);
    // MulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, buffer_out_v_1, buffer_out_v_3);
    // DivVS
    us::L1::m_Div<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_3, two_buffer, buffer_out_v_2);
    // CosV
    us::L1::m_CosV<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, buffer_out_v_3);
    // DiffVS
    us::L1::m_Diff<T, LEN, INCREMENT_V, SIMD_DEPTH>(one_buffer, buffer_out_v_3, buffer_out_v_2);
    // DivVS
    us::L1::m_Div<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, two_buffer, buffer_out_v_4);
    // AbsV
    us::L1::m_AbsV<T, T, LEN, INCREMENT_V, SIMD_DEPTH>(index_from_apo_2_buffer, buffer_out_v_1);
    // lessOrEqualThen
    us::L1::m_LessOrEqualThanS<T, LEN, INCREMENT_V, SIMD_DEPTH, 1>(buffer_out_v_1, buffer_out_v_5);
    // MulVV
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_4, buffer_out_v_5, output_hanning_buffer);
}
}
}
