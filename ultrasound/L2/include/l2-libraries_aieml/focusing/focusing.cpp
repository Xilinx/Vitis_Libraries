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
#include "focusing.hpp"
#include "common_defines.hpp"

#include "l1-libraries_aieml/diff/diff.cpp"
#include "l1-libraries_aieml/sqrt_v/sqrt_v.cpp"
#include "l1-libraries_aieml/square_v/square_v.cpp"
#include "l1-libraries_aieml/sum/sum.cpp"

namespace us {
namespace L2 {

template <typename T>
void Focusing(adf::input_buffer<T>& apo_ref_0,
              adf::input_buffer<T>& xdc_def_0,
              adf::input_buffer<T>& apo_ref_1,
              adf::input_buffer<T>& xdc_def_1,

              adf::output_buffer<T>& focusing_output) {
    T* apo_ref_0_b_in = (T*)apo_ref_0.data();
    T* apo_ref_1_b_in = (T*)apo_ref_1.data();
    T* xdc_def_0_b_in = (T*)xdc_def_0.data();
    T* xdc_def_1_b_in = (T*)xdc_def_1.data();

    T* focusing_output_b_out = (T*)focusing_output.data();

    T buffer_out1[LEN_FOCUSING];
    T buffer_out2[LEN_FOCUSING];
    T buffer_out3[LEN_FOCUSING];
    T buffer_out4[LEN_FOCUSING];

    us::L1::m_Diff<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(apo_ref_0_b_in, xdc_def_0_b_in, buffer_out1);
    us::L1::m_Diff<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(apo_ref_1_b_in, xdc_def_1_b_in, buffer_out2);
    us::L1::m_SquareV<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(buffer_out1, buffer_out3);
    us::L1::m_SquareV<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(buffer_out2, buffer_out4);
    us::L1::m_Sum<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(buffer_out3, buffer_out4, buffer_out1);

    us::L1::m_SqrtV<T, LEN_FOCUSING, INCREMENT_V, SIMD_DEPTH>(buffer_out1, focusing_output_b_out);
};
}
}
