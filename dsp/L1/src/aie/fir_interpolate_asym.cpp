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
This file holds the body of the kernel class for the Asymmetric Interpolation FIR.
Unlike single rate implementations, this interpolation FIR calculates sets of output
vectors in parallel such that the number of lanes in total is a multiple of the
interpolation factor.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#ifdef __X86SIM__
//#define _DSPLIB_FIR_INTERPOLATE_ASYM_HPP_DEBUG_
#endif

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include "kernel_api_utils.hpp"

#include "fir_interpolate_asym.hpp"
#include "fir_interpolate_asym_utils.hpp"
#include "fir_sr_asym_utils.hpp"

#ifndef Y_BUFFER
#define Y_BUFFER yd
#endif

#ifndef ACC_BUFFER
#define ACC_BUFFER bm0
#endif

#ifndef OUT_BUFFER
#define OUT_BUFFER wr0
#endif

#ifndef OUT_BUFFER_SHORT
#define OUT_BUFFER_SHORT vrh0
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {
// What follows are templatized functions to precompute values used by the run-time filter
// function so as to minimise run-time computation and instead look-up precomputed values.
// Specifically the xoffsets arguments to the MAC intrinsic central to this algorithm are calculated here.
template <unsigned TP_INTERPOLATE_FACTOR, typename TT_DATA, typename TT_COEFF, int TP_MODIFY_COEFF_OFFSET = 0>
INLINE_DECL constexpr std::array<int32, TP_INTERPOLATE_FACTOR> fnInitStarts() {
    constexpr unsigned int m_kXbuffSize = 128; // kXbuffSize in Bytes (1024bit) - const for all data/coeff types
    constexpr unsigned int m_kDataRegVsize = m_kXbuffSize / sizeof(TT_DATA); // sbuff size in Bytes
    constexpr unsigned int m_kLanes = fnNumLanesIntAsym<TT_DATA, TT_COEFF, 0>();
    std::array<int32, TP_INTERPOLATE_FACTOR> ret = {};
    for (int phase = 0; phase < TP_INTERPOLATE_FACTOR; ++phase) {
        int effPhase = (phase * m_kLanes + TP_MODIFY_COEFF_OFFSET); // the coefficient phase for the given lane changes
                                                                    // by TP_MODIFY_COEFF_OFFSET due to SSR.
        int coeffPhase = FLOOR(effPhase, TP_INTERPOLATE_FACTOR);
        ret[phase] =
            coeffPhase > 0
                ? (coeffPhase % m_kDataRegVsize)
                : -1 * (-1 * coeffPhase % m_kDataRegVsize); // expecting ret[phase] to have same sign as coeffPhase
    }
    return ret;
};

template <unsigned TP_INTERPOLATE_FACTOR, typename TT_DATA, typename TT_COEFF, int TP_MODIFY_COEFF_OFFSET = 0>
INLINE_DECL constexpr std::array<uint64, TP_INTERPOLATE_FACTOR> fnInitOffsets() {
    std::array<uint64, TP_INTERPOLATE_FACTOR> ret = {0};
#if __HAS_ACCUM_PERMUTES__ == 1
    constexpr unsigned int m_kLanes =
        fnNumLanesIntAsym<TT_DATA, TT_COEFF, 0>(); // kMaxMacs/(sizeof(TT_DATA)*sizeof(TT_COEFF)*m_kColumns); //number
                                                   // of operations in parallel of this type combinations that the
                                                   // vector processor can do.
    static_assert(m_kLanes <= 8, "ERROR: Unsupported data type. Exceeding 32-bit offset range.");
    constexpr unsigned int bitsInNibble = 4;
    constexpr unsigned int m_kXbuffSize = 128; // kXbuffSize in Bytes (1024bit) - const for all data/coeff types
    constexpr unsigned int m_kDataRegVsize = m_kXbuffSize / sizeof(TT_DATA); // sbuff size in Bytes
    uint64 dataEntry = 0;
    for (int phase = 0; phase < TP_INTERPOLATE_FACTOR; ++phase) {
        dataEntry = 0;
        int effPhase = (phase * m_kLanes + TP_MODIFY_COEFF_OFFSET); // the coefficient phase for the given lane changes
                                                                    // by TP_MODIFY_COEFF_OFFSET due to SSR.
        int coeffPhase = FLOOR(effPhase, TP_INTERPOLATE_FACTOR);
        int initStartForPhase =
            coeffPhase > 0
                ? (coeffPhase % m_kDataRegVsize)
                : -1 * (-1 * coeffPhase % m_kDataRegVsize); // expecting ret[phase] to have same sign as coeffPhase
        for (int lane = 0; lane < m_kLanes; ++lane) {
            effPhase = (phase * m_kLanes + lane + TP_MODIFY_COEFF_OFFSET);
            coeffPhase = FLOOR(effPhase, TP_INTERPOLATE_FACTOR);
            int initOffset = coeffPhase > 0 ? (coeffPhase % m_kDataRegVsize) : -1 * (-1 * coeffPhase % m_kDataRegVsize);
            dataEntry += (initOffset - initStartForPhase) << (lane * bitsInNibble);
        }
        // Note that m_dataOffsets does not vary by op (m_dataStarts does), but code is left this way to show derivation
        // of values
        ret[phase] = dataEntry;
    }
#endif
    return ret;
};

// FIR function
//-----------------------------------------------------------------------------------------------------
//#TEMPLATE_FUNCTION_DEFINITION
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>(),
                    TP_API>(inInterface, outInterface);
    filterSelectArch(inInterface, outInterface);
}
// FIR function
//-----------------------------------------------------------------------------------------------------
//#TEMPLATE_FUNCTION_DEFINITION
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                         const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>(),
                    TP_API>(inInterface, outInterface);
    isUpdateRequired = rtpCompare<TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps);

    sendRtpTrigger(isUpdateRequired, outInterface);
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(inTaps);
    }
    filterSelectArch(inInterface, outInterface);
}

// Asymmetric Fractional Interpolation FIR Kernel Function - overloaded (not specialised)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                            T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>(),
                    TP_API>(inInterface, outInterface);
    isUpdateRequired = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(isUpdateRequired, outInterface);
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inInterface, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(m_rawInTaps);
    }
    filterSelectArch(inInterface, outInterface);
}

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // This function hides exposure of the implementation choice from the user.
    if
        constexpr(m_kArch == kArchStreamPhaseSeries) { filterStream(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchPhaseSeries) { filterPhaseSeries(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchPhaseParallel) { filterPhaseParallel(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchStreamPhaseParallel) { filterStreamPhaseParallel(inInterface, outInterface); }
    else {
        filterIncr(inInterface, outInterface);
    }
}
// #undef _DSPLIB_FIR_INTERPOLATE_ASYM_HPP_DEBUG_

// Implementation 1, Here, each of the phases is calculated in series to avoid pulling and pushing
// the accumulator to the stack.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
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

    TT_COEFF* coeff = (TT_COEFF*)m_internalTaps;

    T_buff_256b<TT_COEFF> coe0;                                       // register for coeff values.
    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer

    T_buff_1024b<TT_DATA> sbuff = *ptr_delay; // cache register for data values entering MAC
    using buf_type = typename T_buff_1024b<TT_DATA>::v_type;
#if (__HAS_ACCUM_PERMUTES__ == 1)
    // When device __HAS_ACCUM_PERMUTES__ fixed storage placement can be used to ease mapping
    buf_type chess_storage(Y_BUFFER) sbuffTmp;
    using outTypeInt = typename T_outValIntAsym<TT_DATA, TT_COEFF>::v_type chess_storage(OUT_BUFFER);
    using outTypeIntShort = typename T_outValIntAsym<TT_DATA, TT_COEFF>::v_type chess_storage(OUT_BUFFER_SHORT);
#else
    buf_type sbuffTmp;
    using outTypeInt = typename T_outValIntAsym<TT_DATA, TT_COEFF>::v_type;
    using outTypeIntShort = typename T_outValIntAsym<TT_DATA, TT_COEFF>::v_type;
#endif
    using outType =
        typename std::conditional<(std::is_same<TT_DATA, cint32>::value) && (std::is_same<TT_COEFF, cint32>::value),
                                  outTypeInt, outTypeIntShort>::type;

    outType outValTmp;

    T_accIntAsym<TT_DATA, TT_COEFF> acc;
    T_outValIntAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData;

    unsigned int index = startIndex;
    int xstart;
    unsigned int LCMPhase = 0;
    unsigned int dataLoaded, dataNeeded; // In terms of register locations, not data samples
    constexpr static std::array<int32, TP_INTERPOLATE_FACTOR> m_dataStarts =
        fnInitStarts<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF, TP_MODIFY_MARGIN_OFFSET>();
    constexpr static std::array<uint64, TP_INTERPOLATE_FACTOR> m_dataOffsets =
        fnInitOffsets<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF, TP_MODIFY_MARGIN_OFFSET>();
    int loopSize = (m_kLsize / m_kRepeatFactor);
    TT_COEFF* m_internalTapsCopy = (TT_COEFF*)m_internalTaps; // points to m_internalTaps[0][0][0][0]
    unsigned int inDataLoadPhase, outDataPhase = 0;

    if
        constexpr(TP_CASC_LEN > 1) {
            if (doInit == 1) {
                dataLoaded = 0;
                LCMPhase = phaseOffset;
                index = 2;
                for (unsigned i = 0; i < streamInitNullAccs; i++)
                    chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                        acc = readCascade(inInterface, acc);

                        writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                    }
#pragma unroll(GUARD_ZERO(CEIL(streamInitNullAccs, m_kPhases) - streamInitNullAccs))
                for (unsigned i = streamInitNullAccs; i < CEIL(streamInitNullAccs, m_kPhases); i++) {
                    m_internalTapsCopy = chess_copy(m_internalTapsCopy);
                    coeff = m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps);
                    coe0 = *(T_buff_256b<TT_COEFF>*)coeff;
                    dataNeeded = ((int)(((m_kLanes * ((i % TP_INTERPOLATE_FACTOR) + 1)) - 1) / TP_INTERPOLATE_FACTOR) +
                                  1 + streamInitNullAccs / m_kPhases * totalNeededDataPerStrobe) -
                                 initPhaseDataAccesses;

                    if (dataNeeded > dataLoaded) {
                        if
                            constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                                readStream128(sbuff, index, inInterface);
                                sbuffTmp = sbuff.val;
                                sbuff.val = sbuffTmp;
                                index++;
                            }
                        else {
                            readStream256(sbuff, index, inInterface);
                            index++;
                        }
                        dataLoaded += sizeOf1Read;
                    }
                    xstart = m_kXStart + streamInitNullAccs / m_kPhases * totalNeededDataPerStrobe +
                             m_dataStarts[i % (TP_INTERPOLATE_FACTOR)];
                    acc = readCascade(inInterface, acc);

                    acc = initMacIntAsym(inInterface, acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes,
                                         m_dataOffsets[LCMPhase], xstart);

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                    for (int op = 1; op < m_kNumOps; ++op) {
                        xstart += m_kColumns;
                        coeff = m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps) +
                                (op * m_kLanes * m_kColumns);
                        coe0 = *(T_buff_256b<TT_COEFF>*)coeff;

                        acc = macIntAsym(acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes, m_dataOffsets[LCMPhase],
                                         xstart);
                    }

                    outValTmp = shiftAndSaturateIntAsym(acc, TP_SHIFT).val;
                    outVal.val = outValTmp;

                    // Write cascade. Do nothing if cascade not present.
                    writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase++ % 2);
                        }
                    LCMPhase = (LCMPhase + 1);
                    if (LCMPhase == m_kLCMPhases) { // a mod function without the division.TODO optimize TP_INTERPOLATE
                                                    // to prime factor
                        LCMPhase = 0;
                    }
                }

#pragma unroll(GUARD_ZERO(streamInitAccs))
                for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
#pragma unroll(m_kPhases)
                    // The phase loop effectively multiplies the number of lanes in use to ensures that
                    // an integer number of interpolation polyphases are calculated
                    for (int phase = 0; phase < m_kPhases; ++phase) {
                        m_internalTapsCopy = chess_copy(m_internalTapsCopy);
                        coeff = (m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps));
                        coe0 = *(T_buff_256b<TT_COEFF>*)coeff;
                        // coe0 = null_buff_1024b<TT_DATA>();
                        dataNeeded = ((strobe + streamInitNullStrobes) * totalNeededDataPerStrobe +
                                      (int)((m_kLanes * (phase + 1) - 1) / TP_INTERPOLATE_FACTOR) + 1) -
                                     initPhaseDataAccesses;

                        if (dataNeeded > dataLoaded) {
                            if
                                constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                                    readStream128(sbuff, index, inInterface);
                                    sbuffTmp = sbuff.val;
                                    sbuff.val = sbuffTmp;
                                    index++;
                                }
                            else {
                                readStream256(sbuff, index, inInterface);
                                index++;
                            }
                            dataLoaded += sizeOf1Read;
                        }

                        xstart = m_kXStart + (strobe + streamInitNullStrobes) * m_kLanes + m_dataStarts[phase];
                        acc = readCascade(inInterface, acc);

                        acc = initMacIntAsym(inInterface, acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes,
                                             m_dataOffsets[LCMPhase], xstart);

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                        for (int op = 1; op < m_kNumOps; ++op) {
                            xstart += m_kColumns;
                            coeff = m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps) +
                                    (op * m_kLanes * m_kColumns);
                            coe0 = *(T_buff_256b<TT_COEFF>*)coeff;

                            acc = macIntAsym(acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes, m_dataOffsets[LCMPhase],
                                             xstart);
                        }

                        outValTmp = shiftAndSaturateIntAsym(acc, TP_SHIFT).val;
                        outVal.val = outValTmp;

                        // Write cascade. Do nothing if cascade not present.
                        writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                        if
                            constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                                writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal,
                                                                               outDataPhase++ % 2);
                            }
                        LCMPhase = (LCMPhase + 1);
                        if (LCMPhase == m_kLCMPhases) { // a mod function without the division.TODO optimize
                                                        // TP_INTERPOLATE to prime factor
                            LCMPhase = 0;
                        }
                    }
                }
                loopSize -= CEIL(streamInitNullAccs, m_kRepFactPhases) / (m_kRepeatFactor * m_kPhases);
            }
        }
    doInit = 0;

    coeff = m_internalTapsCopy;
    // loop through window, computing a vector of output for each iteration.
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4) chess_loop_range(
            (m_kLsize / m_kRepeatFactor) - (CEIL(streamInitNullAccs, m_kRepFactPhases) / (m_kRepeatFactor * m_kPhases)),
            (m_kLsize / m_kRepeatFactor)) {
            dataLoaded = 0;
            LCMPhase = 0;
            if
                constexpr(streamInitNullAccs == 0) { index = startIndex; }
            else {
                index = startIndexCasc;
            }
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
#pragma unroll(m_kPhases)
                // The phase loop effectively multiplies the number of lanes in use to ensures that
                // an integer number of interpolation polyphases are calculated
                for (int phase = 0; phase < m_kPhases; ++phase) {
                    coeff = chess_copy(coeff);
                    coe0 = *(T_buff_256b<TT_COEFF>*)coeff;
                    coeff = coeff + (m_kLanes * m_kColumns);

                    dataNeeded = strobe * ((int)((m_kLanes * (m_kPhases)-1) / TP_INTERPOLATE_FACTOR) + 1) +
                                 (int)((m_kLanes * (phase + 1) - 1) / TP_INTERPOLATE_FACTOR) + 1;

                    if (dataNeeded > dataLoaded) {
                        if
                            constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                                readStream128(sbuff, index, inInterface);
                                sbuffTmp = sbuff.val;
                                sbuff.val = sbuffTmp;
                                index++;
                            }
                        else {
                            readStream256(sbuff, index, inInterface);
                            index++;
                        }
                        dataLoaded += sizeOf1Read;
                    }

                    xstart = m_kXStart + strobe * m_kLanes + m_dataStarts[phase];
                    acc = readCascade(inInterface, acc);

                    acc = initMacIntAsym(inInterface, acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes,
                                         m_dataOffsets[LCMPhase], xstart);

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                    for (int op = 1; op < m_kNumOps; ++op) {
                        xstart += m_kColumns;
                        coe0 = *(T_buff_256b<TT_COEFF>*)coeff;
                        if
                            constexpr(m_kPhases == m_kLCMPhases) {
                                if (op == m_kNumOps - 1) {
                                    if (phase == m_kPhases - 1) {
                                        coeff = coeff -
                                                (m_kPhases * m_kLanes * m_kColumns * m_kNumOps - m_kLanes * m_kColumns);
                                    } else {
                                        coeff = coeff + (m_kLanes * m_kColumns);
                                    }
                                } else {
                                    coeff = coeff + (m_kLanes * m_kColumns);
                                }
                            }
                        else {
                            if (op == m_kNumOps - 1) {
                                if (LCMPhase == m_kLCMPhases - 1) {
                                    coeff = coeff -
                                            (m_kLCMPhases * m_kLanes * m_kColumns * m_kNumOps - m_kLanes * m_kColumns);
                                } else {
                                    coeff = coeff + (m_kLanes * m_kColumns);
                                }
                            } else {
                                coeff = coeff + (m_kLanes * m_kColumns);
                            }
                        }
                        acc = macIntAsym(acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes, m_dataOffsets[LCMPhase],
                                         xstart);
                    }
                    if
                        constexpr(m_kNumOps == 1) {
                            if (LCMPhase == m_kLCMPhases - 1) {
                                coeff = coeff - (m_kLCMPhases * m_kLanes * m_kColumns * m_kNumOps);
                            }
                        }

                    // Write cascade. Do nothing if cascade not present.
                    writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                    outValTmp = shiftAndSaturateIntAsym(acc, TP_SHIFT).val;
                    outVal.val = outValTmp;

                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase++ % 2);
                        }
                    LCMPhase = (LCMPhase + 1);
                    if (LCMPhase == m_kLCMPhases) { // a mod function without the division.TODO optimize TP_INTERPOLATE
                                                    // to prime factor
                        LCMPhase = 0;
                    }
                }
            }
        }
    doInit = 0;
    *ptr_delay = sbuff;
}

// Implementation 1, Here, each of the phases is calculated in series to avoid pulling and pushing
// the accumulator to the stack.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
INLINE_DECL void
kernelFilterClass<TT_DATA,
                  TT_COEFF,
                  TP_FIR_LEN,
                  TP_INTERPOLATE_FACTOR,
                  TP_SHIFT,
                  TP_RND,
                  TP_INPUT_WINDOW_VSIZE,
                  TP_CASC_IN,
                  TP_CASC_OUT,
                  TP_FIR_RANGE_LEN,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN,
                  TP_USE_COEFF_RELOAD,
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_API,
                  TP_MODIFY_MARGIN_OFFSET,
                  TP_COEFF_PHASE,
                  TP_COEFF_PHASE_OFFSET,
                  TP_COEFF_PHASES,
                  TP_COEFF_PHASES_LEN,
                  TP_SAT>::filterStreamPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer

    T_buff_1024b<TT_DATA> sbuff = *ptr_delay; // cache register for data values entering MAC
    constexpr unsigned int kSerialPhases = 1;
    constexpr unsigned int kParallelPhases = m_kPhases;
    constexpr unsigned int kInitialLoads =
        CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) / m_kDataLoadVsize;

    // streaming architecture
    static constexpr int coefRangeStartIndex =
        TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset * TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int kFirRangeCoeffOffset =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_INTERPOLATE_FACTOR>() +
        TP_MODIFY_MARGIN_OFFSET; // FIR Cascade Offset for this kernel position
    static constexpr int streamInitNullAccs =
        ((TP_FIR_LEN - TP_FIR_RANGE_LEN - kFirRangeCoeffOffset) / m_kVOutSize) / kParallelPhases;
    static constexpr int streamInitAccs =
        (CEIL(streamInitNullAccs, m_kRepeatFactor) - CEIL(streamInitNullAccs, kSerialPhases)) / kSerialPhases;

    static constexpr unsigned int add1 =
        CEIL(TP_FIR_RANGE_LEN / TP_INTERPOLATE_FACTOR, m_kColumns) -
        TP_FIR_RANGE_LEN / TP_INTERPOLATE_FACTOR; // used to accomodate for the FIR Range Length mapped to MAC
                                                  // operations ceiled to m_kColumns, i.e. TP_FIR_RANGE_LEN % m_kColumns
                                                  // == m_kColumns - 1 => needs data offset of 1 sample
    static constexpr int sizeOf1Read = m_kDataLoadVsize;

    static constexpr int m_kXStart = startIndex * sizeOf1Read - (m_kColumns * (m_kNumOps - 1) + m_kColumns - 1) + add1 -
                                     coefRangeStartIndex / TP_INTERPOLATE_FACTOR;
    static constexpr int streamInitNullStrobes = CEIL(streamInitNullAccs, kSerialPhases) / kSerialPhases;
    T_outVal384<TT_DATA, TT_COEFF> outVal;

    std::array<T_acc384<TT_DATA, TT_COEFF>, kParallelPhases> acc;
    std::array<T_buff_256b<TT_COEFF>, kParallelPhases> coe;
    std::array<T_outVal384<TT_DATA, TT_COEFF>, kParallelPhases> outArray;
    T_buff_256b<TT_DATA> readData;

    unsigned int index = startIndex;
    int xstart;
    unsigned int kXStart = m_kXStart;
    unsigned int LCMPhase = 0;
    unsigned int dataLoaded, dataNeeded; // In terms of register locations, not data samples
    constexpr static std::array<int32, TP_INTERPOLATE_FACTOR> m_dataStarts =
        fnInitStarts<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF, TP_MODIFY_MARGIN_OFFSET>();
    constexpr static std::array<uint64, TP_INTERPOLATE_FACTOR> m_dataOffsets =
        fnInitOffsets<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF, TP_MODIFY_MARGIN_OFFSET>();
    int loopSize = (m_kLsize / m_kRepeatFactor);
    unsigned int inDataLoadPhase, outDataPhase = 0;

    if
        constexpr(TP_CASC_LEN > 1) {
            if (doInit == 1) {
                dataLoaded = 0;
                index = startIndex * sizeOf1Read / m_kVOutSize; // 256-bit chunk
                for (unsigned i = 0; i < streamInitNullAccs; i++)
                    chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            acc[0] = readCascade(inInterface, acc[0]);

                            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[0]);
                        }
                    }
                dataNeeded = m_kVOutSize;

#pragma unroll(GUARD_ZERO(streamInitAccs))
                for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
#pragma unroll(kSerialPhases)
                    // The phase loop effectively multiplies the number of lanes in use to ensures that
                    // an integer number of interpolation polyphases are calculated
                    for (int phase = 0; phase < kSerialPhases; ++phase) {
#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            T_buff_256b<TT_COEFF>* coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase]);
                            coe[phase] = *coeff;
                        }

                        if (dataNeeded > dataLoaded) {
                            readStream256(sbuff, index, inInterface);
                            index++;
                            dataLoaded += m_kDataLoadVsize;
                        }

                        dataNeeded += m_kVOutSize;

#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            acc[phase] = readCascade<TT_DATA, TT_COEFF>(inInterface, acc[phase]);
                            xstart = kXStart + (strobe + streamInitNullStrobes) * m_kLanes + m_dataStarts[0];
                            acc[phase] = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc[phase], sbuff,
                                                                                   xstart, coe[phase], 0);
                        }

#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                        for (int op = 1; op < m_kNumOps; ++op) {
#pragma unroll(kParallelPhases)
                            for (int phase = 0; phase < kParallelPhases; ++phase) {
                                if ((m_kColumns * op) % m_kCoeffRegVsize == 0) {
                                    chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                                        ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase] +
                                         (m_kColumns * op) / m_kCoeffRegVsize);
                                    coe[phase] = *coeff;
                                }
                                xstart = kXStart + m_dataStarts[0] + m_kColumns * op +
                                         (strobe + streamInitNullStrobes) * m_kLanes;
                                acc[phase] = sr_asym::macSrAsym(acc[phase], sbuff, xstart, coe[phase],
                                                                ((m_kColumns * op) % m_kCoeffRegVsize));
                            }
                        }

#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[phase]);
                        }

#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            outArray[phase] = shiftAndSaturate(acc[phase], TP_SHIFT);
                        }
                        if
                            constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
// #define DONT_INTERLEAVE_POLYPHASES
#ifdef DONT_INTERLEAVE_POLYPHASES
#pragma unroll(kParallelPhases)
                                for (int phase = 0; phase < kParallelPhases; ++phase) {
                                    // Write to output window with no interleaving
                                    writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outArray[phase],
                                                                                   outDataPhase++ % 2);
                                }
#else
                                streamInterleaveIntAsym<TT_DATA, TT_COEFF, kParallelPhases, TP_NUM_OUTPUTS>(
                                    outArray, outInterface);
#endif
                            }
                    }
                }
                loopSize -= CEIL(streamInitNullAccs, m_kRepeatFactor) / (m_kRepeatFactor);
            }
        }
    doInit = 0;

    // loop through window, computing a vector of output for each iteration.
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / m_kRepeatFactor) -
                                 (CEIL(streamInitNullAccs, m_kRepFactPhases) / (m_kRepeatFactor * kSerialPhases)),
                             (m_kLsize / m_kRepeatFactor)) {
            dataLoaded = 0;
            index = (startIndex + streamInitAccs) * sizeOf1Read / m_kVOutSize; // 256-bit chunk
            dataNeeded = m_kVOutSize;
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
#pragma unroll(kSerialPhases)
                // The phase loop effectively multiplies the number of lanes in use to ensures that
                // an integer number of interpolation polyphases are calculated
                for (int phase = 0; phase < kSerialPhases; ++phase) {
#pragma unroll(kParallelPhases)
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                            ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase]);
                        coe[phase] = *coeff;
                    }

                    if (dataNeeded > dataLoaded) {
                        readStream256(sbuff, index, inInterface);
                        index++;
                        dataLoaded += m_kDataLoadVsize;
                    }

                    dataNeeded += m_kVOutSize;

#pragma unroll(kParallelPhases)
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        acc[phase] = readCascade<TT_DATA, TT_COEFF>(inInterface, acc[phase]);
                        xstart = kXStart + strobe * m_kLanes + m_dataStarts[0];
                        acc[phase] = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc[phase], sbuff, xstart,
                                                                               coe[phase], 0);
                    }
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                    for (int op = 1; op < m_kNumOps; ++op) {
#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            if ((m_kColumns * op) % m_kCoeffRegVsize == 0) {
                                T_buff_256b<TT_COEFF>* coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase] +
                                                                (m_kColumns * op) / m_kCoeffRegVsize);
                                coe[phase] = *coeff;
                            }
                            xstart = kXStart + m_dataStarts[0] + m_kColumns * op + strobe * m_kLanes;
                            acc[phase] = sr_asym::macSrAsym(acc[phase], sbuff, xstart, coe[phase],
                                                            ((m_kColumns * op) % m_kCoeffRegVsize));
                        }
                    }

#pragma unroll(kParallelPhases)
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[phase]);
                    }

#pragma unroll(kParallelPhases)
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        outArray[phase] = shiftAndSaturate(acc[phase], TP_SHIFT);
                    }
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
// #define DONT_INTERLEAVE_POLYPHASES
#ifdef DONT_INTERLEAVE_POLYPHASES
#pragma unroll(kParallelPhases)
                            for (int phase = 0; phase < kParallelPhases; ++phase) {
                                // Write to output window with no interleaving
                                writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outArray[phase],
                                                                               outDataPhase++ % 2);
                            }
#else
                            streamInterleaveIntAsym<TT_DATA, TT_COEFF, kParallelPhases, TP_NUM_OUTPUTS>(outArray,
                                                                                                        outInterface);
#endif
                        }
                }
            }
        }
    doInit = 0;
    *ptr_delay = sbuff;
}

// Implementation 1, Here, each of the phases is calculated in series to avoid pulling and pushing
// the accumulator to the stack.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterPhaseSeries(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    TT_COEFF* m_internalTapsCopy = (TT_COEFF*)m_internalTaps; // points to m_internalTaps[0][0][0][0]
    T_buff_256b<TT_COEFF> coe0;                               // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;                              // cache register for data values entering MAC
    T_outValIntAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData;
    unsigned int dataLoaded, dataNeeded; // In terms of register locations, not data samples
    unsigned int numDataLoads;
    unsigned int LCMPhase = 0;
    unsigned int xstart = 0;
    uint64 xoffsets = 0;
    // m_dataOffsets allows offsets to be precalculated in the constructor then simply looked up according to loop
    // indices during runtime.
    constexpr static std::array<int32, TP_INTERPOLATE_FACTOR> m_dataStarts =
        fnInitStarts<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();
    constexpr static std::array<uint64, TP_INTERPOLATE_FACTOR> m_dataOffsets =
        fnInitOffsets<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();
    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    constexpr unsigned int kSerialPhases = m_kPhases;
    constexpr unsigned int kParallelPhases = 1;
    constexpr unsigned int kInitialLoads = m_kInitialLoads;
    T_accIntAsym<TT_DATA, TT_COEFF> acc;
    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding

    // loop through window, computing a vector of output for each iteration.
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
#pragma unroll(kSerialPhases)
            // The phase loop effectively multiplies the number of lanes in use to ensures that
            // an integer number of interpolation polyphases are calculated
            for (int phase = 0; phase < kSerialPhases; ++phase) {
                m_internalTapsCopy = chess_copy(m_internalTapsCopy);
                coeff = (T_buff_256b<TT_COEFF>*)(m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps));
                coe0 = *coeff;

                numDataLoads = 0;
                dataLoaded = 0;

                // preamble, load data from window into register
                dataNeeded = m_kDataBuffXOffset * TP_INTERPOLATE_FACTOR + m_kLanes * (phase + 1) +
                             (m_kColumns - 1) * TP_INTERPOLATE_FACTOR;

// load the data registers with enough data for the initial MAC(MUL)
#pragma unroll(kInitialLoads)
                for (int initLoads = 0; initLoads < kInitialLoads; ++initLoads) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
                    dataLoaded += m_kDataLoadVsize * TP_INTERPOLATE_FACTOR; // in effect, since data is duplicated
                    numDataLoads++;
                }
                xstart = m_kDataBuffXOffset + m_dataStarts[phase];
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                // Perform the first term (or 2 or 4) of the FIR polynomial.
                acc = initMacIntAsym(inInterface, acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes,
                                     m_dataOffsets[LCMPhase], xstart);

// loop to work through the operations in the FIR polynomial
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    dataNeeded += m_kColumns * TP_INTERPOLATE_FACTOR;
                    if (dataNeeded > dataLoaded) {
                        upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);

                        dataLoaded += m_kDataLoadVsize * TP_INTERPOLATE_FACTOR;
                        numDataLoads++;
                    }
                    coeff =
                        (T_buff_256b<TT_COEFF>*)(m_internalTapsCopy + (LCMPhase * m_kLanes * m_kColumns * m_kNumOps) +
                                                 (op * m_kLanes * m_kColumns));
                    coe0 = *coeff;
                    xstart += m_kColumns;
                    // perform the MAC, i.e. cacculate some terms of the FIR polynomial
                    acc =
                        macIntAsym(acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes, m_dataOffsets[LCMPhase], xstart);
                }

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                // convert the accumulator type into an integer type for output, downshift and apply any rounding and
                // saturation.
                outVal = shiftAndSaturateIntAsym(acc, TP_SHIFT);

                // Write to output window
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                        *outItr++ = outVal.val;
                        if
                            constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                                *outItr2++ = outVal.val;
                            }
                    }
                LCMPhase = (LCMPhase + 1);
                if (LCMPhase ==
                    m_kLCMPhases) { // a mod function without the division.TODO optimize TP_INTERPOLATE to prime factor
                    LCMPhase = 0;
                }

                // take data pointer back to next start point.
                inItr -= (m_kDataLoadVsize * numDataLoads); // return read pointer to start of next chunk of window.
            }
            inItr += m_kLanes; // after all phases, one m_kLanes of input will have been consumed.
        }
};

// Implementation 2, Polyphases are operated on in parallel and data is interleaved at the end.
// Requires multiple accumulators, so most efficient for devices that offer at least TP_INTERPOLATE_FACTOR amount of
// accs.
// Interleaving is degrading perfromance somehow.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                                T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    TT_COEFF* m_internalTapsCopy = (TT_COEFF*)m_internalTaps; // points to m_internalTaps[0][0][0][0]
    T_buff_1024b<TT_DATA> sbuff;                              // cache register for data values entering MAC
    T_outValIntAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData;
    unsigned int dataLoaded, dataNeeded; // In terms of register locations, not data samples
    unsigned int numDataLoads;
    unsigned int LCMPhase = 0;
    unsigned int xstart = 0;
    uint64 xoffsets = 0;
    // m_dataOffsets allows offsets to be precalculated in the constructor then simply looked up according to loop
    // indices during runtime.
    constexpr static std::array<int32, TP_INTERPOLATE_FACTOR> m_dataStarts =
        fnInitStarts<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();
    constexpr static std::array<uint64, TP_INTERPOLATE_FACTOR> m_dataOffsets =
        fnInitOffsets<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();
    constexpr unsigned int kSerialPhases = 1;
    constexpr unsigned int kParallelPhases = m_kPhases;
    constexpr unsigned int kInitialLoads =
        CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) / m_kDataLoadVsize;
    std::array<T_accIntAsym<TT_DATA, TT_COEFF>, kParallelPhases> acc;
    std::array<T_buff_256b<TT_COEFF>, kParallelPhases> coe;
    std::array<T_outVal<TT_DATA, TT_COEFF>, kParallelPhases> outArray;
    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding

    // loop through window, computing a vector of output for each iteration.
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
#pragma unroll(kSerialPhases)
            // The phase loop effectively multiplies the number of lanes in use to ensures that
            // an integer number of interpolation polyphases are calculated
            for (int phase = 0; phase < kSerialPhases; ++phase) {
#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                        ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase]);
                    coe[phase] = *coeff;
                }

                numDataLoads = 0;
                dataLoaded = 0;

                // preamble, load data from window into register
                dataNeeded = m_kDataBuffXOffset * TP_INTERPOLATE_FACTOR +
                             m_kLanes * (phase + 1) * TP_INTERPOLATE_FACTOR + (m_kColumns - 1) * TP_INTERPOLATE_FACTOR;

// load the data registers with enough data for the initial MAC(MUL)
#pragma unroll(kInitialLoads)
                for (int initLoads = 0; initLoads < kInitialLoads; ++initLoads) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
                    dataLoaded += m_kDataLoadVsize * TP_INTERPOLATE_FACTOR; // in effect, since data is duplicated
                    numDataLoads++;
                }
#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    acc[phase] = readCascade<TT_DATA, TT_COEFF>(inInterface, acc[phase]);
                    xstart = m_kDataBuffXOffset + m_dataStarts[0];
                    acc[phase] = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc[phase], sbuff, xstart,
                                                                           coe[phase], 0);
                }

// loop to work through the operations in the FIR polynomial
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                for (int op = 1; op < m_kNumOps; ++op) {
                    dataNeeded += m_kColumns * TP_INTERPOLATE_FACTOR;
                    if (dataNeeded > dataLoaded) {
                        upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);

                        dataLoaded += m_kDataLoadVsize * TP_INTERPOLATE_FACTOR;
                        numDataLoads++;
                    }
#pragma unroll(kParallelPhases)
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        if ((m_kColumns * op) % m_kCoeffRegVsize == 0) {
                            chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                                ((T_buff_256b<TT_COEFF>*)m_internalTaps2[phase] + (m_kColumns * op) / m_kCoeffRegVsize);
                            coe[phase] = *coeff;
                        }
                        xstart = m_kDataBuffXOffset + m_dataStarts[0] + m_kColumns * op;
                        acc[phase] = sr_asym::macSrAsym(acc[phase], sbuff, xstart, coe[phase],
                                                        ((m_kColumns * op) % m_kCoeffRegVsize));
                    }
                }

#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[phase]);
                }

#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    outArray[phase] = shiftAndSaturateIntAsym(acc[phase], TP_SHIFT);
                }
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
// #define DONT_INTERLEAVE_POLYPHASES
#ifdef DONT_INTERLEAVE_POLYPHASES
#pragma unroll(kParallelPhases)
                        for (int phase = 0; phase < kParallelPhases; ++phase) {
                            // Write to output window with no interleaving
                            *outItr++ = outArray[phase].val;
                        }
#else
                        bufferInterleaveIntAsym<TT_DATA, TT_COEFF, kParallelPhases, TP_NUM_OUTPUTS>(outArray, outItr,
                                                                                                    outItr2);
#endif
                    }

                LCMPhase = (LCMPhase + 1);
                if (LCMPhase ==
                    m_kLCMPhases) { // a mod function without the division.TODO optimize TP_INTERPOLATE to prime factor
                    LCMPhase = 0;
                }

                // take data pointer back to next start point.
                inItr -= (m_kDataLoadVsize * numDataLoads); // return read pointer to start of next chunk of window.
            }
            inItr += m_kLanes; // after all phases, one m_kLanes of input will have been consumed.
        }
};

// Implementation 1, Here, each of the phases is calculated in series to avoid pulling and pushing
// the accumulator to the stack.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
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
                                   TP_INTERPOLATE_FACTOR,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_DUAL_IP,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterIncr(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                       T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff; // cache register for data values entering MAC
    T_accIntAsym<TT_DATA, TT_COEFF> acc;
    T_outValIntAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData;
    unsigned int dataLoaded, dataNeeded; // In terms of register locations, not data samples
    unsigned int numDataLoads;
    unsigned int LCMPhase = 0;
    unsigned int xstart = 0;
    uint64 xoffsets = 0;

    // m_dataOffsets allows offsets to be precalculated in the constructor then simply looked up according to loop
    // indices during runtime.
    constexpr static std::array<int32, TP_INTERPOLATE_FACTOR> m_dataStarts =
        fnInitStarts<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();
    constexpr static std::array<uint64, TP_INTERPOLATE_FACTOR> m_dataOffsets =
        fnInitOffsets<TP_INTERPOLATE_FACTOR, TT_DATA, TT_COEFF>();

    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    inItr += m_kDataWindowOffset; // move input data pointer past the margin padding

// preamble, load data from window into register
// load the data registers with enough data for the initial MAC(MUL)
#pragma unroll(m_kInitialLoadsIncr - 1)
    for (int initLoads = 0; initLoads < m_kInitialLoadsIncr - 1; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads % m_kDataLoadsInReg, inItr);
    }

    // loop through window, computing a vector of output for each iteration.
    for (unsigned i = 0; i < m_kLsize / m_kRepeatFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kRepeatFactor, ) {
            dataLoaded = 0;
            dataNeeded = m_kVOutSize;

            numDataLoads = m_kInitialLoadsIncr - 1;
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                dataNeeded += m_kLanes;
#pragma unroll(m_kPhases)
                // The phase loop effectively multiplies the number of lanes in use to ensures that
                // an integer number of interpolation polyphases are calculated
                for (int phase = 0; phase < m_kPhases; ++phase) {
                    coeff = ((T_buff_256b<TT_COEFF>*)&m_internalTaps[LCMPhase][0][0][0]);
                    coe0 = *coeff;
                    xstart = m_kDataBuffXOffset + m_dataStarts[phase] + strobe * m_kLanes;

                    // Read cascade input. Do nothing if cascade input not present.
                    acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                    // Perform the first term (or 2 or 4) of the FIR polynomial.
                    acc = initMacIntAsym(inInterface, acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes,
                                         m_dataOffsets[LCMPhase], xstart);

// loop to work through the operations in the FIR polynomial
#pragma unroll(GUARD_ZERO((m_kNumOps - 1)))
                    for (int op = 1; op < m_kNumOps; ++op) {
                        coeff = ((T_buff_256b<TT_COEFF>*)&m_internalTaps[LCMPhase][op][0][0]);
                        coe0 = *coeff;
                        xstart += m_kColumns;
                        // perform the MAC, i.e. cacculate some terms of the FIR polynomial
                        acc = macIntAsym(acc, sbuff, coe0, TP_INTERPOLATE_FACTOR, m_kLanes, m_dataOffsets[LCMPhase],
                                         xstart);
                    }

                    // Write cascade. Do nothing if cascade not present.
                    writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                    // convert the accumulator type into an integer type for output, downshift and apply any rounding
                    // and saturation.
                    outVal = shiftAndSaturateIntAsym(acc, TP_SHIFT);
                    // Write to output window
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            *outItr++ = outVal.val;
                            if
                                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                                    *outItr2++ = outVal.val;
                                }
                        }

                    LCMPhase = (LCMPhase + 1);
                    if (LCMPhase == m_kLCMPhases) { // a mod function without the division.TODO optimize TP_INTERPOLATE
                                                    // to prime factor
                        LCMPhase = 0;
                    }
                }
            } // strobe loop
        }     // i loop (Lsize)
};

// FIR filter function overloaded with cascade interface variations
// Standalone kernel specialization. Windowed. no cascade ports, no reload, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          TP_CASC_IN,
                          TP_CASC_OUT,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          TP_USE_COEFF_RELOAD,
                          TP_DUAL_IP,
                          TP_NUM_OUTPUTS,
                          TP_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. Windowed. No cascade ports, static coefficients. dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. Windowed. No cascade ports, with reload coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Single kernel specialization. No cascade ports, Windowed. with reload coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow,
                                          output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow,
           const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_WINDOW_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade,
                                          output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////  STREAM
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  Single Kernel. Static Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

//  Single Kernel. Static Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          output_stream<TT_DATA>* outStream,
                                          output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface);
};

//  Single Kernel. Reloadable Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          output_stream<TT_DATA>* outStream,
                                          const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//  Single Kernel. Reloadable Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

//  Cascaded Kernels - Final Kernel. Static Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Static Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
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

//  Cascaded Kernels - First Kernel. Static Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Middle Kernel. Static Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Reloadable Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Reloadable Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
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

//  Cascaded Kernels - First Kernel. Reloadable Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          output_cascade_cacc* outCascade,
                                          const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//  Cascaded Kernels - Middle Kernel. Reloadable Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_SINGLE,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernelRtp(inInterface, outInterface);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////  Dual Stream

//  Single Kernel. Static Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

//  Single Kernel. Static Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

//  Single Kernel. Reloadable Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

//  Single Kernel. Reloadable Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          0,
                          1,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

//  Cascaded Kernels - Final Kernel. Static Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Static Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
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

//  Cascaded Kernels - First Kernel. Static Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Middle Kernel. Static Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_FALSE,
                          DUAL_IP_DUAL,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Reloadable Coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          1,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
                                          output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->filterKernelRtp(inInterface, outInterface);
};

//  Cascaded Kernels - Final Kernel. Reloadable Coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_FALSE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          2,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
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

//  Cascaded Kernels - First Kernel. Reloadable Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_FALSE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          output_cascade_cacc* outCascade,
                                          const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//  Cascaded Kernels - Middle Kernel. Reloadable Coefficients. Single/Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
void fir_interpolate_asym<TT_DATA,
                          TT_COEFF,
                          TP_FIR_LEN,
                          TP_INTERPOLATE_FACTOR,
                          TP_SHIFT,
                          TP_RND,
                          TP_INPUT_WINDOW_VSIZE,
                          CASC_IN_TRUE,
                          CASC_OUT_TRUE,
                          TP_FIR_RANGE_LEN,
                          TP_KERNEL_POSITION,
                          TP_CASC_LEN,
                          USE_COEFF_RELOAD_TRUE,
                          DUAL_IP_DUAL,
                          TP_NUM_OUTPUTS,
                          USE_STREAM_API,
                          TP_MODIFY_MARGIN_OFFSET,
                          TP_COEFF_PHASE,
                          TP_COEFF_PHASE_OFFSET,
                          TP_COEFF_PHASES,
                          TP_COEFF_PHASES_LEN,
                          TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          input_cascade_cacc* inCascade,
                                          output_cascade_cacc* outCascade) {
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
} // namespaces
