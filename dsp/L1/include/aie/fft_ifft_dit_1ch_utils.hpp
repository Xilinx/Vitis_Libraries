/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include "device_defs.h"

#include "aie_api/aie_adf.hpp"

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

// function to return the accumulator sample width in AIE-ML (Cascade for parallel interconnect)
template <typename TT_DATA>
INLINE_DECL constexpr int fnFFTAccWidthCasc() {
    return -1; // will cause error by default
}
template <>
INLINE_DECL constexpr int fnFFTAccWidthCasc<cint16>() {
    return 32;
};
template <>
INLINE_DECL constexpr int fnFFTAccWidthCasc<cint32>() {
    return 64;
};

// function to return the accumulator vector in samples in AIE-ML (Cascade for parallel interconnect)
template <typename TT_DATA>
INLINE_DECL constexpr int fnFFTCascVWidth() {
    return -1; // will cause error by default
}
template <>
INLINE_DECL constexpr int fnFFTCascVWidth<cint16>() {
    return 8;
};
template <>
INLINE_DECL constexpr int fnFFTCascVWidth<cint32>() {
    return 4;
};
template <>
INLINE_DECL constexpr int fnFFTCascVWidth<cfloat>() {
    return 4;
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
    cfloat temp = {0.0, 0.0};
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

template <typename TT_TWIDDLE, unsigned int T_TW_MODE = 0>
INLINE_DECL constexpr unsigned int getTwShift() {
    printf("Error: unexpected twiddle type\n");
    return 0;
}; // default error trap
template <>
INLINE_DECL constexpr unsigned int getTwShift<cint16, 0>() {
    return 15;
};
template <>
INLINE_DECL constexpr unsigned int getTwShift<cint16, 1>() {
    return 14;
};
template <>
INLINE_DECL constexpr unsigned int getTwShift<cint32, 0>() {
    return 31;
};
template <>
INLINE_DECL constexpr unsigned int getTwShift<cint32, 1>() {
    return 30;
};
template <>
INLINE_DECL constexpr unsigned int getTwShift<cfloat, 0>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int getTwShift<cfloat, 1>() {
    return 0;
};

#define __24_1__

// new FFT functions with vectorization
template <typename TT_INPUT_DATA,
          typename TT_OUTPUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_R,
          unsigned int TP_TWIDDLE_MODE>
void INLINE_DECL stage_radix2_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();

#ifdef __24_1__
    ::aie::fft_dit_r2_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, tw, n, kTwShift, shift, inv, y);
#else  // 23_2
    constexpr unsigned int kStageRadix = 2;
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA,
                               TT_TWIDDLE>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
            *it_out0++ = out[0];
            *it_out1++ = out[1];
        }
#endif // ifdef __24_1__
};

// Stage 0 radix 4. This is used in most internal stages.
template <typename TT_INPUT_DATA,
          typename TT_OUTPUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_R,
          unsigned int TP_TWIDDLE_MODE>
void INLINE_DECL stage_radix4_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw1,
                                  const TT_TWIDDLE* tw2,
                                  const TT_TWIDDLE* tw3,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();

#ifdef __24_1__

    ::aie::fft_dit_r4_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, tw1, tw2, tw3, n, kTwShift, shift, inv,
                                                                             y);

#else // 23_2

    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 4;
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA,
                               TT_TWIDDLE>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw1, tw2, tw3);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / kStageRadix);
    const int block_size = n / (kStageRadix * FFT::out_vector_size);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
            auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
// in AIE1 or at least at one point the compiler failed to optimize the pointer
// handling correctly, so two pointers interlaced were required to allow the
// pointer arithmetic not to become an issue.
// In AIE-ML, or after and update to the compiler, this was no longer needed
#if __FFT_R4_IMPL__ == 0
            *it_out0 = out[0];
            it_out0 += block_size;
            *it_out0 = out[1];
            it_out0 += -block_size + 1;
            *it_out1 = out[2];
            it_out1 += block_size;
            *it_out1 = out[3];
            it_out1 += -block_size + 1;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
            *it_out0 = out[0];
            it_out0 += block_size;
            *it_out0 = out[1];
            it_out0 += block_size;
            *it_out0 = out[2];
            it_out0 += block_size;
            *it_out0 = out[3];
            it_out0 += -3 * block_size + 1;
#endif //__FFT_R4_IMPL__ == 1
        }
#endif // def __24_1__
};

//---------------------------
// r2 comb stage
template <typename TT_DATA, typename TT_TWIDDLE, unsigned TP_TWIDDLE_MODE>
void INLINE_DECL r2comb_dit(const TT_DATA* x,
                            const TT_TWIDDLE* tw,
                            unsigned int n,
                            unsigned int r,
                            unsigned int shift,
                            TT_DATA* __restrict y,
                            bool inv){};

template <>
void INLINE_DECL r2comb_dit<cint16, cint16, 0>(const cint16* x,
                                               const cint16* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint16* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint16, cint16, cint16, 1, 0>(x, tw, n, shift, y, inv);
};
template <>
void INLINE_DECL r2comb_dit<cint16, cint16, 1>(const cint16* x,
                                               const cint16* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint16* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint16, cint16, cint16, 1, 1>(x, tw, n, shift, y, inv);
};

template <>
void INLINE_DECL r2comb_dit<cint32, cint16, 0>(const cint32* x,
                                               const cint16* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint32* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint32, cint32, cint16, 1, 0>(x, tw, n, shift, y, inv);
};
template <>
void INLINE_DECL r2comb_dit<cint32, cint16, 1>(const cint32* x,
                                               const cint16* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint32* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint32, cint32, cint16, 1, 1>(x, tw, n, shift, y, inv);
};

#if __SUPPORTS_32B_TW__ == 1
template <>
void INLINE_DECL r2comb_dit<cint16, cint32, 0>(const cint16* x,
                                               const cint32* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint16* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint16, cint16, cint32, 1, 0>(x, tw, n, shift, y, inv); // Not supported in 23.2
};
template <>
void INLINE_DECL r2comb_dit<cint16, cint32, 1>(const cint16* x,
                                               const cint32* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint16* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint16, cint16, cint32, 1, 1>(x, tw, n, shift, y, inv); // Not supported in 23.2
};

template <>
void INLINE_DECL r2comb_dit<cint32, cint32, 0>(const cint32* x,
                                               const cint32* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint32* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint32, cint32, cint32, 1, 0>(x, tw, n, shift, y, inv);
};
template <>
void INLINE_DECL r2comb_dit<cint32, cint32, 1>(const cint32* x,
                                               const cint32* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cint32* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cint32, cint32, cint32, 1, 1>(x, tw, n, shift, y, inv);
};
#endif //__SUPPORTS_32B_TW__ == 1

template <>
void INLINE_DECL r2comb_dit<cfloat, cfloat, 0>(const cfloat* x,
                                               const cfloat* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cfloat* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cfloat, cfloat, cfloat, 1, 0>(x, tw, n, shift, y, inv);
};
template <>
void INLINE_DECL r2comb_dit<cfloat, cfloat, 1>(const cfloat* x,
                                               const cfloat* tw,
                                               unsigned int n,
                                               unsigned int r,
                                               unsigned int shift,
                                               cfloat* __restrict y,
                                               bool inv) {
    stage_radix2_dit<cfloat, cfloat, cfloat, 1, 1>(x, tw, n, shift, y, inv);
};

//-------------------------------------------------------------------------------------------------
// Unroll_for replacement functions.
// Function to optionally call a rank if it lies within the remit of this kernel
// static float stage handling
// NOte opt_cfloat_stage does not take TP_TWIDDLE_MODE because that is an integer consideration.
template <int stage, int TP_POINT_SIZE, int TP_START_RANK, int TP_END_RANK>
void INLINE_DECL opt_cfloat_stage(
    cfloat* xbuff, cfloat* obuff, cfloat** tmp_bufs, cfloat** tw_table, unsigned int& pingPong, bool& inv) {
    if
        constexpr(stage >= TP_START_RANK && stage < TP_END_RANK) {
            cfloat* outptr = (stage == TP_END_RANK - 1) ? obuff : tmp_bufs[1 - pingPong];
            cfloat* inptr = (stage == TP_START_RANK) ? xbuff : tmp_bufs[pingPong];
            stage_radix2_dit<cfloat, cfloat, cfloat, (TP_POINT_SIZE >> (1 + stage)), 0 /*TP_TWIDDLE_MODE*/>(
                (cfloat*)inptr, (cfloat*)tw_table[stage], TP_POINT_SIZE, 0, (cfloat*)outptr, inv);
            pingPong = 1 - pingPong;
        }
};

// dynamic float stage handling
template <int stage, int TP_POINT_SIZE, int TP_START_RANK, int TP_END_RANK, int kPointSizePower>
void INLINE_DECL opt_cfloat_dyn_stage(cfloat* xbuff,
                                      cfloat* obuff,
                                      cfloat** tmp_bufs,
                                      cfloat** tw_table,
                                      unsigned int& pingPong,
                                      bool& inv,
                                      int ptSizePwr) {
    if
        constexpr(stage >= TP_START_RANK && stage < TP_END_RANK) {
            int firstRank = kPointSizePower - ptSizePwr;
            if (stage >= firstRank) {
                cfloat* outptr = (stage == TP_END_RANK - 1) ? (cfloat*)obuff : (cfloat*)tmp_bufs[1 - pingPong];
                cfloat* inptr =
                    (stage == TP_START_RANK) || (stage == firstRank) ? (cfloat*)xbuff : (cfloat*)tmp_bufs[pingPong];
                stage_radix2_dit<cfloat, cfloat, cfloat, (TP_POINT_SIZE >> (1 + stage)), 0 /*TP_TWIDDLE_MODE*/>(
                    (cfloat*)inptr, (cfloat*)tw_table[stage - firstRank], (1 << ptSizePwr), 0, (cfloat*)outptr, inv);
                pingPong = 1 - pingPong;
            }
        }
};

// Static int stage handling
template <typename TT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          int TP_TWIDDLE_MODE,
          int stage,
          int TP_POINT_SIZE,
          int TP_SHIFT,
          int TP_START_RANK,
          int TP_END_RANK,
          int firstRank,
          int kPointSizePowerCeiled,
          int kPointSizeCeiled>
void INLINE_DECL opt_int_stage(TT_DATA* xbuff,
                               TT_OUT_DATA* obuff,
                               TT_INTERNAL_DATA** tmp_bufs,
                               TT_TWIDDLE** tw_table,
                               unsigned int& pingPong,
                               bool& inv) {
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();
    if
        constexpr(stage >= TP_START_RANK && stage < TP_END_RANK) {
            if
                constexpr(stage == firstRank) { // first stage and radix4
                    TT_DATA* inptr = xbuff;
                    if
                        constexpr(stage == TP_END_RANK - 2) {
                            if
                                constexpr(stage + 2 == kPointSizePowerCeiled) { // Not possible?
                                    TT_OUT_DATA* outptr = obuff;
                                    stage_radix4_dit<TT_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], TP_POINT_SIZE, kTwShift + TP_SHIFT, outptr,
                                        inv);
                                }
                            else {
                                TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)obuff;
                                stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    tw_table[12 + stage - firstRank], TP_POINT_SIZE, kTwShift, outptr, inv);
                            }
                        }
                    else {
                        TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)tmp_bufs[1 - pingPong];
                        stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                         TP_TWIDDLE_MODE>(
                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                            tw_table[12 + stage - firstRank], TP_POINT_SIZE, kTwShift, outptr, inv);
                    }
                }
            else if
                constexpr(stage + 1 == firstRank) { // radix2 stage - can't possible be the final stage overall, so no
                                                    // need for TP_SHIFT clause
                    TT_DATA* inptr = xbuff;
                    if
                        constexpr(stage + 1 == TP_END_RANK - 1) {
                            TT_OUT_DATA* outptr = obuff;
                            stage_radix2_dit<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                             TP_TWIDDLE_MODE>(inptr, tw_table[stage + 1 - firstRank], TP_POINT_SIZE,
                                                              kTwShift, outptr, inv);
                        }
                    else {
                        TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)tmp_bufs[1 - pingPong];
                        stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                         TP_TWIDDLE_MODE>(inptr, tw_table[stage + 1 - firstRank], TP_POINT_SIZE,
                                                          kTwShift, outptr, inv);
                    }
                }
            else { // not the first stage in the kernel
                TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                if
                    constexpr(stage == TP_END_RANK - 2) {
                        TT_OUT_DATA* outptr = obuff;
                        if
                            constexpr(stage + 2 == kPointSizePowerCeiled) {
                                stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    tw_table[12 + stage - firstRank], TP_POINT_SIZE, kTwShift + TP_SHIFT, outptr, inv);
                            }
                        else {
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], TP_POINT_SIZE, kTwShift, outptr, inv);
                        }
                    }
                else {
                    TT_INTERNAL_DATA* outptr = tmp_bufs[1 - pingPong];
                    stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                     TP_TWIDDLE_MODE>(inptr, tw_table[stage - firstRank],
                                                      tw_table[stage - firstRank + 1], tw_table[12 + stage - firstRank],
                                                      TP_POINT_SIZE, kTwShift, outptr, inv);
                }
            }
            pingPong = 1 - pingPong;
        }
};

// dynamic int stage handling
template <typename TT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          int TP_TWIDDLE_MODE,
          int stage,
          int TP_POINT_SIZE,
          int TP_SHIFT,
          int TP_START_RANK,
          int TP_END_RANK,
          int kPointSizePower,
          int kPointSizePowerCeiled,
          int kPointSizeCeiled>
void INLINE_DECL opt_int_dyn_stage(TT_DATA* xbuff,
                                   TT_OUT_DATA* obuff,
                                   TT_INTERNAL_DATA** tmp_bufs,
                                   TT_TWIDDLE** tw_table,
                                   unsigned int& pingPong,
                                   bool& inv,
                                   int ptSizePwr) {
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();

    if
        constexpr(stage >= TP_START_RANK && stage < TP_END_RANK) {
            int firstRank = kPointSizePowerCeiled - ptSizePwr;
            if (stage + 1 >= firstRank) {
                if
                    constexpr(stage == TP_END_RANK - 2) {
                        if
                            constexpr(stage + 2 == kPointSizePowerCeiled) { // end of FFT, so apply shift
                                TT_OUT_DATA* outptr = obuff;
                                if
                                    constexpr(stage == TP_START_RANK) { // TP_START_RANK is rounded to even number.
                                        TT_DATA* inptr = xbuff;
                                        stage_radix4_dit<TT_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                         (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                            tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift + TP_SHIFT,
                                            outptr, inv);
                                    }
                                else { // last in FFT, not start of kernel
                                    TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                                    stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift + TP_SHIFT, outptr,
                                        inv);
                                }
                            }
                        else { // end of kernel, not end of FFT
                            TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)obuff;
                            if
                                constexpr(stage == TP_START_RANK) { // TP_START_RANK is rounded to even number.
                                    TT_DATA* inptr = xbuff;
                                    if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                                  // overall, so no need for TT_OUT_DATA
                                        stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                         (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                            inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), kTwShift, outptr,
                                            inv);
                                    } else {
                                        stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                         (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                            tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                                    }
                                }
                            else {                        // end of kernel, not start kernel.
                                if (stage == firstRank) { // first stage of point size and radix4
                                    TT_DATA* inptr = xbuff;
                                    stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                                } else if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                                     // overall, so no need for TT_OUT_DATA
                                    TT_DATA* inptr = xbuff;
                                    stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                        inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), kTwShift, outptr,
                                        inv);
                                } else { // last in kernel, not first in kernel, not first in FFT
                                    TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                                    stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                                }
                            } // if start of kernel
                        }     // if end of FFT
                    }
                else { // not end of kernel
                    TT_INTERNAL_DATA* outptr = tmp_bufs[1 - pingPong];
                    if
                        constexpr(stage == TP_START_RANK) { // TP_START_RANK is rounded to even number.
                            TT_DATA* inptr = xbuff;
                            if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage overall,
                                                          // so no need for TT_OUT_DATA
                                stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                    inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                            } else {
                                stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                            }
                        }
                    else {                        // not last in kernel, not first in kernel.
                        if (stage == firstRank) { // first stage of point size and radix4
                            TT_DATA* inptr = xbuff;
                            stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                             TP_TWIDDLE_MODE>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                        } else if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                             // overall, so no need for TT_OUT_DATA
                            TT_DATA* inptr = xbuff;
                            stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage)),
                                             TP_TWIDDLE_MODE>(inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr),
                                                              kTwShift, outptr, inv);
                        } else { // not last in kernel, not first in kernel, not first in FFT
                            TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage)), TP_TWIDDLE_MODE>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], (1 << ptSizePwr), kTwShift, outptr, inv);
                        }
                    } // if start of kernel
                }     // if end of FFT
                pingPong = 1 - pingPong;
            } // if  stage involves processing
        }
};

//--------------------------------------------------------------
// Widget functions (dual stream interlace to window and vice versa

//-----------------------------------------------------------------------------------------------------
// read streams, write window/buffer
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readStreamIn(input_stream<TT_DATA>* __restrict inStream0,
                              input_stream<TT_DATA>* __restrict inStream1,
                              TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = __ALIGN_BYTE_SIZE__;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    t128VectorType readVal1;
    t128VectorType readVal2;
    t256VectorType writeVal;
    t128VectorType* out128Ptr0 = (t128VectorType*)&inBuff[0];
    t256VectorType* outPtr0 = (t256VectorType*)&inBuff[0];

    ::std::pair<t128VectorType, t128VectorType> inIntlv;
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    readVal1 = readincr_v<kSamplesIn128b, aie_stream_resource_in::a>(inStream0);
                    readVal2 = readincr_v<kSamplesIn128b, aie_stream_resource_in::b>(inStream1); // read, but ignored
                    *out128Ptr0++ = readVal1;
                }
            outPtr0 = (t256VectorType*)out128Ptr0;
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            readVal1 = readincr_v<kSamplesIn128b, aie_stream_resource_in::a>(inStream0);
            readVal2 = readincr_v<kSamplesIn128b, aie_stream_resource_in::b>(inStream1);
            inIntlv =
                ::aie::interleave_zip(readVal1, readVal2, 1); // convert to complex by interleaving zeros for imag parts
            writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
            *outPtr0++ = writeVal;
        }
}

// overload variant of above, for single stream in - yes this is an s2mm.
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readStreamIn(input_stream<TT_DATA>* __restrict inStream0, TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = __ALIGN_BYTE_SIZE__;
    constexpr int kSamplesIn256b = __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256VectorType readVal1;
    t256VectorType* out256Ptr0 = (t256VectorType*)&inBuff[0];

    for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize + TP_WINDOW_VSIZE / kSamplesIn256b;
         i++) // 16? bytes/ 128bits. 8/128 = 1/16
        chess_prepare_for_pipelining chess_loop_range(
            TP_HEADER_BYTES / kDataReadSize + TP_WINDOW_VSIZE / kSamplesIn256b, ) {
            readVal1 = readincr_v<kSamplesIn256b>(inStream0);
            *out256Ptr0++ = readVal1;
        }
}

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read casc/stream, write window/buffer

template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readCascStreamIn(
    input_cascade<typename t_accType<TT_DATA>::type>* __restrict inStream0, // input_cascade generates a warning
    input_stream<TT_DATA>* __restrict inStream1,
    TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 256 / 8 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t512VectorType = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    input_cascade<accTag>* __restrict inCasc0 = (input_cascade<accTag>*)inStream0;
    accVect_t acc;
    t256VectorType readVal1, readVal1a, readVal1b;
    t256VectorType readVal2;
    t256VectorType read256Val;
    t512VectorType writeVal;
    t256VectorType writeVal2a, writeVal2b;
    t256VectorType* outPtr0 = (t256VectorType*)&inBuff[0];
    ::std::pair<t256VectorType, t256VectorType> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    acc = readincr_v<kCascVWidth, accTag>(inCasc0);   // cascade read;
                    readVal1 = acc.template to_vector<TT_DATA>(0);    // convert acc type to standard type.
                    readVal2 = readincr_v<kSamplesIn256b>(inStream1); // read, but ignored
                    *outPtr0++ = readVal1;
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            acc = readincr_v<kCascVWidth, accTag>(inCasc0); // cascade read;
            readVal1 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
            readVal2 = readincr_v<kSamplesIn256b>(inStream1);
            inIntlv = ::aie::interleave_zip(readVal1, readVal2, 1);
            writeVal = ::aie::concat<t256VectorType, t256VectorType>(inIntlv.first, inIntlv.second);
            writeVal2a = writeVal.template extract<kSamplesIn256b>(0);
            *outPtr0++ = writeVal2a;
            writeVal2b = writeVal.template extract<kSamplesIn256b>(1);
            *outPtr0++ = writeVal2b;
        }
}

#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read stream/Casc, write window/buffer
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readStreamCascIn(
    input_stream<TT_DATA>* __restrict inStream0,
    input_cascade<typename t_accType<TT_DATA>::type>* __restrict inStream1, // input_cascade generates a warning
    TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 256 / 8 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t512VectorType = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    input_cascade<accTag>* __restrict inCasc1 = (input_cascade<accTag>*)inStream1;
    accVect_t acc;
    t256VectorType readVal1, readVal1a, readVal1b;
    t256VectorType readVal2;
    t256VectorType read256Val;
    t512VectorType writeVal;
    t256VectorType writeVal2a, writeVal2b;
    t256VectorType* outPtr0 = (t256VectorType*)&inBuff[0];
    ::std::pair<t256VectorType, t256VectorType> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    acc = readincr_v<kCascVWidth, accTag>(inCasc1);   // cascade read;
                    readVal1 = acc.template to_vector<TT_DATA>(0);    // convert acc type to standard type.
                    readVal2 = readincr_v<kSamplesIn256b>(inStream0); // read, but ignored
                    *outPtr0++ = readVal1;
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            acc = readincr_v<kCascVWidth, accTag>(inCasc1); // cascade read;
            readVal2 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
            readVal1 = readincr_v<kSamplesIn256b>(inStream0);
            inIntlv = ::aie::interleave_zip(readVal1, readVal2, 1);
            writeVal = ::aie::concat<t256VectorType, t256VectorType>(inIntlv.first, inIntlv.second);
            writeVal2a = writeVal.template extract<kSamplesIn256b>(0);
            *outPtr0++ = writeVal2a;
            writeVal2b = writeVal.template extract<kSamplesIn256b>(1);
            *outPtr0++ = writeVal2b;
        }
}
#endif //__SUPPORTS_ACC64__

//-----------------------------------------------------------------------------------------------------
// read window, write streams
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeStreamOut(output_stream<TT_DATA>* __restrict outStream0,
                                output_stream<TT_DATA>* __restrict outStream1,
                                TT_DATA* outBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val;
    t256VectorType read256Val;
    t128VectorType writeVal;
    t128VectorType out128a, out128b;
    t128VectorType* rdptr0 = (t128VectorType*)&outBuff[0];
    t256VectorType* rd256ptr = (t256VectorType*)&outBuff[0];

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    read128Val = *rdptr0++;
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val);
                    writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outStream1, read128Val);
                }
            rd256ptr = (t256VectorType*)rdptr0;
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val = *rd256ptr++;
            out128a = ::aie::filter_even<t256VectorType>(read256Val);
            out128b = ::aie::filter_odd<t256VectorType>(read256Val);
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out128a);
            writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outStream1, out128b);
        }
}

//-----------------------------------------------------------------------------------------------------
// read window, write stream
// overload variant of above for one stream - yes, this is an mm2s.
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeStreamOut(output_stream<TT_DATA>* __restrict outStream0, TT_DATA* outBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32; // bytes
    constexpr int kSamplesIn256b = 32 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256VectorType read256Val;
    t256VectorType writeVal;
    t256VectorType out256a;
    t256VectorType* rdptr0 = (t256VectorType*)&outBuff[0];

    for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize + TP_WINDOW_VSIZE / kSamplesIn256b; i++)
        chess_prepare_for_pipelining chess_loop_range(
            TP_HEADER_BYTES / kDataReadSize + TP_WINDOW_VSIZE / kSamplesIn256b, ) {
            read256Val = *rdptr0++;
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outStream0, read256Val);
        }
}

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read window, write casc/stream
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeCascStreamOut(
    output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream0, // output_cascade generates a warning
    output_stream<TT_DATA>* __restrict outStream1,
    TT_DATA* outBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 32 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t512VectorType = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256VectorType read256Val, read256Val0, read256Val1;
    t512VectorType read512Val;
    t256VectorType writeVal;
    t256VectorType out256a, out256b, out256a0, out256a1;
    t512VectorType out512a;
    t256VectorType* rdptr0 = (t256VectorType*)&outBuff[0];
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    accVect_t acc;
    output_cascade<accTag>* __restrict outCasc0 = (output_cascade<accTag>*)outStream0;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    read256Val = *rdptr0++;
                    acc = ::aie::from_vector<accTag>(read256Val);
                    writeincr<accTag, kCascVWidth>(outCasc0, acc);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outStream1, read256Val);
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val0 = *rdptr0++;
            read256Val1 = *rdptr0++;
            read512Val = ::aie::concat<t256VectorType, t256VectorType>(read256Val0, read256Val1);
            out256a = ::aie::filter_even<t512VectorType>(read512Val);
            out256b = ::aie::filter_odd<t512VectorType>(read512Val);
            acc = ::aie::from_vector<accTag>(out256a);
            writeincr<accTag, kCascVWidth>(outCasc0, acc);
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outStream1, out256b);
        }
}
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read window, write stream/casc
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeStreamCascOut(
    output_stream<TT_DATA>* __restrict outStream0,
    output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream1, // output_cascade generates a warning
    TT_DATA* outBuff) {
    constexpr int TP_HEADER_BYTES = __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 32 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t512VectorType = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256VectorType read256Val, read256Val0, read256Val1;
    t512VectorType read512Val;
    t256VectorType writeVal;
    t256VectorType out256a, out256b, out256b0, out256b1;
    t512VectorType out512b;
    t256VectorType* rdptr0 = (t256VectorType*)&outBuff[0];
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    accVect_t acc;
    output_cascade<accTag>* __restrict outCasc1 = (output_cascade<accTag>*)outStream1;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    read256Val = *rdptr0++;
                    acc = ::aie::from_vector<accTag>(read256Val);
                    writeincr<accTag, kCascVWidth>(outCasc1, acc);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outStream0, read256Val);
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val0 = *rdptr0++;
            read256Val1 = *rdptr0++;
            read512Val = ::aie::concat<t256VectorType, t256VectorType>(read256Val0, read256Val1);
            out256a = ::aie::filter_even<t512VectorType>(read512Val);
            out256b = ::aie::filter_odd<t512VectorType>(read512Val);
            acc = ::aie::from_vector<accTag>(out256b);
            writeincr<accTag, kCascVWidth>(outCasc1, acc);
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outStream0, out256a);
        }
}
#endif //__SUPPORTS_ACC64__
}
}
}
}
} // namespace closures

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
