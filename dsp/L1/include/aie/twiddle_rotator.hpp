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

static constexpr unsigned int usePLffts() {
    return 1;
}
static constexpr unsigned int useAIEffts() {
    return 0;
}

// Returns the point size of the first set of FFTs of the decomposed FFT.
template <unsigned int TP_POINT_SIZE, unsigned int TP_VSS_MODE, unsigned TP_SSR>
constexpr unsigned int fnPtSizeD1() {
    unsigned int ptSizeD1 = -1;
    if (TP_VSS_MODE == usePLffts()) {
        ptSizeD1 = TP_POINT_SIZE / TP_SSR;
    } else if (TP_VSS_MODE == useAIEffts()) {
        // While this is simply the sqrt(point size) for perfect powers of 2 point sizes, for other point sizes like
        // 512, it can be decomposed either as 16 x 32 or 32 x 16.
        // Since the first set of FFT tiles will also contain the twiddle rotations, we choose the ffts with higher
        // performance to be placed on the same kernel as the twiddle rotators to better balance the performance with
        // the second set of FFTs
        // This is counter-intuitively the larger of the two numbers due to lesser overheads in larger point sizes
        // (determined based on experiments on the standalone fft).
        ptSizeD1 = TP_POINT_SIZE == 65536
                       ? 256
                       : TP_POINT_SIZE == 32768
                             ? 256
                             : TP_POINT_SIZE == 16384
                                   ? 128
                                   : TP_POINT_SIZE == 8192
                                         ? 128
                                         : TP_POINT_SIZE == 4096
                                               ? 64
                                               : TP_POINT_SIZE == 2048
                                                     ? 64
                                                     : TP_POINT_SIZE == 1024
                                                           ? 32
                                                           : TP_POINT_SIZE == 512
                                                                 ? 32
                                                                 : TP_POINT_SIZE == 256
                                                                       ? 16
                                                                       : TP_POINT_SIZE == 128
                                                                             ? 16
                                                                             : TP_POINT_SIZE == 64
                                                                                   ? 8
                                                                                   : TP_POINT_SIZE == 32
                                                                                         ? 8
                                                                                         : TP_POINT_SIZE == 16 ? 4 : 0;
    }
    return ptSizeD1;
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