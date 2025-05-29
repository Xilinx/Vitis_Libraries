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
#include "div.hpp"

namespace us {
namespace L1 {

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_Div(adf::input_buffer<int32>& input_vector_1,
                  adf::input_buffer<int32>& input_vector_2,
                  adf::output_buffer<int32>& output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    aie::vector<int32, T_SIMD_DEPTH> op_1_in = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> op_2_in = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res_out = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1_in = *iter_in_1;
            op_2_in = *iter_in_2;

            op_2 = aie::to_float(op_2_in);
            op_1 = aie::to_float(op_1_in);

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            res_out = aie::to_fixed(res, 0);

            *iter_out = res_out;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_Div(adf::input_buffer<int16>& input_vector_1,
                  adf::input_buffer<int16>& input_vector_2,
                  adf::output_buffer<int16>& output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    aie::vector<int16, T_SIMD_DEPTH> op_1_in = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> op_2_in = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res_out = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1_in = *iter_in_1;
            op_2_in = *iter_in_2;

            op_2 = aie::to_float(op_2_in);
            op_1 = aie::to_float(op_1_in);

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            res_out = aie::to_fixed(res, 0);

            *iter_out = res_out;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_Div(adf::input_buffer<float>& input_vector_1,
                  adf::input_buffer<float>& input_vector_2,
                  adf::output_buffer<float>& output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            *iter_out = res;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_Div(adf::input_buffer<bfloat16>& input_vector_1,
                  adf::input_buffer<bfloat16>& input_vector_2,
                  adf::output_buffer<bfloat16>& output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            *iter_out = res;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
void Div(adf::input_buffer<T>& input_vector_1,
         adf::input_buffer<T>& input_vector_2,
         adf::output_buffer<T>& output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");

    m_Div<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector_1, input_vector_2, output_vector);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void DivInternalBuffer(adf::input_buffer<T>& input_vector_1,
                       adf::input_buffer<T>& input_vector_2,
                       adf::output_buffer<T>& output_vector) {
    T* buffer_in_1 = (T*)input_vector_1.data();
    T* buffer_in_2 = (T*)input_vector_2.data();
    T* buffer_out = (T*)output_vector.data();
    m_Div<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in_1, buffer_in_2, buffer_out);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Div(T* input_vector_1, T* input_vector_2, T* output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Div(int32* input_vector_1, int32* input_vector_2, int32* output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    aie::vector<T, T_SIMD_DEPTH> op_1_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> op_2_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res_out = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1_in = *iter_in_1;
            op_2_in = *iter_in_2;

            op_2 = aie::to_float(op_2_in);
            op_1 = aie::to_float(op_1_in);

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            res_out = aie::to_fixed(res, 0);

            *iter_out = res_out;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Div(int16* input_vector_1, int16* input_vector_2, int16* output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    aie::vector<T, T_SIMD_DEPTH> op_1_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> op_2_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res_out = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1_in = *iter_in_1;
            op_2_in = *iter_in_2;

            op_2 = aie::to_float(op_2_in);
            op_1 = aie::to_float(op_1_in);

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            res_out = aie::to_fixed(res, 0);

            *iter_out = res_out;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Div(float* input_vector_1, float* input_vector_2, float* output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            *iter_out = res;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Div(bfloat16* input_vector_1, bfloat16* input_vector_2, bfloat16* output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_1 = *iter_in_1;
            op_2 = *iter_in_2;

            op_2 = aie::inv(op_2);

            res = (aie::mul(op_1, op_2));

            *iter_out = res;

            iter_out++;
            iter_in_1++;
            iter_in_2++;
        }
}

// retrocompatibility

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void divVS(adf::input_buffer<T>& input_vector,
           adf::input_buffer<T>& input_scalar,
           adf::output_buffer<T>& output_vector) {
    m_Div<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector, input_scalar, output_vector);
}
}
}
