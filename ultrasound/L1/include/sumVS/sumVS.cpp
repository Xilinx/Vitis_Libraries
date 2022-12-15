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
void sumVSStream(input_window<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = readincr_v<SPACE_DIMENSION>(in2);
    op3 = aie::broadcast<float, VECDIM>(op2[1]);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        res = aie::add(op1, op3);

        window_writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVOne(input_window<T>* in1, output_stream<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(1);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        res = aie::add(op1, op2);

        writeincr(out, res);
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVOneSW(input_stream<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(1);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<VECDIM>(in1);

        res = aie::add(op1, op2);

        window_writeincr(out, res);
    }
};

} // namespace L1
} // namespace us

// void sumVOne(input_window<float>* in1, output_stream<float>* out){
//
//	aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(1);
//	aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();
//
//	for(unsigned i = 0; i < LENGTH; i+=SIMD_DEPTH){
//
//		window_readincr_v(in1, op1);
//
//		res = aie::add(op1, op2);
//
//		writeincr(out, res);
//	}
//
// };

void sumVOneStreamIn(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(1);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += 16) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::add(op1, op2);

        window_writeincr(out, res);
    }
};
