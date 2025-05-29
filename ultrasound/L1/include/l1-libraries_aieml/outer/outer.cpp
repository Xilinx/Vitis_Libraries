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
#include "outer.hpp"

namespace us {
namespace L1 {

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void Outer(adf::input_buffer<T>& input_vector_1,
           adf::input_buffer<T>& input_vector_2,
           adf::output_buffer<T>& output_matrix) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");

    m_Outer<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_vector_1, input_vector_2, output_matrix);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
inline void m_Outer(adf::input_buffer<int32>& input_vector_1,
                    adf::input_buffer<int32>& input_vector_2,
                    adf::output_buffer<int32>& output_matrix) {
    aie::vector<int32, T_SIMD_DEPTH> op_1 = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> op_2 = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2)).template to_vector<int32>(0);
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
inline void m_Outer(adf::input_buffer<int16>& input_vector_1,
                    adf::input_buffer<int16>& input_vector_2,
                    adf::output_buffer<int16>& output_matrix) {
    aie::vector<int16, T_SIMD_DEPTH> op_1 = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> op_2 = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2)).template to_vector<int16>(0);
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
inline void m_Outer(adf::input_buffer<float>& input_vector_1,
                    adf::input_buffer<float>& input_vector_2,
                    adf::output_buffer<float>& output_matrix) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2));
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
inline void m_Outer(adf::input_buffer<bfloat16>& input_vector_1,
                    adf::input_buffer<bfloat16>& input_vector_2,
                    adf::output_buffer<bfloat16>& output_matrix) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2));
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void OuterInternalBuffer(adf::input_buffer<T>& input_vector_1,
                         adf::input_buffer<T>& input_vector_2,
                         adf::output_buffer<T>& output_matrix) {
    T* buffer_in_1 = (T*)input_vector_1.data();
    T* buffer_in_2 = (T*)input_vector_2.data();
    T* buffer_out = (T*)output_matrix.data();
    m_Outer<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in_1, buffer_in_2, buffer_out);
}

// nested called

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Outer(T* input_vector_1, T* input_vector_2, T* output_matrix) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value,
                  "T must be int32, int16, float or bfloat16");
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Outer(int32* input_vector_1, int32* input_vector_2, int32* output_matrix) {
    aie::vector<int32, T_SIMD_DEPTH> op_1 = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> op_2 = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1, T_LEN);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2)).template to_vector<int32>(0);
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Outer(int16* input_vector_1, int16* input_vector_2, int16* output_matrix) {
    aie::vector<int16, T_SIMD_DEPTH> op_1 = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> op_2 = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1, T_LEN);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2)).template to_vector<int16>(0);
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Outer(float* input_vector_1, float* input_vector_2, float* output_matrix) {
    aie::vector<float, T_SIMD_DEPTH> op_1 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> op_2 = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1, T_LEN);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2));
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Outer(bfloat16* input_vector_1, bfloat16* input_vector_2, bfloat16* output_matrix) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op_1 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> op_2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in_1 = aie::begin(input_vector_1, T_LEN);
    auto iter_in_2 = aie::begin_vector<T_SIMD_DEPTH>(input_vector_2);
    auto iter_out = aie::begin_vector<T_SIMD_DEPTH>(output_matrix);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op_2 = *iter_in_2;

            for (unsigned int j = 0; j < T_SIMD_DEPTH; ++j) chess_prepare_for_pipelining {
                    op_1 = aie::broadcast<T, T_SIMD_DEPTH>(*iter_in_1);

                    res = (aie::mul(op_1, op_2));
                    *iter_out = res;

                    iter_in_1++;
                    iter_out++;
                }

            iter_in_2++;
        }
}

// retrocompatibility

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void outer(adf::input_buffer<T>& input_vector_1,
           adf::input_buffer<T>& input_vector_2,
           adf::output_buffer<T>& output_matrix) {
    T* buffer_in_1 = (T*)input_vector_1.data();
    T* buffer_in_2 = (T*)input_vector_2.data();
    T* buffer_out = (T*)output_matrix.data();
    m_Outer<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in_1, buffer_in_2, buffer_out);
}
}
}
