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
#include "sqrt_v.hpp"

namespace us {
namespace L1 {

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_SqrtV(adf::input_buffer<int32>& input_vector, adf::output_buffer<int32>& output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    aie::vector<int32, T_SIMD_DEPTH> op_in = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res_out = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_in = *iter_in;
            op = aie::to_float(op_in);

            res = aie::sqrt(op);

            res_out = aie::to_fixed(res, 0);
            *iter_out = res_out;

            iter_out++;
            iter_in++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_SqrtV(adf::input_buffer<int16>& input_vector, adf::output_buffer<int16>& output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    aie::vector<int16, T_SIMD_DEPTH> op_in = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res_out = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_in = *iter_in;
            op = aie::to_float(op_in);

            res = aie::sqrt(op);

            res_out = aie::to_fixed(res, 0);
            *iter_out = res_out;

            iter_out++;
            iter_in++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_SqrtV(adf::input_buffer<float>& input_vector, adf::output_buffer<float>& output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::sqrt(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_SqrtV(adf::input_buffer<bfloat16>& input_vector, adf::output_buffer<bfloat16>& output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::sqrt(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
void SqrtV(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");

    m_SqrtV<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector, output_vector);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void SqrtVInternalBuffer(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& output_vector) {
    T* buffer_in = (T*)input_vector.data();
    T* buffer_out = (T*)output_vector.data();
    m_SqrtV<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in, buffer_out);
}

// nested called

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_SqrtV(T* input_vector, T* output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_SqrtV(int32* input_vector, int32* output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    aie::vector<T, T_SIMD_DEPTH> op_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res_out = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_in = *iter_in;
            op = aie::to_float(op_in);

            res = aie::sqrt(op);

            res_out = aie::to_fixed(res, 0);
            *iter_out = res_out;

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_SqrtV(int16* input_vector, int16* output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    aie::vector<T, T_SIMD_DEPTH> op_in = aie::zeros<T, T_SIMD_DEPTH>();
    aie::vector<T, T_SIMD_DEPTH> res_out = aie::zeros<T, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_in = *iter_in;
            op = aie::to_float(op_in);

            res = aie::sqrt(op);

            res_out = aie::to_fixed(res, 0);
            *iter_out = res_out;

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_SqrtV(float* input_vector, float* output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::sqrt(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_SqrtV(bfloat16* input_vector, bfloat16* output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::sqrt(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

// retrcompatibility

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void sqrtV(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& output_vector) {
    T* buffer_in = (T*)input_vector.data();
    T* buffer_out = (T*)output_vector.data();
    m_SqrtV<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in, buffer_out);
}
}
}
