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

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "twiddle_rotator.hpp"
#include "fft_ifft_dit_1ch.hpp"
#include "kernel_api_utils.hpp"
#include "debug_utils.h"
#include "dds_luts.h"
#include "twiddle_rotator_luts.hpp"
#include "fir_utils.hpp"
//#define _DSPLIB_TWID_ROT_DEBUG_
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace ifft_2d_aie_pl {

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE>
void twiddleRotator<TT_DATA, TT_TWIDDLE, TP_PT_SIZE_D1, TP_PT_SIZE_D2, TP_SSR, TP_FFT_NIFFT, TP_PHASE>::twiddleRotation(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    unsigned int phase;
    unsigned int phaseStep = 0;

    // static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int shift = sizeof(TT_TWIDDLE) / 2 * 8 - 1;
    static constexpr unsigned int numDataLanes = fnNumLanes<TT_DATA, TT_TWIDDLE>();
    static constexpr unsigned int numTwLanes = fnNumLanes<TT_TWIDDLE, TT_TWIDDLE>();
    static constexpr unsigned int rptFactor = 2;

    //
    // exp(+1i*2*pi*r*c/ptSize)  where [r, c] refer to the row and column positions in the matrix
    // aie::sincos(arg) is normalized by 1/pi and 'arg' is provided in Q1.31 format.
    // --> we need to shift by (31-log2(ptSize)) in order to normalize the angle
    static constexpr unsigned angleNorm = 31 - (fnLog2<(TP_PT_SIZE_D1 * TP_PT_SIZE_D2)>() - 1);

    using dataVect_t = ::aie::vector<TT_DATA, numDataLanes>;
    using twidVect_t = ::aie::vector<TT_TWIDDLE, numTwLanes>;
    using accVect_t = ::aie::accum<typename tAccBaseTypeMul<TT_DATA, TT_TWIDDLE>::type, numDataLanes>;

    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();
    dataVect_t inData, outVect;
    accVect_t acc;
    unsigned int angle;
    twidVect_t twidAct, twidRotate;
// for aie-ml
#if __SINCOS_IN_HW__ == 0
    ::aie::vector<TT_TWIDDLE, numTwLanes> sincosValCoarse;
    ::aie::vector<TT_TWIDDLE, numTwLanes> sincosValFine;
    ::aie::accum<cacc32, numTwLanes> ddsAccInter;
    TT_TWIDDLE twiddleLut;
#endif

    // Compute initial phase for this transform:
    phase = (count + TP_PHASE) << angleNorm; // 1 << 20

    // Compute vectorized phase ramp:
    phaseStep = 0;
#pragma unroll(numTwLanes)
    for (unsigned ii = 0; ii < numTwLanes; ii++) {
#if __SINCOS_IN_HW__ == 1
        twidRotate.set(::aie::sincos_complex(phaseStep), ii);
#else
        sincosValCoarse[0] = twiddle_coarse[phaseStep >> 22];
        sincosValFine[0] = twiddle_fine[(phaseStep >> 12) & 0x000003FF];
        ddsAccInter = ::aie::mul(sincosValCoarse, sincosValFine);
        twiddleLut = ddsAccInter.template to_vector<TT_TWIDDLE>(shift - 1)[0];
        twidRotate.set(twiddleLut, ii);
#endif
        if
            constexpr(TP_FFT_NIFFT == 1) { phaseStep -= phase; }
        else {
            phaseStep += phase;
        }
    }

    // vPrintf("twidrot = ", twidRotate);
    phase = 0;

    // unrolling loop for better pipelining
    for (int ii = 0; ii < TP_PT_SIZE_D1 / (numDataLanes * 2 * rptFactor); ii++)
        chess_prepare_for_pipelining chess_loop_range(TP_PT_SIZE_D1 / (numDataLanes * 2 * rptFactor), ) {
#pragma unroll(rptFactor)
            for (int un = 0; un < rptFactor; un++) {
#if __SINCOS_IN_HW__ == 1
                twidAct = ::aie::mul(::aie::sincos_complex(phase), twidRotate).template to_vector<TT_TWIDDLE>(15);
#else
                sincosValCoarse[0] = twiddle_coarse[phase >> 22];
                sincosValFine[0] = twiddle_fine[(phase >> 12) & 0x000003FF];
                ddsAccInter = ::aie::mul(sincosValCoarse, sincosValFine);
                twiddleLut = ddsAccInter.template to_vector<TT_TWIDDLE>(shift - 1)[0];
                twidAct = ::aie::mul(twiddleLut, twidRotate).template to_vector<TT_TWIDDLE>(15);
#endif
                inData = *inPtr++;
                acc = ::aie::mul(twidAct.template extract<numDataLanes>(0), inData);
                outVect = acc.template to_vector<TT_DATA>(shift);
                *outPtr++ = outVect;
                inData = *inPtr++;
                acc = ::aie::mul(twidAct.template extract<numDataLanes>(1), inData);
                outVect = acc.template to_vector<TT_DATA>(shift);
                *outPtr++ = outVect;
                phase += phaseStep;
            }
        }
    count = (count >= TP_PT_SIZE_D2 - TP_SSR) ? TP_PHASE : count + TP_SSR;
};
}
}
}
}
}
