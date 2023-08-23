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
void diffMV(adf::input_buffer<T>& __restrict in1,
            adf::input_buffer<T>& __restrict in2,
            adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in1 = in1.data();
    T* __restrict p_in2 = in2.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, SPACE_DIMENSION> op2 = aie::zeros<T, SPACE_DIMENSION>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = aie::load_v<SPACE_DIMENSION>(p_in2);

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
        op1 = aie::load_v<VECDIM>(p_in1);
        p_in1 = byte_incr(p_in1, VECDIM * sizeof(T));

        res = aie::sub(op1, op3);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

} // namespace L1
} // namespace us
