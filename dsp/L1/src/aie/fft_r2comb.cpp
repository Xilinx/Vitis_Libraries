/*
 * Copyright 2021 Xilinx, Inc.
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
FFT/IFFT DIT single channel R2 combiner stage kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>
#include <stdio.h>

using namespace std;

#define __NEW_WINDOW_H__ 1
#define __AIEARCH__ 1
#define __AIENGINE__ 1
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include "fft_com_inc.h"
#include "fft_r2comb.hpp"
#include "kernel_api_utils.hpp"
#include "fft_ifft_dit_1ch_utils.hpp"
#include "fft_r2comb_twiddle_lut.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {

using namespace xf::dsp::aie::fft::dit_1ch;

template <typename TT_TWIDDLE>
inline constexpr TT_TWIDDLE null_tw(){};
template <>
inline constexpr cint16 null_tw<cint16>() {
    return {0, 0};
};
template <>
inline constexpr cfloat null_tw<cfloat>() {
    return {0.0, 0.0};
};

template <typename TT_TWIDDLE, int TP_INDEX, int TP_POINT_SIZE, int TP_PARALLEL_POWER>
constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER)> fnGetR2CombTwTable() {
    constexpr TT_TWIDDLE kzero = null_tw<TT_TWIDDLE>();
    constexpr int kTwiddleTableSize = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER;
    std::array<TT_TWIDDLE, kTwiddleTableSize> twiddles = {kzero};
    constexpr TT_TWIDDLE* twiddle_master = fnGetR2TwiddleMasterBase<TT_TWIDDLE>();
    int idx = ((kR2MasterTableSize >> TP_PARALLEL_POWER) * TP_INDEX);
    int stride = (2 * kR2MasterTableSize / TP_POINT_SIZE);
    for (int i = 0; i < kTwiddleTableSize; i++) {
        twiddles[i] = twiddle_master[idx];
        idx += stride;
    }
    return twiddles;
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX>
__attribute__((noinline)) void
fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX>::
    fft_r2comb_main(input_window<TT_DATA>* __restrict inWindow, output_window<TT_DATA>* __restrict outWindow) {
    alignas(32) static constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER)> twiddles =
        fnGetR2CombTwTable<TT_TWIDDLE, TP_INDEX, TP_POINT_SIZE, TP_PARALLEL_POWER>();

    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.

    bool inv;
    if
        constexpr(TP_FFT_NIFFT == 1) { inv = false; }
    else {
        inv = true;
    }
    // perform the R2 stage here.
    TT_DATA* xbuff = (TT_DATA*)inWindow->ptr;
    TT_DATA* ybuff = (TT_DATA*)outWindow->ptr;
    const int n = (TP_POINT_SIZE >> TP_PARALLEL_POWER);
    unsigned shift = TP_SHIFT + 15;

    for (int i = 0; i < TP_WINDOW_VSIZE; i += TP_POINT_SIZE) {
        r2comb_dit<TT_DATA, TT_TWIDDLE>(xbuff + i, (TT_TWIDDLE*)(&twiddles[0]), n, 0 /* r  */, TP_SHIFT + 15, ybuff + i,
                                        inv);
    }
};
}
}
}
}
}
