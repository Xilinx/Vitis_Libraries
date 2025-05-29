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
#include "samples.hpp"

#include "l1-libraries_aieml/diff/diff.cpp"
#include "l1-libraries_aieml/norm_axis_1/norm_axis_1.cpp"
#include "l1-libraries_aieml/div/div.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"
#include "l1-libraries_aieml/ones/ones.cpp"
#include "l1-libraries_aieml/mul/mul.cpp"

template <typename T>
void Samples(adf::input_buffer<T>& image_points,
             adf::input_buffer<T>& delay,
             adf::input_buffer<T>& xdc_def_positions,
             adf::input_buffer<T>& sampling_frequency,
             adf::input_buffer<T>& speed_of_sound,
             adf::output_buffer<T>& output_vector) {
    T* image_points_buffer = (T*)image_points.data();
    T* xdc_def_positions_buffer = (T*)xdc_def_positions.data();
    T* delay_buffer = (T*)delay.data();
    T* sampling_frequency_buffer = (T*)sampling_frequency.data();
    T* speed_of_sound_buffer = (T*)speed_of_sound.data();

    T* output_vector_buffer = (T*)output_vector.data();

    T buffer_out_v_1[LEN];
    // float buffer_out_v_1_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];
    T buffer_out_v_4[LEN];
    T buffer_out_v_5[LEN];
    T buffer_out_m_1[LEN * M_COLUMNS];
    T buffer_out_m_2[LEN * M_COLUMNS];

    // diffMV
    us::L1::m_Diff<T, LEN, INCREMENT_M, SIMD_DEPTH>(image_points_buffer, xdc_def_positions_buffer, buffer_out_m_1);
    // us::L1::m_Diff< T, LEN, 1, M_COLUMNS >(image_points_buffer, xdc_def_positions_buffer, buffer_out_m_1);

    // normAxis1
    us::L1::m_NormAxis1<T, LEN, 1, M_COLUMNS>(buffer_out_m_1, buffer_out_v_1);
    // buffer_out_v_1_1 = aie::to_float(buffer_out_v_1);

    // divVS (divide by speed of sound)
    us::L1::m_Div<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, speed_of_sound_buffer, buffer_out_v_2);

    // sumVV
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_2, delay_buffer, buffer_out_v_3);

    // mulVS
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_3, sampling_frequency_buffer, buffer_out_v_4);

    // sumVS
    us::L1::m_Ones<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_5);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_4, buffer_out_v_5, output_vector_buffer);
}
