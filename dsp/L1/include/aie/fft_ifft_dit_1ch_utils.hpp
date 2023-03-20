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
                                  const TT_TWIDDLE* tw3,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 4;
    //  const TT_TWIDDLE* dummytw = NULL; //not used, but NULL might fire defensive trap
    unsigned shift_tw = 15;
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw1, tw2, tw3);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / kStageRadix);
    const int block_size = n / (kStageRadix * FFT::out_vector_size);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
            auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
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

#if __SUPPORTS_CFLOAT__ == 1
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
#endif //__SUPPORTS_CFLOAT__

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
    //  printf("stage = %d, TP_START_RANK = %d, TP_END_RANK = %d, firstRank = %d, kPointSizePowerCeiled = %d\n", stage,
    //  TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled);
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
                                        tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15 + TP_SHIFT, outptr,
                                        inv);
                                }
                            else {
                                TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)obuff;
                                stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                 (kPointSizeCeiled >> (2 + stage))>(
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
                            }
                        }
                    else {
                        TT_INTERNAL_DATA* outptr = (TT_INTERNAL_DATA*)tmp_bufs[1 - pingPong];
                        stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                            inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                            tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
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
                                    inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                    tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15 + TP_SHIFT, outptr,
                                    inv);
                            }
                        else {
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
                        }
                    }
                else {
                    TT_INTERNAL_DATA* outptr = tmp_bufs[1 - pingPong];
                    stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                        tw_table[12 + stage - firstRank], TP_POINT_SIZE, FFT_SHIFT15, outptr, inv);
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
                                            tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15 + TP_SHIFT,
                                            outptr, inv);
                                    }
                                else { // last in FFT, not start of kernel
                                    TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                                    stage_radix4_dit<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15 + TP_SHIFT,
                                        outptr, inv);
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
                                            tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr,
                                            inv);
                                    }
                                }
                            else {                        // end of kernel, not start kernel.
                                if (stage == firstRank) { // first stage of point size and radix4
                                    TT_DATA* inptr = xbuff;
                                    stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                                     (kPointSizeCeiled >> (2 + stage))>(
                                        inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
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
                                        tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
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
                                    tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                            }
                        }
                    else {                        // not last in kernel, not first in kernel.
                        if (stage == firstRank) { // first stage of point size and radix4
                            TT_DATA* inptr = xbuff;
                            stage_radix4_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                        } else if (stage + 1 == firstRank) { // radix2 stage - can't possibly be the final stage
                                                             // overall, so no need for TT_OUT_DATA
                            TT_DATA* inptr = xbuff;
                            stage_radix2_dit<TT_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage + 1 - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
                        } else { // not last in kernel, not first in kernel, not first in FFT
                            TT_INTERNAL_DATA* inptr = tmp_bufs[pingPong];
                            stage_radix4_dit<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE,
                                             (kPointSizeCeiled >> (2 + stage))>(
                                inptr, tw_table[stage - firstRank], tw_table[stage - firstRank + 1],
                                tw_table[12 + stage - firstRank], (1 << ptSizePwr), FFT_SHIFT15, outptr, inv);
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
    //  printf("Entering readStreamIn with window size %d\n", (int)(TP_WINDOW_VSIZE+TP_DYN_PT_SIZE*32/sizeof(TT_DATA)));
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    // T_buff_128b<TT_DATA> readVal1;
    // T_buff_128b<TT_DATA> readVal2;
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
                    // readVal1 = stream_readincr_128b(inStream0, 0);
                    // readVal2 = stream_readincr_128b(inStream1, 1); //read but ignored
                    readVal1 = readincr_v<kSamplesIn128b, aie_stream_resource_in::a>(inStream0);
                    readVal2 = readincr_v<kSamplesIn128b, aie_stream_resource_in::b>(inStream1); // read, but ignored
                    *out128Ptr0++ = readVal1;
                }
            outPtr0 = (t256VectorType*)out128Ptr0;
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            // readVal1 = stream_readincr_128b(inStream0, 0);
            // readVal2 = stream_readincr_128b(inStream1, 1);
            readVal1 = readincr_v<kSamplesIn128b, aie_stream_resource_in::a>(inStream0);
            readVal2 = readincr_v<kSamplesIn128b, aie_stream_resource_in::b>(inStream1);
            // inIntlv = ::aie::interleave_zip(readVal1.val ,readVal2.val, 1);//convert to complex by interleaving zeros
            // for imag parts
            inIntlv =
                ::aie::interleave_zip(readVal1, readVal2, 1); // convert to complex by interleaving zeros for imag parts
            writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
            *outPtr0++ = writeVal;
        }
}

// overload variant of above, for single stream in - yes this is an s2mm.
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readStreamIn(input_stream<TT_DATA>* __restrict inStream0, TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType readVal1;
    t128VectorType* out128Ptr0 = (t128VectorType*)&inBuff[0];

    for (int i = 0; i < TP_HEADER_BYTES / 16 + TP_WINDOW_VSIZE / kSamplesIn128b; i++) // 16? bytes/ 128bits. 8/128 =
                                                                                      // 1/16
        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16 + TP_WINDOW_VSIZE / kSamplesIn128b, ) {
            // readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0);
            readVal1 = readincr_v<kSamplesIn128b>(inStream0);
            //    ::aie::print(readVal1, true, " readVal1");
            *out128Ptr0++ = readVal1;
        }
}

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read casc/stream, write window/buffer
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readCascStreamIn(input_stream<cacc64>* __restrict inStream0,
                                  input_stream<TT_DATA>* __restrict inStream1,
                                  TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using accVect_t = ::aie::detail::accum<fnAccClass<TT_DATA>(), // int, cint, FP or CFP
                                           64,                    // acc width
                                           4>; // both cint16 and cint32 use 4 * cacc64kSamplesIn128b>;
    accVect_t acc;
    t128VectorType readVal1, readVal1a, readVal1b;
    t128VectorType readVal2;
    t256VectorType read256Val;
    t256VectorType writeVal;
    t128VectorType* out128Ptr0 = (t128VectorType*)&inBuff[0];
    t256VectorType* outPtr0 = (t256VectorType*)&inBuff[0];
    ::std::pair<t128VectorType, t128VectorType> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            if
                constexpr(kSamplesIn128b == 4) {                   // ie 4 cint16 in 128b
                    for (int i = 0; i < TP_HEADER_BYTES / 16; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                            acc = readincr_v4(inStream0);                  // cascade read;
                            readVal1 = acc.template to_vector<TT_DATA>(0); // convert acc type to standard type.
                            //        readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1);
                            //        //read, but ignored
                            readVal2 = readincr_v<kSamplesIn128b>(inStream1); // read, but ignored
                            *out128Ptr0++ = readVal1;
                        }
                    outPtr0 = (t256VectorType*)out128Ptr0;
                }
            else {                                             // for cint32 handling
                for (int i = 0; i < TP_HEADER_BYTES / 32; i++) // 32? bytes/ 256bits. 8/256 = 1/32
                    chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 32, ) {
                        acc = readincr_v4(inStream0); // cascade read;
                        read256Val =
                            acc.template to_vector<TT_DATA>(0); // convert acc type (cacc64) to standard type (cint32).
                        // readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1); //read 128 bits,
                        // but ignored
                        // readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1); //read 128 bits,
                        // but ignored
                        readVal2 = readincr_v<kSamplesIn128b>(inStream1); // read 128 bits, but ignored
                        readVal2 = readincr_v<kSamplesIn128b>(inStream1); // read 128 bits,  but ignored
                        *outPtr0++ = read256Val;
                    }
            }
        }

    if
        constexpr(kSamplesIn128b == 4) {                                   // ie 4 cint16 in 128b
            constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    acc = readincr_v4(inStream0);                  // cascade read;
                    readVal1 = acc.template to_vector<TT_DATA>(0); // convert acc type to standard type.
                    //      readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1);
                    readVal2 = readincr_v<kSamplesIn128b>(inStream1);
                    inIntlv = ::aie::interleave_zip(readVal1, readVal2, 1);
                    writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                    *outPtr0++ = writeVal;
                }
        }
    else {
        constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2) / 2; // number of 512 bit ops in Window
        for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                // here, the cascade port reads 4 samples, but the stream reads only 2, hence
                // this asymmetrically unrolled loop.
                acc = readincr_v4(inStream0);                    // cascade read;
                read256Val = acc.template to_vector<TT_DATA>(0); // convert acc type to standard type.
                readVal1a = read256Val.template extract<kSamplesIn128b>(0);
                readVal1b = read256Val.template extract<kSamplesIn128b>(1);

                //      readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1);
                readVal2 = readincr_v<kSamplesIn128b>(inStream1);
                inIntlv = ::aie::interleave_zip(readVal1a, readVal2, 1);
                writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                *outPtr0++ = writeVal;
                //      readVal2 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream1);
                readVal2 = readincr_v<kSamplesIn128b>(inStream1);
                inIntlv = ::aie::interleave_zip(readVal1b, readVal2, 1);
                writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                *outPtr0++ = writeVal;
            }
    }
}
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read stream/Casc, write window/buffer
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void readStreamCascIn(input_stream<TT_DATA>* __restrict inStream0,
                                  input_stream<cacc64>* __restrict inStream1,
                                  TT_DATA* inBuff) {
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using accVect_t = ::aie::detail::accum<fnAccClass<TT_DATA>(), // int, cint, FP or CFP
                                           64,                    // acc width
                                           4>;                    // boths cint16 and cint32 use 4*cacc64
    accVect_t acc;
    t128VectorType readVal1;
    t128VectorType readVal2, readVal2a, readVal2b;
    t256VectorType read256Val;
    t256VectorType writeVal;
    t128VectorType* out128Ptr0 = (t128VectorType*)&inBuff[0];
    t256VectorType* outPtr0 = (t256VectorType*)&inBuff[0];
    ::std::pair<t128VectorType, t128VectorType> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            if
                constexpr(kSamplesIn128b == 4) {                   // ie 4 cint16 in 128b
                    for (int i = 0; i < TP_HEADER_BYTES / 16; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                            //        readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0);
                            //        //read, but ignored
                            readVal1 = readincr_v<kSamplesIn128b>(inStream0); // read, but ignored
                            acc = readincr_v4(inStream1);                     // cascade read;
                            readVal2 = acc.template to_vector<TT_DATA>(0);    // convert acc type to standard type.
                            *out128Ptr0++ = readVal1;
                        }
                    outPtr0 = (t256VectorType*)out128Ptr0;
                }
            else {                                             // cint32 handling
                for (int i = 0; i < TP_HEADER_BYTES / 32; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                    chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 32, ) {
                        // readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0); //read, but
                        // ignored
                        // readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0); //read, but
                        // ignored
                        readVal1 = readincr_v<kSamplesIn128b>(inStream0); // read, but ignored
                        readVal1 = readincr_v<kSamplesIn128b>(inStream0); // read, but ignored
                        acc = readincr_v4(inStream1);                     // cascade read;
                        read256Val = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
                        *outPtr0++ = read256Val;
                    }
            }
        }

    if
        constexpr(kSamplesIn128b == 4) {                                   // ie 4 cint16 in 128b
            constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    //      readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0); //read, but
                    //      ignored
                    readVal1 = readincr_v<kSamplesIn128b>(inStream0); // read, but ignored
                    acc = readincr_v4(inStream1);                     // cascade read;
                    readVal2 = acc.template to_vector<TT_DATA>(0);    // convert acc type to standard type.
                    inIntlv = ::aie::interleave_zip(readVal1, readVal2,
                                                    1); // convert to complex by interleaving zeros for imag parts
                    writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                    *outPtr0++ = writeVal;
                }
        }
    else {
        constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2) / 2; // number of 256 bit ops in Window
        for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                acc = readincr_v4(inStream1);                    // cascade read;
                read256Val = acc.template to_vector<TT_DATA>(0); // convert acc type to standard type.
                readVal2a = read256Val.template extract<kSamplesIn128b>(0);
                readVal2b = read256Val.template extract<kSamplesIn128b>(1);
                //      readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0);
                readVal1 = readincr_v<kSamplesIn128b>(inStream0);
                inIntlv = ::aie::interleave_zip(readVal1, readVal2a, 1);
                writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                *outPtr0++ = writeVal;
                //      readVal1 = readincr_v<kSamplesIn128b,aie_stream_resource_in::a>(inStream0);
                readVal1 = readincr_v<kSamplesIn128b>(inStream0);
                inIntlv = ::aie::interleave_zip(readVal1, readVal2b, 1);
                writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                *outPtr0++ = writeVal;
            }
    }
}
#endif //__SUPPORTS_ACC64__

//-----------------------------------------------------------------------------------------------------
// read window, write streams
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeStreamOut(output_stream<TT_DATA>* __restrict outStream0,
                                output_stream<TT_DATA>* __restrict outStream1,
                                TT_DATA* outBuff) {
    //  printf("Entering writeStreamOut with window size %d\n",
    //  (int)(TP_WINDOW_VSIZE+TP_DYN_PT_SIZE*32/sizeof(TT_DATA)));
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val;
    // T_buff_128b<TT_DATA> read128Val;
    t256VectorType read256Val;
    t128VectorType writeVal;
    // T_buff_128b<TT_DATA> out128a, out128b;
    t128VectorType out128a, out128b;
    t128VectorType* rdptr0 = (t128VectorType*)&outBuff[0];
    t256VectorType* rd256ptr = (t256VectorType*)&outBuff[0];

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    // read128Val.val = *rdptr0++;
                    read128Val = *rdptr0++;
                    // stream_writeincr_128b(outStream0, read128Val, 0);
                    // stream_writeincr_128b(outStream1, read128Val, 1);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val);
                    writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outStream1, read128Val);
                }
            rd256ptr = (t256VectorType*)rdptr0;
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val = *rd256ptr++;
            // out128a.val = ::aie::filter_even<t256VectorType>(read256Val);
            // out128b.val = ::aie::filter_odd<t256VectorType>(read256Val);
            // stream_writeincr_128b(outStream0, out128a, 0);
            // stream_writeincr_128b(outStream1, out128b, 1);
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
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val;
    t128VectorType writeVal;
    t128VectorType out128a;
    t128VectorType* rdptr0 = (t128VectorType*)&outBuff[0];

    //  printf("in writeStreamOut\n");

    for (int i = 0; i < TP_HEADER_BYTES / 16 + TP_WINDOW_VSIZE / kSamplesIn128b; i++)
        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16 + TP_WINDOW_VSIZE / kSamplesIn128b, ) {
            read128Val = *rdptr0++;
            //    ::aie::print(read128Val, true, " read128Val ");
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val);
        }
}

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read window, write casc/stream
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeCascStreamOut(output_stream<cacc64>* __restrict outStream0,
                                    output_stream<TT_DATA>* __restrict outStream1,
                                    TT_DATA* outBuff) {
    //  printf("Entering writeStreamOut with window size %d\n",
    //  (int)(TP_WINDOW_VSIZE+TP_DYN_PT_SIZE*32/sizeof(TT_DATA)));
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val, read128Val0, read128Val1;
    t256VectorType read256Val;
    t128VectorType writeVal;
    t128VectorType out128a, out128b, out128a0, out128a1;
    t256VectorType out256a;
    t128VectorType* rdptr0 = (t128VectorType*)&outBuff[0];
    t256VectorType* rd256ptr = (t256VectorType*)&outBuff[0];
    using accVect_t = ::aie::detail::accum<fnAccClass<TT_DATA>(), // int, cint, FP or CFP
                                           64,                    // acc width
                                           4>; // Ought to be kSamplesIn128b>;, but both cint16 and cint32 use 4*cacc64
    accVect_t acc;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            if
                constexpr(kSamplesIn128b == 4) { // ie 4 cint16 in 128b
                    for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                            read128Val = *rdptr0++;
                            acc = ::aie::from_vector<cacc64>(read128Val);
                            writeincr_v4(outStream0, acc);
                            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, read128Val);
                        }
                }
            else {
                for (int i = 0; i < TP_HEADER_BYTES / 32; i++)
                    chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 32, ) {
                        read128Val0 = *rdptr0++;
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, read128Val0);
                        read128Val1 = *rdptr0++;
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, read128Val1);
                        read256Val = ::aie::concat<t128VectorType, t128VectorType>(read128Val0, read128Val1);
                        acc = ::aie::from_vector<cacc64>(read256Val);
                        writeincr_v4(outStream0, acc);
                    }
            }
            rd256ptr = (t256VectorType*)rdptr0;
        }

    if
        constexpr(kSamplesIn128b == 4) {                                   // ie 4 cint16 in 128b
            constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
            for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    read256Val = *rd256ptr++;
                    out128a = ::aie::filter_even<t256VectorType>(read256Val);
                    out128b = ::aie::filter_odd<t256VectorType>(read256Val);
                    acc = ::aie::from_vector<cacc64>(out128a);
                    writeincr_v4(outStream0, acc);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, out128b);
                }
        }
    else {                                                                 // cint32 handling
        constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2) / 2; // number of 512 bit ops in Window
        for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                read256Val = *rd256ptr++;
                out128a0 = ::aie::filter_even<t256VectorType>(read256Val);
                out128b = ::aie::filter_odd<t256VectorType>(read256Val);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, out128b);
                read256Val = *rd256ptr++;
                out128a1 = ::aie::filter_even<t256VectorType>(read256Val);
                out128b = ::aie::filter_odd<t256VectorType>(read256Val);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream1, out128b);
                out256a = ::aie::concat<t128VectorType, t128VectorType>(out128a0, out128a1);
                acc = ::aie::from_vector<cacc64>(out256a);
                writeincr_v4(outStream0, acc);
            }
    }
}
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
//-----------------------------------------------------------------------------------------------------
// read window, write stream/casc
template <typename TT_DATA, unsigned int TP_DYN_PT_SIZE, unsigned int TP_WINDOW_VSIZE>
INLINE_DECL void writeStreamCascOut(output_stream<TT_DATA>* __restrict outStream0,
                                    output_stream<cacc64>* __restrict outStream1,
                                    TT_DATA* outBuff) {
    constexpr int TP_HEADER_BYTES = 32 * TP_DYN_PT_SIZE;
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val, read128Val0, read128Val1;
    t256VectorType read256Val;
    t128VectorType writeVal;
    t128VectorType out128a, out128b, out128b0, out128b1;
    t256VectorType out256b;
    t128VectorType* rdptr0 = (t128VectorType*)&outBuff[0];
    t256VectorType* rd256ptr = (t256VectorType*)&outBuff[0];
    using accVect_t = ::aie::detail::accum<fnAccClass<TT_DATA>(), // int, cint, FP or CFP
                                           64,                    // acc width
                                           4>;                    // both cint16 and cint32 use 4*cacc64
    accVect_t acc;
    static constexpr unsigned int kAccVect = 4; // cacc64 samples in 384bus

    if
        constexpr(TP_HEADER_BYTES > 0) {
            if
                constexpr(kSamplesIn128b == 4) { // ie 4 cint16 in 128b

                    for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                        chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                            read128Val = *rdptr0++;
                            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val);
                            acc = ::aie::from_vector<cacc64>(read128Val);
                            writeincr_v4(outStream1, acc);
                        }
                }
            else { // cint32 handling
                for (int i = 0; i < TP_HEADER_BYTES / 32; i++)
                    chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 32, ) {
                        read128Val0 = *rdptr0++;
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val0);
                        read128Val1 = *rdptr0++;
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, read128Val1);
                        read256Val = ::aie::concat<t128VectorType, t128VectorType>(read128Val0, read128Val1);
                        acc = ::aie::from_vector<cacc64>(read256Val);
                        writeincr_v4(outStream1, acc);
                    }
            }
            rd256ptr = (t256VectorType*)rdptr0;
        }

    if
        constexpr(kSamplesIn128b == 4) {                                   // ie 4 cint16 in 128b
            constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2); // number of 256 bit ops in Window
            for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    read256Val = *rd256ptr++;
                    out128a = ::aie::filter_even<t256VectorType>(read256Val);
                    out128b = ::aie::filter_odd<t256VectorType>(read256Val);
                    acc = ::aie::from_vector<cacc64>(out128b);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out128a);
                    writeincr_v4(outStream1, acc);
                }
        }
    else {
        // cint32 handling
        constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2) / 2; // number of 512 bit ops in Window
        for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                read256Val = *rd256ptr++;
                out128a = ::aie::filter_even<t256VectorType>(read256Val);
                out128b0 = ::aie::filter_odd<t256VectorType>(read256Val);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out128a);
                read256Val = *rd256ptr++;
                out128a = ::aie::filter_even<t256VectorType>(read256Val);
                out128b1 = ::aie::filter_odd<t256VectorType>(read256Val);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out128a);
                out256b = ::aie::concat<t128VectorType, t128VectorType>(out128b0, out128b1);
                acc = ::aie::from_vector<cacc64>(out256b);
                writeincr_v4(outStream1, acc);
            }
    }
}
#endif //__SUPPORTS_ACC64__
}
}
}
}
} // namespace closures

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
