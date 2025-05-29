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
#include "ones.hpp"

namespace us {
namespace L1 {

template <typename T, const unsigned T_LEN, const unsigned T_INCREMENT, const unsigned T_SIMD_DEPTH>
void Ones(adf::output_buffer<T>& output_vector) {
    T* __restrict p_out = output_vector.data();

    aie::vector<T, T_SIMD_DEPTH> res = aie::broadcast<T, T_SIMD_DEPTH>(1);

    for (unsigned i = 0; i < T_LEN; i += T_INCREMENT) {
        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, T_SIMD_DEPTH * sizeof(T));
    }
};

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void OnesInternalBuffer(adf::output_buffer<T>& output_vector) {
    T* buffer_out = (T*)output_vector.data();
    m_Ones<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(buffer_out);
}

// nested called

template <typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_Ones(T* output_vector) {
    aie::vector<T, T_SIMD_DEPTH> res = aie::broadcast<T, T_SIMD_DEPTH>(1);

    for (unsigned i = 0; i < T_LEN; i += T_INCREMENT) {
        aie::store_v(output_vector, res);
        output_vector = byte_incr(output_vector, T_SIMD_DEPTH * sizeof(T));
    }
}

// retrocompatibility

template <typename T, const unsigned int T_LEN, const unsigned int T_INCREMENT, const unsigned T_SIMD_DEPTH>
void ones(adf::output_buffer<T>& output_vector) {
    Ones<T, T_LEN, T_INCREMENT, T_SIMD_DEPTH>(output_vector);
}
}
}