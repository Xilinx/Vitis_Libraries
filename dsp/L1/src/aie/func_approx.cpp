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
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
INLINE_DECL void kernelFuncApproxClass<TT_DATA,
                                       TP_COARSE_BITS,
                                       TP_FINE_BITS,
                                       TP_DOMAIN_MODE,
                                       TP_WINDOW_VSIZE,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_SAT,
                                       TP_USE_LUT_RELOAD>::funcApproxKernel(T_inputIF<TT_DATA> inInterface,
                                                                            T_outputIF<TT_DATA> outInterface) {
    if
        constexpr(useLutAPI == 0) {
            if
                constexpr(isFloatingPoint) { funcApproxBasicFloat(inInterface, outInterface); }
            else {
                funcApproxBasic(inInterface, outInterface);
            }
        }
    else {
        if
            constexpr(isFloatingPoint) { funcApproxLutAPIFloat(inInterface, outInterface); }
        else {
            funcApproxLutAPI(inInterface, outInterface);
        }
    }
};

template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
INLINE_DECL void kernelFuncApproxClass<TT_DATA,
                                       TP_COARSE_BITS,
                                       TP_FINE_BITS,
                                       TP_DOMAIN_MODE,
                                       TP_WINDOW_VSIZE,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_SAT,
                                       TP_USE_LUT_RELOAD>::funcApproxBasic(T_inputIF<TT_DATA> inInterface,
                                                                           T_outputIF<TT_DATA> outInterface) {
    TT_DATA* staticLutPtr0 = m_staticLut0;
    TT_DATA* rtpLutPtr0 = m_rtpLutPtr0;
    TT_DATA* staticLutPtr1 = m_staticLut1;
    TT_DATA* rtpLutPtr1 = m_rtpLutPtr1;
    TT_DATA* __restrict lutPtr0 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr0 : staticLutPtr0;
    TT_DATA* __restrict lutPtr1 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr1 : staticLutPtr1;

    set_rnd_mode<rnd_floor>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using compVect_t = ::aie::vector<complex_tt_data_t<TT_DATA>, kSamplesInVect>;

    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;

    dataVect_t dataVect, idxVect, outVect;
    dataVect_t fineVect, slopeVect, offsetVect;
    accVect_t acc;

    dataVect_t* __restrict inPtr = (dataVect_t*)inInterface.inWindow;
    dataVect_t* __restrict outPtr = (dataVect_t*)outInterface.outWindow;

    TT_DATA* slopePtr = &m_slopeBuff[0];
    TT_DATA* offsetPtr = &m_offsetBuff[0];
    dataVect_t* slopeVectPtr = (dataVect_t*)m_slopeBuff;
    dataVect_t* offsetVectPtr = (dataVect_t*)m_offsetBuff;

    complex_tt_data_t<TT_DATA>* __restrict ptr1 = reinterpret_cast<complex_tt_data_t<TT_DATA>*>(lutPtr0);
    complex_tt_data_t<TT_DATA>* __restrict ptr2 = reinterpret_cast<complex_tt_data_t<TT_DATA>*>(lutPtr1);

    compVect_t compVect;

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            // dataVect = *inPtr++;
            // shift down data to get LUT address
            // idxVect = ::aie::downshift(*inPtr++, TP_FINE_BITS);
            idxVect = ::aie::detail::shift<TT_DATA, kSamplesInVect>::run(*inPtr++, 0, TP_FINE_BITS);
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
    set_rnd_mode<TP_RND>();

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
}
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
INLINE_DECL void kernelFuncApproxClass<TT_DATA,
                                       TP_COARSE_BITS,
                                       TP_FINE_BITS,
                                       TP_DOMAIN_MODE,
                                       TP_WINDOW_VSIZE,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_SAT,
                                       TP_USE_LUT_RELOAD>::funcApproxBasicFloat(T_inputIF<TT_DATA> inInterface,
                                                                                T_outputIF<TT_DATA> outInterface) {
    TT_DATA* staticLutPtr0 = m_staticLut0;
    TT_DATA* rtpLutPtr0 = m_rtpLutPtr0;
    TT_DATA* staticLutPtr1 = m_staticLut1;
    TT_DATA* rtpLutPtr1 = m_rtpLutPtr1;
    TT_DATA* __restrict lutPtr0 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr0 : staticLutPtr0;
    TT_DATA* __restrict lutPtr1 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr1 : staticLutPtr1;

    set_rnd_mode<rnd_floor>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;

    dataVect_t dataVect, offsetVect, slopeVect;

    TT_DATA* slopePtr = &m_offsetBuff[0];
    TT_DATA* offsetPtr = &m_slopeBuff[0];
    dataVect_t* slopeVectPtr = (dataVect_t*)m_offsetBuff;
    dataVect_t* offsetVectPtr = (dataVect_t*)m_slopeBuff;

    ::aie::vector<int32, kSamplesInVect> idxVect, intDataVect;
    accVect_t acc;
    dataVect_t* __restrict inPtr = (dataVect_t*)inInterface.inWindow;
    dataVect_t* __restrict outPtr = (dataVect_t*)outInterface.outWindow;

    cfloat* __restrict ptr1 = reinterpret_cast<cfloat*>(lutPtr0);
    cfloat* __restrict ptr2 = reinterpret_cast<cfloat*>(lutPtr1);

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            // Load vector of x (floats)
            dataVect = *inPtr++;
            // cast vector to int16 and downshift to find LUT index
            intDataVect =
                ::aie::to_fixed<int32, kSamplesInVect>(dataVect, TP_FINE_BITS + TP_COARSE_BITS - TP_DOMAIN_MODE);
            idxVect = ::aie::detail::shift<int32, kSamplesInVect>::run(intDataVect, 0, TP_FINE_BITS);
// idxVect = ::aie::downshift(intDataVect, TP_FINE_BITS);

#pragma unroll(kSamplesInVect)
            for (int j = 0; j < kSamplesInVect; j++) {
                *slopePtr++ = ptr1[idxVect[j]].real;
                *offsetPtr++ = ptr2[idxVect[j]].imag;
            }
        }
    chess_separator();
    set_rnd_mode<TP_RND>();

    inPtr -= (TP_WINDOW_VSIZE / kSamplesInVect);

    // FIX: Reset slope/offset vector pointers before second phase and avoid extra prefetch causing OOB read
    slopeVectPtr = (dataVect_t*)m_offsetBuff;
    offsetVectPtr = (dataVect_t*)m_slopeBuff;

    for (int i = 0; i < (TP_WINDOW_VSIZE / (kSamplesInVect)); i++)
        chess_prepare_for_pipelining chess_loop_range((TP_WINDOW_VSIZE / (kSamplesInVect)), ) {
            dataVect = *inPtr++;
            offsetVect = *offsetVectPtr++;
            slopeVect = *slopeVectPtr++;
            acc = ::aie::mac(::aie::from_vector<accType_t<TT_DATA> >(offsetVect), dataVect, slopeVect);
            *outPtr++ = acc.template to_vector<TT_DATA>();
        }
}

template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
INLINE_DECL void kernelFuncApproxClass<TT_DATA,
                                       TP_COARSE_BITS,
                                       TP_FINE_BITS,
                                       TP_DOMAIN_MODE,
                                       TP_WINDOW_VSIZE,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_SAT,
                                       TP_USE_LUT_RELOAD>::funcApproxLutAPI(T_inputIF<TT_DATA> inInterface,
                                                                            T_outputIF<TT_DATA> outInterface) {
    // RTP function approximation implementation
    TT_DATA* staticLutPtr0 = m_staticLut0;
    TT_DATA* rtpLutPtr0 = m_rtpLutPtr0;
    TT_DATA* staticLutPtr1 = m_staticLut1;
    TT_DATA* rtpLutPtr1 = m_rtpLutPtr1;
    TT_DATA* __restrict lutPtr0 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr0 : staticLutPtr0;
    TT_DATA* __restrict lutPtr1 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr1 : staticLutPtr1;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;
    using lut_type = ::aie::lut<4, TT_DATA, TT_DATA>; // declare lookup table type with parallel access = 4

    dataVect_t dataVect;
    dataVect_t outVect;

    dataVect_t* __restrict inPtr = (dataVect_t*)inInterface.inWindow;
    dataVect_t* __restrict outPtr = (dataVect_t*)outInterface.outWindow;

    // My lut requires Number elements in the LUT (not accounting for repetition).
    const lut_type my_lut(kLutSize / 2, (TT_DATA*)lutPtr0, (TT_DATA*)lutPtr1);
    ::aie::linear_approx<TT_DATA, lut_type> linear_ap(my_lut, TP_FINE_BITS /*step bits*/, 0 /*bias*/,
                                                      TP_FINE_BITS /*shift offset*/);
    for (int i = 0; i < (TP_WINDOW_VSIZE / kSamplesInVect); i++) {
        // Load vector of x
        dataVect = *inPtr++;
        outVect = linear_ap.compute(dataVect).template to_vector<TT_DATA>(TP_FINE_BITS);
        *outPtr++ = outVect;
    }
}
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
INLINE_DECL void kernelFuncApproxClass<TT_DATA,
                                       TP_COARSE_BITS,
                                       TP_FINE_BITS,
                                       TP_DOMAIN_MODE,
                                       TP_WINDOW_VSIZE,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_SAT,
                                       TP_USE_LUT_RELOAD>::funcApproxLutAPIFloat(T_inputIF<TT_DATA> inInterface,
                                                                                 T_outputIF<TT_DATA> outInterface) {
    // RTP function approximation implementation
    TT_LUT* staticLutPtr0 = (TP_USE_LUT_RELOAD == 1) ? nullptr : (TT_LUT*)&m_staticLut0[0];
    TT_LUT* rtpLutPtr0 = m_rtpLutPtr0;
    TT_LUT* staticLutPtr1 = (TP_USE_LUT_RELOAD == 1) ? nullptr : (TT_LUT*)&m_staticLut1[0];
    TT_LUT* rtpLutPtr1 = m_rtpLutPtr1;
    TT_LUT* __restrict lutPtr0 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr0 : staticLutPtr0;
    TT_LUT* __restrict lutPtr1 = (TP_USE_LUT_RELOAD == 1) ? rtpLutPtr1 : staticLutPtr1;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using accVect_t = ::aie::accum<accType_t<TT_DATA>, kSamplesInVect>;
    using lut_type = ::aie::lut<4, float, TT_DATA>; // declare lookup table type with parallel access = 4

    dataVect_t dataVect;
    dataVect_t outVect;

    dataVect_t* __restrict inPtr = (dataVect_t*)inInterface.inWindow;
    dataVect_t* __restrict outPtr = (dataVect_t*)outInterface.outWindow;

    // My lut requires Number elements in the LUT (not accounting for repetition).
    const lut_type my_lut(kLutSize / 2, (TT_LUT*)lutPtr0, (TT_DATA*)lutPtr1);
    ::aie::linear_approx<TT_DATA, lut_type> linear_ap(my_lut, -TP_COARSE_BITS + TP_DOMAIN_MODE /*step bits*/,
                                                      0 /*bias*/, TP_SHIFT /*shift offset*/);
    for (int i = 0; i < (TP_WINDOW_VSIZE / kSamplesInVect); i++) {
        // Load vector of x
        dataVect = *inPtr++;
        outVect = linear_ap.compute(dataVect).template to_vector<TT_DATA>();
        *outPtr++ = outVect;
    }
}

template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
NOINLINE_DECL void func_approx<TT_DATA,
                               TP_COARSE_BITS,
                               TP_FINE_BITS,
                               TP_DOMAIN_MODE,
                               TP_WINDOW_VSIZE,
                               TP_SHIFT,
                               TP_RND,
                               TP_SAT,
                               TP_USE_LUT_RELOAD>::approx(input_buffer<TT_DATA>& __restrict inWindow,
                                                          output_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindow = outWindow.data();
    this->funcApproxKernel(inInterface, outInterface);
};
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
func_approx<TT_DATA, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT, 1>::
    approxRtp(input_buffer<TT_DATA>& __restrict inWindow,
              output_buffer<TT_DATA>& __restrict outWindow,
              const TT_LUT (&lut0)[lutSize],
              const TT_LUT (&lut1)[lutSize]) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindow = outWindow.data();
    this->m_rtpLutPtr0 = (TT_LUT*)lut0;
    this->m_rtpLutPtr1 = (TT_LUT*)lut1;
    this->funcApproxKernel(inInterface, outInterface);
};
}
}
}
}
