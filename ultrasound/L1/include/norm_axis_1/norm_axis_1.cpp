/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
void norm_axis_1(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    // euclidean norm per row, max 128 bit simd
    T* __restrict p_in = in1.data();
    auto outIter = aie::begin(out);

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> pow2 = aie::zeros<T, VECDIM>();

    T res;

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in);
        p_in = byte_incr(p_in, VECDIM * sizeof(T));

        pow2 = aie::mul_square(op1);
        res = aie::sqrt(aie::reduce_add(pow2));

        *outIter++ = res;
    }
};

} // namespace L1
} // namespace us
