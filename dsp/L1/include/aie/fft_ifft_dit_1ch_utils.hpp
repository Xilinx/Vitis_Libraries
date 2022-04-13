/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

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

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

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
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / kStageRadix);
    const int block_size = n / (kStageRadix * FFT::out_vector_size);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
            auto out = fft.dit(*it_stage++, shift_tw, shift, inv);

            *it_out0 = out[0];
            it_out0 += block_size;
            *it_out0 = out[1];
            it_out0 += -block_size + 1;
            *it_out1 = out[2];
            it_out1 += block_size;
            *it_out1 = out[3];
            it_out1 += -block_size + 1;
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

//-------------------------------------------------------------------------------------------------
// Unroll_for replacement functions.
// Function to optionally call a rank if it lies within the remit of this kernel
// static float stage handling
template <int stage, int TP_POINT_SIZE, int TP_START_RANK, int TP_END_RANK>
void INLINE_DECL opt_cfloat_stage(
    cfloat* xbuff, cfloat* obuff, cfloat** tmp_bufs, cfloat** tw_table, unsigned int& pingPong, bool& inv) {
    if
        constexpr(stage >= TP_START_RANK && stage < TP_END_RANK) {
            cfloat* outptr = (stage == TP_END_RANK - 1) ? obuff : tmp_bufs[1 - pingPong];
            cfloat* inptr = (stage == TP_START_RANK) ? xbuff : tmp_bufs[pingPong];
            stage_radix2_dit<cfloat, cfloat, cfloat, (TP_POINT_SIZE >> (1 + stage))>(
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
                stage_radix2_dit<cfloat, cfloat, cfloat, (TP_POINT_SIZE >> (1 + stage))>(
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
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        TP_POINT_SIZE, FFT_SHIFT15 + TP_SHIFT, outptr, inv);
                                }
                            else {
                                TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)obuff;
                                stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage))>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], TP_POINT_SIZE,
                                    FFT_SHIFT15, outptr, inv);
                            }
                        }
                    else {
                        TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)tmp_bufs[1 - pingPong];
                        stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], TP_POINT_SIZE,
                            FFT_SHIFT15, outptr, inv);
                    }
                }
            else if
                constexpr(stage + 1 == firstRank) { // radix2 stage - can't possible be the final stage overall, so no
                                                    // need for TP_SHIFT clause
                    TT_DATA* inptr = xbuff;
                    if
                        constexpr(stage + 1 == TP_END_RANK - 1) {
                            TT_OUT_DATA* outptr = obuff;
                            stage_radix2_dit<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage + 1 - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
                        }
                    else {
                        TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)tmp_bufs[1 - pingPong];
                        stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                            inptr, tw_table[stage + 1 - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
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
                                                 (kPointSizeCeiled >> (2 + stage))>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], TP_POINT_SIZE,
                                    FFT_SHIFT15 + TP_SHIFT, outptr, inv);
                            }
                        else {
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], TP_POINT_SIZE,
                                FFT_SHIFT15, outptr, inv);
                        }
                    }
                else {
                    TT_INTERNAL_DATA* outptr = tmp_bufs[1 - pingPong];
                    stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], TP_POINT_SIZE, FFT_SHIFT15,
                        outptr, inv);
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
                                                         (kPointSizeCeiled >> (2 + stage))>(
                                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                            (1 << ptSizePwr), FFT_SHIFT15 + TP_SHIFT, outptr, inv);
                                    }
                                else { // last in FFT, not start of kernel
                                    TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                                    stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        (1 << ptSizePwr), FFT_SHIFT15 + TP_SHIFT, outptr, inv);
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
                                                         (kPointSizeCeiled >> (2 + stage))>(
                                            inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), FFT_SHIFT15,
                                            outptr, inv);
                                    } else {
                                        stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                         (kPointSizeCeiled >> (2 + stage))>(
                                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                            (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                                    }
                                }
                            else {                        // end of kernel, not start kernel.
                                if (stage == firstRank) { // first stage of point size and radix4
                                    TT_DATA* inptr = xbuff;
                                    stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                                } else if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                                     // overall, so no need for TT_OUT_DATA
                                    TT_DATA* inptr = xbuff;
                                    stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr,
                                        inv);
                                } else { // last in kernel, not first in kernel, not first in FFT
                                    TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                                    stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
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
                                                 (kPointSizeCeiled >> (2 + stage))>(
                                    inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                            } else {
                                stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage))>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                            }
                        }
                    else {                        // not last in kernel, not first in kernel.
                        if (stage == firstRank) { // first stage of point size and radix4
                            TT_DATA* inptr = xbuff;
                            stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], (1 << ptSizePwr),
                                FFT_SHIFT15, outptr, inv);
                        } else if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                             // overall, so no need for TT_OUT_DATA
                            TT_DATA* inptr = xbuff;
                            stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                        } else { // not last in kernel, not first in kernel, not first in FFT
                            TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1], (1 << ptSizePwr),
                                FFT_SHIFT15, outptr, inv);
                        }
                    } // if start of kernel
                }     // if end of FFT
                pingPong = 1 - pingPong;
            } // if  stage involves processing
        }
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
