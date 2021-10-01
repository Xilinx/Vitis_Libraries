/*
 * Copyright 2021 Xilinx, Inc.
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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_

/*
FFT (1 channel DIT) Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
//#include <adf.h>
#include <type_traits>
#include <typeinfo>

#include "aie_api/aie_adf.hpp"

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
template <typename T_D, typename T_TW>
struct T_acc {};
template <>
struct T_acc<int16, cint16> {
    v8cacc48 valUpper = null_v8cacc48();
    v8cacc48 valLower = null_v8cacc48();
};
template <>
struct T_acc<cint16, cint16> {
    v8cacc48 valUpper = null_v8cacc48();
    v8cacc48 valLower = null_v8cacc48();
};
template <>
struct T_acc<int32, cint16> {
    v4cacc80 valUpper = null_v4cacc80();
    v4cacc80 valLower = null_v4cacc80();
};
template <>
struct T_acc<int32, cint32> {
    v4cacc80 valUpper = null_v4cacc80();
    v4cacc80 valLower = null_v4cacc80();
};
template <>
struct T_acc<cint32, cint16> {
    v4cacc80 valUpper = null_v4cacc80();
    v4cacc80 valLower = null_v4cacc80();
};
template <>
struct T_acc<cint32, cint32> {
    v2cacc80 valUpper = null_v2cacc80();
    v2cacc80 valLower = null_v2cacc80();
};
template <>
struct T_acc<float, cfloat> {
    v4cfloat valUpper = null_v4cfloat();
    v4cfloat valLower = null_v4cfloat();
};
template <>
struct T_acc<cfloat, cfloat> {
    v4cfloat valUpper = null_v4cfloat();
    v4cfloat valLower = null_v4cfloat();
};

template <typename T_D>
struct T_dXreg {};
template <>
struct T_dXreg<cint16> {
    v8cint16 val;
};
template <>
struct T_dXreg<cint32> {
    v8cint32 val;
};
template <>
struct T_dXreg<cfloat> {
    v8cfloat val;
};

template <typename T_D>
struct T_dYreg {};
template <>
struct T_dYreg<cint16> {
    v16cint16 val;
};
template <>
struct T_dYreg<cint32> {
    v8cint32 val;
};
template <>
struct T_dYreg<cfloat> {
    v8cfloat val;
};

template <typename T_TW>
struct T_twreg {};
template <>
struct T_twreg<cint16> {
    v8cint16 val;
};
template <>
struct T_twreg<cfloat> {
    v8cfloat val;
};

template <typename T_D>
T_D INLINE_DECL unitVector(){};
template <>
cint16 INLINE_DECL unitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cint32 INLINE_DECL unitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cfloat INLINE_DECL unitVector<cfloat>() {
    cfloat temp;
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

// new FFT functions with vectorization
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix2_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    constexpr unsigned int kStageRadix = 2;

    constexpr unsigned int shift_tw = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0++ = out[0];
            *it_out1++ = out[1];
        }
};

// Stage 0 radix 4. This is used in most internal stages.
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix4_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw1,
                                  const TT_TWIDDLE* tw2,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 4;
    const TT_TWIDDLE* dummytw = NULL; // not used, but NULL might fire defensive trap
    unsigned shift_tw = 15;
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw1, tw2, dummytw);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0 = out[0];
            it_out0 += kIndexStep;
            *it_out0 = out[1];
            it_out0 += kIndexStep;
            *it_out0 = out[2];
            it_out0 += kIndexStep;
            *it_out0 = out[3];
            it_out0 += -(3 * kIndexStep) + 1;
        }
};

//---------------------------
// r2 comb stage
template <typename TT_DATA, typename TT_TWIDDLE>
void INLINE_DECL r2comb_dit(const TT_DATA* x,
                            const TT_TWIDDLE* tw,
                            unsigned int n,
                            unsigned int r,
                            unsigned int shift,
                            TT_DATA* __restrict y,
                            bool inv){};

template <>
void INLINE_DECL r2comb_dit<cint16, cint16>(const cint16* x,
                                            const cint16* tw,
                                            unsigned int n,
                                            unsigned int r,
                                            unsigned int shift,
                                            cint16* __restrict y,
                                            bool inv) {
    stage_radix2_dit<cint16, cint16, cint16, 1>(x, tw, n, shift, y, inv);
};

template <>
void INLINE_DECL r2comb_dit<cint32, cint16>(const cint32* x,
                                            const cint16* tw,
                                            unsigned int n,
                                            unsigned int r,
                                            unsigned int shift,
                                            cint32* __restrict y,
                                            bool inv) {
    stage_radix2_dit<cint32, cint32, cint16, 1>(x, tw, n, shift, y, inv);
};

template <>
void INLINE_DECL r2comb_dit<cfloat, cfloat>(const cfloat* x,
                                            const cfloat* tw,
                                            unsigned int n,
                                            unsigned int r,
                                            unsigned int shift,
                                            cfloat* __restrict y,
                                            bool inv) {
    stage_radix2_dit<cfloat, cfloat, cfloat, 1>(x, tw, n, shift, y, inv);
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
