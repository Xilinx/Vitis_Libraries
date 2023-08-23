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
void divVSSpeedOfSound(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::broadcast<T, VECDIM>(INVERSE_SPEED_OF_SOUND);
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = aie::load_v<VECDIM>(p_in);
        p_in = byte_incr(p_in, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void divVSStream(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out) {
    T* __restrict p_in = in1.data();
    T* __restrict p_out = out.data();

    aie::vector<float, SIMD_DEPTH> op1 = aie::zeros<float, SIMD_DEPTH>();
    aie::vector<float, SIMD_DEPTH> op2 = aie::broadcast<float, SIMD_DEPTH>(INVERSE_SPEED_OF_SOUND);
    aie::vector<float, SIMD_DEPTH> res = aie::zeros<float, SIMD_DEPTH>();

    for (unsigned i = 0; i < LENGTH; i += INCREMENT_VECTOR) {
        op1 = aie::load_v<VECDIM>(p_in);
        p_in = byte_incr(p_in, VECDIM * sizeof(T));

        res = aie::mul(op1, op2);

        aie::store_v(p_out, res);
        p_out = byte_incr(p_out, VECDIM * sizeof(T));
    }
};

} // namespace L1
} // namespace us
