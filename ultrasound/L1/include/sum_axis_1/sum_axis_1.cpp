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
void sum_axis_1(input_window<T>* in1, output_window<T>* out) {
    // we perform a reduced_add over every row, so we are limited using 128 bit simd
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        window_writeincr(out, aie::reduce_add(op1));
    }
};

} // namespace L1
} // namespace us
