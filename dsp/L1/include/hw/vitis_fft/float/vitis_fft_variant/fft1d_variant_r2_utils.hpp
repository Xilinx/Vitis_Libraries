/*
 * Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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

#ifndef FFT1D_VARIANT_R2_UTILS_HPP
#define FFT1D_VARIANT_R2_UTILS_HPP

#include "fft_1d_variant_types.hpp"

namespace xf {
namespace dsp {
namespace fft {

template <typename T>
inline void calculate_radix2(const T& in0, const T& in1, const T& twiddle, T& out0, T& out1) {
#pragma HLS INLINE
    T tw_mult = twiddle * in1;
    out0 = in0 + tw_mult;
    out1 = in0 - tw_mult;
}

} // namespace fft
} // namespace dsp
} // namespace xf

#endif // FFT1D_VARIANT_R2_UTILS_HPP
