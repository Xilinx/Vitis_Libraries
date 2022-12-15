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
void diffMV(input_stream<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = readincr_v<SPACE_DIMENSION>(in2);

#if SIMD_DEPTH == 4
    op3 = op2;
#endif
#if SIMD_DEPTH == 8
    op3 = aie::concat(op2, op2);
#endif
#if SIMD_DEPTH == 16
    op3 = aie::concat(op2, op2, op2, op2);
#endif

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<VECDIM>(in1);

        res = aie::sub(op1, op3);

        window_writeincr(out, res);
    }
};

// void diffMVDelay2(input_stream<float>* in1, output_window<float>* out){
//
//	aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, 4> op2 = aie::zeros<float, 4>();
//	aie::vector<float, SIMD_DEPTH> op3 = aie::zeros<float, SIMD_DEPTH>();
//	aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();
//
//	op2[0] = 0;
//	op2[1] = 0;
//	op2[2] = 0.02;
//	op2[3] = 0;
//
//	op3 = aie::concat(op2, op2, op2, op2);
//
//	for(unsigned i = 0; i < LENGTH; i+=INCREMENT_MATRIX){
//
//		op1 = readincr_v<SIMD_DEPTH>(in1);
//
//		res = aie::sub(op1, op3);
//
//		window_writeincr(out, res);
//	}
//
// };

} // namespace L1
} // namespace us
