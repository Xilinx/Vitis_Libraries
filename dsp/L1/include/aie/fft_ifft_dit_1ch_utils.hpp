#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_

/*
FFT (1 channel DIT) Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include <type_traits>
#include <typeinfo>

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

//#include "fft_ifft_dit_1ch.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
// Specialised type for final accumulator. Concat of two polyphases
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
T_D unitVector(){};
template <>
cint16 unitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cint32 unitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cfloat unitVector<cfloat>() {
    cfloat temp;
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

// Radix 2, Stage 0, mixed I/O. cint16 twiddles

//------------------------------------
// Radix 2 and 4 stage functions
// Stage 0 refers to the fact that the samples in a vector remain together from input to output.
// In later stages some sample interleaving may occur as the granularity of the trellis becomes
// smaller than the vector size.
// up refers to the change from cint16 input to the higher precision cint32.
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE>
void INLINE_DECL stage0_radix2_dit(const TT_INPUT_DATA* x,
                                   const TT_TWIDDLE* tw,
                                   unsigned int n,
                                   unsigned int r,
                                   unsigned int shift,
                                   TT_OUTPUT_DATA* __restrict y,
                                   bool inv){};

// In stage1 (Stockham addressing), address and pointer handling requires a degree of interleave
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE>
void INLINE_DECL stage1_radix2_dit(const TT_INPUT_DATA* x,
                                   const TT_TWIDDLE* tw,
                                   unsigned int n,
                                   unsigned int r,
                                   unsigned int shift,
                                   TT_OUTPUT_DATA* __restrict y,
                                   bool inv){};

// Final stage of radix2.
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE>
void INLINE_DECL stage2_radix2_dit(const TT_INPUT_DATA* x,
                                   const TT_TWIDDLE* tw,
                                   unsigned int n,
                                   unsigned int r,
                                   unsigned int shift,
                                   TT_OUTPUT_DATA* __restrict y,
                                   bool inv){};

// Stage 0 radix 4. This is used in most internal stages.
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE>
void INLINE_DECL stage0_radix4_dit(const TT_INPUT_DATA* x,
                                   const TT_TWIDDLE* tw1,
                                   const TT_TWIDDLE* tw2,
                                   unsigned int n,
                                   unsigned int r,
                                   unsigned int shift,
                                   TT_OUTPUT_DATA* __restrict y,
                                   bool inv){};

// State 1 radix 4 dit. The last radix 4 stage.
template <typename TT_INPUT_DATA, typename TT_OUTPUT_DATA, typename TT_TWIDDLE>
void INLINE_DECL stage1_radix4_dit(const TT_INPUT_DATA* x,
                                   const TT_TWIDDLE* tw1,
                                   const TT_TWIDDLE* tw2,
                                   unsigned int n,
                                   unsigned int shift,
                                   TT_OUTPUT_DATA* __restrict outputcb,
                                   bool inv){};

template <>
void INLINE_DECL stage0_radix2_dit<cint16, cint32, cint16>(const cint16_t* x,
                                                           const cint16_t* tw,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift,
                                                           cint32_t* __restrict y,
                                                           bool inv) {
    constexpr unsigned int kStockhamStage = 0;
    constexpr unsigned int kStageRadix = 2;
    constexpr unsigned int kImplFactor =
        2; // For this specialization only there are 4 vectors out rather than 2 so vector size is effectively doubled

    unsigned shift_tw = 15;
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint16, cint32>; // type = cint32, stage = 0, radix = 2

    FFT fft(r);

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * kImplFactor * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0++ = out[0];
            *it_out0++ = out[1];
            *it_out1++ = out[2];
            *it_out1++ = out[3];
        }
};

template <>
void INLINE_DECL stage0_radix2_dit<cint32, cint32, cint16>(const cint32_t* x,
                                                           const cint16_t* tw,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift,
                                                           cint32_t* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 2;
    unsigned int shift_tw = 15;
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint32>; // type = cint32, stage = 0, radix = 2

    FFT fft(r);

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out_lo++ = out[0];
            *it_out_hi++ = out[1];
        }
};

template <>
void INLINE_DECL stage0_radix2_dit<cfloat, cfloat, cfloat>(const cfloat* x,
                                                           const cfloat* tw,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift,
                                                           cfloat* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 2;
    unsigned shift_tw = 0;                                           // not actually used by calc
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cfloat>; // type = cfloat, stage = 0, radix = 2

    FFT fft(r);

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j) // sizeof cfloat = 4
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out_lo++ = out[0];
            *it_out_hi++ = out[1];
        }
};

template <>
void INLINE_DECL stage1_radix2_dit<cfloat, cfloat, cfloat>(const cfloat* x,
                                                           const cfloat* tw,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift, // ignored for cfloat.
                                                           cfloat* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 1;
    const unsigned int kStageRadix = 2;

    unsigned shift_tw = 0;                                           // not actually used by calc
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cfloat>; // type = cfloat, stage = 1, radix = 2

    FFT fft; // no 'r' required. This is implied by the fact that this is radix 2 and the penultimate stage.

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out_lo++ = out[0];
            *it_out_hi++ = out[1];
        }
}

template <>
void INLINE_DECL stage2_radix2_dit<cfloat, cfloat, cfloat>(const cfloat* x,
                                                           const cfloat* tw,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift, // ignored for cfloat.
                                                           cfloat* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 2;
    const unsigned int kStageRadix = 2;
    unsigned shift_tw = 0;                                           // Not actually usedby calc
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cfloat>; // type = cfloat, stage = 2, radix = 2

    FFT fft; // no 'r' required. This is implied by the fact that this is radix 2 and the last stage.

    auto it_stage = fft.begin_stage(x, tw);
    auto it_out_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out_lo++ = out[0];
            *it_out_hi++ = out[1];
        }
}

template <>
void INLINE_DECL stage0_radix4_dit<cint16, cint32, cint16>(const cint16_t* x,
                                                           const cint16_t* tw1,
                                                           const cint16_t* tw2,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift,
                                                           cint32_t* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 4;
    unsigned shift_tw = 15;
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint16, cint32>; // type = cint32, stage = 0, radix = 2

    FFT fft(r);

    auto it_stage = fft.begin_stage(x, tw1, tw2);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0 = out[0];
            it_out0 += kIndexStep;
            *it_out0 = out[2];
            it_out0 += kIndexStep;
            *it_out0 = out[1];
            it_out0 += kIndexStep;
            *it_out0 = out[3];
            it_out0 += -(3 * kIndexStep) + 1;
        }
};

template <>
void INLINE_DECL stage0_radix4_dit<cint32, cint32, cint16>(const cint32_t* x,
                                                           const cint16_t* tw1,
                                                           const cint16_t* tw2,
                                                           unsigned int n,
                                                           unsigned int r,
                                                           unsigned int shift,
                                                           cint32_t* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 0;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 4;
    unsigned int shift_tw = 15;

    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint32>;

    FFT fft(r);

    auto it_stage = fft.begin_stage(x, tw1, tw2);
    auto it_out0 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out1 = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 2);
    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0 = out[0];
            it_out0 += kIndexStep;
            *it_out0 = out[2];
            it_out0 += (-kIndexStep) + 1;
            *it_out1 = out[1];
            it_out1 += kIndexStep;
            *it_out1 = out[3];
            it_out1 += (-kIndexStep) + 1;
        }
}

template <>
void INLINE_DECL stage1_radix4_dit<cint32, cint32, cint16>(const cint32_t* __restrict x,
                                                           const cint16_t* __restrict tw1,
                                                           const cint16_t* __restrict tw2,
                                                           unsigned int n,
                                                           unsigned int shift,
                                                           cint32_t* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 1;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 2;
    unsigned int shift_tw = 15; // twiddle is signed cint16.

    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint32>;

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw1, tw2);
    auto it_out0_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out0_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 4);
    auto it_out1_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / 4);
    auto it_out1_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 3 * n / 4);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0_lo++ = out[0];
            *it_out1_lo++ = out[1];
            *it_out0_hi++ = out[2];
            *it_out1_hi++ = out[3];
        }
}

template <>
void INLINE_DECL stage1_radix4_dit<cint32, cint16, cint16>(const cint32_t* x,
                                                           const cint16_t* tw1,
                                                           const cint16_t* tw2,
                                                           unsigned int n,
                                                           unsigned int shift,
                                                           cint16_t* __restrict y,
                                                           bool inv) {
    const unsigned int kStockhamStage = 1;
    const unsigned int kStageRadix = 4;
    const int kIndexStep = n >> 2;
    unsigned shift_tw = 15;                                                  // shift;
    using FFT = ::aie::fft_dit<kStockhamStage, kStageRadix, cint32, cint16>; // type = cint32, stage = 0, radix = 2

    FFT fft;

    auto it_stage = fft.begin_stage(x, tw1, tw2);
    auto it_out0_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y);
    auto it_out0_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + n / 4);
    auto it_out1_lo = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 2 * n / 4);
    auto it_out1_hi = ::aie::begin_restrict_vector<FFT::out_vector_size>(y + 3 * n / 4);

    for (int j = 0; j < n / (kStageRadix * FFT::out_vector_size); ++j)
        chess_prepare_for_pipelining chess_loop_range(1, ) {
            const auto out = fft.dit(*it_stage++, shift_tw, shift, inv);
            *it_out0_lo++ = out[0];
            *it_out1_lo++ = out[1];
            *it_out0_hi++ = out[2];
            *it_out1_hi++ = out[3];
        }
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_UTILS_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
