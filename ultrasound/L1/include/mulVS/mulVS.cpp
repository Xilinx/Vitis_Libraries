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

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVS(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        op2 = aie::load_v<VECDIM>(p_in2);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));
        p_in2 = byte_incr(p_in2, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
}

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVSStream(input_stream<T>* in1, input_stream<T>* in2, output_stream<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op2 = readincr_v<SPACE_DIMENSION>(in2);

        for (unsigned j = 0; j < SPACE_DIMENSION; ++j) {
            op1 = readincr_v<VECDIM>(in1);

            op3 = aie::broadcast<T, VECDIM>(op2[j]);

            res = aie::mul(op1, op3);

            writeincr(out, res);
        }
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVSStreamIn(input_window<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = readincr_v<SPACE_DIMENSION>(in2);

    op3 = aie::broadcast<float, VECDIM>(op2[1]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op3);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVPi(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(PI); // 2*PI/2
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVPiStreamIn(input_stream<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(2 * PI / 2);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<VECDIM>(in1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulV1e_16(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<float, VECDIM> op2 = aie::broadcast<float, VECDIM>(1e-16);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVSWS(adf::input_buffer<T>& __restrict in1,
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

    op3 = aie::broadcast<T, VECDIM>(op2[0]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::mul(op1, op3);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

void mulVSSamplingFrequency(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(SAMPLING_FREQUENCY);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += SIMD_DEPTH) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

void mulV1eMin16(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(1e-16);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

void mulVTwo(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(2);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

// void mulVSStreamIn(input_window<float>* in1, input_stream<float>* in2, output_window<float>* out){
//
//	aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SPACE_DIMENSION> op2 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SIMD_DEPTH> op3 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();
//
//	op2 = readincr_v<SPACE_DIMENSION>(in2);
//
//	op3 = aie::concat(op2, op2, op2, op2);
//
//	for(unsigned i = 0; i < LENGTH; i+=INCREMENT_VECTOR){
//
//		window_readincr_v(in1, op1);
//
//		res = aie::mul(op1, op3);
//
//		window_writeincr(out, res);
//	}
//
// };

void mulV2FNumber(input_window<float>* in1, output_stream<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(2 * F_NUMBER);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        writeincr(out, res);
    }
};

void mulV2Pi2(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(2 * PI / 2);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVHalf(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(0.5);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulVHalfInt(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(0.5);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; ++i) {
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulLinHalf(adf::output_buffer<T>& __restrict out) {
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(0.5);
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
        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

void mulVSCRStream(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(0.5);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; ++i) {
        op1 = readincr_v<SIMD_DEPTH>(in1);
        res = aie::mul(op1, op2);
        window_writeincr(out, res);
    }
};

void mulVSCRWindowIn(input_window<float>* in1, input_window<float>* in2, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, 4> op3 = aie::zeros<float, 4>();
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; i += 4) { // POINTS_PER_ITERATION / 4, 4 points per iteration
        window_readincr_v(in2, op3);
        for (unsigned j = 0; j < 4; ++j) {
            window_readincr_v(in1, op1);
            op2 = aie::broadcast<float, SIMD_DEPTH>(op3[j]);
            res = aie::mul(op1, op2);
            window_writeincr(out, res);
        }
    }
};

void mulVSCRStreamIn(input_window<float>* in1, input_stream<float>* in2, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, 4> op3 = aie::zeros<float, 4>();
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; i += 4) { // POINTS_PER_ITERATION / 4, 4 points per iteration
        op3 = readincr_v<4>(in2);
        for (unsigned j = 0; j < 4; ++j) {
            window_readincr_v(in1, op1);
            op2 = aie::broadcast<float, SIMD_DEPTH>(op3[j]);
            res = aie::mul(op1, op2);
            window_writeincr(out, res);
        }
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulLinSCRStreamIn(adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
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
        op2 = aie::load_v<VECDIM>(p_in2);
        p_in2 = byte_incr(p_in2, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

void mulVSCRStreamIn2(input_stream<float>* in1, input_stream<float>* in2, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, 4> op3 = aie::zeros<float, 4>();
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < POINTS_PER_ITERATION; i += 4) { // POINTS_PER_ITERATION / 4, 4 points per iteration
        op3 = readincr_v<4>(in2);
        for (unsigned j = 0; j < 4; ++j) {
            op1 = readincr_v<SIMD_DEPTH>(in1);
            op2 = aie::broadcast<float, SIMD_DEPTH>(op3[j]);
            res = aie::mul(op1, op2);
            window_writeincr(out, res);
        }
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulVSCRSWindow(adf::input_buffer<T>& __restrict in1,
                    adf::input_buffer<T>& __restrict in2,
                    adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; ++i) {
        op1 = aie::load_v<VECDIM>(p_in1);
        op2 = aie::load_v<VECDIM>(p_in2);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));
        p_in2 = byte_incr(p_in2, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

} // namespace L1
} // namespace us
