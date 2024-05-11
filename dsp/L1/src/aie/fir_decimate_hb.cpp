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

#ifdef __X86SIM__
// #define _DSPLIB_FIR_DECIMATE_HB_HPP_DEBUG_
// #define _DSPLIB_FIR_SR_SYM_DEBUG_
#endif

#include "kernel_api_utils.hpp"
#include "fir_decimate_hb.hpp"
#include "fir_decimate_hb_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {

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
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
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
                                                         const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
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
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
                                                                                                      outInterface);
    m_coeffnEq = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, (TP_FIR_LEN + 1) / 4 + 1, TP_DUAL_IP>(inInterface, m_oldInTaps, outInterface);
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
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // windowAcquire(inInterface);
    // 3 possible architectures depending on size of data/coef types, fir_len and input window size
    // Using a single data buffer for x and y (forward & reverse) or seperate
    if
        constexpr(m_kArch == kArch1Buff) { filter1buff(inInterface, outInterface); }
    else if
        constexpr((m_kArch == kArch2Buff) && (m_kArchZigZag == kArchZigZag)) {
            filter2buffzigzag(inInterface, outInterface);
        }
    else {
        filter2buff(inInterface, outInterface);
    }
    // windowRelease(inInterface);
};
// #undef _DSPLIB_FIR_DECIMATE_HB_HPP_DEBUG_

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
                                   TP_SAT>::filter2buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // Pointers to coefficient storage and explicit registers to hold values
    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_phaseOneTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_256b<TT_COEFF>* __restrict coeffCt = (T_buff_256b<TT_COEFF>*)m_phaseOneTapsCt;
    T_buff_256b<TT_COEFF> coeCt = *coeffCt; // register for coeff values.
    T_buff_128b<TT_DATA> xReadData;
    T_buff_128b<TT_DATA> yReadData;
    T_buff_FirDecHb<TT_DATA, TT_COEFF, m_kArch> xbuff;
    T_buff_FirDecHb<TT_DATA, TT_COEFF, m_kArch> ybuff;
    T_accFirDecHb<TT_DATA, TT_COEFF, m_kArch> acc;
    T_outValFiRDecHb<TT_DATA, TT_COEFF, m_kArch> outVal;
    unsigned int xDataNeeded, xDataLoaded, xNumDataLoads;
    unsigned int yDataNeeded, yDataLoaded, yNumDataLoads, ySplice;
    unsigned int xstart, ystart, coeffstart;
    unsigned int lastXstart, lastYstart;

    // output size can be less than data load size, so this iterator needs greater precision to reach finer addresses.
    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    auto yinItr = (TP_DUAL_IP == 0 || TP_API == USE_STREAM_API || TP_KERNEL_POSITION != 0)
                      ? fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                            ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                            : ::aie::begin_random_circular(*(inInterface.inWindowCirc))
                      : ::aie::begin_random_circular(*(inInterface.inWindowReverse));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding
    yinItr += m_kySpliceStart;    // Fast forward to 128b boundary containing first Y data

    for (int i = 0; i < m_kLsize / m_kPasses; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kPasses, ) {
#pragma unroll(m_kPasses)
            for (int pass = 0; pass < m_kPasses; ++pass) {
                coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[0][0]);
                coe0 = *coeff;
                xNumDataLoads = 0;
                xDataLoaded = 0;
                xstart = m_kDataBuffXOffset;
                ystart = m_kyStart; // first column of first load
                coeffstart = 0;

                // Pre-loading the ybuff differs from the xbuff load because ystart is not in general aligned to loads.
                yNumDataLoads = 0;
                yDataLoaded = 0;
                ySplice = 0;

                // m_kInitLoadsXneeded/m_kInitLoadsYneeded always an even number - either 2 or ceiled to 4.
                constexpr unsigned int xLoadsToDo =
                    (m_kLoadSize == 128 ? m_kInitLoadsXneeded / 2 : m_kInitLoadsXneeded);
                constexpr unsigned int yLoadsToDo =
                    (m_kLoadSize == 128 ? m_kInitLoadsYneeded / 2 : m_kInitLoadsYneeded);
                constexpr unsigned int sizeAdjust = (m_kLoadSize == 128 ? 2 : 1);
// preload xdata from window into xbuff register
#pragma unroll(xLoadsToDo)
                for (unsigned int initLoads = 0; initLoads < xLoadsToDo; ++initLoads) {
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * sizeAdjust)>(
                        xbuff, (xNumDataLoads / sizeAdjust) % m_kDataLoadsInReg, inItr);
                    xNumDataLoads += sizeAdjust;
                    xDataLoaded += sizeAdjust * m_kDataLoadVsize;
                }
// preload ydata from window into ybuff register
#pragma unroll(yLoadsToDo)
                for (unsigned int initLoads = 0; initLoads < yLoadsToDo; ++initLoads) {
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * sizeAdjust)>(
                        ybuff, (ySplice / sizeAdjust) % m_kDataLoadsInReg, yinItr);
                    ySplice += sizeAdjust;
                }

                xDataNeeded = m_kDataBuffXOffset + (m_kLanes + m_kColumns - 2) * kDecimateFactor +
                              1;                                      // e.g. D0 to D8 is 9, not 10.
                yDataNeeded = (m_kColumns - 1) * kDecimateFactor - 1; // datum is lane 0, but y needs go backwards.
                yDataLoaded += ystart % m_kDataLoadVsize;             // i.e. how many do we have in hand
                yinItr -= (m_kInitLoadsYneeded + 1) * m_kDataLoadVsize;
                ySplice = m_kNumOps * m_kDataLoadsInReg - 1; // allow ysplice to count down to 0

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);

                // Initial multiply
                if (m_kNumOps == 1 && m_kCtPresent == 1) {
                    lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    acc = initMacDecHbCt(acc, xbuff, lastXstart, ybuff, lastYstart,
                                         (m_kColumns - 1) * kDecimateFactor - 1, // m_kCtOffset
                                         coeCt, 0);
                } else {
                    acc = initMacDecHb(acc, xbuff, xstart, ybuff, ystart, coe0, /*coeffstart = */ 0);
                }

// In the operations loop, x and y buffs load at different times because y can start mid-splice.
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    if (op < m_kNumOps - 1) {
                        xDataNeeded += m_kColumns * kDecimateFactor;
                        yDataNeeded += m_kColumns * kDecimateFactor;
                    } else {
                        xDataNeeded += (m_kColumns - 1) * kDecimateFactor + 1;
                        yDataNeeded += (m_kColumns - 1) * kDecimateFactor + 1;
                    }
                    if (xDataNeeded > xDataLoaded) {
                        // Load xdata from window into xbuff register
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg,
                                                                               inItr);
                        xNumDataLoads++;
                        xDataLoaded += m_kDataLoadVsize;
                    }
                    if (yDataNeeded > yDataLoaded) {
                        // Load ydata from window into ybuff register
                        fnLoadYIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(ybuff, ySplice % m_kDataLoadsInReg,
                                                                               yinItr);
                        ySplice--;
                        yNumDataLoads++;
                        yDataLoaded += m_kDataLoadVsize; // 0 maps to mkDataLoadsVsize
                    }
                    xstart += m_kColumns * kDecimateFactor;
                    ystart -= m_kColumns * kDecimateFactor;
                    coeffstart += m_kColumns;
                    if (op % (m_kCoeffRegVsize / m_kColumns) == 0) {
                        // Load coefficients coe0 register
                        coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[op][0]);
                        coe0 = *coeff;
                    }
                    if ((op >= m_kNumOps - 1) && (m_kCtPresent == 1)) {
                        // Last operation includes the centre tap
                        lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        // Final multiply operation
                        acc = firDecHbMacSymCt(acc, xbuff, lastXstart, ybuff, lastYstart,
                                               (m_kColumns - 1) * kDecimateFactor - 1, // m_kCtOffset
                                               coeCt, 0);
                    } else {
                        // Multiply operation
                        acc =
                            firDecHbMacSym(acc, xbuff, xstart, ybuff, ystart, coe0, /*coeffstart = */ op * m_kColumns);
                    }
                } // operations loop
                inItr -= xNumDataLoads * m_kDataLoadVsize -
                         kDecimateFactor * m_kLanes; // return read pointer to start of next chunk of window.
                yinItr += (yNumDataLoads + 1) * m_kDataLoadVsize +
                          kDecimateFactor * m_kLanes; // return read pointer to start of next chunk of window.
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                // The data for the centre tap is the same data as required for the last op of the top phase, so is
                // already loaded

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, pass % 2, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, pass % 2, outItr2);
            }
        } // i loop (splice of window)
};

//-----------------------------------------------------------------------------------------------------
// This function processes data in the forward direction as normal, but then
// recognizes the fact that only one further incremental load is needed to start
// from the centre tap of the next set of outputs.
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
                                   TP_SAT>::filter2buffzigzag(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // Pointers to coefficient storage and explicit registers to hold values
    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_phaseOneTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_256b<TT_COEFF>* __restrict coeffCt = (T_buff_256b<TT_COEFF>*)m_phaseOneTapsCt;
    T_buff_256b<TT_COEFF> coeCt = *coeffCt; // register for coeff values.
    T_buff_128b<TT_DATA> yReadData;
    T_buff_FirDecHb<TT_DATA, TT_COEFF, m_kArch> xbuff;
    T_buff_FirDecHb<TT_DATA, TT_COEFF, m_kArch> ybuff;
    T_accFirDecHb<TT_DATA, TT_COEFF, m_kArch> acc;
    T_outValFiRDecHb<TT_DATA, TT_COEFF, m_kArch> outVal;
    unsigned int xDataNeeded, xDataLoaded, xNumDataLoads;
    unsigned int yDataNeeded, yDataLoaded, yNumDataLoads, ySplice;
    unsigned int xstart, ystart, coeffstart;
    unsigned int lastXstart, lastYstart;

    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    auto yinItr = (TP_DUAL_IP == 0 || TP_API == USE_STREAM_API || TP_KERNEL_POSITION != 0)
                      ? fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                            ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                            : ::aie::begin_random_circular(*(inInterface.inWindowCirc))
                      : ::aie::begin_random_circular(*(inInterface.inWindowReverse));

    // use separate window pointers for zag (reverse computation iterations)
    auto inItrRev = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                        ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                        : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    auto yinItrRev = (TP_DUAL_IP == 0 || TP_API == USE_STREAM_API || TP_KERNEL_POSITION != 0)
                         ? fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                               ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                               : ::aie::begin_random_circular(*(inInterface.inWindowCirc))
                         : ::aie::begin_random_circular(*(inInterface.inWindowReverse));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItrRev += m_kDataWindowOffset + (m_kNumOps - 1) * m_kDataLoadVsize - m_kDataLoadVsize * 0;

    // Starting chunk - reverse loads + realignment to xwindowPointer
    yinItrRev += m_kySpliceStart + (m_kInitLoadsYneeded - (m_kNumOps - 1)) * m_kDataLoadVsize;

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding
    yinItr += m_kySpliceStart;    // Fast forward to 128b boundary containing first Y data

    xNumDataLoads = 0;
    ySplice = 0;
// preload ydata from window into ybuff register
#pragma unroll(m_kInitLoadsYneeded)
    for (unsigned int initLoads = 0; initLoads < m_kInitLoadsYneeded; ++initLoads) {
        if ((m_kInitLoadsYneeded - initLoads) >= 2 && m_kLoadSize == 128) {
            fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * 2)>(ybuff, (ySplice / 2) % m_kDataLoadsInReg,
                                                                         yinItr);
            initLoads++;
            ySplice++;

        } else {
            // Note that initial Y loads are forwards, hence use the forward direction load function.
            fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(ybuff, ySplice % m_kDataLoadsInReg, yinItr);
        }
        ySplice++;
    }
    // Reset y window pointer
    yinItr -= (m_kInitLoadsYneeded + 1) * m_kDataLoadVsize;

// preload xdata from window into xbuff register
#pragma unroll(m_kInitLoadsXneeded)
    for (unsigned int initLoads = 0; initLoads < m_kInitLoadsXneeded; ++initLoads) {
        if ((m_kInitLoadsXneeded - initLoads) >= 2 && m_kLoadSize == 128) {
            fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * 2)>(xbuff, (xNumDataLoads / 2) % m_kDataLoadsInReg,
                                                                         inItr);
            xNumDataLoads++;
            initLoads++;
        } else {
            fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg, inItr);
        }
        xNumDataLoads++;
    }

    for (int i = 0; i < m_kLsize / 2; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize / 2, ) {
            coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[0][0]);
            coe0 = *coeff;
            xNumDataLoads = m_kInitLoadsXneeded;
            xDataLoaded = m_kInitLoadsXneeded * m_kDataLoadVsize;
            xstart = m_kDataBuffXOffset;
            ystart = m_kyStart; // first column of first load
            coeffstart = 0;
            yNumDataLoads = 0;
            // Pre-loading the ybuff differs from the xbuff load because ystart is not in general aligned to loads.
            xDataNeeded =
                m_kDataBuffXOffset + (m_kLanes + m_kColumns - 2) * kDecimateFactor + 1; // e.g. D0 to D8 is 9, not 10.
            yDataNeeded = (m_kColumns - 1) * kDecimateFactor - 1; // datum is lane 0, but y needs go backwards.
            yDataLoaded = ystart % m_kDataLoadVsize;              // i.e. how many do we have in hand
            ySplice = m_kNumOps * m_kDataLoadsInReg - 1;          // allow ysplice to count down to 0

// Unroll this so that we see constant splices.
#pragma unroll(m_kRepeatFactor)
            for (int dataLoadPhase = 0; dataLoadPhase < m_kRepeatFactor; dataLoadPhase++) {
                ///////////////////////// Forward /////////////////////////////////////////////////
                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);

                // Initial multiply
                if (m_kNumOps == 1 && m_kCtPresent == 1) {
                    lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    acc = initMacDecHbCt(acc, xbuff, lastXstart, ybuff, lastYstart,
                                         (m_kColumns - 1) * kDecimateFactor - 1, // m_kCtOffset
                                         coeCt, 0);
                } else {
                    acc = initMacDecHb(acc, xbuff, xstart, ybuff, ystart, coe0, /*coeffstart = */ 0);
                }

// In the operations loop, x and y buffs load at different times because y can start mid-splice.
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    if (op < m_kNumOps - 1) {
                        xDataNeeded += m_kColumns * kDecimateFactor;
                        yDataNeeded += m_kColumns * kDecimateFactor;
                    } else {
                        xDataNeeded += (m_kColumns - 1) * kDecimateFactor + 1;
                        yDataNeeded += (m_kColumns - 1) * kDecimateFactor + 1;
                    }
                    if (xDataNeeded > xDataLoaded) {
                        // Load xdata from window into xbuff register
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg,
                                                                               inItr);
                        xNumDataLoads++;
                        xDataLoaded += m_kDataLoadVsize;
                    }
                    if (yDataNeeded > yDataLoaded) {
                        // Load ydata from window into ybuff register
                        fnLoadYIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(ybuff, ySplice % m_kDataLoadsInReg,
                                                                               yinItr);
                        ySplice--;
                        yNumDataLoads++;
                        yDataLoaded += m_kDataLoadVsize; // 0 maps to mkDataLoadsVsize
                    }

                    xstart += m_kColumns * kDecimateFactor;
                    ystart -= m_kColumns * kDecimateFactor;
                    coeffstart += m_kColumns;
                    if (op % (m_kCoeffRegVsize / m_kColumns) == 0) {
                        // Load coefficients coe0 register
                        coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[op][0]);
                        coe0 = *coeff;
                    }
                    if ((op >= m_kNumOps - 1) && (m_kCtPresent == 1)) {
                        // Last operation includes the centre tap
                        lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        // Final multiply operation
                        acc = firDecHbMacSymCt(acc, xbuff, lastXstart, ybuff, lastYstart,
                                               (m_kColumns - 1) * kDecimateFactor - 1, // m_kCtOffset
                                               coeCt, 0);
                    } else {
                        // Multiply operation
                        acc =
                            firDecHbMacSym(acc, xbuff, xstart, ybuff, ystart, coe0, /*coeffstart = */ op * m_kColumns);
                    }
                } // operations loop

                yinItr += ((m_kNumOps + 1) / m_kOpsEachLoad) * m_kDataLoadVsize; // moved from prep for forward stage

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                // The data for the centre tap is the same data as required for the last op of the top phase, so is
                // already loaded

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0,
                                                                       outItr); // stream 0 in zig
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr2);
                    }

                ////////////////// Prepare for Reverse /////////////////////
                // Need a chunk more data for reverse midpoint calc
                xDataNeeded += m_kDataNeededLastOp;
                yDataNeeded += m_kDataNeededLastOp;

                if (xDataNeeded > xDataLoaded) {
                    // Load xdata from window into xbuff register - read dec
                    fnLoadYIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg,
                                                                           inItr);
                    // we don't ajust splice for this load.
                    // xNumDataLoads++;
                    xDataLoaded += m_kDataLoadVsize;
                }
                // This could be a decrement or increment depending on num_ops (fir_len)
                // 23 tap cint16 int16 would be +1, 55 tap cint16 int16 would be -1
                inItr += m_kDataLoadVsize * m_kWindowModX;
                if (yDataNeeded > yDataLoaded) {
                    // Load ydata from window into ybuff register, in forward direction for next chunk of outputs
                    // need to adjust ySplice
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(ybuff, (ySplice - 1) % m_kDataLoadsInReg,
                                                                           yinItrRev);
                    // we don't ajust splice for this load.
                    yDataLoaded += m_kDataLoadVsize;
                }
                ////////////////// Reverse ////////////////////////////////

                // Final xystart values
                xstart = m_kDataBuffXOffset + m_kDataNeededEachOp * (m_kNumOps);
                ystart = m_kyStart - m_kDataNeededEachOp * (m_kNumOps);

                // coefstart already starts at last mid point.

                lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                // midpoint multiply operation
                if (m_kCtPresent == 1) {
                    acc = initMacDecHbCt(acc, xbuff, lastXstart, ybuff, lastYstart,
                                         (m_kColumns - 1) * kDecimateFactor - 1, // m_kCtOffset
                                         coeCt, 0);
                } else {
                    acc = initMacDecHb(acc, xbuff, xstart, ybuff, ystart, coe0,
                                       /*coeffstart = */ m_kColumns * (m_kNumOps - 1));
                }

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    xDataNeeded += m_kDataNeededEachOp;
                    yDataNeeded += m_kDataNeededEachOp;

                    if (xDataNeeded > xDataLoaded) {
                        // Load xdata from window into xbuff register
                        fnLoadYIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(
                            xbuff, (m_kDataLoadsInReg - xNumDataLoads) % m_kDataLoadsInReg, inItrRev);
                        xNumDataLoads++;
                        xDataLoaded += m_kDataLoadVsize;
                    }
                    if (yDataNeeded > yDataLoaded) {
                        // Load ydata from window into ybuff register
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(
                            ybuff, (m_kDataLoadsInReg - ySplice) % m_kDataLoadsInReg, yinItrRev);
                        ySplice--;
                        yNumDataLoads++;
                        yDataLoaded += m_kDataLoadVsize; // 0 maps to mkDataLoadsVsize
                    }

                    // reverse direction
                    xstart -= m_kDataNeededEachOp;
                    ystart += m_kDataNeededEachOp;
                    coeffstart -= m_kColumns;
                    // m_kNumOps-op gives the forward-equivalent op.
                    if ((m_kNumOps - op) % (m_kCoeffRegVsize / m_kColumns) == 0) {
                        // Load a CoeffRegSize worth of coefficients up to (m_kNumOps - op)
                        coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[(m_kNumOps - op) -
                                                                         (m_kCoeffRegVsize / m_kColumns)][0]);
                        coe0 = *coeff;
                    }
                    // Reverse loop always has standard mac, as centre tap calc is initial calc
                    acc = firDecHbMacSym(acc, xbuff, xstart, ybuff, ystart, coe0,
                                         /*coeffstart = */ m_kColumns * (m_kNumOps - 1 - op));
                } // operations loop

                inItrRev += ((m_kNumOps + 1) / m_kOpsEachLoad) * m_kDataLoadVsize; // moved from prep for forward stage

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 1,
                                                                       outItr); // stream 1 in zag
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 1, outItr2);

                ///////////////// Prepare for Forward //////////////////

                xNumDataLoads = m_kInitLoadsXneeded - 1; // we only need one load
                xDataLoaded = (m_kInitLoadsXneeded - 1) * m_kDataLoadVsize;
                yNumDataLoads = m_kInitLoadsYneeded - 1;                    // we only need one load;
                yDataLoaded = (m_kInitLoadsYneeded - 1) * m_kDataLoadVsize; // i.e. how many do we have in hand
                ySplice = m_kInitLoadsYneeded - 1;                          // we only need one load

// preload xdata from window into xbuff register
#pragma unroll(1)
                for (unsigned int initLoads = 0; initLoads < 1; ++initLoads) {
                    if ((1 - initLoads) >= 2 && m_kLoadSize == 128) {
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * 2)>(
                            xbuff, (xNumDataLoads / 2) % m_kDataLoadsInReg, inItr);
                        xNumDataLoads++;
                        initLoads++;
                        xDataLoaded += m_kDataLoadVsize;
                    } else {
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg,
                                                                               inItr);
                    }
                    // fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(xbuff, xNumDataLoads % m_kDataLoadsInReg,
                    // inWindow);
                    xNumDataLoads++;
                    xDataLoaded += m_kDataLoadVsize;
                }
// preload ydata from window into ybuff register
#pragma unroll(1)
                for (unsigned int initLoads = 0; initLoads < 1; ++initLoads) {
                    if ((1 - initLoads) >= 2 && m_kLoadSize == 128) {
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, (m_kLoadSize * 2)>(
                            ybuff, (ySplice / 2) % m_kDataLoadsInReg, yinItrRev);
                        initLoads++;
                        ySplice++;

                    } else {
                        // Note that initial Y loads are forwards, hence use the forward direction load function.
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(ybuff, ySplice % m_kDataLoadsInReg,
                                                                               yinItrRev);
                    }
                    ySplice++;
                }

                xNumDataLoads = m_kInitLoadsXneeded;
                xDataLoaded = m_kInitLoadsXneeded * m_kDataLoadVsize;
                xstart = m_kDataBuffXOffset;
                ystart = m_kyStart; // first column of first load
                coeffstart = 0;
                // Pre-loading the ybuff differs from the xbuff load because ystart is not in general aligned to loads.
                xDataNeeded = m_kDataBuffXOffset + (m_kLanes + m_kColumns - 2) * kDecimateFactor +
                              1;                                      // e.g. D0 to D8 is 9, not 10.
                yDataNeeded = (m_kColumns - 1) * kDecimateFactor - 1; // datum is lane 0, but y needs go backwards.
                yDataLoaded = ystart % m_kDataLoadVsize;              // i.e. how many do we have in hand
                ySplice = m_kNumOps * m_kDataLoadsInReg - 1;          // allow ysplice to count down to 0

                // reset window pointers to be reading from correct slices
                // TODO: check if we need floor or ceil num loads calc
                yinItrRev -= ((m_kNumOps - 1) / m_kOpsEachLoad) * m_kDataLoadVsize;

            } // strobe loop
        }     // i loop (splice of window)
};

//-----------------------------------------------------------------------------------------------------
// The filterSmall variant of this function is for cases where all the data samples required may be loaded into
// the sbuff such that the single buffer may be used for both xbuff and ybuff.
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
                                   TP_SAT>::filter1buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // Pointers to coefficient storage and explicit registers to hold values
    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_phaseOneTaps;
    T_buff_256b<TT_COEFF> coe0, coe1; // register for coeff values.
    T_buff_256b<TT_COEFF>* __restrict coeffCt = (T_buff_256b<TT_COEFF>*)m_phaseOneTapsCt;
    T_buff_256b<TT_COEFF> coeCt = *coeffCt; // register for coeff values.
    T_buff_FirDecHb<TT_DATA, TT_COEFF, m_kArch> sbuff;
    T_accFirDecHb<TT_DATA, TT_COEFF, m_kArch> acc; // 1 for small 1buff algo.
    T_outValFiRDecHb<TT_DATA, TT_COEFF, m_kArch> outVal;

    unsigned int xDataLoaded, xDataNeeded, numDataLoads;
    unsigned int xstart, ystart, coeffstart;

    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding
    // Architecture never requires more than 1 register set of coeffs. No longer true for 16-bit data and 32-bit coeffs
    coeff = ((T_buff_256b<TT_COEFF>*)&m_phaseOneTaps[0][0]);
    coe0 = *coeff++;
    coe1 = *coeff++;

// preamble, load data from window into register
#pragma unroll(m_kInitialLoads - m_kIncrLoadsTopUp)
    for (unsigned int initLoads = 0; initLoads < m_kInitialLoads - m_kIncrLoadsTopUp; ++initLoads) {
        fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(sbuff, initLoads, inItr);
    }

    for (unsigned i = 0; i < m_kLsize / m_kIncrRepeatFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kIncrRepeatFactor, ) {
            numDataLoads = 0;
            xDataLoaded = (m_kInitialLoads - m_kIncrLoadsTopUp) * m_kDataLoadVsize;
            xDataNeeded = m_kInitialLoads * m_kDataLoadVsize;
#pragma unroll(m_kIncrRepeatFactor)
            for (unsigned dataLoadPhase = 0; dataLoadPhase < m_kIncrRepeatFactor; dataLoadPhase++) {
                // coeff =  ((T_buff_256b<TT_COEFF> *)&m_phaseOneTaps[0][0]);
                // coe0 = *coeff++;
                if (xDataNeeded > xDataLoaded) {
                    // Load xdata from window into xbuff register
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(
                        sbuff, (m_kInitialLoads - m_kIncrLoadsTopUp) + numDataLoads, inItr);
                    numDataLoads++;
                    xDataLoaded += m_kDataLoadVsize;
                }
                if (xDataNeeded > xDataLoaded) {
                    // Load xdata from window into xbuff register
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kArch, m_kLoadSize>(
                        sbuff, (m_kInitialLoads - m_kIncrLoadsTopUp) + numDataLoads, inItr);
                    numDataLoads++;
                    xDataLoaded += m_kDataLoadVsize;
                }
                xDataNeeded += m_kVOutSize * kDecimateFactor;

                xstart = dataLoadPhase * m_kVOutSize * kDecimateFactor + m_kDataBuffXOffset;
                ystart = dataLoadPhase * m_kVOutSize * kDecimateFactor + m_kDataBuffXOffset + (TP_FIR_LEN - 1) -
                         m_kFirRangeOffset * kDecimateFactor;
                coeffstart = 0;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Initial multiply
                if (m_kNumOps == 1 && m_kCtPresent == 1) {
                    acc = initMacDecHbCt(acc, sbuff, xstart - m_kFinalOpSkew, ystart + m_kFinalOpSkew, m_kCtOffset,
                                         coeCt, 0); // Final multiply, if there's only one
                } else {
                    acc = initMacDecHb(acc, sbuff, xstart, ystart, coe0, coeffstart);
                }

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    xstart += m_kColumns * kDecimateFactor;
                    ystart -= m_kColumns * kDecimateFactor;
                    coeffstart += m_kColumns;
                    // if (op % (m_kCoeffRegVsize/m_kColumns) == 0) {
                    //     coe0 = *coeff++;
                    // }
                    if (op == m_kNumOps - 1 && m_kCtPresent == 1) {
                        // Final multiply
                        acc = firDecHbMacSymCt1buff(acc, sbuff, xstart - m_kFinalOpSkew, ystart + m_kFinalOpSkew,
                                                    m_kCtOffset, coeCt, 0);
                    } else {
                        // Multiply operation
                        acc = firDecHbMacSym1buff(acc, sbuff, xstart, ystart,
                                                  (op >= (m_kCoeffRegVsize / m_kColumns)) ? coe1 : coe0, coeffstart);
                    }
                }
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2,
                                                                               outItr2);
            }
        }
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
void fir_decimate_hb<
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

// Single kernel specialization. No cascade ports, Windowed. single input, with static coefficients, dual outputs
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<
    TT_DATA,
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
    USE_COEFF_RELOAD_FALSE,
    2,
    USE_WINDOW_API,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                    output_circular_buffer<TT_DATA>& __restrict outWindow,
                    output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
void fir_decimate_hb<TT_DATA,
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
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_WINDOW_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_circular_buffer<TT_DATA>& outWindow,
                                     output_circular_buffer<TT_DATA>& outWindow2,
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     1,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_circular_buffer<TT_DATA>& outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     2,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_circular_buffer<TT_DATA>& outWindow,
           output_circular_buffer<TT_DATA>& outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     1,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_circular_buffer<TT_DATA>& outWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     2,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_circular_buffer<TT_DATA>& outWindow,
           output_circular_buffer<TT_DATA>& outWindow2,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
void fir_decimate_hb<TT_DATA,
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
                                     input_stream_cacc48* inCascade,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// dual output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_WINDOW_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
void fir_decimate_hb<TT_DATA,
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
                                     input_stream_cacc48* inCascade,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, dual output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_WINDOW_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow,
                                     output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
void fir_decimate_hb<TT_DATA,
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
                                     output_stream_cacc48* outCascade,
                                     output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, no reload
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     1,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_stream_cacc48* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
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
void fir_decimate_hb<TT_DATA,
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
                                     output_stream_cacc48* outCascade,
                                     output_async_buffer<TT_DATA>& broadcastWindow,
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, with reload
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     1,
                     USE_WINDOW_API,
                     TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_stream_cacc48* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
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
void fir_decimate_hb<TT_DATA,
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
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade,
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
void fir_decimate_hb<TT_DATA,
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
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade,
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream<TT_DATA>* outStream,
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
}

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2,
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Single Output
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     1,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Dual Input
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     1,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream_cacc48* outCascade,
                                     output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Dual Input
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_FALSE,
                     1,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade,
                                     output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Single Output
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     1,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernelRtp(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Dual Output
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     2,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream<TT_DATA>* outStream,
                                     output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_stream_cacc48* outCascade,
                                     const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Dual Input
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
void fir_decimate_hb<
    TT_DATA,
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
    DUAL_IP_DUAL,
    USE_COEFF_RELOAD_TRUE,
    1,
    USE_STREAM_API,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                    output_stream_cacc48* outCascade,
                    output_async_buffer<TT_DATA>& broadcastWindow,
                    const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
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
void fir_decimate_hb<TT_DATA,
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
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Dual Input
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
void fir_decimate_hb<TT_DATA,
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
                     DUAL_IP_DUAL,
                     USE_COEFF_RELOAD_TRUE,
                     1,
                     USE_STREAM_API,
                     TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                     input_stream_cacc48* inCascade,
                                     output_stream_cacc48* outCascade,
                                     output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernelRtp(inInterface, outInterface);
};
}
}
}
}
}
