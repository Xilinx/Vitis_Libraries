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
#ifndef _DSPLIB_FFT_IFFT_1D_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_1D_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "fft_ifft_dit_1ch_graph.hpp"
#include "twiddle_rotator.hpp"

using namespace adf;
using namespace xf::dsp::aie::fft::dit_1ch;
using namespace xf::dsp::aie::fft::twidRot;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace vss_1d {

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_WINDOW_SIZE_CALC,
          unsigned int TP_DIM,
          unsigned int TP_SSR,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_PT_SIZE_D2_CEIL = TP_PT_SIZE_D2,
          unsigned int TP_ROT_FAN_SIZE = 8>
class create_par_kernels_vss_decomp {
   public:
    static void create(kernel (&m_fftTwRotKernels)[TP_SSR],
                       std::array<std::array<TT_TWIDDLE, TP_PT_SIZE_D2_CEIL / TP_SSR * TP_ROT_FAN_SIZE>, TP_SSR>& twRot,
                       std::array<std::array<TT_TWIDDLE, TP_PT_SIZE_D2_CEIL / TP_SSR * TP_PT_SIZE_D1 / TP_ROT_FAN_SIZE>,
                                  TP_SSR>& twMain) {
        std::array<TT_TWIDDLE, TP_PT_SIZE_D2_CEIL / TP_SSR * TP_ROT_FAN_SIZE> twRotKernel;
        std::array<TT_TWIDDLE, TP_PT_SIZE_D2_CEIL / TP_SSR * TP_PT_SIZE_D1 / TP_ROT_FAN_SIZE> twMainKernel;
        memcpy(&twRotKernel, &twRot[TP_DIM], TP_PT_SIZE_D2_CEIL / TP_SSR * TP_ROT_FAN_SIZE * sizeof(TT_TWIDDLE));
        memcpy(&twMainKernel, &twMain[TP_DIM],
               TP_PT_SIZE_D2_CEIL / TP_SSR * TP_PT_SIZE_D1 / TP_ROT_FAN_SIZE * sizeof(TT_TWIDDLE));
        m_fftTwRotKernels[TP_DIM] =
            kernel::create_object<twiddleRotator<TT_DATA, TT_TWIDDLE, TP_WINDOW_SIZE_CALC, TP_PT_SIZE_D1, TP_PT_SIZE_D2,
                                                 TP_SSR, TP_FFT_NIFFT, TP_DIM> >(twRotKernel, twMainKernel);
        runtime<ratio>(m_fftTwRotKernels[TP_DIM]) = 0.2;
        // Source files
        source(m_fftTwRotKernels[TP_DIM]) = "twiddle_rotator.cpp";
        headers(m_fftTwRotKernels[TP_DIM]) = {"twiddle_rotator.hpp"};
        if
            constexpr(TP_DIM != 0) {
                create_par_kernels_vss_decomp<TT_DATA, TT_TWIDDLE, TP_WINDOW_SIZE_CALC, TP_DIM - 1, TP_SSR,
                                              TP_PT_SIZE_D1, TP_PT_SIZE_D2, TP_FFT_NIFFT, TP_DYN_PT_SIZE, TP_RND,
                                              TP_SAT, TP_PT_SIZE_D2_CEIL, TP_ROT_FAN_SIZE>::create(m_fftTwRotKernels,
                                                                                                   twRot, twMain);
            }
    }
};

/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// vss_fft_ifft_1d template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief vss_fft_ifft_1d is a single-channel, decomposed FFT that contains the AIE sub-part of the VSS FFT Mode 2
 *offering.
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the transform function. \n
 *         This is a typename and must be cfloat for this VSS Mode. \n
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be cfloat for this VSS Mode.
 * @tparam TP_POINT_SIZE is an unsigned integer which describes the number of samples in
 *         the transform. \n This must be 2^N where N is an integer in the range
 *         5 to 16 inclusive for AIE devices and 6 to 16 inclusive for AIE-ML devices.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0). Inverse FFT is not supported
 *for VSS Mode 2.
 * @tparam TP_SHIFT is not applicable for this VSS Mode.
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams minimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
 * @tparam TP_SSR is an unsigned integer to describe the number of parallel computational paths into which the
 *implementation will be split to improve the performance.
 *         Higher SSR relates to higher performance. For VSS MODE = 2, SSR needs to be set to be a power of 2.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *         \n
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML and AIE-MLv2 device.
 *\n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds an n-bit signed value in the
 *range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_TWIDDLE_MODE describes the magnitude of integer twiddles. It has no effect for cfloat. \n
 *         - 0: Max amplitude. Values at 2^15 (for TT_TWIDDLE=cint16) and 2^31 (TT_TWIDDLE=cint32) will saturate and so
 *introduce errors
 *         - 1: 0.5 amplitude. Twiddle values are 1/2 that of mode 0 so as to avoid twiddle saturation. However,
 *twiddles are one bit less precise versus mode 0.
 *
  **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class vss_fft_ifft_1d_front_only : public graph {
   public:
    // FFT twiddle rotation kernels that follow the first set of FFT operations.
    kernel m_fftTwRotKernels[TP_SSR];
    // This is a port that interfaces with a PL kernel internal to the VSS.
    port_array<input, TP_SSR> front_i;
    // This is a port that interfaces with a PL kernel internal to the VSS.
    port_array<output, TP_SSR> front_o;

   private:
    static constexpr unsigned int kIntDynPtSize = 0;
    static constexpr unsigned int kIntParPow = 0;
    static constexpr unsigned int kIntCascLen = 1;
    static constexpr unsigned int kIntUseWidg = 0;
    static constexpr unsigned int kHeaderBytes = kIntDynPtSize > 0 ? 32 : 0;
    static constexpr unsigned int kPtSizeD1 = fnPtSizeD1<TP_POINT_SIZE, usePLffts(), TP_SSR>(); // 1024
    static constexpr unsigned int kPtSizeD2 = TP_SSR;                                           // 4
    static constexpr unsigned int kPtSizeD2Ceil = fnCeil<kPtSizeD2, TP_SSR>();                  // 4
    static constexpr unsigned int kFirstFFTShift = TP_SHIFT / 2;
    static constexpr unsigned int kSecondFFTShift = TP_SHIFT - TP_SHIFT / 2;
    static constexpr unsigned int kWindowSizeRaw = kPtSizeD1;
    static constexpr unsigned int kWindowSizeCalc = kWindowSizeRaw * 2 * sizeof(TT_DATA) <= __DATA_MEM_BYTES__
                                                        ? kWindowSizeRaw
                                                        : __DATA_MEM_BYTES__ / (2 * sizeof(TT_DATA));
    static constexpr unsigned int kRotFanSize =
        TP_POINT_SIZE / TP_SSR * sizeof(TT_TWIDDLE) <= __DATA_MEM_BYTES__ ? 1 : fnNumLanes<TT_TWIDDLE, TT_TWIDDLE>();
    static constexpr int kInv = TP_FFT_NIFFT == 1 ? -1 : 1;
    static_assert(TP_POINT_SIZE % TP_SSR == 0, "TP_SSR has to be a multiple of TP_POINT_SIZE");
    void createTwidRotKernels() {
        std::array<std::array<TT_TWIDDLE, kRotFanSize>, kPtSizeD2> twRotTmp;
        std::array<std::array<TT_TWIDDLE, kPtSizeD2Ceil / TP_SSR * kRotFanSize>, TP_SSR> twRot;
        std::array<std::array<TT_TWIDDLE, kPtSizeD1 / kRotFanSize>, kPtSizeD2> twMainTmp;
        std::array<std::array<TT_TWIDDLE, kPtSizeD2Ceil / TP_SSR * kPtSizeD1 / kRotFanSize>, TP_SSR> twMain;
        int32 kScaleFactor =
            std::is_same<TT_TWIDDLE, cfloat>() ? 1 : std::is_same<TT_TWIDDLE, cint32>() ? (1 << 31) - 1 : (1 << 15) - 1;

        TT_TWIDDLE val;
        // calculate all fans
        for (int rr = 0; rr < kPtSizeD2; rr++) {
            for (unsigned ii = 0; ii < kRotFanSize; ii++) {
                val.real = cos(kInv * (2 * M_PI * rr * ii) / TP_POINT_SIZE) *
                           kScaleFactor; // cos(( 2 * pi * rr * ii ) / point_size)
                val.imag = sin(kInv * (2 * M_PI * rr * ii) / TP_POINT_SIZE) * kScaleFactor;
                twRotTmp[rr][ii] = val;
            }
            memcpy(&twRot[rr % TP_SSR][(rr / TP_SSR) * kRotFanSize], &twRotTmp[rr][0],
                   kRotFanSize * sizeof(TT_TWIDDLE));
            for (unsigned ii = 0; ii < kPtSizeD1 / kRotFanSize; ii++) {
                val.real = cos(kInv * (2 * M_PI * rr * ii * kRotFanSize) / TP_POINT_SIZE) *
                           kScaleFactor; // cos(( 2 * pi * rr * ii ) / point_size)
                val.imag = sin(kInv * (2 * M_PI * rr * ii * kRotFanSize) / TP_POINT_SIZE) * kScaleFactor;
                twMainTmp[rr][ii] = val;
            }
            memcpy(&twMain[rr % TP_SSR][(rr / TP_SSR) * (kPtSizeD1 / kRotFanSize)], &twMainTmp[rr][0],
                   kPtSizeD1 / kRotFanSize * sizeof(TT_TWIDDLE));
        }
        create_par_kernels_vss_decomp<TT_DATA, TT_TWIDDLE, kWindowSizeCalc, TP_SSR - 1, TP_SSR, kPtSizeD1, kPtSizeD2,
                                      TP_FFT_NIFFT, kIntDynPtSize, TP_RND, TP_SAT, kPtSizeD2Ceil,
                                      kRotFanSize>::create(m_fftTwRotKernels, twRot, twMain);
    }

   public:
    // FFT graph that performs the initial set of FFT calculations
    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           kPtSizeD1,
                           TP_FFT_NIFFT,
                           kFirstFFTShift,
                           kIntCascLen,
                           kIntDynPtSize,
                           kWindowSizeCalc,
                           TP_API,
                           kIntParPow,
                           kIntUseWidg,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE>
        frontFFTGraph[TP_SSR];

    /**
     * @brief This is the constructor function for the AIE sub-portion of the VSS FFT IP.
     **/
    vss_fft_ifft_1d_front_only() {
        createTwidRotKernels();
        for (int i = 0; i < TP_SSR; i++) {
            connect<>(front_i[i], frontFFTGraph[i].in[0]);
            connect<>(frontFFTGraph[i].out[0], m_fftTwRotKernels[i].in[0]);
            dimensions(m_fftTwRotKernels[i].in[0]) = {kWindowSizeCalc + kHeaderBytes / sizeof(TT_DATA)};

            connect<>(m_fftTwRotKernels[i].out[0], front_o[i]);
            dimensions(m_fftTwRotKernels[i].out[0]) = {kWindowSizeCalc + kHeaderBytes / sizeof(TT_DATA)};
        }
    };
};

} // namespace vss_1d
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_1D_GRAPH_HPP_