/*
 * Copyright 2022 Xilinx, Inc.
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
void diffSV(input_stream<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op3 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op3 = readincr_v<SPACE_DIMENSION>(in1);
    op2 = aie::broadcast<float, VECDIM>(op3[1]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<SIMD_DEPTH>(in2);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffOneV(input_stream<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = aie::broadcast<float, VECDIM>(1);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffOneVWW(input_window<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = aie::broadcast<float, VECDIM>(1);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);
        ;

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffOneLin(output_window<T>* out) {
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
        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffOneV(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(1);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += SIMD_DEPTH) {
        window_readincr_v(in1, op1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffOneVStreamIn(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(1);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffTwoV(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(2);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        window_readincr_v(in1, op1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffTwoLin(output_window<T>* out) {
    aie::vector<T, SIMD_DEPTH> op1 = aie::zeros<T, SIMD_DEPTH>();
    aie::vector<T, SIMD_DEPTH> op2 = aie::broadcast<T, SIMD_DEPTH>(2);
    aie::vector<T, SIMD_DEPTH> res = aie::zeros<T, SIMD_DEPTH>();

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
        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffTwoVStreamIn(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(2);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffThreeV(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(3);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        window_readincr_v(in1, op1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffThreeLin(output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(3);
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
        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

void diffThreeVStreamIn(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(3);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::sub(op2, op1);

        window_writeincr(out, res);
    }
};

} // namespace L1
} // namespace us
