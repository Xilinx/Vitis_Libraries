/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
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

#ifndef FFT_1D_VARIANT_TYPES_HPP
#define FFT_1D_VARIANT_TYPES_HPP

#include <complex>
#include <ap_int.h>
#include <hls_stream.h>

namespace xf {
namespace dsp {
namespace fft {

typedef std::complex<float> complex_t;

template <int NUM_SSR>
struct struct_fft_ssr {
    complex_t data[NUM_SSR];
    complex_t& operator[](int i) { return data[i]; }
    const complex_t& operator[](int i) const { return data[i]; }
};

struct struct_fft_ssr2 {
    complex_t data[2];
    struct_fft_ssr2() { data[0] = data[1] = complex_t(0.0f, 0.0f); }
    struct_fft_ssr2(complex_t d0, complex_t d1) {
        data[0] = d0;
        data[1] = d1;
    }
    complex_t& operator[](int i) { return data[i]; }
    const complex_t& operator[](int i) const { return data[i]; }
};

// Token protocol: 00=idle, 01=first(write only), 10=last(read only), 11=normal(write+read)
typedef ap_uint<2> token_t;

const token_t TOKEN_IDLE = 0b00;
const token_t TOKEN_FIRST = 0b01;
const token_t TOKEN_LAST = 0b10;
const token_t TOKEN_NORMAL = 0b11;

} // namespace fft
} // namespace dsp
} // namespace xf

#endif // FFT_1D_VARIANT_TYPES_HPP
