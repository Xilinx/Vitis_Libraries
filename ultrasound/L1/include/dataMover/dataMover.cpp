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

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void dataMover(input_stream<T>* in1, output_window<T>* out1) {
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; ++i) {
        res = aie::broadcast<float, VECDIM>(readincr(in1));
        window_writeincr(out1, res);
    }
};

} // namespace L1
} // namespace us
