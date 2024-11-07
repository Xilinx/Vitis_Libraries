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
/*
widget_2ch_real_fft kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
#include "widget_2ch_real_fft.hpp"
#include "kernel_api_utils.hpp"
#include "widget_2ch_real_fft_utils.hpp"
// #include "widget_2ch_real_fft_traits.hpp"

// #define _DSPLIB_WIDGET_2CH_REAL_FFT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget_2ch_real_fft {

//--------------------------------------------------------------------
// Base specialization, used for static size window API configurations
template <typename TT_DATA, unsigned int TP_POINT_SIZE, unsigned int TP_WINDOW_VSIZE>
NOINLINE_DECL void widget_2ch_real_fft<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE>::widget_2ch_real_fft_main(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    using dataVect_t = ::aie::vector<TT_DATA, kVecSampleNum>;
    using dataVectDouble_t = ::aie::vector<TT_DATA, 2 * kVecSampleNum>;
    using dataAcc_t = ::aie::accum<cacc64, kVecSampleNum>;

    dataVect_t* __restrict inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* __restrict outPtr = (dataVect_t*)outWindow.data();

    for (int frame = 0; frame < kNumFrames; frame++) chess_prepare_for_pipelining chess_loop_count(kNumFrames) {
            // First loop iteration which needs to kickstart the process..
            dataVect_t vectForw = inPtr[0];
            dataVect_t vectBack0 = ::aie::conj(
                ::aie::reverse(vectForw)); // In the following loop iterations this will be set to vectBack1.
            dataVect_t vectBack1 = ::aie::conj(::aie::reverse(inPtr[kNumVecs - 1]));
            dataVectDouble_t vectBackDouble = ::aie::concat(vectBack0, vectBack1);
            vectBackDouble = ::aie::shuffle_up(vectBackDouble, 1);
            dataVect_t vectBack = vectBackDouble.template extract<kVecSampleNum>(1);
            int k = 0;

            disentangle<TT_DATA, kVecSampleNum>(vectForw, vectBack, outPtr, k, kNumVecs);

            for (k = 1; k < kNumVecs / 2; k++) chess_prepare_for_pipelining chess_loop_count(kNumVecs / 2 - 1) {
                    dataVect_t vectForw = inPtr[k];
                    dataVect_t vectBack0 = vectBack1; // No need to re-read data..
                    vectBack1 = ::aie::conj(::aie::reverse(inPtr[kNumVecs - k - 1]));
                    dataVectDouble_t vectBackDouble = ::aie::concat(vectBack0, vectBack1);
                    vectBackDouble = ::aie::shuffle_up(vectBackDouble, 1);
                    dataVect_t vectBack = vectBackDouble.template extract<kVecSampleNum>(1);

                    disentangle<TT_DATA, kVecSampleNum>(vectForw, vectBack, outPtr, k, kNumVecs);
                }

            inPtr += kNumVecs;
            outPtr += kNumVecs;
        }
};
}
}
}
}
