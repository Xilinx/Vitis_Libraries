/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernels.hpp"

namespace us {
namespace L1 {

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffVS(adf::input_buffer<T>& __restrict in1,
            adf::input_buffer<T>& __restrict in2,
            adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = aie::load_v<SPACE_DIMENSION>(p_in2);
    op3 = aie::broadcast<T, VECDIM>(op2[1]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::sub(op1, op3);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffVSStreamOut(adf::input_buffer<T>& __restrict in1,
                     adf::input_buffer<T>& __restrict in2,
                     adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = aie::load_v<SPACE_DIMENSION>(p_in2);

    op3 = aie::broadcast<T, VECDIM>(op2[1]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::sub(op1, op3);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffLinOne(adf::output_buffer<T>& __restrict out) {
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(1);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

#if SIMD_DEPTH == 16

    op1[0] = 1.0;
    op1[1] = 1.06666667;
    op1[2] = 1.13333333;
    op1[3] = 1.2;
    op1[4] = 1.26666667;
    op1[5] = 1.33333333;
    op1[6] = 1.4;
    op1[7] = 1.46666667;
    op1[8] = 1.53333333;
    op1[9] = 1.6;
    op1[10] = 1.66666667;
    op1[11] = 1.73333333;
    op1[12] = 1.8;
    op1[13] = 1.86666667;
    op1[14] = 1.93333333;
    op1[15] = 2.0;

#endif
#if SIMD_DEPTH == 4

    op1[0] = 1.0;
    op1[1] = 1.33333333;
    op1[2] = 1.66666667;
    op1[3] = 2;
#endif
#if SIMD_DEPTH == 8

    op1[0] = 1.0;
    op1[1] = 1.14285714;
    op1[2] = 1.28571429;
    op1[3] = 1.42857143;
    op1[4] = 1.57142857;
    op1[5] = 1.71428571;
    op1[6] = 1.85714286;
    op1[7] = 2.0;

#endif

    for (unsigned i = 0; i < LEN; ++i) {
        res = aie::sub(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffLinTwo(adf::output_buffer<T>& __restrict out) {
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(2);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

#if SIMD_DEPTH == 16

    op1[0] = 1.0;
    op1[1] = 1.06666667;
    op1[2] = 1.13333333;
    op1[3] = 1.2;
    op1[4] = 1.26666667;
    op1[5] = 1.33333333;
    op1[6] = 1.4;
    op1[7] = 1.46666667;
    op1[8] = 1.53333333;
    op1[9] = 1.6;
    op1[10] = 1.66666667;
    op1[11] = 1.73333333;
    op1[12] = 1.8;
    op1[13] = 1.86666667;
    op1[14] = 1.93333333;
    op1[15] = 2.0;

#endif
#if SIMD_DEPTH == 4

    op1[0] = 1.0;
    op1[1] = 1.33333333;
    op1[2] = 1.66666667;
    op1[3] = 2;
#endif
#if SIMD_DEPTH == 8

    op1[0] = 1.0;
    op1[1] = 1.14285714;
    op1[2] = 1.28571429;
    op1[3] = 1.42857143;
    op1[4] = 1.57142857;
    op1[5] = 1.71428571;
    op1[6] = 1.85714286;
    op1[7] = 2.0;

#endif

    for (unsigned i = 0; i < LEN; ++i) {
        res = aie::sub(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

} // namespace L1
} // namespace us
