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

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "twiddle_rotator.hpp"
#include "fft_ifft_dit_1ch.hpp"
#include "kernel_api_utils.hpp"
#include "dds_luts.h"
#include "fir_utils.hpp"
//#define _DSPLIB_TWID_ROT_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace twidRot {

// Helper function template to handle output writes
// Each specialization declares its own output types
template <unsigned int TP_API, typename TT_DATA_TYPE, unsigned int NUM_DATA_LANES>
inline void writeOutputData(T_outputIF<TT_DATA_TYPE, TP_API>& outInterface,
                            const ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES>& outVect,
                            ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES>*& outPtr) {
    if
        constexpr(TP_API == 0) {
            // Memory/Window output mode - use passed pointer (initialized per kernel invocation)
            *outPtr++ = outVect;
        }
    else {
        // Stream output mode - use interface directly (no static caching)
        ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES / 2> out128a =
            ::aie::filter_even< ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES> >(outVect);
        ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES / 2> out128b =
            ::aie::filter_odd< ::aie::vector<TT_DATA_TYPE, NUM_DATA_LANES> >(outVect);
        writeincr<aie_stream_resource_out::a, TT_DATA_TYPE, NUM_DATA_LANES / 2>(outInterface.outStream0, out128a);
        writeincr<aie_stream_resource_out::b, TT_DATA_TYPE, NUM_DATA_LANES / 2>(outInterface.outStream1, out128b);
    }
}

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_WINDOW_SIZE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE,
          unsigned int TP_API>
NOINLINE_DECL void twiddleRotator<TT_DATA,
                                  TT_TWIDDLE,
                                  TP_WINDOW_SIZE,
                                  TP_PT_SIZE_D1,
                                  TP_PT_SIZE_D2,
                                  TP_SSR,
                                  TP_FFT_NIFFT,
                                  TP_PHASE,
                                  TP_API>::twiddleRotationMain(T_inputIF<TT_DATA> inInterface,
                                                               T_outputIF<TT_DATA, TP_API> outInterface) {
    using dataVect_t = ::aie::vector<TT_DATA, m_kNumDataLanes>;
    using twidVect_t = ::aie::vector<TT_TWIDDLE, m_kNumTwLanes>;
    using accVect_t = ::aie::accum<typename tAccBaseTypeMul<TT_DATA, TT_TWIDDLE>::type, m_kNumDataLanes>;

    dataVect_t* inPtr = (dataVect_t*)inInterface.inWindow->data();

    // Initialize output pointer at start of each kernel invocation (not static)
    dataVect_t* outPtr = nullptr;
    if
        constexpr(TP_API == 0) { outPtr = (dataVect_t*)outInterface.outWindow->data(); }

    dataVect_t inData, outVect;
    dataVect_t inData2, outVect2;
    accVect_t acc;
    accVect_t acc2;
    unsigned int angle;
    twidVect_t twidAct;
    twidVect_t twidTmp;
    twidVect_t twidExt;
    if
        constexpr(m_kTwFanSize == 1) {
            twidPtr = (twMainPtr == 0) ? m_kTwRot : twidPtr + m_kNumTwLanes;
            twidExt = *(twidVect_t*)twidPtr;
        }
    else {
        twidPtr = (twMainPtr == 0) ? m_kTwRot : twidPtr;
    }

    //
    for (int frame = 0; frame < TP_WINDOW_SIZE / TP_PT_SIZE_D1; frame++) {
        if
            constexpr(m_kTwFanSize > 1) { twidExt = *(twidVect_t*)twidPtr; }
        if
            constexpr(sizeof(TT_DATA) == sizeof(TT_TWIDDLE)) {
                for (int ii = 0; ii < (TP_PT_SIZE_D1 / (m_kNumDataLanes * m_kRptFactor)); ii++)
                    chess_prepare_for_pipelining chess_loop_range(
                        (TP_PT_SIZE_D1 / (m_kNumDataLanes * m_kRptFactor)), ) { // factor of 2 two is because of the
                                                                                // difference in size of TT_DATA and
                                                                                // TT_TWIDDLE
#pragma unroll(m_kRptFactor)
                        for (int un = 0; un < m_kRptFactor; un++) {
                            if
                                constexpr(m_kTwFanSize > 1) {
                                    if
                                        constexpr(!std::is_same<TT_DATA, cfloat>()) {
                                            twidAct = ::aie::mul(m_kTwMain[twMainPtr], twidExt)
                                                          .template to_vector<TT_TWIDDLE>(m_kShift);
                                        }
                                    else {
                                        twidAct =
                                            ::aie::mul(m_kTwMain[twMainPtr], twidExt).template to_vector<TT_TWIDDLE>();
                                    }
                                    twMainPtr = twMainPtr + 1;
                                }
                            else {
                                twidAct = *(twidVect_t*)(m_kTwMain + twMainPtr);
                                twMainPtr = twMainPtr + m_kNumTwLanes;
                            }
                            inData = *inPtr++;
                            acc = ::aie::mul(twidAct, inData);
                            if
                                constexpr(!std::is_same<TT_DATA, cfloat>()) {
                                    outVect = acc.template to_vector<TT_DATA>(m_kShift);
                                }
                            else {
                                outVect = acc.template to_vector<TT_DATA>();
                            }
                            writeOutputData<TP_API, TT_DATA, m_kNumDataLanes>(outInterface, outVect, outPtr);
                        }
                    }
            }
        else {
            for (int ii = 0; ii < (TP_PT_SIZE_D1 / (m_kNumDataLanes * 2 * m_kRptFactor)); ii++)
                chess_prepare_for_pipelining chess_loop_range(
                    (TP_PT_SIZE_D1 / (m_kNumDataLanes * 2 * m_kRptFactor)), ) { // factor of 2 two is because of the
                                                                                // difference in size of TT_DATA and
                                                                                // TT_TWIDDLE
#pragma unroll(m_kRptFactor)
                    for (int un = 0; un < m_kRptFactor; un++) {
                        if
                            constexpr(m_kTwFanSize > 1) {
                                twidAct =
                                    ::aie::mul(m_kTwMain[twMainPtr], twidExt).template to_vector<TT_TWIDDLE>(m_kShift);
                                twMainPtr = twMainPtr + 1;
                            }
                        else {
                            twidAct = *(twidVect_t*)(m_kTwMain + twMainPtr);
                            twMainPtr = twMainPtr + m_kNumTwLanes;
                        }
                        inData = *inPtr++;
                        acc = ::aie::mul(twidAct.template extract<m_kNumDataLanes>(0), inData);
                        outVect = acc.template to_vector<TT_DATA>(m_kShift);
                        writeOutputData<TP_API, TT_DATA, m_kNumDataLanes>(outInterface, outVect, outPtr);
                        inData2 = *inPtr++;
                        acc2 = ::aie::mul(twidAct.template extract<m_kNumDataLanes>(1), inData2);
                        outVect2 = acc2.template to_vector<TT_DATA>(m_kShift);
                        writeOutputData<TP_API, TT_DATA, m_kNumDataLanes>(outInterface, outVect2, outPtr);
                    }
                }
        }
        if
            constexpr(m_kTwFanSize > 1) { twidPtr = twidPtr + m_kNumTwLanes; }
    }

    if
        constexpr(m_kTwFanSize > 1) {
            if (twMainPtr == m_ktwMainSize) {
                twMainPtr = 0;
            }
        }
    else {
        if
            constexpr(m_kTwFanSize == 1) {
                if (twMainPtr == m_ktwMainSize) {
                    twMainPtr = 0;
                }
            }
    }
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_WINDOW_SIZE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE,
          unsigned int TP_API>
NOINLINE_DECL void twiddleRotator<TT_DATA,
                                  TT_TWIDDLE,
                                  TP_WINDOW_SIZE,
                                  TP_PT_SIZE_D1,
                                  TP_PT_SIZE_D2,
                                  TP_SSR,
                                  TP_FFT_NIFFT,
                                  TP_PHASE,
                                  TP_API>::twiddleRotation(input_buffer<TT_DATA>& __restrict inWindow,
                                                           output_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA, TP_API> outInterface;
    inInterface.inWindow = &inWindow;
    outInterface.outWindow = &outWindow;
    this->twiddleRotationMain(inInterface, outInterface);
}

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_WINDOW_SIZE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE,
          unsigned int TP_API>
NOINLINE_DECL void twiddleRotator<TT_DATA,
                                  TT_TWIDDLE,
                                  TP_WINDOW_SIZE,
                                  TP_PT_SIZE_D1,
                                  TP_PT_SIZE_D2,
                                  TP_SSR,
                                  TP_FFT_NIFFT,
                                  TP_PHASE,
                                  TP_API>::twiddleRotationStream(input_buffer<TT_DATA>& __restrict inWindow,
                                                                 output_stream<TT_DATA>* outStream0,
                                                                 output_stream<TT_DATA>* outStream1) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA, TP_API> outInterface;
    inInterface.inWindow = &inWindow;
    outInterface.outStream0 = outStream0;
    outInterface.outStream1 = outStream1;
    this->twiddleRotationMain(inInterface, outInterface);
}
}
}
}
}
}
