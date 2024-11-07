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
Single Rate Asymmetrical FIR kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
//#define _DSPLIB_FIR_SR_ASYM_HPP_DEBUG_

#ifdef __X86SIM__
// #define _DSPLIB_FIR_SR_ASYM_HPP_DEBUG_
#endif
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // broadcast Window buffer through cascade, when required
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>(),
                    TP_API, TP_DUAL_IP, m_kKernelPosEnum>(inInterface, outInterface);

    isUpdateRequired = checkHeaderForUpdate<TP_API, TP_USE_COEFF_RELOAD, TP_CASC_IN, TT_DATA, TP_DUAL_IP>(inInterface);

    if (isUpdateRequired) {
        // read coefficient information from data stream Header and update Taps array, when required
        bufferReloadSSR<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN, TP_DUAL_IP, TP_CASC_IN>(inInterface, m_rawInTaps);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(m_rawInTaps);
    }

    // select architecture and run FIR iteration
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                         const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>(),
                    TP_API, TP_DUAL_IP, m_kKernelPosEnum>(inInterface, outInterface);
    isUpdateRequired = rtpCompare<TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps);
    // cascade output on the last kernel in cascade chain should not send rtp trigger - should only send output.
    if
        constexpr(m_kHasCascOutputForCascadeChain) { sendRtpTrigger(isUpdateRequired, outInterface); }
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(inTaps);
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                            T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>(),
                    TP_API, TP_DUAL_IP, m_kKernelPosEnum>(inInterface, outInterface);
    isUpdateRequired = getRtpTrigger(); // 0 - equal, 1 - not equal
    sendRtpTrigger(isUpdateRequired, outInterface);
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inInterface, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(m_rawInTaps);
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
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
// #undef _DSPLIB_FIR_SR_ASYM_HPP_DEBUG_
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff; // input data value cache.
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded, numDataLoads;
    unsigned int dataNeeded;
    unsigned int dataLoadPhase = 0;

    auto inItr = (TP_API == USE_WINDOW_API && TP_CASC_IN == true && (TP_KERNEL_POSITION != 0))
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc)); // output size can be less than data
                                                                                  // load size, so this iterator needs
                                                                                  // greater precision to reach finer
                                                                                  // addressed.

    constexpr bool hasOutWindow = (TP_API == 0 && TP_CASC_OUT == false);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_CASC_OUT == false);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);
    TT_DATA* frameStart = (TT_DATA*)inInterface.inWindow;
    TT_DATA* __restrict inPointer;
    using outDataVect_t = ::aie::vector<TT_DATA, outVal.getLanes()>;
    outDataVect_t* __restrict outPtr = (outDataVect_t*)outInterface.outWindowPtr;

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    // inItr += TRUNC((m_kFirInitOffset), m_kOffsetResolution);
    inPointer = frameStart + (TRUNC((m_kFirInitOffset), m_kOffsetResolution));

    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);

            coe0 = *coeff;
            coeff++;

            numDataLoads = 0;
            dataLoaded = 0;

            // Preamble, calculate and load data from window into register
            dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;

#pragma unroll(m_kInitialLoads)
            for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
                upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads, inPointer);
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
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inPointer);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                acc = macSrAsym(acc, sbuff, (op + m_kDataBuffXOffset), coe0, (op % m_kCoeffRegVsize));
            }

            // inItr -= m_kDataLoadVsize * numDataLoads - m_kVOutSize;
            inPointer -= m_kDataLoadVsize * numDataLoads - m_kVOutSize;

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

            outVal = shiftAndSaturate(acc, TP_SHIFT);
            // Write to output window
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2, outPtr);
            if
                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                    writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
                                                                           outItr2);
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2, coe3;                     // register for coeff values.
    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer
    T_buff_1024b<TT_DATA> sbuff = *ptr_delay; // initialize data register with data allocated on heap which will contain
                                              // previous iterations margin samples.
    T_acc384<TT_DATA, TT_COEFF> acc;
    T_outVal384<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded;
    unsigned int dataNeeded;
    unsigned int dataLoadPhase = 0;
    unsigned int dataStorePhase = 0;
    int dataOffset = 0;
    int numDataLoads = 0;

    static_assert((TP_INPUT_WINDOW_VSIZE % m_kVOutSize == 0) && (TP_INPUT_WINDOW_VSIZE >= m_kVOutSize),
                  "ERROR: WindowSize is not a multiple of lanes.");
    static_assert(
        ((m_kLsize / streamRptFactor) > 0),
        "ERROR: Window Size is too small, needs to be a multiple of the number of samples in a 1024b Buffer.");

    int loopSize = (m_kLsize / streamRptFactor);
    static_assert(((m_kLsize % streamRptFactor) == 0),
                  "ERROR: Inner loop unrolling not possible for given parameters. Modify Window Size.");
    int startDataLoads = marginLoadsMappedToBuff + streamInitAccs * m_kVOutSize / m_kDataLoadVsize;
    int startDataOffset = (streamDataOffsetWithinBuff) % m_kSamplesInBuff;

    coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
    coe0 = *coeff++;
    // Load more pre-loop and hold each coe
    if (m_kFirLenCeilCols > m_kCoeffRegVsize) {
        coe1 = *coeff++;
    }
    if (m_kFirLenCeilCols > 2 * m_kCoeffRegVsize) {
        coe2 = *coeff++;
    }
    if (m_kFirLenCeilCols > 3 * m_kCoeffRegVsize) {
        coe3 = *coeff;
    }

    // Init pre-loop to deal with m_kFirInitOffset. Only generate for cascaded designs
    if (doInit == 1) {
        dataLoaded = 0;
        dataNeeded = m_kVOutSize;

        for (unsigned i = 0; i < streamInitNullAccs; i++)
            chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                acc = readCascade(inInterface, acc);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                if
                    constexpr(TP_CASC_LEN == 1) {
                        outVal = shiftAndSaturate(acc, TP_SHIFT);
                        // writeOutput<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                        // dataStorePhase%2,0);
                        writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal,
                                                                       dataStorePhase % 2); // guaranteed stream here
                        dataStorePhase++;
                    }
            }

#pragma unroll(GUARD_ZERO(streamInitAccs))
        for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
            int xoffset = (streamInitNullAccs + strobe) * m_kVOutSize + streamDataOffsetWithinBuff;

            if (dataNeeded > dataLoaded) {
                if (m_kStreamReadWidth == 256) {
                    readStream256(
                        sbuff, (marginLoadsMappedToBuff + strobe * m_kVOutSize / m_kDataLoadVsize) % m_kDataLoadsInReg,
                        inInterface);
                } else {
                    readStream128(
                        sbuff, (marginLoadsMappedToBuff + strobe * m_kVOutSize / m_kDataLoadVsize) % m_kDataLoadsInReg,
                        inInterface, dataLoadPhase % 2);
                }
                dataLoaded += m_kDataLoadVsize;
            }
            dataNeeded += m_kVOutSize;

            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
            coe0 = *coeff++;

            // Read cascade input. Do nothing if cascade input not present.
            acc = readCascade(inInterface, acc);
            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, xoffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                acc = macSrAsym(acc, sbuff, xoffset + op, coe0, (op % m_kCoeffRegVsize));
            }
            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
            if
                constexpr(TP_CASC_LEN == 1) {
                    outVal = shiftAndSaturate(acc, TP_SHIFT);
                    // writeOutput<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataStorePhase%2,0);
                    writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal,
                                                                   dataStorePhase % 2); // guaranteed stream here
                    dataStorePhase++;
                }
            dataLoadPhase++;
        }
        loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
    }
    //}

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = startDataLoads;
            dataOffset = startDataOffset;
            dataLoaded = 0;
            dataNeeded = m_kVOutSize;
            dataLoadPhase = streamInitAccs;
            if
                constexpr(TP_CASC_LEN == 1 && streamInitNullAccs != 0) {
                    dataStorePhase = streamInitAccs + streamInitNullAccs;
                }
            else {
                dataStorePhase = streamInitAccs;
            }
// unroll streamRptFactor times
#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
                if (dataNeeded > dataLoaded) {
                    // read only 128-bits for cint32/cint32 or 256-bits every odd loop
                    if (m_kStreamReadWidth == 256) {
                        readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                    } else {
                        // readStream128(sbuff,numDataLoads% m_kDataLoadsInReg, inInterface, dataLoadPhase%2);
                        if (dataLoadPhase % 2 == 0) {
                            readStream128(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface, 0);
                        } else {
                            readStream128(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface, 1);
                        }
                    }
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }

                dataNeeded += m_kVOutSize;

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, strobe * m_kVOutSize + dataOffset, coe0,
                                                       0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    acc =
                        macSrAsym(acc, sbuff, strobe * m_kVOutSize + (op + dataOffset), coe0, (op % m_kCoeffRegVsize));
                }

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                outVal = shiftAndSaturate(acc, TP_SHIFT);
                // Write to output
                // writeOutput<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataStorePhase%2,0);
                writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal,
                                                               dataStorePhase % 2); // guaranteed stream here
                dataStorePhase++;
                dataLoadPhase++;
            }
        }

    doInit = 0;

    // store sbuff for next iteration
    *ptr_delay = sbuff;
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                       TP_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filterIncLoads(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                               T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;            // input data value cache.
    T_acc<TT_DATA, TT_COEFF> acc;
    T_outVal<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded, dataNeeded, numDataLoads;
    unsigned int dataLoadPhase = 0;

    auto inItr = (TP_API == USE_WINDOW_API && TP_CASC_IN == true && (TP_KERNEL_POSITION != 0))
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    constexpr bool hasOutWindow = (TP_API == 0 && TP_CASC_OUT == false);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_CASC_OUT == false);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);
    TT_DATA* frameStart = (TT_DATA*)inInterface.inWindow;
    TT_DATA* __restrict inPointer;
    using outDataVect_t = ::aie::vector<TT_DATA, outVal.getLanes()>;
    outDataVect_t* __restrict outPtr = (outDataVect_t*)outInterface.outWindowPtr;

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    // inItr += (TRUNC((m_kFirInitOffset), m_kOffsetResolution));
    inPointer = frameStart + (TRUNC((m_kFirInitOffset), m_kOffsetResolution));
    int numLoads = 0;
#pragma unroll GUARD_ZERO(m_kInitialLoads - m_kPerLoopLoads)
    for (int initLoads = 0; initLoads < m_kInitialLoads - m_kPerLoopLoads; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads % m_kDataLoadsInReg, inPointer);
        numLoads++;
    }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / m_kIncLoadsRptFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kIncLoadsRptFactor, ) {
            numDataLoads = 0;
            dataLoaded = 0;
            dataNeeded = 0;
#pragma unroll(m_kIncLoadsRptFactor)
            for (unsigned strobe = 0; strobe < m_kIncLoadsRptFactor; strobe++) {
                if (dataNeeded >= dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(
                        sbuff, ((m_kInitialLoads - m_kPerLoopLoads) + numDataLoads) % m_kDataLoadsInReg, inPointer);
                    numDataLoads++;
                    dataLoaded += m_kDataLoadVsize;
                    numLoads++;

                    if
                        constexpr(m_kLanes > m_kDataLoadVsize) {
                            upd_win_incr_256b<TT_DATA>(
                                sbuff, ((m_kInitialLoads - m_kPerLoopLoads) + numDataLoads) % m_kDataLoadsInReg,
                                inPointer);
                            numDataLoads++;
                            dataLoaded += m_kDataLoadVsize;
                            numLoads++;
                        }
                }

                dataNeeded += m_kVOutSize;

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff++;

                acc = readCascade(inInterface, acc);
                acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff,
                                                       strobe * m_kVOutSize + m_kDataBuffXOffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    acc = macSrAsym(acc, sbuff, strobe * m_kVOutSize + (op + m_kDataBuffXOffset), coe0,
                                    (op % m_kCoeffRegVsize));
                }
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturate(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
                                                                       outPtr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                               dataLoadPhase++ % 2, outItr2);
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
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
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterZigZag(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
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

    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    // window_incr(inInterface.inWindow,(TRUNC((m_kFirInitOffset),(16/sizeof(TT_DATA)))));
    inItr += (TRUNC((m_kFirInitOffset), m_kOffsetResolution));

#pragma unroll(m_kInitialLoads)
    for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads, inItr);
    }

    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) // DIV 2
        chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
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
                        upd_win_incr_256b<TT_DATA>(sbuff, xRegSplicePtrOp1, inItr);
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
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
                                                                       outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                               dataLoadPhase++ % 2, outItr2);

                // ---------------------------------------------- PREP FOR REVERSE
                // ---------------------------------------------- //
                dataNeeded += m_kVOutSize;
                // top up with 1 load (potentially)
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, xRegSplicePtr, inItr);
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

                // window_decr<TT_DATA>(inInterface.inWindow,m_kSamplesInBuff+m_kDataLoadVsize);
                inItr -= (m_kSamplesInBuff + m_kDataLoadVsize);

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
                        upd_win_decr_256b<TT_DATA>(sbuff, xRegSplicePtrOp2, inItr);
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
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
                                                                       outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                               dataLoadPhase++ % 2, outItr2);

                // adjust pointers and counters for forward operation in next iteration (may be claused out for last
                // iteration)
                xRegSplicePtr =
                    (xRegSplicePtr + 1) %
                    m_kDataLoadsInReg; // The first splice to overwrite on the way down is the one just loaded.
                dataLoaded = m_kDataLoadVsize * m_kDataLoadsInReg - m_kDataLoadVsize; // a full buffer.
                dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;
                xstart = (xstart2 + m_kVOutSize) % m_kSamplesInBuff;
                // window_incr<TT_DATA>(inInterface.inWindow,m_kSamplesInBuff+m_kDataLoadVsize);
                inItr += (m_kSamplesInBuff + m_kDataLoadVsize);
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
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 TP_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filter(input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    constexpr int kdummy = 16;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel, Static coefficients, single output
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterSingleKernelSingleOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterSingleKernelDualOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                    input_stream_cacc48* inCascade,
                                                    output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                  input_stream_cacc48* inCascade,
                                                  output_circular_buffer<TT_DATA>& __restrict outWindow,
                                                  output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernel(input_circular_buffer<
                          TT_DATA,
                          extents<inherited_extent>,
                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
                      output_stream_cacc48* outCascade,
                      output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernelWithoutBroadcast(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernel(input_async_buffer<TT_DATA>& __restrict inWindow,
                                             input_stream_cacc48* inCascade,
                                             output_stream_cacc48* outCascade,
                                             output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelNoBdcst(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                    input_stream_cacc48* inCascade,
                                                    output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelWithoutBroadcast(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                             input_stream_cacc48* inCascade,
                                                             output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterLastChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterMidChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterMidChainFirstSSRKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
}

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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_FALSE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
}

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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterSingleKernelSingleOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterSingleKernelDualOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                    input_stream_cacc48* inCascade,
                                                    output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                  input_stream_cacc48* inCascade,
                                                  output_circular_buffer<TT_DATA>& __restrict outWindow,
                                                  output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernel(input_circular_buffer<
                          TT_DATA,
                          extents<inherited_extent>,
                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
                      output_stream_cacc48* outCascade,
                      output_async_buffer<TT_DATA>& __restrict broadcastWindow,
                      const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernelWithoutBroadcast(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernel(input_async_buffer<TT_DATA>& __restrict inWindow,
                                             input_stream_cacc48* inCascade,
                                             output_stream_cacc48* outCascade,
                                             output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernelRtp(inInterface, outInterface);
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
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelWithoutBroadcast(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                             input_stream_cacc48* inCascade,
                                                             output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};

// specialisations for io buffer ssr

// Reloadable coefficients
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterLastChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterMidChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernelRtp(inInterface, outInterface);
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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterMidChainFirstSSRKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
}

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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelNoBdcst(input_async_buffer<TT_DATA>& __restrict inWindow,
                                                    input_stream_cacc48* inCascade,
                                                    output_stream_cacc48* outCascade) {
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
}

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
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 DUAL_IP_SINGLE,
                 USE_WINDOW_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::
    filterFirstKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<kdummy> >*)&inWindow;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
}
// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// 1. Single kernel cases
// 1.1 Static coefficients

// 1.1.1 Single Kernel, Static Coefficients, Single Input. Dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                                             output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// 1.1.2 Single Kernel, Static Coefficients, SINGLE input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                                           output_stream<TT_DATA>* outStream,
                                                           output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

// 1.1.3 Single kernel, Static coefficients, Dual Input, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                                           input_stream<TT_DATA>* inStream2,
                                                           output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// 1.1.4 Single kernel, Static coefficients, Dual Input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
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

// 1.2 Reloadable coefficients

// 1.2.1 Single Kernel, Reloadable coefficients, single input, single output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                                             output_stream<TT_DATA>* outStream,
                                                             const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 1.2.2 Single kernel, Reloadable coefficients, single input, dual output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                                           output_stream<TT_DATA>* outStream,
                                                           output_stream<TT_DATA>* outStream2,
                                                           const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 1.2.3 Single kernel, Reloadable coefficients, dual input single output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                                           input_stream<TT_DATA>* inStream2,
                                                           output_stream<TT_DATA>* outStream,
                                                           const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 1.2.4 Single kernel, Reloadable coefficients, dual input dual output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterSingleKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
                                                         input_stream<TT_DATA>* inStream2,
                                                         output_stream<TT_DATA>* outStream,
                                                         output_stream<TT_DATA>* outStream2,
                                                         const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 2. Multiple kernels
// 2.1 Static coefficients

// 2.1.1 Final kernel, Static coefficients, Single Input, Single Output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                                            input_stream_cacc48* inCascade,
                                                            output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// 2.1.2 Final kernel, Static coefficients, single input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
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

// 2.1.3 Final kernel, Static coefficients, Dual input, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                                          input_stream<TT_DATA>* inStream2,
                                                          input_stream_cacc48* inCascade,
                                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

// 2.1.4 Final kernel, Static coefficients, dual input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for the first kernel in a cascade chain.
// 2.1.5 First kernel, Static coefficients, Single Input, dont care output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFirstKernelSingleIP(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// 2.1.6 First kernel, Static coefficients, Dual Input, dont care output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFirstKernelDualIP(input_stream<TT_DATA>* inStream,
                                                  input_stream<TT_DATA>* inStream2,
                                                  output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// 2.1.7 Middle Kernel, Static coefficients, Single Input, Dont care output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelSingleIP(input_stream<TT_DATA>* inStream,
                                                     input_stream_cacc48* inCascade,
                                                     output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// 2.1.8 Middle kernel, Static coefficients, Dual Input, Dont care output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelDualIP(input_stream<TT_DATA>* inStream,
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

// 2.2 Multiple Kernels, Reloadable coefficients

// 2.2.1 Final kernel, Reloadable coefficients, Single input, single output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
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
// 2.2.2 Final kernel, Reloadable coefficients, single input, dual output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
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

// 2.2.3 Final kernel. Reloadable coefficients, dual input, single output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                                          input_stream<TT_DATA>* inStream2,
                                                          input_stream_cacc48* inCascade,
                                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

// 2.2.4 Final kernel. Reloadable coefficients, dual input, dual output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFinalKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
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

// 2.2.5 First kernel, Reloadable coefficients, Single Input, Dont care output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFirstKernelSingleIP(input_stream<TT_DATA>* inStream,
                                                    output_stream_cacc48* outCascade,
                                                    const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 2.2.6 First kernel, Reloadable coefficients, Dual Input, Dont care output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterFirstKernelDualIP(input_stream<TT_DATA>* inStream,
                                                  input_stream<TT_DATA>* inStream2,
                                                  output_stream_cacc48* outCascade,
                                                  const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// 2.2.7 Middle kernel, Reloadable coefficients, single input, dont care output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelSingleIP(input_stream<TT_DATA>* inStream,
                                                     input_stream_cacc48* inCascade,
                                                     output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};

// 2.2.8 Middle kernel Reloadable coefficients, Dual input, dont care output
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
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
                 USE_COEFF_RELOAD_TRUE,
                 TP_NUM_OUTPUTS,
                 TP_DUAL_IP,
                 USE_STREAM_API,
                 TP_MODIFY_MARGIN_OFFSET,
                 TP_COEFF_PHASE,
                 TP_COEFF_PHASE_OFFSET,
                 TP_COEFF_PHASES,
                 TP_COEFF_PHASES_LEN,
                 TP_SAT>::filterMiddleKernelDualIP(input_stream<TT_DATA>* inStream,
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
