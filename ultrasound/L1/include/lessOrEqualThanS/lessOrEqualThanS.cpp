/*
 * Copyright 2021 Xilinx, Inc.
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

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
void lessOrEqualThanS(input_window<T>* in1, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res;

    for (unsigned i = 0; i < LEN; i += VECDIM) {
        window_readincr_v(in1, op1);

        for (unsigned j = 0; j < VECDIM; ++j) {
            res[j] = (T)(op1[j] <= static_cast<T>(SCALAR));
        }

        window_writeincr(out, res);
    }
};

} // namespace L1
} // namespace us