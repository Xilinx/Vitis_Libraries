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
#include "diff.hpp"

namespace us {
namespace L1 {

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void Diff(adf::input_buffer<T>& input_vector_1,
          adf::input_buffer<T>& input_vector_2,
          adf::output_buffer<T>& output_vector) {
    aie::vector<T, T_SIMD_DEPTH> op_1 = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> op_2 = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            res = aie::sub(op_1, op_2);

            *iter_out = res;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void DiffInternalBuffer(adf::input_buffer<T>& input_vector_1,
                        adf::input_buffer<T>& input_vector_2,
                        adf::output_buffer<T>& output_vector) {
    T* buffer_in_1 = (T*)input_vector_1.data();
    T* buffer_in_2 = (T*)input_vector_2.data();
    T* buffer_out = (T*)output_vector.data();
    m_Diff<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in_1, buffer_in_2, buffer_out);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Diff(T* input_vector_1, T* input_vector_2, T* output_vector) {
    aie::vector<T, T_SIMD_DEPTH> op_1 = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> op_2 = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            res = aie::sub(op_1, op_2);

            *iter_out = res;

            iter_out++;
            iter_in_2++;
            iter_in_1++;
        }
}

// retrocompatibility

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void diffVV(adf::input_buffer<T>& input_vector_1,
            adf::input_buffer<T>& input_vector_2,
            adf::output_buffer<T>& output_vector) {
    Diff<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector_1, input_vector_2, output_vector);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void diffMM(adf::input_buffer<T>& input_matrix_1,
            adf::input_buffer<T>& input_matrix_2,
            adf::output_buffer<T>& output_matrix) {
    Diff<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_matrix_1, input_matrix_2, output_matrix);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void diffVS(adf::input_buffer<T>& input_vector,
            adf::input_buffer<T>& input_scalar,
            adf::output_buffer<T>& output_vector) {
    Diff<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector, input_scalar, output_vector);
}
}
}