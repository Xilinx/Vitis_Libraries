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
#include "catmull_rom.hpp"

#include "l1-libraries_aieml/mul/mul.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"

namespace us {
namespace L2 {

template <typename T>
void CatmullRomA1A2(

    adf::input_buffer<T>& point_0,
    adf::input_buffer<T>& point_1,
    adf::input_buffer<T>& point_2,
    adf::input_buffer<T>& t1t,
    adf::input_buffer<T>& tt0,
    adf::input_buffer<T>& t2t,
    adf::input_buffer<T>& tt1,

    adf::output_buffer<T>& output_a1,
    adf::output_buffer<T>& output_a2

    ) {
    T* point_0_buffer = (T*)point_0.data();
    T* point_1_buffer = (T*)point_1.data();
    T* point_2_buffer = (T*)point_2.data();
    T* t1t_buffer = (T*)t1t.data();
    T* tt0_buffer = (T*)tt0.data();
    T* t2t_buffer = (T*)t2t.data();
    T* tt1_buffer = (T*)tt1.data();

    T* output_a1_buffer = (T*)output_a1.data();
    T* output_a2_buffer = (T*)output_a2.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];

    // A1
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t1t_buffer, point_0_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt0_buffer, point_1_buffer, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, output_a1_buffer);

    // A2
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t2t_buffer, point_1_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt1_buffer, point_2_buffer, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, output_a2_buffer);
}

template <typename T>
void CatmullRomA3B1(

    adf::input_buffer<T>& output_a1,
    adf::input_buffer<T>& output_a2,
    adf::input_buffer<T>& point_2,
    adf::input_buffer<T>& point_3,
    adf::input_buffer<T>& t3t,
    adf::input_buffer<T>& tt2,
    adf::input_buffer<T>& t2t,
    adf::input_buffer<T>& tt0,

    adf::output_buffer<T>& output_a3,
    adf::output_buffer<T>& output_b1) {
    T* output_a1_buffer = (T*)output_a1.data();
    T* output_a2_buffer = (T*)output_a2.data();
    T* point_2_buffer = (T*)point_2.data();
    T* point_3_buffer = (T*)point_3.data();
    T* t3t_buffer = (T*)t3t.data();
    T* tt2_buffer = (T*)tt2.data();
    T* t2t_buffer = (T*)t2t.data();
    T* tt0_buffer = (T*)tt0.data();

    T* output_a3_buffer = (T*)output_a3.data();
    T* output_b1_buffer = (T*)output_b1.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];

    // A3
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t3t_buffer, point_2_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt2_buffer, point_3_buffer, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, output_a3_buffer);

    // B1
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t2t_buffer, output_a1_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt0_buffer, output_a2_buffer, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, output_b1_buffer);
}

template <typename T>
void CatmullRomB2C(

    adf::input_buffer<T>& output_a2,
    adf::input_buffer<T>& output_a3,
    adf::input_buffer<T>& output_b1,
    adf::input_buffer<T>& t3t,
    adf::input_buffer<T>& tt1,
    adf::input_buffer<T>& t2t,

    adf::output_buffer<T>& output_catmull_rom) {
    T* output_a2_buffer = (T*)output_a2.data();
    T* output_a3_buffer = (T*)output_a3.data();
    T* output_b1_buffer = (T*)output_b1.data();
    T* t3t_buffer = (T*)t3t.data();
    T* tt1_buffer = (T*)tt1.data();
    T* t2t_buffer = (T*)t2t.data();

    T* output_catmull_rom_buffer = (T*)output_catmull_rom.data();

    T buffer_out_v_1[LEN];
    T buffer_out_v_2[LEN];
    T buffer_out_v_3[LEN];

    // B2
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t3t_buffer, output_a2_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt1_buffer, output_a3_buffer, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, buffer_out_v_3);

    // C - Output
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(t2t_buffer, output_b1_buffer, buffer_out_v_1);
    us::L1::m_Mul<T, LEN, INCREMENT_V, SIMD_DEPTH>(tt1_buffer, buffer_out_v_3, buffer_out_v_2);
    us::L1::m_Sum<T, LEN, INCREMENT_V, SIMD_DEPTH>(buffer_out_v_1, buffer_out_v_2, output_catmull_rom_buffer);
}
}
}
