/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
namespace mixed_radix_fft {

#define __24_1__

// new FFT functions with vectorization
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE, unsigned int TP_R>
void INLINE_DECL stage_radix2_dit(const TT_INPUT_DATA* x,
                                  const TT_TWIDDLE* tw,
                                  const unsigned int& n,
                                  const unsigned int& shift,
                                  TT_OUTPUT_DATA* __restrict y,
                                  const bool& inv) {
    constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
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
    constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;

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
    constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
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
    constexpr unsigned int kTwShift = std::is_same<TT_INPUT_DATA, cfloat>::value ? 0 : 15;
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
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix5_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                &tw_table[tw_ptrs[TW_BASE + 3]], TP_POINT_SIZE,
                FFT_SHIFT15, // compensate for twiddles only
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
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles only
                 obuff,                  // the destination buffer
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
                 FFT_SHIFT15,            // compensate for twiddles only
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
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles only
                 obuff,                  // the destination buffer
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
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix3_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], TP_POINT_SIZE,
                FFT_SHIFT15, // compensate for twiddles only
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
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles only
                 obuff,                  // the destination buffer
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
                 FFT_SHIFT15,            // compensate for twiddles only
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
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles only
                 obuff,                  // the destination buffer
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
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix2_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], TP_POINT_SIZE,
                FFT_SHIFT15, // compensate for twiddle only. Only apply TP_SHIFT in final stage
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
                 TP_POINT_SIZE, FFT_SHIFT15 + TP_SHIFT,
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
                 FFT_SHIFT15,            // compensate for twiddle only. Only apply TP_SHIFT in final stage
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
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddle only. Only apply TP_SHIFT in final stage
                 obuff,                  // the destination buffer
                 inv);
        }
    chess_memory_fence();
};

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
    // first, but not last of this kernel
    if
        constexpr(stage == TP_START_RANK && stage != TP_END_RANK - 1) {
            stage_radix4_dit<TT_IN_DATA, TT_INTERNAL_DATA, TT_TWIDDLE, TP_POINT_SIZE / RMOD>(
                xbuff, &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]], &tw_table[tw_ptrs[TW_BASE + 2]],
                TP_POINT_SIZE,
                FFT_SHIFT15, // compensate for twiddles only
                tmp_bufs[1 - pingPong], inv);
            pingPong = 1 - pingPong;
        }
    // first and last of this kernel
    else if
        constexpr(stage == TP_START_RANK && stage == TP_END_RANK - 1) {
            stage_radix4_dit<TT_IN_DATA,  // Not first stage, so using internal data type
                             TT_OUT_DATA, // last stage so output is data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (xbuff,                            // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles AND perform final stage shift
                 obuff,                  // the destination buffer
                 inv);
        }
    // not first, nor last of this kernel
    else if
        constexpr(stage > TP_START_RANK && stage < TP_END_RANK - 1) {
            stage_radix4_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_INTERNAL_DATA, // nor last, so use internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 FFT_SHIFT15,            // compensate for twiddles only
                 tmp_bufs[1 - pingPong], // the destination buffer
                 inv);
            pingPong = 1 - pingPong;
        }
    // not first, but last of this kernel
    else if
        constexpr(stage == TP_END_RANK - 1) {
            stage_radix4_dit<TT_INTERNAL_DATA, // Not first stage, so using internal data type
                             TT_OUT_DATA,      // nor last, so use internal data type
                             TT_TWIDDLE,
                             TP_POINT_SIZE / RMOD> // the 'r' factor.
                (tmp_bufs[pingPong],               // input
                 &tw_table[tw_ptrs[TW_BASE]], &tw_table[tw_ptrs[TW_BASE + 1]],
                 &tw_table[tw_ptrs[TW_BASE + 2]], // the twiddles
                 TP_POINT_SIZE,
                 FFT_SHIFT15 + TP_SHIFT, // compensate for twiddles only
                 obuff,                  // the destination buffer
                 inv);
        }

    chess_memory_fence();
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_MIXED_RADIX_FFT_UTILS_HPP_
