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
#ifndef _DSPLIB_TWIDDLE_ROTATOR_HPP_
#define _DSPLIB_TWIDDLE_ROTATOR_HPP_

#include <adf.h>
#include <vector>
#include "fir_utils.hpp"
#include "fft_ifft_dit_1ch_traits.hpp"
using namespace adf;
using namespace xf::dsp::aie::fft::dit_1ch;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace twidRot {

template <unsigned int TP_POINT_SIZE>
constexpr unsigned int fnPtSizeD1() {
    unsigned int sqrtVal =
        TP_POINT_SIZE == 65536
            ? 256
            : TP_POINT_SIZE == 32768
                  ? 128
                  : TP_POINT_SIZE == 16384
                        ? 128
                        : TP_POINT_SIZE == 8192
                              ? 64
                              : TP_POINT_SIZE == 4096
                                    ? 64
                                    : TP_POINT_SIZE == 2048
                                          ? 32
                                          : TP_POINT_SIZE == 1024
                                                ? 32
                                                : TP_POINT_SIZE == 512
                                                      ? 16
                                                      : TP_POINT_SIZE == 256
                                                            ? 16
                                                            : TP_POINT_SIZE == 128
                                                                  ? 8
                                                                  : TP_POINT_SIZE == 64
                                                                        ? 8
                                                                        : TP_POINT_SIZE == 32
                                                                              ? 4
                                                                              : TP_POINT_SIZE == 16 ? 4 : 0;
    return sqrtVal;
}

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_WINDOW_SIZE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE>
class twiddleRotator {
    static constexpr unsigned int m_kSampleRatioPLAIE = 2;
    static constexpr unsigned int m_kNumDataLanes = fnNumLanes<TT_DATA, TT_TWIDDLE>();
    static constexpr unsigned int m_kNumPadSamples =
        fnCeil<TP_PT_SIZE_D1, TP_SSR * m_kSampleRatioPLAIE>() - TP_PT_SIZE_D1;
    static constexpr unsigned int m_kPtSizeD2Ceil = fnCeil<TP_PT_SIZE_D2, TP_SSR>();
    static constexpr unsigned int m_kShift = sizeof(TT_TWIDDLE) / 2 * 8 - 1;
    static constexpr unsigned int m_kNumTwLanes = fnNumLanes<TT_TWIDDLE, TT_TWIDDLE>();
    static constexpr unsigned int m_kTwFanSize =
        (TP_PT_SIZE_D1 * TP_PT_SIZE_D2) / TP_SSR * sizeof(TT_TWIDDLE) <= 32768 ? 1 : m_kNumTwLanes;
    static constexpr unsigned int m_kRptFactor = 4;
    static constexpr unsigned int m_ktwRotSize = m_kPtSizeD2Ceil / TP_SSR * m_kTwFanSize;
    static constexpr unsigned int m_ktwMainSize = m_kPtSizeD2Ceil / TP_SSR * TP_PT_SIZE_D1 / m_kTwFanSize;

    unsigned twMainPtr = 0;
    TT_TWIDDLE (&m_kTwRot)[m_ktwRotSize];
    TT_TWIDDLE (&m_kTwMain)[m_ktwMainSize];
    TT_TWIDDLE* twidPtr = m_kTwRot;

   public:
    // Constructor
    twiddleRotator(TT_TWIDDLE (&twRot)[(m_kPtSizeD2Ceil / TP_SSR) * m_kTwFanSize],
                   TT_TWIDDLE (&twMain)[m_kPtSizeD2Ceil / TP_SSR * TP_PT_SIZE_D1 / m_kTwFanSize])
        : m_kTwRot(twRot), m_kTwMain(twMain) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_PARAMETER(m_kTwRot);
        REGISTER_PARAMETER(m_kTwMain);
        REGISTER_FUNCTION(twiddleRotator::twiddleRotation);
    }

    // Main function
    void twiddleRotation(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}
}
#endif