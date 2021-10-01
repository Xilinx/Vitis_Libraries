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
Single Rate Asymmetrical FIR kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __NEW_WINDOW_H__ 1
#define __AIEARCH__ 1
#define __AIENGINE__ 1
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "fir_sr_asym.hpp"
#include "kernel_api_utils.hpp"
#include "fir_sr_asym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {
// Single Rate Asymmetrical FIR run-time function
// According to template parameter the input may be a window, or window and cascade input
// Similarly the output interface may be a window or a cascade output
// FIR function
//----------------------------------------------------------------------------------------------------------------------
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    filterSelectArch(inInterface, outInterface);
};

// FIR function
//----------------------------------------------------------------------------------------------------------------------
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                    const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    m_coeffnEq = rtpCompare(inTaps, m_oldInTaps);

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload(inTaps, m_oldInTaps, outInterface);
        firReload(inTaps);
    }
    filterSelectArch(inInterface, outInterface);
};

// FIR function
//----------------------------------------------------------------------------------------------------------------------
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                       T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    m_coeffnEq = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_FIR_LEN>(inInterface, m_oldInTaps, outInterface);
        firReload(m_oldInTaps);
    }
    filterSelectArch(inInterface, outInterface);
};

// FIR function
//----------------------------------------------------------------------------------------------------------------------
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowReset<TT_DATA, TP_CASC_IN, TP_DUAL_IP, TP_API>(inInterface);
    // windowAcquire(inInterface);

    if
        constexpr(m_kArch == kArchIncLoads) { filterIncLoads(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchZigZag) { filterZigZag(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchStream) { filterStream(inInterface, outInterface); }
    else {
        filterBasic(inInterface, outInterface);
    }
    windowRelease(inInterface);
};

// ----------------------------------------------------- Basic ----------------------------------------------------- //
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                   T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat();
    T_buff_256b<TT_COEFF>* restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff; // input data value cache.
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded, numDataLoads;
    unsigned int dataNeeded;
    unsigned int dataLoadPhase = 0;

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    window_incr(inInterface.inWindow, (TRUNC((m_kFirInitOffset), (16 / sizeof(TT_DATA)))));

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);

            coe0 = *coeff;
            coeff++;

            numDataLoads = 0;
            dataLoaded = 0;

            // Preamble, calculate and load data from window into register
            dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;

#pragma unroll(m_kInitialLoads)
            for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
                upd_win_incr_256b<TT_DATA>(
                    sbuff, numDataLoads,
                    inInterface.inWindow); // Update sbuff with data from input window. 00++|____|____|____
                dataLoaded += m_kDataLoadVsize;
                numDataLoads++;
            }
            // Read cascade input. Do nothing if cascade input not present.
            acc = readCascade(inInterface, acc);
            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, m_kDataBuffXOffset, coe0, 0);
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                dataNeeded += m_kColumns;
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface.inWindow);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                acc = macSrAsym(acc, sbuff, (op + m_kDataBuffXOffset), coe0, (op % m_kCoeffRegVsize));
            }

            window_decr(inInterface.inWindow, (m_kDataLoadVsize * numDataLoads -
                                               m_kVOutSize)); // return read pointer to start of next chunk of window.

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

            outVal = shiftAndSaturate(acc, TP_SHIFT);
            // Write to output window
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++);
        }
};

// ----------------------------------------------------- Stream ----------------------------------------------------- //
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat();
    T_buff_256b<TT_COEFF>* restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2;                          // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;                                     // input data value cache.
    static T_buff_1024b<TT_DATA> delay = null_buff_1024b<TT_DATA>(); // retains margin data between calls
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded;
    unsigned int dataNeeded;
    unsigned int dataLoadPhase = 0;
    int dataOffset = 0;
    int numDataLoads = 0;

    constexpr int streamRptFactor = m_kSamplesInBuff / m_kVOutSize;

    constexpr int marginLoadsMappedToBuff = (m_kFirMargin % m_kSamplesInBuff) / m_kDataLoadVsize;
    constexpr int streamDataOffsetWithinBuff = (m_kFirInitOffset) % m_kSamplesInBuff;
    constexpr int streamInitNullAccs = ((TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset) / m_kVOutSize);
    constexpr int streamInitAccs = (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);
    int loopSize = (m_kLsize / streamRptFactor);
    int startDataLoads = marginLoadsMappedToBuff + streamInitAccs / (m_kDataLoadVsize / m_kVOutSize);
    int startDataOffset = (streamDataOffsetWithinBuff) % m_kSamplesInBuff;

    sbuff = delay;

    coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
    coe0 = *coeff++;
    // Load more pre-loop and hold each coe
    if (m_kFirLenCeilCols >= m_kCoeffRegVsize) {
        coe1 = *coeff++;
    }
    if (m_kFirLenCeilCols >= 2 * m_kCoeffRegVsize) {
        coe2 = *coeff;
    }

    // when firInitOffset requires special handlin
    static int doInit = (TP_CASC_LEN == 1 || streamInitNullAccs == 0) ? 0 : 1;
    // Init pre-loop to deal with m_kFirInitOffset. Only generate for cascaded designs
    if
        constexpr(TP_CASC_LEN > 1) {
            if (doInit == 1) {
                dataLoaded = 0;
                dataNeeded = m_kVOutSize;

                for (unsigned i = 0; i < streamInitNullAccs; i++)
                    chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                        acc = readCascade(inInterface, acc);
                        writeCascade<TT_DATA, TT_COEFF>(outInterface, null_acc<TT_DATA, TT_COEFF>());
                    }

#pragma unroll(GUARD_ZERO(streamInitAccs))
                for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
                    int xoffset = (streamInitNullAccs + strobe) * m_kVOutSize + streamDataOffsetWithinBuff;

                    if (dataNeeded > dataLoaded) {
                        // read only 128-bits for cint32/cint32 or 256-bits every odd loop
                        readStream256(sbuff, (marginLoadsMappedToBuff + strobe / (m_kDataLoadVsize / m_kVOutSize)) %
                                                 m_kDataLoadsInReg,
                                      inInterface);
                        dataLoaded += m_kDataLoadVsize;
                    }
                    dataNeeded += m_kVOutSize;

                    // Read cascade input. Do nothing if cascade input not present.
                    acc = readCascade(inInterface, acc);
                    // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                    acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, xoffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                    for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                        acc = macSrAsym(acc, sbuff, xoffset + op,
                                        op >= 2 * m_kCoeffRegVsize ? coe2 : op >= m_kCoeffRegVsize ? coe1 : coe0,
                                        (op % m_kCoeffRegVsize));
                    }
                    // Write cascade. Do nothing if cascade not present.
                    writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                }
                loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
            }
        }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = startDataLoads;
            dataOffset = startDataOffset;
            dataLoaded = 0;
            dataNeeded = m_kVOutSize;
// unroll streamRptFactor times
#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
                // if (dataNeeded > dataLoaded && numDataLoads < m_kLsize) {
                if (dataNeeded > dataLoaded) {
                    // read only 128-bits for cint32/cint32 or 256-bits every odd loop
                    readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }

                dataNeeded += m_kVOutSize;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, strobe * m_kVOutSize + dataOffset, coe0,
                                                       0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    acc = macSrAsym(acc, sbuff, strobe * m_kVOutSize + (op + dataOffset),
                                    op >= 2 * m_kCoeffRegVsize ? coe2 : op >= m_kCoeffRegVsize ? coe1 : coe0,
                                    (op % m_kCoeffRegVsize));
                }
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                outVal = shiftAndSaturate(acc, TP_SHIFT);
                // Write to output
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++);
            }
        }

    doInit = 0;

    // store sbuff for next iteration
    delay = sbuff;
};

// ---------------------------------------------------- IncLoads ---------------------------------------------------- //
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
void kernelFilterClass<TT_DATA,
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
                       TP_USE_COEFF_RELOAD,
                       TP_NUM_OUTPUTS,
                       TP_DUAL_IP,
                       TP_API>::filterIncLoads(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                               T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat();
    T_buff_256b<TT_COEFF>* restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;            // input data value cache.
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded, dataNeeded, numDataLoads;
    unsigned int dataLoadPhase = 0;

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    window_incr(inInterface.inWindow, (TRUNC((m_kFirInitOffset), (16 / sizeof(TT_DATA)))));

#pragma unroll(m_kInitialLoads - 1)
    for (int initLoads = 0; initLoads < m_kInitialLoads - 1; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads % m_kDataLoadsInReg, inInterface.inWindow);
    }

    coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
    coe0 = *coeff++;
    // Load more pre-loop and hold each coe
    if (m_kFirLenCeilCols >= m_kCoeffRegVsize) {
        coe1 = *coeff++;
    }
    if (m_kFirLenCeilCols >= 2 * m_kCoeffRegVsize) {
        coe2 = *coeff;
    }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / m_kIncLoadsRptFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kIncLoadsRptFactor, ) {
            numDataLoads = 0;
            dataLoaded = 0;
            dataNeeded = 0;
// unroll m_kIncLoadsRptFactor times
#pragma unroll(m_kIncLoadsRptFactor)
            for (unsigned strobe = 0; strobe < m_kIncLoadsRptFactor; strobe++) {
                if (dataNeeded >= dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, ((m_kInitialLoads - 1) + numDataLoads) % m_kDataLoadsInReg,
                                               inInterface.inWindow);
                    numDataLoads++;
                    dataLoaded += m_kDataLoadVsize;
                }
                dataNeeded += m_kVOutSize;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff,
                                                       strobe * m_kVOutSize + m_kDataBuffXOffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    acc = macSrAsym(acc, sbuff, strobe * m_kVOutSize + (op + m_kDataBuffXOffset),
                                    op >= 2 * m_kCoeffRegVsize ? coe2 : op >= m_kCoeffRegVsize ? coe1 : coe0,
                                    (op % m_kCoeffRegVsize));
                }

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                outVal = shiftAndSaturate(acc, TP_SHIFT);
                // Write to output window
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++);
                // writeOutput<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal);  // is this causing a
                // qor hit, compared to writeWindow?
                // writeWindow<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal);
            }
        }
};

// ----------------------------------------------------- ZigZag ----------------------------------------------------- //
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
inline void kernelFilterClass<TT_DATA,
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
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_DUAL_IP,
                              TP_API>::filterZigZag(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd(TP_RND);
    set_sat();

    T_buff_256b<TT_COEFF>* restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff; // input data value cache.
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded, dataNeeded, dataNeededTemp;
    unsigned int xRegSplicePtr, xRegSplicePtrOp1, xRegSplicePtrOp2;
    unsigned int xstart, xstart2;
    unsigned int zstart, zstart2;
    unsigned int dataLoadPhase = 0;

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    window_incr(inInterface.inWindow, (TRUNC((m_kFirInitOffset), (16 / sizeof(TT_DATA)))));

#pragma unroll(m_kInitialLoads)
    for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads, inInterface.inWindow);
    }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) // DIV 2
        chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
            coe0 = *coeff;

            xRegSplicePtr = m_kInitialLoads;
            dataLoaded = m_kInitialLoads * m_kDataLoadVsize;
            xstart = m_kDataBuffXOffset;

            // Preamble, calculate and load data from window into register
            dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;
// The following strobe loop is here only to allow loop unrolling for performance
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
                // ---------------------------------------------- FORWARD ----------------------------------------------
                // //
                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                zstart = 0;
                xstart = m_kDataBuffXOffset + strobe * m_kVOutSize * m_kZigZagFactor;
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, xstart, coe0, zstart);
                xRegSplicePtrOp1 = xRegSplicePtr; // tight control of variable scope to allow unrolling
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    dataNeeded += m_kColumns;
                    if (dataNeeded > dataLoaded) {
                        upd_win_incr_256b<TT_DATA>(sbuff, xRegSplicePtrOp1, inInterface.inWindow);
                        dataLoaded += m_kDataLoadVsize;
                        xRegSplicePtrOp1 = (xRegSplicePtrOp1 + 1) % m_kDataLoadsInReg;
                    }
                    if (op % m_kCoeffRegVsize == 0) {
                        coeff++;
                        coe0 = *coeff;
                    }
                    xstart = (xstart + m_kColumns) % m_kSamplesInBuff; //(op + m_kDataBuffXOffset);
                    zstart = (op % m_kCoeffRegVsize);
                    acc = macSrAsym(acc, sbuff, xstart, coe0, zstart);
                }
                xRegSplicePtr = xRegSplicePtrOp1;
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturate(acc, TP_SHIFT); // prep acc for output
                // Write to output window
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++);

                // ---------------------------------------------- PREP FOR REVERSE
                // ---------------------------------------------- //
                dataNeeded += m_kVOutSize;
                // top up with 1 load (potentially)
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, xRegSplicePtr, inInterface.inWindow);
                    xRegSplicePtr = (xRegSplicePtr + 1) % m_kDataLoadsInReg;
                }
                // prep for zag
                xRegSplicePtr =
                    (xRegSplicePtr + m_kDataLoadsInReg - 1) %
                    m_kDataLoadsInReg; // The first splice to overwrite on the way down is the one just loaded.
                xstart2 = (xstart + m_kVOutSize) %
                          m_kSamplesInBuff; // m_kDataBuffXOffset+m_kVOutSize+(m_kFirLenCeilCols-m_kColumns);
                zstart2 = (m_kFirLenCeilCols - m_kColumns) % m_kCoeffRegVsize;
                dataLoaded = m_kDataLoadVsize * m_kDataLoadsInReg; // a full buffer.
                dataNeeded = CEIL(xstart2 + m_kVOutSize + m_kColumns - 1, m_kDataLoadVsize) -
                             xstart2; // alignment padding plus actual needed

                window_decr<TT_DATA>(inInterface.inWindow, m_kSamplesInBuff + m_kDataLoadVsize);

                // ---------------------------------------------- REVERSE ----------------------------------------------
                // //
                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, xstart2, coe0, zstart2);
                xRegSplicePtrOp2 = xRegSplicePtr % m_kDataLoadsInReg;
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kFirLenCeilCols - 2 * m_kColumns; op >= 0; op -= m_kColumns) {
                    dataNeeded += m_kColumns;
                    if (dataNeeded > dataLoaded) {
                        upd_win_decr_256b<TT_DATA>(sbuff, xRegSplicePtrOp2, inInterface.inWindow);
                        dataLoaded += m_kDataLoadVsize;
                        xRegSplicePtrOp2 = (xRegSplicePtrOp2 + m_kDataLoadsInReg - 1) %
                                           m_kDataLoadsInReg; // down? Think of it as a pointer
                    }
                    if (op % m_kCoeffRegVsize == (m_kCoeffRegVsize - m_kColumns)) {
                        coeff--;
                        coe0 = *coeff;
                    }
                    xstart2 = (xstart2 - m_kColumns) % m_kSamplesInBuff; //(op + m_kDataBuffXOffset+m_kVOutSize);
                    zstart2 = (op % m_kCoeffRegVsize);
                    acc = macSrAsym(acc, sbuff, xstart2, coe0, zstart2);
                }
                // ---------------------------------------------- PREP FOR FORWARD
                // ---------------------------------------------- //

                // Complicated way of saying xRegSplicePtr -= (m_kFirLenCeilCols/m_kColumns) -1
                xRegSplicePtr = xRegSplicePtrOp2;

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                // Write to output window
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++);

                // adjust pointers and counters for forward operation in next iteration (may be claused out for last
                // iteration)
                xRegSplicePtr =
                    (xRegSplicePtr + 1) %
                    m_kDataLoadsInReg; // The first splice to overwrite on the way down is the one just loaded.
                dataLoaded = m_kDataLoadVsize * m_kDataLoadsInReg - m_kDataLoadVsize; // a full buffer.
                dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;
                xstart = (xstart2 + m_kVOutSize) % m_kSamplesInBuff;
                window_incr<TT_DATA>(inInterface.inWindow, m_kSamplesInBuff + m_kDataLoadVsize);
            } // strobe
        }     // Lsize
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Static coefficients
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
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
void fir_sr_asym<TT_DATA,
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
                 TP_USE_COEFF_RELOAD,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 TP_API>::filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outWindow = outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel, Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outWindow = outWindow;
    outInterface.outWindow2 = outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
                 TT_COEFF,
                 TP_FIR_LEN,
                 TP_SHIFT,
                 TP_RND,
                 TP_INPUT_WINDOW_VSIZE,
                 CASC_IN_FALSE,
                 CASC_OUT_FALSE,
                 TP_FIR_RANGE_LEN,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN,
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outWindow = outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
                 TT_COEFF,
                 TP_FIR_LEN,
                 TP_SHIFT,
                 TP_RND,
                 TP_INPUT_WINDOW_VSIZE,
                 CASC_IN_FALSE,
                 CASC_OUT_FALSE,
                 TP_FIR_RANGE_LEN,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN,
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outWindow = outWindow;
    outInterface.outWindow2 = outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_window<TT_DATA>* outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow;
    outInterface.outWindow2 = outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         output_stream_cacc48* outCascade,
                                         output_window<TT_DATA>* broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade,
                                         output_window<TT_DATA>* broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_window<TT_DATA>* outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow;
    outInterface.outWindow2 = outWindow2;
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         output_stream_cacc48* outCascade,
                                         output_window<TT_DATA>* broadcastWindow,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API>::filter(input_window<TT_DATA>* inWindow,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade,
                                         output_window<TT_DATA>* broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = broadcastWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel, Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
                 TT_COEFF,
                 TP_FIR_LEN,
                 TP_SHIFT,
                 TP_RND,
                 TP_INPUT_WINDOW_VSIZE,
                 CASC_IN_FALSE,
                 CASC_OUT_FALSE,
                 TP_FIR_RANGE_LEN,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN,
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         output_stream<TT_DATA>* outStream,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
                 TT_COEFF,
                 TP_FIR_LEN,
                 TP_SHIFT,
                 TP_RND,
                 TP_INPUT_WINDOW_VSIZE,
                 CASC_IN_FALSE,
                 CASC_OUT_FALSE,
                 TP_FIR_RANGE_LEN,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN,
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         output_stream_cacc48* outCascade,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_SINGLE,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};

// ----------------------------------------------------------------------------
// ----------------------------- DUAL STREAM ----------------------------------
// ----------------------------------------------------------------------------

// Single kernel, Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
                 TT_COEFF,
                 TP_FIR_LEN,
                 TP_SHIFT,
                 TP_RND,
                 TP_INPUT_WINDOW_VSIZE,
                 CASC_IN_FALSE,
                 CASC_OUT_FALSE,
                 TP_FIR_RANGE_LEN,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN,
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 1,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_FALSE,
                 2,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 2,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         output_stream_cacc48* outCascade,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void fir_sr_asym<TT_DATA,
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
                 USE_COEFF_RELOAD_TRUE,
                 1,
                 DUAL_IP_DUAL,
                 USE_STREAM_API>::filter(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};
}
}
}
}
}
