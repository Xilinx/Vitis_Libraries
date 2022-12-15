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
void absV(input_window<T>* in1, output_stream<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        res = aie::abs(op1);

        writeincr(out, res);
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void absVWS(input_window<T>* in1, output_stream<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        res = aie::abs(op1);

        writeincr(out, res);
    }
};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void absVSW(input_stream<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<VECDIM>(in1);

        res = aie::abs(op1);

        window_writeincr(out, res);
    }
};

void absVStreamIn(input_stream<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        op1 = readincr_v<SIMD_DEPTH>(in1);

        res = aie::abs(op1);

        window_writeincr(out, res);
    }
};

} // namespace L1
} // namespace us
