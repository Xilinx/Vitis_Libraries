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
#include "image_points.hpp"

#include "l1-libraries_aieml/ones/ones.cpp"
#include "l1-libraries_aieml/outer/outer.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"

template <typename T>
void ImagePoints(adf::input_buffer<T>& start_position,
                 adf::input_buffer<T>& directions,
                 adf::input_buffer<T>& samples_arange,
                 adf::output_buffer<T>& output_matrix) {
    T* start_position_buffer = (T*)start_position.data();
    T* directions_buffer = (T*)directions.data();
    T* samples_arange_buffer = (T*)samples_arange.data();
    T* output_matrix_buffer = (T*)output_matrix.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_m_1[LEN * M_COLUMNS];
    T buffer_out_m_2[LEN * M_COLUMNS];

    // ones
    us::L1::m_Ones<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1);
    // outer 1
    us::L1::m_Outer<T, LEN, M_COLUMNS, M_COLUMNS>(buffer_out_v_1, start_position_buffer, buffer_out_m_1);
    // outer 2
    us::L1::m_Outer<T, LEN, M_COLUMNS, M_COLUMNS>(samples_arange_buffer, directions_buffer, buffer_out_m_2);
    // sumMM
    us::L1::m_Sum<T, LEN, INCREMENT_M, SIMD_DEPTH>(buffer_out_m_1, buffer_out_m_2, output_matrix_buffer);
}
