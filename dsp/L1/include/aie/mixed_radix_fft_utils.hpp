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
#ifndef _DSPLIB_MIXED_RADIX_FFT_UTILS_HPP_
#define _DSPLIB_MIXED_RADIX_FFT_UTILS_HPP_

#include "device_defs.h"

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
#include "aie_api/fft.hpp"

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

#define __24_1__ // TODO Now 24_2 ?

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

// new FFT functions with vectorization
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix2_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    //    constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, 0 /*TP_TWIDDLE_MODE*/>();
#ifdef __24_1__
    // New rank-level entry to AIE API FFT.
    ::aie::fft_dit_r2_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, tw, n, kTwShift, shift, inv, y);
#else  // 23_2
    constexpr unsigned int kStageRadix = 2;

    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>; // type = cint32, stage = 0, radix = 2

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
#endif //__24_1__
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
    const unsigned kStageRadix = 4;
    // constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, 0 /*TP_TWIDDLE_MODE*/>();

#ifdef __24_1__
    ::aie::fft_dit_r4_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, tw1, tw2, tw3, n, kTwShift, shift, inv,
                                                                             y);
#else
    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>;

    FFT fft;

    int block_size = FFT::block_size(n);

    auto it_stage = fft.begin_stage(x, tw1, tw2, tw3);

    if
        constexpr(TP_R == 1) {
            auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
            auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / kStageRadix);
            auto it_out2 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / kStageRadix);
            auto it_out3 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 3 * n / kStageRadix);

            for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
                    const auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
                    *it_out0++ = out[0];
                    *it_out1++ = out[1];
                    *it_out2++ = out[2];
                    *it_out3++ = out[3];
                }
        }
    else { // Currently worse performance if using the 4 pointer version on TP_R (vectorization) > 1
        auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
        auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / kStageRadix);

        for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
                const auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
                *it_out0 = out[0];
                it_out0 += block_size;
                *it_out0 = out[1];
                it_out0 += -block_size + 1;
                *it_out1 = out[2];
                it_out1 += block_size;
                *it_out1 = out[3];
                it_out1 += -block_size + 1;
            }
    }

/*
const unsigned int kStockhamStage = 0;
const unsigned int kStageRadix = 4;
constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
using FFT = ::aie::fft_dit< TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>; // type = cint32, stage = 0, radix =
2

FFT fft;

auto it_stage  = fft.begin_stage(x, tw1, tw2, tw3);
auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2*n / kStageRadix);
const int block_size = n / (kStageRadix * FFT::out_vector_size);

for (int j = 0; j < block_size; ++j)
  chess_prepare_for_pipelining
  chess_loop_range(1,)
  {
    auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
    //in AIE1 or at least at one point the compiler failed to optimize the pointer
    //handling correctly, so two pointers interlaced were required to allow the
    //pointer arithmetic not to become an issue.
    // In AIE-ML, or after and update to the compiler, this was no longer needed
#if __FFT_R4_IMPL__ == 0
    *it_out0 = out[0]; it_out0 +=  block_size;
    *it_out0 = out[1]; it_out0 += -block_size + 1;
    *it_out1 = out[2]; it_out1 +=  block_size;
    *it_out1 = out[3]; it_out1 += -block_size + 1;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    *it_out0 = out[0]; it_out0 +=  block_size;
    *it_out0 = out[1]; it_out0 +=  block_size;
    *it_out0 = out[2]; it_out0 +=  block_size;
    *it_out0 = out[3]; it_out0 += -3*block_size + 1;
#endif //__FFT_R4_IMPL__ == 1
  }
*/
#endif //__24_1__
};

template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix3_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* twb,
                                  const TT_TWIDDLE* twc,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    // constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, 0 /*TP_TWIDDLE_MODE*/>();
    const unsigned kStageRadix = 3;
#ifdef __24_1__
    ::aie::fft_dit_r3_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, twb, twc, n, kTwShift, shift, inv, y);
#else

    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>;

    FFT fft;

// Output locations are separated by n / Radix elements
#define one_third_Q15 10923
    unsigned n_div_3 = (n * one_third_Q15) >> 15;
    int block_size = FFT::block_size(n);

    auto it_stage = fft.begin_stage(x, twb, twc);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n_div_3);
    auto it_out2 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n_div_3);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
            *it_out0++ = out[0];
            *it_out1++ = out[1];
            *it_out2++ = out[2];
        }
#endif //__24_1__
};

template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix5_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw0,
                                  const TT_TWIDDLE* tw1,
                                  const TT_TWIDDLE* tw2,
                                  const TT_TWIDDLE* tw3,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    // constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, 0 /*TP_TWIDDLE_MODE*/>();
    const unsigned kStageRadix = 5;
#ifdef __24_1__
    ::aie::fft_dit_r5_stage<TP_R, TT_INPUT_DATA, TT_OUTPUT_DATA, TT_TWIDDLE>(x, tw0, tw1, tw2, tw3, n, kTwShift, shift,
                                                                             inv, y);
#else

    using FFT = ::aie::fft_dit<TP_R, kStageRadix, TT_INPUT_DATA, TT_OUTPUT_DATA>;

    FFT fft;

    // Output locations are separated by n / Radix elements
    int block_size = FFT::block_size(n);

    auto it_stage = fft.begin_stage(x, tw0, tw1, tw2, tw3);
    auto it_out = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, kTwShift, shift, inv);
            *it_out = out[0];
            it_out += block_size;
            *it_out = out[1];
            it_out += block_size;
            *it_out = out[2];
            it_out += block_size;
            *it_out = out[3];
            it_out += block_size;
            *it_out = out[4];
            it_out += -4 * block_size + 1;
        }
#endif //__24_1__
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

//-------------------------------------
// Static radix5 int stage handling
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_TWIDDLE,
          int TP_START_RANK,
          int TP_END_RANK,
          int stage,
          int TP_POINT_SIZE,
          int RMOD,
          int TP_SHIFT,
          int TW_BASE>
void INLINE_DECL opt_r5_stage(TT_IN_DATA* xbuff,
                              TT_OUT_DATA* obuff,
                              TT_INTERNAL_DATA** tmp_bufs,
                              unsigned int& pingPong,
                              bool& inv,
                              TT_TWIDDLE* tw_table,
                              int* tw_ptrs) {
    constexpr int kTwShift = is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;

    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix5_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                &tw_table[tw_ptrs[TW_BASE + 3]], TP_POINT_SIZE,
                kTwShift, // compensate for twiddles only
                tmp_bufs[1 - pingPong], inv);
            pingPong = 1 - pingPong;
        }
    // first and last of this kernel
    else if
        constexpr(stage == TP_START_RANK && stage == TP_END_RANK - 1) {
            stage_radix5_dit<TT_IN_DATA,  // Not first stage, so using internal data type
                             TT_OUT_DATA, // r5 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (xbuff,                            // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                 &tw_table[tw_ptrs[TW_BASE + 3]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles only
                 obuff,               // the destination buffer
                 inv);
            // pingPong = 1-pingPong; //unnecessary
        }
    // not first, nor last of this kernel
    else if
        constexpr(stage > TP_START_RANK && stage < TP_END_RANK - 1) {
            stage_radix5_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_INTERNAL_DATA, // r5 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                 &tw_table[tw_ptrs[TW_BASE + 3]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift,               // compensate for twiddles only
                 tmp_bufs[1 - pingPong], // the destination buffer
                 inv);
            pingPong = 1 - pingPong;
        }
    // not first, but last of this kernel
    else if
        constexpr(stage == TP_END_RANK - 1) {
            stage_radix5_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_OUT_DATA,      // r5 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                 &tw_table[tw_ptrs[TW_BASE + 3]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles only
                 obuff,               // the destination buffer
                 inv);
            // pingPong = 1-pingPong; //unnecessary
        }
    chess_memory_fence();
};

// Static radix3 int stage handling
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_TWIDDLE,
          int TP_START_RANK,
          int TP_END_RANK,
          int stage,
          int TP_POINT_SIZE,
          int RMOD,
          int TP_SHIFT,
          int TW_BASE>
void INLINE_DECL opt_r3_stage(TT_IN_DATA* xbuff,
                              TT_OUT_DATA* obuff,
                              TT_INTERNAL_DATA** tmp_bufs,
                              unsigned int& pingPong,
                              bool& inv,
                              TT_TWIDDLE* tw_table,
                              int* tw_ptrs) {
    constexpr int kTwShift = is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;
    // printf("\n IN R3 : TP_POINT_SIZE / RMOD = %d", TP_POINT_SIZE / RMOD);
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix3_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], TP_POINT_SIZE,
                kTwShift, // compensate for twiddles only
                tmp_bufs[1 - pingPong], inv);
            pingPong = 1 - pingPong;
        }
    // first and last of this kernel
    else if
        constexpr(stage == TP_START_RANK && stage == TP_END_RANK - 1) {
            stage_radix3_dit<TT_IN_DATA,  // Not first stage, so using internal data type
                             TT_OUT_DATA, // r3 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD>                             // the 'r' factor.
                (xbuff,                                                        // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles only
                 obuff,               // the destination buffer
                 inv);
        }
    // not first, nor last of this kernel
    else if
        constexpr(stage > TP_START_RANK && stage < TP_END_RANK - 1) {
            stage_radix3_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_INTERNAL_DATA, // r3 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD>                             // the 'r' factor.
                (tmp_bufs[pingPong],                                           // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift,               // compensate for twiddles only
                 tmp_bufs[1 - pingPong], // the destination buffer
                 inv);
            pingPong = 1 - pingPong;
        }
    // not first, but last of this kernel
    else if
        constexpr(stage == TP_END_RANK - 1) {
            stage_radix3_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_OUT_DATA,      // r3 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD>                             // the 'r' factor.
                (tmp_bufs[pingPong],                                           // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles only
                 obuff,               // the destination buffer
                 inv);
        }
    chess_memory_fence();
};

// Static radix2 int stage handling
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_TWIDDLE,
          int TP_START_RANK,
          int TP_END_RANK,
          int stage,
          int TP_POINT_SIZE,
          int RMOD,
          int TP_SHIFT,
          int TW_BASE>
void INLINE_DECL opt_r2_stage(TT_IN_DATA* xbuff,
                              TT_OUT_DATA* obuff,
                              TT_INTERNAL_DATA** tmp_bufs,
                              unsigned int& pingPong,
                              bool& inv,
                              TT_TWIDDLE* tw_table,
                              int* tw_ptrs) {
    constexpr int kTwShift = is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;

    // printf("\n IN R2 : TP_POINT_SIZE / RMOD = %d", TP_POINT_SIZE / RMOD);
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix2_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], TP_POINT_SIZE,
                kTwShift, // compensate for twiddle only. Only apply TP_SHIFT in final stage
                tmp_bufs[1 - pingPong], inv);
            pingPong = 1 - pingPong;
        }
    // first and last of this kernel
    else if
        constexpr(stage == TP_START_RANK && stage == TP_END_RANK - 1) {
            stage_radix2_dit<TT_IN_DATA,  // Not first stage, so using internal data type
                             TT_OUT_DATA, // last stage so output is data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (xbuff,                            // input
                 &tw_table[tw_ptrs[TW_BASE]],      // the twiddles
                 TP_POINT_SIZE, kTwShift + TP_SHIFT,
                 obuff, // the destination buffer
                 inv);
        }
    // not first, nor last of this kernel
    else if
        constexpr(stage > TP_START_RANK && stage < TP_END_RANK - 1) {
            stage_radix2_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_INTERNAL_DATA, // r2 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]],      // the twiddles
                 TP_POINT_SIZE,
                 kTwShift,               // compensate for twiddle only. Only apply TP_SHIFT in final stage
                 tmp_bufs[1 - pingPong], // the destination buffer
                 inv);
            pingPong = 1 - pingPong;
        }
    // not first, but last of this kernel
    else if
        constexpr(stage == TP_END_RANK - 1) {
            stage_radix2_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_OUT_DATA,      // r2 is never last stage so output is internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]],      // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddle only. Only apply TP_SHIFT in final stage
                 obuff,               // the destination buffer
                 inv);
        }
    chess_memory_fence();
};

template <typename TT_IN_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE>
void INLINE_DECL my_r5_fft_stage(const TT_IN_DATA* __restrict x,
                                 const TT_TWIDDLE* __restrict tw0,
                                 const TT_TWIDDLE* __restrict tw1,
                                 const TT_TWIDDLE* __restrict tw2,
                                 const TT_TWIDDLE* __restrict tw3,
                                 const unsigned& n,
                                 const unsigned& vectorization,
                                 const unsigned& shift_tw,
                                 const unsigned& shift,
                                 bool& inv,
                                 TT_INTERNAL_DATA* out) {
    constexpr unsigned vec_dummy = 4;
    constexpr unsigned stage = 0;
    using FFT = ::aie::detail::fft_dit<vec_dummy, stage, 5, TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>;
    using iterator =
        ::aie::detail::restrict_vector_iterator<TT_INTERNAL_DATA, FFT::out_vector_size, 1, aie_dm_resource::none>;

    FFT fft(shift_tw, shift, inv);

    // Output locations are separated by n / Radix elements, divide by the vector size used by the iterator
    int block_size = FFT::block_size(n);

    auto it_stage = fft.begin_stage(x, tw0, tw1, tw2, tw3, vectorization);
    auto it_out = iterator(out);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(2, ) {
            const auto out = fft.dit(*it_stage++);
            *it_out = out[0];
            it_out += block_size;
            *it_out = out[1];
            it_out += block_size;
            *it_out = out[2];
            it_out += block_size;
            *it_out = out[3];
            it_out += block_size;
            *it_out = out[4];
            it_out += -4 * block_size + 1;
        }
}

template <typename TT_IN_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE>
void INLINE_DECL my_r3_fft_stage(const TT_IN_DATA* __restrict x,
                                 const TT_TWIDDLE* __restrict tw0,
                                 const TT_TWIDDLE* __restrict tw1,
                                 const unsigned& n,
                                 const unsigned& vectorization,
                                 const unsigned& shift_tw,
                                 const unsigned& shift,
                                 bool& inv,
                                 TT_INTERNAL_DATA* out) {
    static constexpr unsigned radix = 3;
    static constexpr unsigned one_third_Q15 = 10923;
    if
        constexpr(std::is_same_v<TT_TWIDDLE, cint32>)
            REQUIRES_MSG(vectorization >= 2, "Only vectorizations >= 2 are supported");
    else
        REQUIRES_MSG(vectorization >= 4, "Only vectorizations >= 4 are supported");

    constexpr unsigned vec_dummy = 4;
    constexpr unsigned stage = 0;
    using FFT = ::aie::detail::fft_dit<vec_dummy, stage, radix, TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>;
    using iterator =
        ::aie::detail::restrict_vector_iterator<TT_INTERNAL_DATA, FFT::out_vector_size, 1, aie_dm_resource::none>;

    FFT fft(shift_tw, shift, inv);

    // Output locations are separated by n / Radix elements
    unsigned n_div_3 = (n * one_third_Q15) >> 15;
    int block_size = FFT::block_size(n);

    auto it_stage = fft.begin_stage(x, tw0, tw1, vectorization);
    auto it_out0 = iterator(out);
    auto it_out1 = iterator(out + n_div_3);
    auto it_out2 = iterator(out + 2 * n_div_3);

    for (int j = 0; j < block_size; ++j) chess_prepare_for_pipelining chess_loop_range(2, ) {
            const auto out = fft.dit(*it_stage++);
            *it_out0++ = out[0];
            *it_out1++ = out[1];
            *it_out2++ = out[2];
        }
}

// Static radix4 int stage handling
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_INTERNAL_DATA,
          typename TT_TWIDDLE,
          int TP_START_RANK,
          int TP_END_RANK,
          int stage,
          int TP_POINT_SIZE,
          int RMOD,
          int TP_SHIFT,
          int TW_BASE>
void INLINE_DECL opt_r4_stage(TT_IN_DATA* xbuff,
                              TT_OUT_DATA* obuff,
                              TT_INTERNAL_DATA** tmp_bufs,
                              unsigned int& pingPong,
                              bool& inv,
                              TT_TWIDDLE* tw_table,
                              int* tw_ptrs) {
    constexpr int kTwShift = is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;

    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            // printf("\n stage == TP_START_RANK && stage != TP_END_RANK - 1 \n");
            stage_radix4_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                TP_POINT_SIZE,
                kTwShift, // compensate for twiddles only
                tmp_bufs[1 - pingPong], inv);
            pingPong = 1 - pingPong;
        }
    // first and last of this kernel
    else if
        constexpr(stage == TP_START_RANK && stage == TP_END_RANK - 1) {
            // printf("\n stage == TP_START_RANK && stage == TP_END_RANK - 1 \n");
            stage_radix4_dit<TT_IN_DATA,  // Not first stage, so using internal data type
                             TT_OUT_DATA, // last stage so output is data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (xbuff,                            // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles AND perform final stage shift
                 obuff,               // the destination buffer
                 inv);
        }
    // not first, nor last of this kernel
    else if
        constexpr(stage > TP_START_RANK && stage < TP_END_RANK - 1) {
            // printf("\n stage > TP_START_RANK && stage < TP_END_RANK - 1 \n");
            stage_radix4_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_INTERNAL_DATA, // nor last, so use internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift,               // compensate for twiddles only
                 tmp_bufs[1 - pingPong], // the destination buffer
                 inv);
            pingPong = 1 - pingPong;
        }
    // not first, but last of this kernel
    else if
        constexpr(stage == TP_END_RANK - 1) {
            // printf("\n stage == TP_END_RANK - 1 \n");
            stage_radix4_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_OUT_DATA,      // nor last, so use internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 kTwShift + TP_SHIFT, // compensate for twiddles only
                 obuff,               // the destination buffer
                 inv);
        }
    // else {printf("\n No condition satisfied.\n");}
    chess_memory_fence();
};

//-------------------------------------
// Dynamic radix5 int stage handling
template <typename TT_IN_DATA, typename TT_OUT_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE, int TP_SHIFT>
void NOINLINE_DECL opt_r5_dyn_stage(TT_IN_DATA* xbuff,
                                    TT_OUT_DATA* obuff,
                                    TT_INTERNAL_DATA** tmp_bufs,
                                    unsigned int& pingPong,
                                    const unsigned int& n, // the runtime pointsize
                                    const unsigned int& r, // 'r'-factor
                                    bool& inv,
                                    int& end_rank, // start_rank is always 0 since no cascading
                                    int& stage,
                                    TT_TWIDDLE* tw_table,
                                    int32* tw_ptrs,
                                    int& tw_base // points to place in indices (and other factors)
                                    ) {
    constexpr unsigned int kTwShift =
        is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;
    const unsigned int start_rank = 0;
    // first, but not last of this kernel
    if (stage == start_rank && stage != end_rank - 1) {
        my_r5_fft_stage<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            xbuff, &tw_table[tw_ptrs[tw_base + 3]], &tw_table[tw_ptrs[tw_base + 2]],
            &tw_table[tw_ptrs[tw_base + 1]],                       // reverse order!
            &tw_table[tw_ptrs[tw_base]], n, r, kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]);
        // call to AIE API function has been replaced to have more control into the loop configuration
        pingPong = 1 - pingPong;
    }
    // not first, nor last of this kernel
    else
    // if (stage > start_rank && stage < end_rank - 1)
    {
        // call to AIE API function has been replaced to have more control into the loop configuration
        my_r5_fft_stage<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base + 3]], &tw_table[tw_ptrs[tw_base + 2]],
            &tw_table[tw_ptrs[tw_base + 1]],                       // reverse order!
            &tw_table[tw_ptrs[tw_base]], n, r, kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]                            // the destination buffer
            );
        pingPong = 1 - pingPong;
    }
    chess_memory_fence();
};

//-------------------------------------
// Dynamic radix3 int stage handling
template <typename TT_IN_DATA, typename TT_OUT_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE, int TP_SHIFT>
void NOINLINE_DECL opt_r3_dyn_stage(TT_IN_DATA* xbuff,
                                    TT_OUT_DATA* obuff,
                                    TT_INTERNAL_DATA** tmp_bufs,
                                    unsigned int& pingPong,
                                    const unsigned int& n, // the runtime pointsize
                                    const unsigned int& r, // 'r'-factor
                                    bool& inv,
                                    int& end_rank, // start_rank is always 0 since no cascading
                                    int& stage,
                                    TT_TWIDDLE* tw_table,
                                    int32* tw_ptrs,
                                    int& tw_base // points to place in indices (and other factors)
                                    ) {
    constexpr unsigned int kTwShift =
        is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;
    const unsigned int start_rank = 0;

    // first, but not last of this kernel
    if (stage == start_rank && stage != end_rank - 1) {
        // ::aie::fft_dit_r3_stage<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
        //     xbuff, &tw_table[tw_ptrs[tw_base]], &tw_table[tw_ptrs[tw_base + 1]], n, r,
        //     kTwShift, kTwShift, // compensate for twiddles only
        //      inv, tmp_bufs[1 - pingPong]);

        my_r3_fft_stage<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(xbuff, &tw_table[tw_ptrs[tw_base + 1]],
                                                                  &tw_table[tw_ptrs[tw_base]], n, r, kTwShift,
                                                                  kTwShift, // compensate for twiddles only
                                                                  inv, tmp_bufs[1 - pingPong]);
        pingPong = 1 - pingPong;
    }
    // not first, nor last of this kernel
    else //(stage > start_rank && stage < end_rank - 1)
    {
        // ::aie::fft_dit_r3_stage<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
        //     tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base]], &tw_table[tw_ptrs[tw_base + 1]], n, r,
        //     kTwShift, kTwShift, // compensate for twiddles only
        //     inv, tmp_bufs[1 - pingPong] // the destination buffer
        //     );
        my_r3_fft_stage<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base + 1]], &tw_table[tw_ptrs[tw_base]], n, r, kTwShift,
            kTwShift,                   // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong] // the destination buffer
            );

        pingPong = 1 - pingPong;
    }
    // for pointsize 24

    chess_memory_fence();
};

//-------------------------------------
// Dynamic radix2 int stage handling
template <typename TT_IN_DATA, typename TT_OUT_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE, int TP_SHIFT>
void NOINLINE_DECL opt_r2_dyn_stage(TT_IN_DATA* xbuff,
                                    TT_OUT_DATA* obuff,
                                    TT_INTERNAL_DATA** tmp_bufs,
                                    unsigned int& pingPong,
                                    const unsigned int& n, // the runtime pointsize
                                    const unsigned int& r, // 'r'-factor
                                    bool& inv,
                                    int& end_rank, // start_rank is always 0 since no cascading
                                    int& stage,
                                    TT_TWIDDLE* tw_table,
                                    int32* tw_ptrs,
                                    int& tw_base // points to place in indices (and other factors)
                                    ) {
    constexpr unsigned int kTwShift =
        is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;
    const unsigned int start_rank = 0;

    // first, but not last of this kernel
    if (stage == start_rank && stage != end_rank - 1) {
        ::aie::fft_dit_r2_stage<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            xbuff, &tw_table[tw_ptrs[tw_base]], n, r, kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]);
        pingPong = 1 - pingPong;
    }
    // not first, nor last of this kernel
    else if (stage > start_rank && stage < end_rank - 1) {
        ::aie::fft_dit_r2_stage<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base]], n, r, kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]                                                // the destination buffer
            );
        pingPong = 1 - pingPong;
    }
    // not first, but last of this kernel
    else if (stage == end_rank - 1) {
        ::aie::fft_dit_r2_stage<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base]], n, r, kTwShift,
            kTwShift + TP_SHIFT, // compensate for twiddles only
            inv, obuff           // the destination buffer
            );
        // pingPong = 1-pingPong; //unnecessary
    }
    chess_memory_fence();
};

//-------------------------------------
// Dynamic radix4 int stage handling
template <typename TT_IN_DATA, typename TT_OUT_DATA, typename TT_INTERNAL_DATA, typename TT_TWIDDLE, int TP_SHIFT>
void NOINLINE_DECL opt_r4_dyn_stage(TT_IN_DATA* xbuff,
                                    TT_OUT_DATA* obuff,
                                    TT_INTERNAL_DATA** tmp_bufs,
                                    unsigned int& pingPong,
                                    const unsigned int& n, // the runtime pointsize
                                    const unsigned int& r, // 'r'-factor
                                    bool& inv,
                                    int& end_rank, // start_rank is always 0 since no cascading
                                    int& stage,
                                    TT_TWIDDLE* tw_table,
                                    int32* tw_ptrs,
                                    int& tw_base // points to place in indices (and other factors)
                                    ) {
    constexpr unsigned int kTwShift =
        is_same<TT_TWIDDLE, cfloat>::value ? 0 : is_same<TT_TWIDDLE, cint16>::value ? 15 : 31;
    const unsigned int start_rank = 0;
    // printf("\n INSIDE opt_r4_stage : pingPong=%d", pingPong);
    // printf("\n start_rank=%d \n end_rank=%d \n stage=%d \n", start_rank, end_rank, stage);
    // printf("\n About to enter a stage_radix4_dit with parameters. Finding which parameters based on 4 possible
    // conditions... \n");

    // first, but not last of this kernel
    // int addrIn = (stage == start_rank && stage != end_rank - 1) ? xbuff : tmp_bufs[pingPong];

    if (stage == start_rank && stage != end_rank - 1) {
        ::aie::fft_dit_r4_stage<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            xbuff, &tw_table[tw_ptrs[tw_base]], &tw_table[tw_ptrs[tw_base + 1]], &tw_table[tw_ptrs[tw_base + 2]], n, r,
            kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]);
        pingPong = 1 - pingPong;
    }
    // not first, nor last of this kernel
    else if (stage > start_rank && stage < end_rank - 1) {
        ::aie::fft_dit_r4_stage<TT_INTERNAL_DATA, TT_INTERNAL_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base]], &tw_table[tw_ptrs[tw_base + 1]],
            &tw_table[tw_ptrs[tw_base + 2]], n, r, kTwShift, kTwShift, // compensate for twiddles only
            inv, tmp_bufs[1 - pingPong]                                // the destination buffer
            );
        pingPong = 1 - pingPong;
    }
    // not first, but last of this kernel
    else if (stage == end_rank - 1) {
        ::aie::fft_dit_r4_stage<TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE>(
            tmp_bufs[pingPong], &tw_table[tw_ptrs[tw_base]], &tw_table[tw_ptrs[tw_base + 1]],
            &tw_table[tw_ptrs[tw_base + 2]], n, r, kTwShift, kTwShift + TP_SHIFT, // compensate for twiddles only
            inv, obuff                                                            // the destination buffer
            );
        // pingPong = 1-pingPong; //unnecessary
    }
    chess_memory_fence();
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_MIXED_RADIX_FFT_UTILS_HPP_
