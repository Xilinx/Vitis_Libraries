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

void divMS(input_window<float>* in1, output_window<float>* out) {
    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(INVERSE_SPEED_OF_SOUND);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_MATRIX) {
        window_readincr_v(in1, op1);

        res = aie::mul(op1, op2);

        window_writeincr(out, res);
    }
};
