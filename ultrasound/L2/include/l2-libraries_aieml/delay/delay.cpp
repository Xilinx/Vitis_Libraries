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
#include "delay.hpp"

#include "l1-libraries_aieml/abs_v/abs_v.cpp"
#include "l1-libraries_aieml/diff/diff.cpp"
#include "l1-libraries_aieml/div/div.cpp"
#include "l1-libraries_aieml/mul/mul.cpp"
#include "l1-libraries_aieml/norm_axis_1/norm_axis_1.cpp"
#include "l1-libraries_aieml/sign/sign.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"
#include "l1-libraries_aieml/sum_axis_1/sum_axis_1.cpp"
#include "l1-libraries_aieml/tile_v/tile_v.cpp"

namespace us {
namespace L2 {

template <typename T>
void Delay1(

    adf::input_buffer<T>& image_points_from_pl_1,
    adf::input_buffer<T>& tx_def_ref_point,
    adf::input_buffer<T>& tx_def_delay_distance_1,
    adf::input_buffer<T>& direction,

    adf::output_buffer<T>& sign_delay_output

    ) {
    T* image_points_from_pl_1_buffer = (T*)image_points_from_pl_1.data();
    T* tx_def_delay_distance_1_buffer = (T*)tx_def_delay_distance_1.data();
    T* tx_def_ref_point_buffer = (T*)tx_def_ref_point.data();
    T* direction_buffer = (T*)direction.data();

    T* sign_delay_output_buffer = (T*)sign_delay_output.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_m_1[LEN * M_COLUMNS];
    T buffer_out_m_2[LEN * M_COLUMNS];
    T buffer_out_m_3[LEN * M_COLUMNS];

    // DiffMV
    us::L1::m_Diff<T, LEN, INCREMENT_M, SIMD_DEPTH>(image_points_from_pl_1_buffer, tx_def_ref_point_buffer,
                                                    buffer_out_m_1);
    // TileV
    us::L1::m_TileV<T, LEN, INCREMENT_M, SIMD_DEPTH>(direction_buffer, buffer_out_m_2);
    // MulMM
    us::L1::m_Mul<T, LEN, INCREMENT_M, SIMD_DEPTH>(buffer_out_m_1, buffer_out_m_2, buffer_out_m_3);
    // SumAxis1
    us::L1::m_SumAxis1<T, LEN, 1, M_COLUMNS>(buffer_out_m_3, buffer_out_v_1);
    // AbsV
    us::L1::m_AbsV<T, T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2);
    // DiffVS
    us::L1::m_Diff<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, tx_def_delay_distance_1_buffer, buffer_out_v_1);
    // Sign
    us::L1::m_Sign<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, sign_delay_output_buffer);
}

template <typename T>
void Delay2(

    adf::input_buffer<T>& sign_delay_input,
    adf::input_buffer<T>& image_points_from_pl_2,
    adf::input_buffer<T>& tx_def_delay_distance_2,
    adf::input_buffer<T>& tx_def_focal_point,
    adf::input_buffer<T>& t_start,
    adf::input_buffer<T>& speed_of_sound,

    adf::output_buffer<T>& delay_output

    ) {
    T* sign_delay_input_buffer = (T*)sign_delay_input.data();
    T* image_points_from_pl_2_buffer = (T*)image_points_from_pl_2.data();
    T* tx_def_delay_distance_2_buffer = (T*)tx_def_delay_distance_2.data();
    T* tx_def_focal_point_buffer = (T*)tx_def_focal_point.data();
    T* t_start_buffer = (T*)t_start.data();
    T* speed_of_sound_buffer = (T*)speed_of_sound.data();

    T* delay_output_buffer = (T*)delay_output.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];
    T buffer_out_m_1[LEN * M_COLUMNS];

    // DiffMV
    us::L1::m_Diff<T, LEN, INCREMENT_M, SIMD_DEPTH>(image_points_from_pl_2_buffer, tx_def_focal_point_buffer,
                                                    buffer_out_m_1);
    // NormAxis1
    us::L1::m_NormAxis1<T, LEN, 1, M_COLUMNS>(buffer_out_m_1, buffer_out_v_1);
    //	//MulVV
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(sign_delay_input_buffer, buffer_out_v_1, buffer_out_v_3);
    //	//SumVS
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_3, tx_def_delay_distance_2_buffer, buffer_out_v_1);
    //	//DivVS
    us::L1::m_Div<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, speed_of_sound_buffer, buffer_out_v_2);
    //	//DiffVS
    us::L1::m_Diff<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, t_start_buffer, delay_output_buffer);
}
}
}
