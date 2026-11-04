/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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

#ifndef FFT1D_VARIANT_R4_UTILS_HPP
#define FFT1D_VARIANT_R4_UTILS_HPP

#include "fft_1d_variant_types.hpp"

namespace xf {
namespace dsp {
namespace fft {

/**
 * Cooley–Tukey radix-4 DIT butterfly (one of N/4 butterflies in a stage).
 * Premultiply inputs by twiddles w1,w2,w3 then apply 4-point DFT (forward FFT
 * convention exp(-j*2*pi*k/N)).
 * INLINE off: separate module for scheduling / retiming vs. host pipeline (see fft1d_variant_r2_utils).
 */
template <typename T>
inline void calculate_radix4_dit(const T& x0,
                                 const T& x1,
                                 const T& x2,
                                 const T& x3,
                                 const T& w1,
                                 const T& w2,
                                 const T& w3,
                                 T& o0,
                                 T& o1,
                                 T& o2,
                                 T& o3) {
#pragma HLS INLINE off
    const T y0 = x0;
    const T y1 = w1 * x1;
    const T y2 = w2 * x2;
    const T y3 = w3 * x3;
    const T j(0.0f, 1.0f);
    // 4-point DFT (forward): X[k] = sum_n y[n]*exp(-j*2*pi*k*n/4)
    o0 = y0 + y1 + y2 + y3;
    o1 = y0 - j * y1 - y2 + j * y3;
    o2 = y0 - y1 + y2 - y3;
    o3 = y0 + j * y1 - y2 - j * y3;
}

} // namespace fft
} // namespace dsp
} // namespace xf

#endif // FFT1D_VARIANT_R4_UTILS_HPP
