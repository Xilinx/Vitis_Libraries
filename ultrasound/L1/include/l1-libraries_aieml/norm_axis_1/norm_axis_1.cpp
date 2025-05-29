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
#include "norm_axis_1.hpp"

namespace us {
namespace L1 {

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_NormAxis1(adf::input_buffer<int32>& input_matrix, adf::output_buffer<int32>& output_vector) {
    aie::vector<int32, T_SIMD_DEPTH> op = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> pow2 = aie::zeros<int32, T_SIMD_DEPTH>();

    aie::vector<float, T_SIMD_DEPTH> op_f = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res_f = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    // auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);
    auto iter_out = aie::begin(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op)).template to_vector<T>(0);
            res = aie::reduce_add_v(pow2);

            op_f = aie::to_float(res);

            res_f = aie::sqrt(op_f);

            res = aie::to_fixed(res_f, 0);

            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
};

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_NormAxis1(adf::input_buffer<int16>& input_matrix, adf::output_buffer<int16>& output_vector) {
    aie::vector<int16, T_SIMD_DEPTH> op = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> pow2 = aie::zeros<int16, T_SIMD_DEPTH>();

    aie::vector<float, T_SIMD_DEPTH> op_f = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res_f = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    // auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);
    auto iter_out = aie::begin(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op)).template to_vector<T>(0);
            res = aie::reduce_add_v(pow2);

            op_f = aie::to_float(res);

            res_f = aie::sqrt(op_f);

            res = aie::to_fixed(res_f, 0);

            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
};

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_NormAxis1(adf::input_buffer<float>& input_matrix, adf::output_buffer<float>& output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    // aie::vector< float, T_SIMD_DEPTH > res = aie::zeros< float, T_SIMD_DEPTH >();
    aie::vector<float, T_SIMD_DEPTH> pow2 = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    // auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);
    auto iter_out = aie::begin(output_vector);

    float res;

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op));
            res = aie::sqrt(aie::reduce_add(pow2));
            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_NormAxis1(adf::input_buffer<bfloat16>& input_matrix, adf::output_buffer<bfloat16>& output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> pow2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    // auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);
    auto iter_out = aie::begin(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op));
            res = aie::sqrt(aie::reduce_add(pow2));
            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
}

/*
template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
inline void m_NormAxis1(adf::input_buffer<cint16>& input_matrix, adf::output_buffer<cint16>& output_vector){

    aie::vector<cint16, T_SIMD_DEPTH> op = aie::zeros<cint16, T_SIMD_DEPTH>();
        aie::vector< cint16, T_SIMD_DEPTH > res = aie::zeros< cint16, T_SIMD_DEPTH >();
    aie::vector<cint16, T_SIMD_DEPTH> pow2 = aie::zeros<cint16, T_SIMD_DEPTH>();

        auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_matrix);
        //auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);
        auto iter_out = aie::begin(output_vector);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
                chess_prepare_for_pipelining {

        op = *iter_in;

        pow2 = (aie::mul_square(op)).template to_vector< T >(0);
                res = aie::sqrt(aie::reduce_add(pow2));
                *iter_out = res[0];

                iter_out++;
                iter_in++;
    }

}
*/

// generic type not supported

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
void NormAxis1(adf::input_buffer<T>& input_matrix, adf::output_buffer<T>& output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value || std::is_same<T, cint16>::value,
                  "T must be int32, int16, float, cint16 or bfloat16");

    m_NormAxis1<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(input_matrix, output_vector);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void NormAxis1InternalBuffer(adf::input_buffer<T>& input_matrix, adf::output_buffer<T>& output_vector) {
    T* buffer_in = (T*)input_matrix.data();
    T* buffer_out = (T*)output_vector.data();
    m_NormAxis1<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in, buffer_out);
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(int32* input_matrix, int32* output_vector) {
    aie::vector<int32, T_SIMD_DEPTH> op = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> res = aie::zeros<int32, T_SIMD_DEPTH>();
    aie::vector<int32, T_SIMD_DEPTH> pow2 = aie::zeros<int32, T_SIMD_DEPTH>();

    aie::vector<float, T_SIMD_DEPTH> op_f = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res_f = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    auto iter_out = aie::begin(output_vector, T_LEN);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op)).template to_vector<T>(0);
            res = aie::reduce_add_v(pow2);

            op_f = aie::to_float(res);
            res_f = aie::sqrt(op_f);

            res = aie::to_fixed(res_f, 0);

            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(int16* input_matrix, int16* output_vector) {
    aie::vector<int16, T_SIMD_DEPTH> op = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> res = aie::zeros<int16, T_SIMD_DEPTH>();
    aie::vector<int16, T_SIMD_DEPTH> pow2 = aie::zeros<int16, T_SIMD_DEPTH>();

    aie::vector<float, T_SIMD_DEPTH> op_f = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> res_f = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    auto iter_out = aie::begin(output_vector, T_LEN);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op)).template to_vector<T>(0);
            res = aie::reduce_add_v(pow2);

            op_f = aie::to_float(res);
            res_f = aie::sqrt(op_f);

            res = aie::to_fixed(res_f, 0);

            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
}
/*
template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_NormAxis1(float *input_matrix, float *output_vector){

    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
        aie::vector< float, T_SIMD_DEPTH > res = aie::zeros< float, T_SIMD_DEPTH >();
    aie::vector<float, T_SIMD_DEPTH> pow2 = aie::zeros<float, T_SIMD_DEPTH>();

        auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_matrix);
        auto iter_out = aie::begin(output_vector, T_LEN);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
                chess_prepare_for_pipelining {

        op = *iter_in;

        pow2 = (aie::mul_square(op));
                res = aie::sqrt(aie::reduce_add(pow2));
                *iter_out = res[0];

                iter_out++;
                iter_in++;
    }
}
*/
template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(float* input_matrix, float* output_vector) {
    aie::vector<float, T_SIMD_DEPTH> op = aie::zeros<float, T_SIMD_DEPTH>();
    aie::vector<float, T_SIMD_DEPTH> pow2 = aie::zeros<float, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    auto iter_out = aie::begin(output_vector, T_LEN);

    float res;

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op));
            res = aie::sqrt(aie::reduce_add(pow2));
            *iter_out = res;

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(bfloat16* input_matrix, bfloat16* output_vector) {
    aie::vector<bfloat16, T_SIMD_DEPTH> op = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> res = aie::zeros<bfloat16, T_SIMD_DEPTH>();
    aie::vector<bfloat16, T_SIMD_DEPTH> pow2 = aie::zeros<bfloat16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    auto iter_out = aie::begin(output_vector, T_LEN);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op));
            res = aie::sqrt(aie::reduce_add(pow2));
            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(cint16* input_matrix, cint16* output_vector) {
    aie::vector<cint16, T_SIMD_DEPTH> op = aie::zeros<cint16, T_SIMD_DEPTH>();
    aie::vector<cint16, T_SIMD_DEPTH> pow2 = aie::zeros<cint16, T_SIMD_DEPTH>();
    aie::vector<cint16, T_SIMD_DEPTH> res = aie::zeros<cint16, T_SIMD_DEPTH>();

    auto iter_in = aie::begin_vector<T_SIMD_DEPTH>(input_matrix);
    auto iter_out = aie::begin(output_vector, T_LEN);

    for (unsigned int i = 0; i < T_LEN; i += T_INCREMENT) chess_prepare_for_pipelining {
            op = *iter_in;

            pow2 = (aie::mul_square(op));
            res = aie::sqrt(aie::reduce_add(pow2));
            *iter_out = res[0];

            iter_out++;
            iter_in++;
        }
}

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_NormAxis1(T* input_matrix, T* output_vector) {
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, bfloat16>::value || std::is_same<T, cint16>::value,
                  "T must be int32, int16, float or bfloat16");
}

// retrocompatibility

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
void norm_axis_1(adf::input_buffer<T>& input_matrix, adf::output_buffer<T>& output_vector) {
    T* buffer_in = (T*)input_matrix.data();
    T* buffer_out = (T*)output_vector.data();

    m_NormAxis1<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_in, buffer_out);
}
}
}
