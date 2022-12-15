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
void sumMM(input_window<T>* in1, input_window<T>* in2, output_stream<T>* out) {
    // matrix of beamf are nx3, so we pad with 1 zero and operate row by row, count = n° rows
    aie::vector<float, VECDIM> op1 = aie::zeros<float, VECDIM>();
    aie::vector<float, VECDIM> op2 = aie::zeros<float, VECDIM>();
    aie::vector<float, VECDIM> res = aie::zeros<float, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);
        window_readincr_v(in2, op2);

        res = aie::add(op1, op2);

        writeincr(out, res);
    }
};
} // namespace L1
} // namespace us

void sumMM2Out(input_window<float>* in1,
               input_window<float>* in2,
               output_stream<float>* out1,
               output_stream<float>* out2) {
    // matrix of beamf are nx3, so we pad with 1 zero and operate row by row, count = n° rows
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += SPACE_DIMENSION) {
        window_readincr_v(in1, op1);
        window_readincr_v(in2, op2);

        res = aie::add(op1, op2);

        writeincr(out1, res);
        writeincr(out2, res);
    }
};
