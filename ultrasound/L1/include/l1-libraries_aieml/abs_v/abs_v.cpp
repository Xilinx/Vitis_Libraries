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
#include "abs_v.hpp"

namespace us {
namespace L1 {

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<int32>& input_vector, adf::output_buffer<int32>& output) {
    aie::vector<int32, T_SIMD_DEPTH> op = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<int16>& input_vector, adf::output_buffer<int16>& output) {
    aie::vector<int16, T_SIMD_DEPTH> op = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<float>& input_vector, adf::output_buffer<float>& output) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::mul_square(op);
            res = aie::sqrt(res);
            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<cint16>& input_vector, adf::output_buffer<int32>& output) {
    aie::vector<cint16, T_SIMD_DEPTH> op = aie::zeros<cint16, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs_square(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void AbsV(adf::input_buffer<T_IN>& input_vector, adf::output_buffer<T_OUT>& output) {
    static_assert(std::is_same<T_IN, int32_t>::value || std::is_same<T_IN, int16_t>::value ||
                      std::is_same<T_IN, float>::value || std::is_same<T_IN, cint16>::value,
                  "T_IN must be int32, float or cint16");

    m_AbsV<T_IN, T_OUT, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector, output);
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(int32* input_vector, int32* output) {
    aie::vector<int32, T_SIMD_DEPTH> op = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(int16* input_vector, int16* output) {
    aie::vector<int16, T_SIMD_DEPTH> op = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(cint16* input_vector, int32* output) {
    aie::vector<cint16, T_SIMD_DEPTH> op = aie::zeros<cint16, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::abs_square(op);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(float* input_vector, float* output) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_vector);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            res = aie::mul_square(op);
            res = aie::sqrt(res);

            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void AbsVInternalBuffer(adf::input_buffer<T_IN>& input_vector, adf::output_buffer<T_OUT>& output) {
    static_assert(std::is_same<T_IN, int32_t>::value || std::is_same<T_IN, int16_t>::value ||
                      std::is_same<T_IN, float>::value || std::is_same<T_IN, cint16>::value,
                  "T_IN must be int32, float or cint16");

    T_IN* buffer_in = (T_IN*)input_vector.data();
    T_OUT* buffer_out = (T_OUT*)output.data();
    m_AbsV<T_IN, T_OUT, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in, buffer_out);
}
}
}
