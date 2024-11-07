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
#ifndef _DSPLIB_WIDGET_2CH_REAL_FFT_UTILS_HPP_
#define _DSPLIB_WIDGET_2CH_REAL_FFT_UTILS_HPP_

/*
Widget for 2-Channel Real FFT Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "widget_2ch_real_fft.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget_2ch_real_fft {

template <typename TT, unsigned int vecSize>
INLINE_DECL void disentangle(::aie::vector<TT, vecSize>& vectForw,
                             ::aie::vector<TT, vecSize>& vectBack,
                             ::aie::vector<TT, vecSize>* __restrict(&outPtr),
                             int& k,
                             const int& kNumVecs) {
    using dataAcc_t = ::aie::accum<cacc64, vecSize>;
    using dataVect_t = ::aie::vector<TT, vecSize>;

    constexpr TT kMinusJ = {0, -1};

    dataAcc_t accFFT1 = ::aie::add(::aie::mul(vectForw, 1), vectBack);
    dataAcc_t accFFT2 = ::aie::sub(::aie::mul(vectForw, 1), vectBack);

    dataVect_t vectFFT2 = accFFT2.template to_vector<TT>();
    outPtr[k] = accFFT1.template to_vector<TT>(1);
    outPtr[kNumVecs / 2 + k] = ::aie::mul(kMinusJ, vectFFT2).template to_vector<TT>(1);
};

template <>
INLINE_DECL void disentangle(::aie::vector<cbfloat16, 8>& vectForw,
                             ::aie::vector<cbfloat16, 8>& vectBack,
                             ::aie::vector<cbfloat16, 8>* __restrict(&outPtr),
                             int& k,
                             const int& kNumVecs) {
    constexpr cbfloat16 kMinusHalfJ = {0.0, -0.5};
    constexpr cbfloat16 kHalfReal = {0.5, 0.0};

    outPtr[k] = ::aie::mul(::aie::add(vectForw, vectBack), kHalfReal);
    outPtr[kNumVecs / 2 + k] = ::aie::mul(::aie::sub(vectForw, vectBack), kMinusHalfJ);
};
}
}
}
}

#endif // _DSPLIB_WIDGET_2CH_REAL_FFT_UTILS_HPP_
