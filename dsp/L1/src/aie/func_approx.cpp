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
Function Approximation kernal code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include "device_defs.h"

// #define _DSPLIB_FUNC_APPROX_HPP_DEBUG_

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
//#include "func_approx_traits.hpp"
#include "func_approx.hpp"

#include "func_approx_utils.hpp"

#include "kernel_api_utils.hpp"
// #include "approx_luts.h"

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {

// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
func_approx<TT_DATA, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT>::
    funcApproxMain(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using compVect_t = ::aie::vector<complex_tt_data_t<TT_DATA>, kSamplesInVect>;

    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;

    dataVect_t dataVect, idxVect, outVect;
    dataVect_t fineVect, slopeVect, offsetVect;
    accVect_t acc;

    dataVect_t* __restrict inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* __restrict outPtr = (dataVect_t*)outWindow.data();

    TT_DATA* __restrict slopePtr = &m_slopeBuff[0];
    TT_DATA* __restrict offsetPtr = &m_offsetBuff[0];
    dataVect_t* __restrict slopeVectPtr = (dataVect_t*)m_slopeBuff;
    dataVect_t* __restrict offsetVectPtr = (dataVect_t*)m_offsetBuff;

    complex_tt_data_t<TT_DATA>* __restrict ptr1 = reinterpret_cast<complex_tt_data_t<TT_DATA>*>(&m_lut_ab[0]);
    complex_tt_data_t<TT_DATA>* __restrict ptr2 = reinterpret_cast<complex_tt_data_t<TT_DATA>*>(&m_lut_cd[0]);

    compVect_t compVect;

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            // dataVect = *inPtr++;
            // shift down data to get LUT address
            idxVect = ::aie::downshift(*inPtr++, TP_FINE_BITS);
            if
                constexpr(sizeof(TT_DATA) == 2) {
#pragma unroll(kSamplesInVect)
                    for (int j = 0; j < kSamplesInVect; j++) {
                        compVect[j] = ptr1[idxVect[j]];
                        // compVect[j + 1] = ptr2[idxVect[j + 1]];
                    }
                    *slopeVectPtr++ = ::aie::real(compVect);
                    *offsetVectPtr++ = ::aie::imag(compVect);
                }
            else {
#pragma unroll(kSamplesInVect)
                for (int j = 0; j < kSamplesInVect; j++) {
                    *slopePtr++ = ptr1[idxVect[j]].real;
                    *offsetPtr++ = ptr2[idxVect[j]].imag;
                }
            }
        }
    if
        constexpr(sizeof(TT_DATA) == 2) {
            slopeVectPtr -= (TP_WINDOW_VSIZE / kSamplesInVect);
            offsetVectPtr -= (TP_WINDOW_VSIZE / kSamplesInVect);
        }
    inPtr -= (TP_WINDOW_VSIZE / kSamplesInVect);
    chess_separator();

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            dataVect = *inPtr++;
            fineVect = ::aie::bit_and(fineMask, dataVect);
            offsetVect = *offsetVectPtr++;
            slopeVect = *slopeVectPtr++;

            // bit_and with a mask to get only the TP_FINE_BITS of dataVect
            acc = ::aie::from_vector<accType_t<TT_DATA> >(offsetVect, TP_FINE_BITS + TP_SHIFT);
            acc = ::aie::mac(acc, slopeVect, fineVect);
            *outPtr++ = acc.template to_vector<TT_DATA>(TP_FINE_BITS + TP_SHIFT);
        }
};

// Base specialization, used for static size window API configurations
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
func_approx<float, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT>::
    funcApproxMain(input_buffer<float>& __restrict inWindow, output_buffer<float>& __restrict outWindow) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    typedef float TT_DATA;
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;

    dataVect_t dataVect, offsetVect, slopeVect;

    TT_DATA* __restrict slopePtr = &m_offsetBuff[0];
    TT_DATA* __restrict offsetPtr = &m_slopeBuff[0];
    dataVect_t* __restrict slopeVectPtr = (dataVect_t*)m_offsetBuff;
    dataVect_t* __restrict offsetVectPtr = (dataVect_t*)m_slopeBuff;

    ::aie::vector<int32, kSamplesInVect> idxVect, intDataVect;
    accVect_t acc;
    dataVect_t* __restrict inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* __restrict outPtr = (dataVect_t*)outWindow.data();

    cfloat* __restrict ptr1 = reinterpret_cast<cfloat*>(&m_lut_ab[0]);
    cfloat* __restrict ptr2 = reinterpret_cast<cfloat*>(&m_lut_cd[0]);

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            // Load vector of x (floats)
            dataVect = *inPtr++;
            // cast vector to int16 and downshift to find LUT index
            intDataVect =
                ::aie::to_fixed<int32, kSamplesInVect>(dataVect, TP_FINE_BITS + TP_COARSE_BITS - TP_DOMAIN_MODE);
            idxVect = ::aie::downshift(intDataVect, TP_FINE_BITS);

#pragma unroll(kSamplesInVect)
            for (int j = 0; j < kSamplesInVect; j++) {
                *slopePtr++ = ptr1[idxVect[j]].real;
                *offsetPtr++ = ptr2[idxVect[j]].imag;
            }
        }
    chess_separator();
    inPtr -= (TP_WINDOW_VSIZE / kSamplesInVect);

    dataVect = *inPtr++;
    offsetVect = *offsetVectPtr++;
    slopeVect = *slopeVectPtr++;
    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            acc = ::aie::mac(::aie::from_vector<accType_t<TT_DATA> >(offsetVect), dataVect, slopeVect);
            *outPtr++ = acc.template to_vector<TT_DATA>();

            dataVect = *inPtr++;
            offsetVect = *offsetVectPtr++;
            slopeVect = *slopeVectPtr++;
        }
};
#ifdef _SUPPORTS_BFLOAT16_
// Kernel function - funcApproxLutAPI
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
func_approx<int16, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT>::
    funcApproxLutAPI(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    typedef int16 TT_DATA;
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;
    using lut_type = ::aie::lut<4, TT_DATA, TT_DATA>; // declare lookup table type with parallel access = 4

    dataVect_t dataVect;
    dataVect_t outVect;

    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();

    // My lut requires Number elements in the LUT (not accounting for repetition).
    const lut_type my_lut(kLutSize / 2, (TT_DATA*)m_lut_ab, (TT_DATA*)m_lut_cd);
    ::aie::linear_approx<TT_DATA, lut_type> linear_ap(my_lut, TP_FINE_BITS /*step bits*/, 0 /*bias*/,
                                                      TP_FINE_BITS /*shift offset*/);
    for (int i = 0; i < (TP_WINDOW_VSIZE / kSamplesInVect); i++) {
        // Load vector of x
        dataVect = *inPtr++;
        outVect = linear_ap.compute(dataVect).template to_vector<TT_DATA>(TP_FINE_BITS);
        *outPtr++ = outVect;
    }
};
// Base specialization, used for static size window API configurations
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
func_approx<bfloat16, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT>::
    funcApproxLutAPI(input_buffer<bfloat16>& __restrict inWindow, output_buffer<bfloat16>& __restrict outWindow) {
    typedef bfloat16 TT_DATA;
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;
    using lut_type = ::aie::lut<4, float, TT_DATA>; // declare lookup table type with parallel access = 4

    dataVect_t dataVect;
    dataVect_t outVect;

    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();

    // My lut requires Number elements in the LUT (not accounting for repetition).
    const lut_type my_lut(kLutSize / 2, (float*)m_lut_ab, (TT_DATA*)m_lut_cd);
    ::aie::linear_approx<TT_DATA, lut_type> linear_ap(my_lut, -TP_COARSE_BITS + TP_DOMAIN_MODE /*step bits*/,
                                                      0 /*bias*/, TP_SHIFT /*shift offset*/);
    for (int i = 0; i < (TP_WINDOW_VSIZE / kSamplesInVect); i++) {
        // Load vector of x
        dataVect = *inPtr++;
        outVect = linear_ap.compute(dataVect).template to_vector<TT_DATA>();
        *outPtr++ = outVect;
    }
};
#endif //_SUPPORTS_BFLOAT16_
}
}
}
}
