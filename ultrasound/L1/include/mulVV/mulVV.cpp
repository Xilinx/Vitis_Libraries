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

// void mulVV(input_window<float>* in1, input_window<float>* in2, output_window<float>* out){
//
//	aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SIMD_DEPTH> op2 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();
//
//	for(unsigned i = 0; i < POINTS_PER_ITERATION; ++i){
//
//		window_readincr_v(in1, op1);
//		window_readincr_v(in2, op2);
//
//		res = aie::mul(op1, op2);
//
//		window_writeincr(out, res);
//	}
//
// };

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVV(input_window<T>* in1, input_window<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);
        window_readincr_v(in2, op2);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulVVStream(input_window<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, SIMD_DEPTH> op1 = aie::zeros<T, SIMD_DEPTH>();
    aie::vector<T, SIMD_DEPTH> op2 = aie::zeros<T, SIMD_DEPTH>();
    aie::vector<T, SIMD_DEPTH> res = aie::zeros<T, SIMD_DEPTH>();

    for (unsigned i = 0; i < LEN; ++i) {
        window_readincr_v(in1, op1);
        op2 = readincr_v<SIMD_DEPTH>(in2);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVVStreamOut(input_window<T>* in1, input_window<T>* in2, output_stream<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);
        window_readincr_v(in2, op2);

        res = aie::mul(op1, op2);

        writeincr(out, res);
    }
};

} // namespace L1
} // namespace us
