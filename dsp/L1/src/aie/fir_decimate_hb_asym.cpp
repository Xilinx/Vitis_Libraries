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
Halfband decimation FIR kernel code.
This file captures the body of run-time code for the kernel class and a higher wrapping 'cascade' layer which has
specializations for
 combinations of inputs and outputs. That is, in a chain of kernels, the first will have an input window, and a cascade
out stream.
 The next, potentially multiple, kernel(s) will each have an input window and cascade stream and will output a cascade
steam. The final kernel
 will have an input window and cascade stream and an output window only.
 The cascade layer class is called fir_interpolate_hb with the kernel-layer (operational) class called
kernelFilterClass.
 The fir_interpolate_hb class has a member of the kernelFilterClass..

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
//#define _DSPLIB_FIR_DECIMATE_HB_ASYM_HPP_DEBUG_

#ifdef __X86SIM__
// #define _DSPLIB_FIR_DECIMATE_HB_ASYM_HPP_DEBUG_
// #define _DSPLIB_FIR_SR_SYM_DEBUG_
#endif

#include "kernel_api_utils.hpp"
#include "fir_decimate_hb_asym.hpp"
#include "fir_decimate_hb_asym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb_asym {

// FIR function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    filterSelectArch(inInterface, outInterface);
};

// FIR function - overloaded (not specialised) with taps for reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                         const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    m_coeffnEq = rtpCompare(inTaps, m_oldInTaps);

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload(inTaps, m_oldInTaps, outInterface);
        firReload(inTaps);
    }
    // Make sure coeffs rearrangement is fenced off from next stage
    chess_memory_fence();
    filterSelectArch(inInterface, outInterface);
};

// FIR function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                            T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    m_coeffnEq = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, (TP_FIR_LEN + 1) / 2 + 1, TP_DUAL_IP>(inInterface, m_oldInTaps, outInterface);
        firReload(m_oldInTaps);
    }
    // Make sure coeffs rearrangement is fenced off from next stage
    chess_memory_fence();
    filterSelectArch(inInterface, outInterface);
};

// FIR function - overloaded (not specialised) with taps for reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat_mode<TP_SAT>();
    if
        constexpr(TP_API == 0) { filterBasic(inInterface, outInterface); }
    else {
        filterStream(inInterface, outInterface);
    }
};

//-----------------------------------------------------------------------------------------------------
// The filterBig1 variant of this function is for cases where 2 separate buffers must be used, one for forward data
// and the other for reverse data. This is for int32 and smaller
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // Pointers to coefficient storage and explicit registers to hold values
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>* __restrict coeff =
        (::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_internalTaps;
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)> coe0; // register for coeff values.
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>* __restrict coeffCt =
        (::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_phaseTwoTap;
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)> coeCt = *coeffCt; // register for coeff values.

    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)> sbuff;
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)> ph1Data, ph2Data;
    T_acc384<TT_DATA, TT_COEFF> acc;
    ::aie::vector<TT_DATA, m_kLanes> tempOutVal; // only for debug
    T_outVal384<TT_DATA, TT_COEFF> outVal;

    unsigned int numDataLoads;
    unsigned int dataLoaded, dataNeeded;
    unsigned int xstart, ystart;
    unsigned int lowPhOffset =
        ((m_kFirMarginLen - m_kDataWindowOffset) / kDecimateFactor - TP_FIR_LEN / 4 - 1 + m_kSamplesInDataBuff) %
        m_kSamplesInDataBuff;

    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1;
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding
    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    int extractIndex = 0;
    int singlePhLoads = 0;

    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = (::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_internalTapsCopy;
            coe0 = *coeff;

            numDataLoads = 0;
            dataLoaded = 0;

            dataNeeded = (m_kDataBuffXOffset + m_kLanes + m_kColumns - 2) * kDecimateFactor + 1;
            extractIndex = 0;
            singlePhLoads = 0;
            for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
                upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads, inItr);
                dataLoaded += m_kDataLoadVsize;
                numDataLoads++;
            }
            for (int sh = 0; sh < 2; sh++) {
                ph1Data.insert(
                    singlePhLoads % m_kDataLoadsInReg,
                    ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                            sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                        .first);
                ph2Data.insert(
                    singlePhLoads % m_kDataLoadsInReg,
                    ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                            sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                        .second);
                singlePhLoads++;
                extractIndex += kDecimateFactor;
            }
            acc = readCascade(inInterface, acc);
            acc = macDecHbAsym<TT_DATA, TT_COEFF>(acc, ph1Data, m_kDataBuffXOffset, coe0, 0);
            if
                constexpr(m_kCTOp <= 0) {
                    if
                        constexpr(m_kHasCT) { acc = macDecHbAsym(acc, ph2Data, lowPhOffset, coeCt, 0); }
                }
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                dataNeeded += m_kColumns * kDecimateFactor;
                if (dataNeeded > dataLoaded) {
                    for (int l = 0; l < kDecimateFactor; l++) {
                        upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
                        dataLoaded += m_kDataLoadVsize;
                        numDataLoads++;
                    }

                    ph1Data.insert(singlePhLoads % m_kDataLoadsInReg,
                                   ::aie::interleave_unzip(
                                       sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                       sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                                       .first);
                    ph2Data.insert(singlePhLoads % m_kDataLoadsInReg,
                                   ::aie::interleave_unzip(
                                       sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                       sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                                       .second);
                    extractIndex += kDecimateFactor;
                    singlePhLoads++;
                }
                if (op % m_kCoeffRegVsize == 0) {
                    coeff++;
                    coe0 = *coeff;
                }
                acc = macDecHbAsym(acc, ph1Data, (op + m_kDataBuffXOffset), coe0, (op % m_kCoeffRegVsize));
                if (op == m_kCTOp) {
                    if
                        constexpr(m_kHasCT) { acc = macDecHbAsym(acc, ph2Data, lowPhOffset, coeCt, 0); }
                }
            }
            outVal.val = acc.val.template to_vector<TT_DATA>(TP_SHIFT);

            // lowPhOffset += m_kLanes;
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            inItr -= m_kDataLoadVsize * numDataLoads - kDecimateFactor * m_kVOutSize;
        }
    doInit = 0;
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat_mode<TP_SAT>();
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>* __restrict coeff;
    coeff = (::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_internalTaps;
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)> coe0, coe1, coe2, coe3; // register for coeff values.
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>* __restrict coeffCt =
        (::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_phaseTwoTap;
    ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)> coeCt = *coeffCt; // register for coeff values.
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>* ptr_delay =
        (::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>*)delay;        // heap storage pointer
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)> sbuff = *ptr_delay; // initialize data register with data
                                                                           // allocated on heap which will contain
                                                                           // previous iterations margin samples.
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)> ph1Data, ph2Data;
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>* ptr_ph1Data =
        (::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>*)storePh1Data;
    ph1Data = *ptr_ph1Data;
    ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>* ptr_ph2Data =
        (::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>*)storePh2Data;
    ph2Data = *ptr_ph2Data;
    T_acc384<TT_DATA, TT_COEFF> acc;
    ::aie::vector<TT_DATA, m_kLanes> tempOutVal;
    ::aie::vector<TT_DATA, m_kLanes> outVal;

    int singlePhLoads = 0;
    int extractIndex = 0;
    unsigned int dataLoaded = 0;
    unsigned int dataNeeded = m_kLanes * kDecimateFactor;
    int numDataLoads = 0;

    static_assert((TP_INPUT_WINDOW_VSIZE % m_kLanes == 0) && (TP_INPUT_WINDOW_VSIZE >= m_kLanes),
                  "ERROR: WindowSize is not a multiple of lanes.");
    static_assert(
        ((m_kLsize / streamRptFactor) > 0),
        "ERROR: Window Size is too small, needs to be a multiple of the number of samples in a 1024b Buffer.");

    int loopSize = (m_kLsize / streamRptFactor);
    static_assert(((m_kLsize % streamRptFactor) == 0),
                  "ERROR: Inner loop unrolling not possible for given parameters. Modify Window Size.");

    if (doInit == 1) {
        for (unsigned i = 0; i < streamInitNullAccs; i++)
            chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                acc = readCascade(inInterface, acc);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                if
                    constexpr(TP_CASC_LEN == 1) { writeincr(outInterface.outStream, outVal); }
            }

#pragma unroll(GUARD_ZERO(streamInitAccs))
        for (unsigned strobe = 0; strobe < streamInitAccs; strobe++) {
            if (dataNeeded > dataLoaded) {
                readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                dataLoaded += m_kDataLoadVsize;
                numDataLoads++;
                readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                dataLoaded += m_kDataLoadVsize;
                numDataLoads++;
            }

            dataNeeded += m_kLanes * kDecimateFactor;
            ph1Data.insert(
                singlePhLoads % m_kDataLoadsInReg,
                ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                        sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                    .first);
            ph2Data.insert(
                singlePhLoads % m_kDataLoadsInReg,
                ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                        sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                    .second);

            singlePhLoads++;
            extractIndex += kDecimateFactor;
            coeff = ((::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_internalTaps);
            coe0 = *coeff++;
            acc = readCascade(inInterface, acc);
            acc = macDecHbAsym<TT_DATA, TT_COEFF>(acc, ph1Data, m_kDataBuffXOffsetStrm + (strobe * m_kLanes), coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                acc = macDecHbAsym(acc, ph1Data, (op + m_kDataBuffXOffsetStrm + (strobe * m_kLanes)), coe0,
                                   (op % m_kCoeffRegVsize));
            }
            if
                constexpr(m_kHasCT == 1) {
                    acc = macDecHbAsym<TT_DATA, TT_COEFF>(acc, ph2Data, lowPhOffset + (strobe * m_kLanes), coeCt, 0);
                }
            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
            outVal = acc.val.template to_vector<TT_DATA>(TP_SHIFT);
            if
                constexpr(TP_CASC_OUT == CASC_OUT_FALSE) { writeincr(outInterface.outStream, outVal); }
        }
        loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
    }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = streamInitAccs * kDecimateFactor;
            dataLoaded = 0;
            dataNeeded = m_kLanes * kDecimateFactor;
            singlePhLoads = streamInitAccs;
            extractIndex = streamInitAccs * kDecimateFactor;
#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
                if (dataNeeded > dataLoaded) {
                    readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                    readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                dataNeeded += m_kLanes * kDecimateFactor;
                ph1Data.insert(
                    singlePhLoads % m_kDataLoadsInReg,
                    ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                            sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                        .first);
                ph2Data.insert(
                    singlePhLoads % m_kDataLoadsInReg,
                    ::aie::interleave_unzip(sbuff.template extract<m_kLanes>(extractIndex % m_kDataLoadsInReg),
                                            sbuff.template extract<m_kLanes>((extractIndex + 1) % m_kDataLoadsInReg), 1)
                        .second);

                singlePhLoads++;
                extractIndex += kDecimateFactor;
                coeff = ((::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)>*)m_internalTaps);
                coe0 = *coeff++;
                acc = readCascade(inInterface, acc);
                acc = macDecHbAsym<TT_DATA, TT_COEFF>(
                    acc, ph1Data, m_kDataBuffXOffsetStrm + streamInitAccs * m_kLanes + (strobe * m_kLanes), coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    acc = macDecHbAsym(acc, ph1Data,
                                       (op + m_kDataBuffXOffsetStrm + streamInitAccs * m_kLanes + (strobe * m_kLanes)),
                                       coe0, (op % m_kCoeffRegVsize));
                }
                if
                    constexpr(m_kHasCT == 1) {
                        acc = macDecHbAsym<TT_DATA, TT_COEFF>(acc, ph2Data, mainLoopLowPhOffset + (strobe * m_kLanes),
                                                              coeCt, 0);
                    }
                tempOutVal = acc.val.template to_vector<TT_DATA>(0);
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                outVal = acc.val.template to_vector<TT_DATA>(TP_SHIFT);
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) { writeincr(outInterface.outStream, outVal); }
            }
        }
    doInit = 0;
    // store sbuff for next iteration
    *ptr_delay = sbuff;
    *ptr_ph1Data = ph1Data;
    *ptr_ph2Data = ph2Data;
};
//----------------------------------------------------------------------------
// Start of Cascade-layer class body and specializations

// Standalone kernel specialization with no cascade ports, Windowed. a single input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_SHIFT,
    TP_RND,
    TP_INPUT_WINDOW_VSIZE,
    TP_CASC_IN,
    TP_CASC_OUT,
    TP_FIR_RANGE_LEN,
    TP_KERNEL_POSITION,
    TP_CASC_LEN,
    TP_DUAL_IP,
    TP_USE_COEFF_RELOAD,
    TP_NUM_OUTPUTS,
    TP_API,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                    output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_LEN,
                          0,
                          1,
                          DUAL_IP_SINGLE,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                                extents<inherited_extent>,
                                                                margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                          output_circular_buffer<TT_DATA>& outWindow,
                                          const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                                extents<inherited_extent>,
                                                                margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                                extents<inherited_extent>,
                                                                margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow,
                                          const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual  input, no
// reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, with
// reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_WINDOW_API,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          output_stream<TT_DATA>* outStream,
                                          const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
}

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          0,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_FALSE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_DUAL_IP,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          0,
                          TP_CASC_LEN,
                          DUAL_IP_SINGLE,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          output_cascade_cacc* outCascade,
                                          const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT>
void fir_decimate_hb_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          DUAL_IP_SINGLE,
                          USE_COEFF_RELOAD_TRUE,
                          1,
                          USE_STREAM_API,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};
}
}
}
}
}
