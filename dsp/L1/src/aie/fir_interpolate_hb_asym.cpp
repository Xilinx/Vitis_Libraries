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
Halfband interpolating FIR kernel code.
 This file captures the body of run-time code for the kernel class and a higher wrapping 'cascade' layer which has
specializations for
 combinations of inputs and outputs. That is, in a chain of kernels, the first will have an input window, and a cascade
out stream.
 The next, potentially multiple, kernel(s) will each have an input window and cascade stream and will output a cascade
steam. The final kernel
 will have an input window and cascade stream and an output window only.
 The cascade layer class is called fir_interpolate_hb_asym with the kernel-layer (operational) class called
kernelFilterClass.
 The fir_interpolate_hb_asym class has a member of the kernelFilterClass.

 Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
//#define _DSPLIB_FIR_INTERPOLATE_HB_ASYM_DEBUG_

#ifdef __X86SIM__
// #define _DSPLIB_FIR_INTERPOLATE_HB_ASYM_DEBUG_
#endif

#include "debug_utils.h"

#include "kernel_api_utils.hpp"
#include "fir_interpolate_hb_asym.hpp"
#include "fir_interpolate_hb_utils.hpp"
#include "fir_sr_asym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb_asym {

// FIR function
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>(),
                    TP_CASC_API>(inInterface, outInterface);
    chess_memory_fence();
    filterSelectArch(inInterface, outInterface);
};

// FIR function - overloaded (not specialized) with taps for reload
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                         const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>(),
                    TP_CASC_API>(inInterface, outInterface);
    m_coeffnEq = rtpCompare(inTaps, m_oldInTaps);

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload(inTaps, m_oldInTaps, outInterface);
        firReload(inTaps);
    }
    chess_memory_fence();
    filterSelectArch(inInterface, outInterface);
};

// FIR function
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                            T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>(),
                    TP_CASC_API>(inInterface, outInterface);
    m_coeffnEq = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed

        bufferReload<TT_DATA, TT_COEFF, (TP_FIR_LEN + 1) / 2 + 1, TP_DUAL_IP>(inInterface, m_oldInTaps, outInterface);
        firReload(m_oldInTaps);
    }
    chess_memory_fence();
    filterSelectArch(inInterface, outInterface);
};

// FIR function - overloaded (not specialized) with taps for reload
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    if
        constexpr(m_kArch == kArchIncLoads) filterIncLoads(inInterface, outInterface);
    else if
        constexpr(m_kArch == kArchBasic) filterBasic(inInterface, outInterface);
    else if
        constexpr(m_kArch == kArchStream) filterStream(inInterface, outInterface);
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2, coe3;                              // register for coeff values.
    T_buff_256b<TT_COEFF>* ctCoeffptr = (T_buff_256b<TT_COEFF>*)m_phaseTwoTap; // register for centre tap coeff value.
    T_buff_256b<TT_COEFF> ctCoeff = *ctCoeffptr;
    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer
    T_buff_1024b<TT_DATA> sbuff = *ptr_delay; // initialize data register with data allocated on heap which will contain
                                              // previous iterations margin samples.
    T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHighPolyPhase, accLowPolyPhase;
    ::aie::vector<TT_DATA, m_kLanes> outVal;

    unsigned int dataLoaded;
    unsigned int dataNeeded;
    int dataOffset = 0;
    int numDataLoads = 0;
    unsigned int ctPos;

    static_assert((TP_INPUT_WINDOW_VSIZE % m_kLanes == 0) && (TP_INPUT_WINDOW_VSIZE >= m_kLanes),
                  "ERROR: WindowSize is not a multiple of lanes.");
    static_assert(
        ((m_kLsize / streamRptFactor) > 0),
        "ERROR: Window Size is too small, needs to be a multiple of the number of samples in a 1024b Buffer.");
    static_assert(((m_kLsize % streamRptFactor) == 0),
                  "ERROR: Inner loop unrolling not possible for given parameters. Modify Window Size.");

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

    int loopSize = (m_kLsize / streamRptFactor);

    // Init pre-loop to deal with m_kFirInitOffset. Only generate for cascaded designs
    if (doInit == 1) {
        dataLoaded = 0;
        dataNeeded = m_kLanes;

        for (unsigned i = 0; i < streamInitNullAccs; i++)
            chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                accHighPolyPhase = readCascade(inInterface, accHighPolyPhase);
                accLowPolyPhase = readCascade(inInterface, accLowPolyPhase);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accHighPolyPhase);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accLowPolyPhase);
                if
                    constexpr(TP_CASC_LEN == 1) {
                        writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                            outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outInterface.outStream);
                    }
            }

#pragma unroll(GUARD_ZERO(streamInitAccs))
        for (unsigned strobe = 0; strobe < streamInitAccs; strobe++) {
            ctPos = marginLoadsMappedToBuff * m_kDataLoadVsize - TP_FIR_LEN / 4 + streamInitNullAccs * m_kLanes +
                    strobe * m_kLanes;
            int xoffset = (streamInitNullAccs + strobe) * m_kLanes + streamDataOffsetWithinBuff;

            if (dataNeeded > dataLoaded) {
                readStream256(sbuff,
                              (marginLoadsMappedToBuff + strobe * m_kLanes / m_kDataLoadVsize) % m_kDataLoadsInReg,
                              inInterface);
                dataLoaded += m_kDataLoadVsize;
            }
            dataNeeded += m_kLanes;

            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
            coe0 = *coeff++;

            // Read cascade input. Do nothing if cascade input not present.
            accHighPolyPhase = readCascade(inInterface, accHighPolyPhase);
            accLowPolyPhase = readCascade(inInterface, accLowPolyPhase);
            accHighPolyPhase =
                sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, accHighPolyPhase, sbuff, xoffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                accHighPolyPhase =
                    sr_asym::macSrAsym(accHighPolyPhase, sbuff, xoffset + op, coe0, (op % m_kCoeffRegVsize));
            }
            // Write cascade. Do nothing if cascade not present
            if (strobe >= lowPolyPhaseNullAccs - streamInitNullAccs) {
                if
                    constexpr(m_kHasCT) {
                        accLowPolyPhase.val =
                            mulCentreTap1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_API>(sbuff, ctPos, ctCoeff);
                    }
            }
            writeCascade<TT_DATA, TT_COEFF>(outInterface, accHighPolyPhase);
            writeCascade<TT_DATA, TT_COEFF>(outInterface, accLowPolyPhase);
            if
                constexpr(TP_CASC_LEN == 1) {
                    writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                        outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outInterface.outStream);
                }
        }
        loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
    }

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = startDataLoads;
            dataOffset = startDataOffset;
            dataLoaded = 0;
            dataNeeded = m_kLanes;

#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
                ctPos = startDataLoads * m_kDataLoadVsize - (TP_FIR_LEN) / 4 + strobe * m_kLanes +
                        streamInitNullAccs * m_kLanes;

                if (dataNeeded > dataLoaded) {
                    readStream256(sbuff, numDataLoads % m_kDataLoadsInReg, inInterface);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }

                dataNeeded += m_kLanes;

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                accHighPolyPhase = readCascade(inInterface, accHighPolyPhase);
                accLowPolyPhase = readCascade(inInterface, accLowPolyPhase);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                accHighPolyPhase = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, accHighPolyPhase, sbuff,
                                                                             strobe * m_kLanes + dataOffset, coe0, 0);
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    accHighPolyPhase = sr_asym::macSrAsym(
                        accHighPolyPhase, sbuff, strobe * m_kLanes + (op + dataOffset), coe0, (op % m_kCoeffRegVsize));
                }
                if
                    constexpr(m_kHasCT) {
                        accLowPolyPhase.val =
                            mulCentreTap1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_API>(sbuff, ctPos, ctCoeff);
                    }
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accHighPolyPhase);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accLowPolyPhase);
                writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                    outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outInterface.outStream);
            }
        }
    doInit = 0;
    // store sbuff for next iteration
    *ptr_delay = sbuff;
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF>* ctCoeffptr = (T_buff_256b<TT_COEFF>*)m_phaseTwoTap; // register for centre tap coeff value.
    T_buff_256b<TT_COEFF> ctCoeff = *ctCoeffptr;
    T_buff_256b<TT_COEFF> coe0, coe1; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;      // input data value cache.
    T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHighPolyPhase, accLowPolyPhase;
    ::aie::vector<TT_DATA, m_kLanes> tempOutVal;
    unsigned int dataLoaded, numDataLoads;
    unsigned int dataNeeded;
    unsigned int ctPos;
    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc)); // output size can be less than data
                                                                                  // load size, so this iterator needs
                                                                                  // greater precision to reach finer
                                                                                  // addressed.

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);

    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    inItr += m_kXDataLoadInitOffset;
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
            dataNeeded = m_kInitDataNeeded;
            ctPos = (m_kFirMarginLen - m_kXDataLoadInitOffset - TP_FIR_LEN / 4) % m_kSamplesInBuff;

            accHighPolyPhase = readCascade(inInterface, accHighPolyPhase);
            accLowPolyPhase = readCascade(inInterface, accLowPolyPhase);

            for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
                upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads,
                                           inItr); // Update sbuff with data from input window. 00++|____|____|____
                dataLoaded += m_kDataLoadVsize;
                numDataLoads++;
            }

            if
                constexpr(m_kCTOp <= 0) {
                    if
                        constexpr(m_kHasCT) {
                            accLowPolyPhase.val =
                                mulCentreTap1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_API>(sbuff, ctPos, ctCoeff);
                        }
                }

            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            accHighPolyPhase = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, accHighPolyPhase, sbuff,
                                                                         m_kDataBuffXOffset, coe0, 0);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                dataNeeded += m_kColumns;
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                if (op == m_kCTOp) {
                    if
                        constexpr(m_kHasCT) {
                            accLowPolyPhase.val =
                                mulCentreTap1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_API>(sbuff, ctPos, ctCoeff);
                        }
                }
                accHighPolyPhase = sr_asym::macSrAsym(accHighPolyPhase, sbuff, (op + m_kDataBuffXOffset), coe0,
                                                      (op % m_kCoeffRegVsize));
            }

            inItr -= m_kDataLoadVsize * numDataLoads - m_kLanes;

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, accHighPolyPhase);
            writeCascade<TT_DATA, TT_COEFF>(outInterface, accLowPolyPhase);

            // Write to output window
            writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(outInterface, accHighPolyPhase,
                                                                                     accLowPolyPhase, TP_SHIFT, outItr);
            if
                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                    writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                        outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outItr2);
        }
};

// -------------------------------------------------- asym architecture
// -------------------------------------------------- //
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
          unsigned int TP_UPSHIFT_CT,
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
                                   TP_UPSHIFT_CT,
                                   TP_API,
                                   TP_SAT>::filterIncLoads(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    // const unsigned int     kSamplesInWindow = window_size(inWindow);  //number of samples in window
    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;                                                // register for coeff values.
    T_buff_256b<TT_COEFF>* ctCoeffptr = (T_buff_256b<TT_COEFF>*)m_phaseTwoTap; // register for centre tap coeff value.
    T_buff_256b<TT_COEFF> ctCoeff = *ctCoeffptr;
    T_buff_256b<TT_DATA> readData;
    T_buff_128b<TT_DATA> readData_128;
    T_buff_1024b<TT_DATA> sbuff;
    T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHighPolyPhase, accLowPolyPhase;
    unsigned int dataLoaded, dataNeeded, numDataLoads;
    unsigned int xstart, ystart, coeffstart;
    unsigned int loadSplice;
    unsigned int ctPos;
    ::aie::vector<TT_DATA, m_kLanes> tempOutVal;
    numDataLoads = 0;
    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, 2 * fnNumLanes<TT_DATA, TT_COEFF>()>(
        *outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, 2 * fnNumLanes<TT_DATA, TT_COEFF>()>(
        *outInterface.outWindow2);

    inItr += m_kXDataLoadInitOffset;

#pragma unroll(GUARD_ZERO(m_kInitialLoads - 2))
    for (unsigned int initLoads = 0; initLoads < m_kInitialLoads - 2; ++initLoads) {
        read256Ptr = (t_256vect*)&*inItr;
        inItr += k256Vsize;
        readData.val = *read256Ptr;
        sbuff.val.insert(initLoads % m_kDataLoadsInReg, readData.val);
    }

    coeff = ((T_buff_256b<TT_COEFF>*)&m_internalTaps);
    coe0 = *coeff++;

    // Loop through window outputting a vector of values each time.
    // Lsize is therefore a ratio of window size to output vector size
    for (unsigned i = 0; i < m_kLsize / m_kDataLoadsInReg; i++)
        // Allow optimizations in the kernel compilation for this loop
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kDataLoadsInReg, ) {
            numDataLoads = 0;
            dataLoaded = 0;
            dataNeeded = 0;
#pragma unroll(m_kDataLoadsInReg)
            for (unsigned strobe = 0; strobe < (m_kDataLoadsInReg); strobe++) {
                coeff = ((T_buff_256b<TT_COEFF>*)&m_internalTaps);
                coe0 = *coeff++;
                ctPos = m_kFirMarginLen - m_kXDataLoadInitOffset - (TP_FIR_LEN) / 4 + strobe * m_kLanes;

                if (dataNeeded >= dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, ((m_kInitialLoads - 2) + numDataLoads) % m_kDataLoadsInReg,
                                               inItr);
                    numDataLoads++;
                    dataLoaded += m_kDataLoadVsize;
                    upd_win_incr_256b<TT_DATA>(sbuff, ((m_kInitialLoads - 2) + numDataLoads) % m_kDataLoadsInReg,
                                               inItr);
                    numDataLoads++;
                    dataLoaded += m_kDataLoadVsize;
                }
                dataNeeded += m_kLanes;
                accHighPolyPhase = readCascade(inInterface, accHighPolyPhase);
                accLowPolyPhase = readCascade(inInterface, accLowPolyPhase);
                accHighPolyPhase = sr_asym::initMacSrAsym<TT_DATA, TT_COEFF>(
                    inInterface, accHighPolyPhase, sbuff, strobe * m_kLanes + m_kDataBuffXOffset, coe0, 0);
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    accHighPolyPhase =
                        sr_asym::macSrAsym(accHighPolyPhase, sbuff, strobe * m_kLanes + (op + m_kDataBuffXOffset), coe0,
                                           (op % m_kCoeffRegVsize));
                }

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accHighPolyPhase);
                if
                    constexpr(m_kHasCT) {
                        accLowPolyPhase.val =
                            mulCentreTap1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_API>(sbuff, ctPos, ctCoeff);
                    }
                writeCascade<TT_DATA, TT_COEFF>(outInterface, accLowPolyPhase);
                writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                    outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                        writeOutputSel<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API>(
                            outInterface, accHighPolyPhase, accLowPolyPhase, TP_SHIFT, outItr2);
            }
        }
};

//----------------------------------------------------------------------------
// Start of Cascade-layer class body and specializations

// FIR filter function overloaded with cascade interface variations
// This is a (default) specialization of the main class for when there is only one kernel for the whole filter.
// Windowed. single output, static coeffs, single output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             TP_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Single kernel specialization. No cascade ports, Windowed. single input, static  coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_SINGLE,
                             USE_COEFF_RELOAD_FALSE,
                             2,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
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
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_SINGLE,
                             USE_COEFF_RELOAD_TRUE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
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
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_SINGLE,
                             USE_COEFF_RELOAD_TRUE,
                             2,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow2,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_DUAL,
                             USE_COEFF_RELOAD_FALSE,
                             2,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowReverse,
           output_circular_buffer<TT_DATA>& __restrict outWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow2) {
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
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_DUAL,
                             USE_COEFF_RELOAD_TRUE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowReverse,
           output_circular_buffer<TT_DATA>& __restrict outWindow,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
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
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_DUAL,
                             USE_COEFF_RELOAD_TRUE,
                             2,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowReverse,
           output_circular_buffer<TT_DATA>& __restrict outWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow2,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA,
                                                                extents<inherited_extent> >& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA,
                                                                extents<inherited_extent> >& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
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

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, reload,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA,
                                                                extents<inherited_extent> >& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
                                             output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, reload,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA,
                                                                extents<inherited_extent> >& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
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

// FIR filter function overloaded with cascade interface variations
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, no reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_SINGLE,
                             USE_COEFF_RELOAD_FALSE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, no reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowReverse,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             DUAL_IP_SINGLE,
                             USE_COEFF_RELOAD_TRUE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& __restrict broadcastWindow,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
           input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowReverse,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& __restrict broadcastWindow,
           const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, no
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA>& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
                                             output_cascade_cacc* outCascade,
                                             output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_WINDOW_API,
                             TP_SAT>::filter(input_async_buffer<TT_DATA>& __restrict inWindow,
                                             input_cascade_cacc* inCascade,
                                             output_cascade_cacc* outCascade,
                                             output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             output_stream<TT_DATA>* outStream,
                                             output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             output_stream<TT_DATA>* outStream,
                                             const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             output_stream<TT_DATA>* outStream,
                                             output_stream<TT_DATA>* outStream2,
                                             const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->filterKernel(inInterface, outInterface, inTaps);
}

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single input. Single Output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             input_cascade_cacc* inCascade,
                                             output_stream<TT_DATA>* outStream,
                                             output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             USE_COEFF_RELOAD_FALSE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             USE_COEFF_RELOAD_FALSE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             input_cascade_cacc* inCascade,
                                             output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_SINGLE> inInterface;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             input_cascade_cacc* inCascade,
                                             output_stream<TT_DATA>* outStream,
                                             output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
                             TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                             output_cascade_cacc* outCascade,
                                             const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single Input
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
void fir_interpolate_hb_asym<TT_DATA,
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
                             0,
                             USE_COEFF_RELOAD_TRUE,
                             1,
                             TP_UPSHIFT_CT,
                             USE_STREAM_API,
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
}
}
}
}
}
