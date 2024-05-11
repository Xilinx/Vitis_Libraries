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
Asymmetric Decimator FIR kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#ifdef __X86SIM__
// #define _DSPLIB_FIR_DECIMATE_ASYM_HPP_DEBUG_
#endif

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "device_defs.h"
#include "kernel_api_utils.hpp"
#include "fir_sr_asym_utils.hpp"
#include "fir_decimate_asym.hpp"
#include "fir_decimate_asym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {

#define Y_BUFFER ya
#define X_BUFFER xd
#define Z_BUFFER wc0

#if __MIN_REGSIZE__ == 128
#define CHESS_STORAGE_DEF chess_storage(Z_BUFFER)
#else
// clear out constraint
#define CHESS_STORAGE_DEF
#endif
// Asymmetrical Decimation FIR Kernel Function
// According to template parameter the input may be a window, or window and cascade input
// Similarly the output interface may be a window or a cascade output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                                   TP_DECIMATE_FACTOR,
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
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    filterSelectArch(inInterface, outInterface);
}

// Asymmetrical Decimation FIR Kernel Function - overloaded (not specialised)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                                   TP_DECIMATE_FACTOR,
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
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    isUpdateRequired = rtpCompare<TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps);

    sendRtpTrigger(isUpdateRequired, outInterface);
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inTaps, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(inTaps);
        chess_memory_fence();
    }
    filterSelectArch(inInterface, outInterface);
}

// Asymmetrical Decimation FIR Kernel Function
// According to template parameter the input may be a window, or window and cascade input
// Similarly the output interface may be a window or a cascade output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                                   TP_DECIMATE_FACTOR,
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
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_API>(inInterface,
                                                                                                 outInterface);
    isUpdateRequired = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(isUpdateRequired, outInterface);
    if (isUpdateRequired) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, TP_COEFF_PHASES_LEN>(inInterface, m_rawInTaps, outInterface);
        firReload<TP_COEFF_PHASE, TP_COEFF_PHASE_OFFSET, TP_COEFF_PHASES, TP_COEFF_PHASES_LEN>(m_rawInTaps);
        chess_memory_fence();
    }
    filterSelectArch(inInterface, outInterface);
};
// Asymmetrical Decimation FIR Kernel Function - overloaded (not specialised )
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                                   TP_DECIMATE_FACTOR,
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
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    windowReset<TT_DATA, TP_CASC_IN, TP_DUAL_IP, TP_API>(inInterface);
    if
        constexpr(m_kArch == kArchBasic) { filterBasic(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchIncrStrobe) { filterIncrStrobe(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchStream) { filterStream(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchStreamPhaseParallel) { filterStreamPhaseParallel(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArchPhaseParallel) { filterPhaseParallel(inInterface, outInterface); }
    windowRelease(inInterface);
}

// -------------------------------------------------------------- Basic
// -------------------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                       TP_DECIMATE_FACTOR,
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
    // The plus one in this calculation converts index to width
    static constexpr unsigned int m_kInitialLoads =
        CEIL(m_kDataBuffXOffset + (TP_DECIMATE_FACTOR * (m_kLanes - 1)) + (m_kColumns - 1) + 1, m_kInitLoadVsize) /
        m_kInitLoadVsize;
    // static constexpr unsigned int  m_kInitialLoads= m_kInitLoads1BBasic;

    using coe_type = typename T_buff_256b<TT_COEFF>::v_type;
    using buf_type = typename T_buff_1024b<TT_DATA>::v_type;

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;
    T_accDecAsym<TT_DATA, TT_COEFF> acc;
    T_outValDecAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for sbuff
    unsigned int dataLoaded, dataNeeded, numDataLoads, initNumDataLoads;
    unsigned int xstart, xstartUpper, splice; // upper is used for upper lanes in high decimation factor handling

    TT_COEFF* m_internalTapsCopy = m_internalTaps;

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

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += (TRUNC((m_kFirInitOffset), (m_kWinAccessByteSize / sizeof(TT_DATA))));

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
            coe_type CHESS_STORAGE_DEF coe_h = coeff->val;
            coeff++;
            coe0.val = coe_h;

            // Preamble, calc number of samples for first mul.
            splice = 0;
            numDataLoads = 0;
            initNumDataLoads = 0;
            dataLoaded = 0;
            dataNeeded = m_kDataBuffXOffset + (m_kLanes - 1) * TP_DECIMATE_FACTOR + m_kColumns - 1 +
                         1; // final plus one turns index into width

#pragma unroll(m_kInitialLoads)
            for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
                upd_win_incr_256b<TT_DATA>(sbuff, initNumDataLoads % m_kInitLoadsInReg, inItr);
                dataLoaded += m_kInitLoadVsize;
                initNumDataLoads++;
            }

            // Read cascade input. Do nothing if cascade input not present.
            acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);

            xstart = m_kDataBuffXOffset;
            xstartUpper = xstart + m_kLanes / 2 * TP_DECIMATE_FACTOR; // upper half of lanes

            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            acc = initMacDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(inInterface, acc, sbuff, xstart, coe0,
                                                                                0, m_kDecimateOffsets, xstartUpper);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                dataNeeded += m_kColumns;
                if (dataNeeded > dataLoaded) {
                    splice =
                        (numDataLoads + initNumDataLoads * m_kInitLoadVsize / m_kDataLoadVsize) % m_kDataLoadsInReg;
                    fnLoadXIpData<TT_DATA, m_kDataLoadSize>(sbuff, splice, inItr);
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }
                if (op % m_kCoeffRegVsize == 0) {
                    coe_type CHESS_STORAGE_DEF coe_h = coeff->val;
                    coeff++;
                    coe0.val = coe_h;
                    if (m_kDFX == kHighDF) {
                        chess_separator_scheduler();
                    }
                }
                xstart += m_kColumns;
                xstartUpper += m_kColumns;

                acc = macDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                    acc, sbuff, xstart, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets, xstartUpper);
            }
            // if (m_kDFX == kHighDF) {
            //     chess_separator_scheduler();
            // }

            // Go back by the number of input samples loaded minus  (i.e forward) by the number of samples consumed
            inItr -= (initNumDataLoads * m_kInitLoadVsize + m_kDataLoadVsize * numDataLoads -
                      m_kVOutSize * TP_DECIMATE_FACTOR); // return read pointer to start of next chunk of window.

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

            outVal = shiftAndSaturateDecAsym(acc, TP_SHIFT);
            // Write to output window
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                   0 /*stream phase - irrelevant*/, outItr);
            if
                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                    writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                           0 /*stream phase - irrelevant*/, outItr2);
                }
        } // for i
};

// -------------------------------------------------------------- PhaseParallel
// -------------------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                       TP_DECIMATE_FACTOR,
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
                       TP_SAT>::filterPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // The plus one in this calculation converts index to width
    // static constexpr unsigned int  m_kInitialLoads  = CEIL(m_kDataBuffXOffset+(TP_DECIMATE_FACTOR*(m_kLanes -
    // 1))+(m_kColumns - 1)+1,m_kInitLoadVsize)/m_kInitLoadVsize;
    // static constexpr unsigned int  m_kInitialLoads= m_kInitLoads1BBasic;

    using coe_type = typename T_buff_256b<TT_COEFF>::v_type;
    using buf_type = typename T_buff_1024b<TT_DATA>::v_type;

    constexpr unsigned int kParallelPhases = TP_DECIMATE_FACTOR;

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;
    T_accDecAsym<TT_DATA, TT_COEFF> acc_0;
    T_outValDecAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for sbuff
    unsigned int dataLoaded, dataNeeded, numDataLoads, initNumDataLoads;
    unsigned int xstart, xstartUpper, splice; // upper is used for upper lanes in high decimation factor handling
    std::array<T_accDecAsym<TT_DATA, TT_COEFF>, kParallelPhases> acc;
    std::array<T_buff_256b<TT_COEFF>, kParallelPhases> coe;
    std::array<T_buff_1024b<TT_DATA>, kParallelPhases> sbuffArray;
    constexpr unsigned int kInitialLoads =
        CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) / m_kDataLoadVsize;
    static constexpr unsigned int kFirLenCeilCols =
        CEIL(CEIL(TP_FIR_RANGE_LEN, kParallelPhases) / kParallelPhases, m_kColumns);
    static constexpr unsigned int kDataMappedToPhaseOffset =
        (kParallelPhases - ((m_kFirMargin - m_kFirInitWinOffset) % kParallelPhases)) % kParallelPhases;
    static constexpr std::array<unsigned int, kParallelPhases> kDataMappedToPhasestartOffset =
        fnPhaseStartOffsets<kDataMappedToPhaseOffset, kParallelPhases>();

    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    TT_DATA* inWindowPtr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                               ? (TT_DATA*)inInterface.inWindowLin->data()
                               : (TT_DATA*)inInterface.inWindowCirc->data();
    auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0)
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    // auto inItr = (TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) ?
    //                 ::aie::begin_circular(*(inInterface.inWindowLin)) :
    //                 ::aie::begin_circular(*(inInterface.inWindowCirc)) ;

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += m_kFirInitWinOffset;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
            coe_type CHESS_STORAGE_DEF coe_h = coeff->val;
            coeff++;
            coe0.val = coe_h;
#pragma unroll(kParallelPhases)
            for (int phase = 0; phase < kParallelPhases; ++phase) {
                // T_buff_256b<TT_COEFF>*      __restrict coeff   =  ((T_buff_256b<TT_COEFF>
                // *)m_internalTaps2[fnCoeffPhase(phase,kParallelPhases)]);
                T_buff_256b<TT_COEFF>* coeff =
                    ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)]);
                coe[phase] = *coeff;
            }

            // Preamble, calc number of samples for first mul.
            splice = 0;
            numDataLoads = 0;
            initNumDataLoads = 0;
            dataLoaded = 0;
            dataNeeded = m_kDataBuffXOffset + m_kLanes + m_kColumns - 1;

#pragma unroll(kInitialLoads)
            for (int initLoads = 0; initLoads < kInitialLoads; ++initLoads) {
#if DONT_USE_ITERATORS == 1
                bufferLoadAndDeinterleave<TT_DATA, kParallelPhases>(sbuffArray, inWindowPtr, initNumDataLoads++,
                                                                    kDataMappedToPhaseOffset);
#else
                bufferLoadAndDeinterleave<TT_DATA, kParallelPhases>(sbuffArray, inItr, initNumDataLoads++,
                                                                    kDataMappedToPhaseOffset);
#endif
                dataLoaded += m_kInitLoadVsize;
            }

            acc[0] = readCascade<TT_DATA, TT_COEFF>(inInterface, acc[0]);

// Init Vector operation. VMUL if cascade not present, otherwise VMAC
#pragma unroll(kParallelPhases)
            for (int phase = 0; phase < kParallelPhases; ++phase) {
                xstart = CEIL(m_kDataBuffXOffset, kParallelPhases) / kParallelPhases -
                         CEIL(phase, kParallelPhases) / kParallelPhases + kDataMappedToPhasestartOffset[phase];
                // xstart = CEIL(m_kDataBuffXOffset, kParallelPhases)/kParallelPhases - phase +
                // kDataMappedToPhasestartOffset[phase];
                acc[0] = sr_asym::macSrAsym<TT_DATA, TT_COEFF>(acc[0], sbuffArray[phase], xstart, coe[phase], 0);
            }

#pragma unroll(GUARD_ZERO((kFirLenCeilCols / (m_kColumns) - 1)))
            for (int op = m_kColumns; op < kFirLenCeilCols; op += m_kColumns) {
                dataNeeded += m_kColumns;
                if (dataNeeded > dataLoaded) {
                    splice =
                        (numDataLoads + initNumDataLoads * m_kInitLoadVsize / m_kDataLoadVsize) % m_kDataLoadsInReg;
#if DONT_USE_ITERATORS == 1
                    bufferLoadAndDeinterleave<TT_DATA, kParallelPhases>(sbuffArray, inWindowPtr, splice,
                                                                        kDataMappedToPhaseOffset);
#else
                    bufferLoadAndDeinterleave<TT_DATA, kParallelPhases>(sbuffArray, inItr, splice,
                                                                        kDataMappedToPhaseOffset);
#endif
                    dataLoaded += m_kDataLoadVsize;
                    numDataLoads++;
                }

                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    if ((op) % m_kCoeffRegVsize == 0) {
                        chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                            ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)] +
                             (op) / m_kCoeffRegVsize);
                        coe[phase] = *coeff;
                    }
                    // xstart = CEIL(m_kDataBuffXOffset, kParallelPhases)/kParallelPhases + op;
                    xstart = CEIL(m_kDataBuffXOffset, kParallelPhases) / kParallelPhases -
                             CEIL(phase, kParallelPhases) / kParallelPhases + kDataMappedToPhasestartOffset[phase] + op;
                    // xstart = CEIL(m_kDataBuffXOffset, kParallelPhases)/kParallelPhases - phase +
                    // kDataMappedToPhasestartOffset[phase] + op;
                    acc[0] =
                        sr_asym::macSrAsym(acc[0], sbuffArray[phase], xstart, coe[phase], ((op) % m_kCoeffRegVsize));
                }
            }

            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[0]);
#pragma unroll(kParallelPhases)
            for (int phase = 0; phase < kParallelPhases; ++phase) {
            }

#pragma unroll(kParallelPhases - 1)
            for (int phase = 1; phase < kParallelPhases; ++phase) {
                // acc[0].val = acc[0].val + acc[phase].val;
            }
            outVal = shiftAndSaturateDecAsym(acc[0], TP_SHIFT);

            // // Write to output window with no interleaving
            // *outItr++ =  outVal.val;

            // Go back by the number of input samples loaded minus  (i.e forward) by the number of samples consumed
            inItr -= (kParallelPhases * kInitialLoads * m_kInitLoadVsize +
                      kParallelPhases * m_kDataLoadVsize * numDataLoads -
                      kParallelPhases * m_kVOutSize); // return read pointer to start of next chunk of window.

            // Write to output window
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                   0 /*stream phase - irrelevant*/, outItr);
            if
                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                    writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                           0 /*stream phase - irrelevant*/, outItr2);
                }

        } // for i
};

// -------------------------------------------------------------- IncrLoads
// -------------------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                       TP_DECIMATE_FACTOR,
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
                       TP_SAT>::filterIncrStrobe(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    static constexpr unsigned int m_kDataUsedRptp1 =
        m_kDataBuffXOffset + TP_FIR_RANGE_LEN - 1 + TP_DECIMATE_FACTOR * m_kVOutSize * (m_kRepeatFactor + 1);
    static constexpr unsigned int m_kDataUsedRpt =
        m_kDataBuffXOffset + TP_FIR_RANGE_LEN - 1 + TP_DECIMATE_FACTOR * m_kVOutSize * (m_kRepeatFactor);
    static constexpr unsigned int m_kLoadsAfterRepeatP1 = CEIL(m_kDataUsedRptp1, m_kInitLoadVsize) / m_kInitLoadVsize;
    static constexpr unsigned int m_kLoadsAfterRepeat = CEIL(m_kDataUsedRpt, m_kInitLoadVsize) / m_kInitLoadVsize;
    static constexpr unsigned int m_kInitDataNeeded =
        m_kDataBuffXOffset + TP_FIR_RANGE_LEN + TP_DECIMATE_FACTOR * (m_kLanes - 1);
    static constexpr unsigned int m_kInitLoads1BIncLd =
        CEIL(m_kInitDataNeeded, m_kInitLoadVsize) / m_kInitLoadVsize + m_kLoadsAfterRepeat - m_kLoadsAfterRepeatP1;
    static constexpr unsigned int m_kInitialLoads = m_kInitLoads1BIncLd;

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff; // input data value cache.
    T_accDecAsym<TT_DATA, TT_COEFF> acc;
    T_outValDecAsym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for sbuff
    unsigned int dataLoaded, dataNeeded, numDataLoads;
    unsigned int initDataLoaded, initDataNeeded, initNumDataLoads;
    unsigned int xstart, xstartUpper; // upper is used for upper lanes in high decimation factor handling

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

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += (TRUNC((m_kFirInitOffset), (m_kWinAccessByteSize / sizeof(TT_DATA))));

    initNumDataLoads = 0;
    // Preamble, calc number of samples for first mul.
    initDataLoaded = 0;
    initDataNeeded = m_kInitDataNeeded - TP_DECIMATE_FACTOR * m_kVOutSize;
// numDataLoads = m_kInitialLoads % m_kInitLoadsInReg;

#pragma unroll(m_kInitialLoads)
    for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initNumDataLoads % m_kInitLoadsInReg, inItr);
        initDataLoaded += m_kInitLoadVsize;
        initNumDataLoads++;
    }
    numDataLoads = initNumDataLoads;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / m_kRepeatFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kRepeatFactor, ) {
            dataNeeded = initDataNeeded;
            dataLoaded = initDataLoaded;
            numDataLoads = initNumDataLoads;

// The strobe loop is the number of iterations of Lsize required for conditions to return to the same state (typically 4
// due to the ratio of
// load size(256b) to xbuff size(1024b))
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
                dataNeeded += TP_DECIMATE_FACTOR * m_kVOutSize;

                // it might take more than one load to top up the buffer of input data to satisfy the need for the next
                // vector of outputs
                if (dataNeeded > dataLoaded) {
                    upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kInitLoadsInReg, inItr);
                    dataLoaded += m_kInitLoadVsize;
                    numDataLoads++;

                    if (dataNeeded > dataLoaded) {
                        upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kInitLoadsInReg, inItr);
                        dataLoaded += m_kInitLoadVsize;
                        numDataLoads++;
                    }
                }

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff;
                coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                xstart = m_kDataBuffXOffset + strobe * TP_DECIMATE_FACTOR * m_kLanes;
                xstartUpper = xstart + m_kLanes / 2 * TP_DECIMATE_FACTOR; // upper half of lanes

                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                    inInterface, acc, sbuff, xstart, coe0, 0, m_kDecimateOffsets, xstartUpper);

#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    xstart += m_kColumns;
                    xstartUpper += m_kColumns;

                    acc = macDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                        acc, sbuff, xstart, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets, xstartUpper);
                }
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturateDecAsym(acc, TP_SHIFT);
                // Write to output window
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal,
                                                                       0 /*stream phase - irrelevant*/, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(
                            outInterface, outVal, 0 /*stream phase - irrelevant*/, outItr2);
                    }
            } // for strobe
        }     // for i
};

// ----------------------------------------------------- Stream ----------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                       TP_DECIMATE_FACTOR,
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
    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1, coe2;                           // register for coeff values.
    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer
    T_buff_1024b<TT_DATA> sbuff = *ptr_delay; // initialize data register with data allocated on heap
    T_accDecAsym<TT_DATA, TT_COEFF> acc;
    T_outValDecAsym<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded;
    unsigned int dataNeeded;
    unsigned int inDataLoadPhase, outDataPhase = 0;
    int dataOffset = 0;
    int numDataLoads = 0;
    unsigned int xstart, xstartUpper; // upper is used for upper lanes in high decimation factor handling
    constexpr unsigned int m_kInitialLoads =
        CEIL(m_kDataBuffXOffset + (TP_DECIMATE_FACTOR * (m_kLanes - 1)) + (m_kColumns - 1) + 1, m_kInitLoadVsize) /
        m_kInitLoadVsize;

    constexpr unsigned int kDataLoadsInReg = 1024 / m_kStreamReadWidth;

    int loopSize = (m_kLsize / streamRptFactor);
    int startDataLoads = marginLoadsMappedToBuff +
                         (streamInitAccs - 1) * TP_DECIMATE_FACTOR * m_kVOutSize / m_kStreamLoadVsize +
                         kMinDataLoadCycles;

    // modulo operation can be removed because streamDataOffsetWithinBuff is already obtained from a modulo operation.
    int startDataOffset = (streamDataOffsetWithinBuff) % m_kSamplesInBuff;

    coe0 = *coeff++;

    // Init pre-loop to deal with m_kFirInitOffset. Only generate for cascaded designs
    // if constexpr (TP_CASC_LEN > 1) {
    if (doInit) {
        numDataLoads = marginLoadsMappedToBuff;
        dataLoaded = 0;
        dataNeeded = kMinDataNeeded;

        for (unsigned i = 0; i < streamInitNullAccs; i++)
            chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                acc = readCascade(inInterface, acc);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
            }

        inDataLoadPhase = 0;
#pragma unroll(GUARD_ZERO(streamInitAccs))
        for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
            int xoffset = (streamInitNullAccs + strobe) * TP_DECIMATE_FACTOR * m_kVOutSize + streamDataOffsetWithinBuff;
            xstartUpper = 0;

            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
            coe0 = *coeff++;
            // coeff++;

            if (strobe == 0) {
#pragma unroll GUARD_ZERO(kMinDataLoadCycles)
                for (int i = 0; i < kMinDataLoadCycles; i++) {
                    if (dataNeeded > dataLoaded) {
                        if (m_kStreamReadWidth == 256) {
                            readStream256(sbuff, numDataLoads % kDataLoadsInReg, inInterface);
                        } else {
                            readStream128(sbuff, numDataLoads % kDataLoadsInReg, inInterface, inDataLoadPhase++ % 2);
                        }
                        dataLoaded += m_kStreamLoadVsize;
                        numDataLoads++;
                    }
                }
            } else {
#pragma unroll(TP_DECIMATE_FACTOR)
                for (int i = 0; i < TP_DECIMATE_FACTOR; i++) {
                    if (dataNeeded > dataLoaded) {
                        if (m_kStreamReadWidth == 256) {
                            readStream256(sbuff, numDataLoads % kDataLoadsInReg, inInterface);
                        } else {
                            readStream128(sbuff, numDataLoads % kDataLoadsInReg, inInterface, inDataLoadPhase++ % 2);
                        }
                        dataLoaded += m_kStreamLoadVsize;
                        numDataLoads++;
                    }
                }
            }
            dataNeeded += TP_DECIMATE_FACTOR * m_kVOutSize;

            // Read cascade input. Do nothing if cascade input not present.
            acc = readCascade(inInterface, acc);
            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            acc = initMacDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(inInterface, acc, sbuff, xoffset, coe0,
                                                                                0, m_kDecimateOffsets, xstartUpper);
#pragma unroll(GUARD_ZERO(((m_kFirLenCeilCols / (m_kColumns)) - 1)))
            for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
                acc = macDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                    acc, sbuff, xoffset + op, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets, xstartUpper);
            }

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
            outVal = shiftAndSaturate(acc, TP_SHIFT);
            writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase++ % 2);
        }
        if
            constexpr(streamInitNullAccs == 0) { loopSize -= 1; }
        else {
            loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
        }
    }
    //}
    doInit = 0;
    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = startDataLoads;
            dataOffset = startDataOffset;
            dataLoaded = 0;
            dataNeeded = 0;
            inDataLoadPhase = streamInitAccs;
// unroll streamRptFactor times
#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff++;

                dataNeeded += TP_DECIMATE_FACTOR * m_kVOutSize;
#pragma unroll(TP_DECIMATE_FACTOR)
                for (int i = 0; i < TP_DECIMATE_FACTOR; i++) {
                    if (dataNeeded > dataLoaded) {
                        if (m_kStreamReadWidth == 256) {
                            readStream256(sbuff, numDataLoads % kDataLoadsInReg, inInterface);
                        } else {
                            readStream128(sbuff, numDataLoads % kDataLoadsInReg, inInterface, inDataLoadPhase++ % 2);
                        }
                        dataLoaded += m_kStreamLoadVsize;
                        numDataLoads++;
                    }
                }

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                acc = initMacDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                    inInterface, acc, sbuff, strobe * TP_DECIMATE_FACTOR * m_kVOutSize + dataOffset, coe0, 0,
                    m_kDecimateOffsets, xstartUpper);
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    acc = macDecAsym<TT_DATA, TT_COEFF, m_kDFX, TP_DECIMATE_FACTOR>(
                        acc, sbuff, strobe * TP_DECIMATE_FACTOR * m_kVOutSize + (op + dataOffset), coe0,
                        (op % m_kCoeffRegVsize), m_kDecimateOffsets, xstartUpper);
                }

                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);
                outVal = shiftAndSaturate(acc, TP_SHIFT);
                // Write to output
                writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase % 2);
                outDataPhase++;
            }
        }

    doInit = 0;

    // store sbuff for next iteration
    *ptr_delay = sbuff;
};

// --------------------------------------------------StreamPhaseParallel-----------------------------------------------
// //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
                       TP_DECIMATE_FACTOR,
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
                       TP_SAT>::filterStreamPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    static_assert((TP_INPUT_WINDOW_VSIZE % m_kVOutSize == 0) && (TP_INPUT_WINDOW_VSIZE >= m_kVOutSize),
                  "ERROR: WindowSize is not a multiple of lanes.");
    static_assert(
        ((m_kLsize / streamRptFactor) > 0),
        "ERROR: Window Size is too small, needs to be a multiple of the number of samples in a 1024b Buffer.");

    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_1024b<TT_DATA>* ptr_delay = (T_buff_1024b<TT_DATA>*)delay; // heap storage pointer
    // T_buff_1024b<TT_DATA>       sbuff = *ptr_delay; // initialize data register with data allocated on heap
    // T_accDecAsym<TT_DATA,TT_COEFF>     acc;
    T_outValDecAsym<TT_DATA, TT_COEFF> outVal;
    unsigned int dataLoaded;
    unsigned int dataNeeded;
    unsigned int inDataLoadPhase, outDataPhase = 0;
    int dataOffset = 0;
    int numDataLoads = 0;
    unsigned int xstart;

    constexpr unsigned int kDataLoadsInReg = 1024 / m_kStreamReadWidth;

    int loopSize = (m_kLsize / streamRptFactor);
    // int startDataLoads  = marginLoadsMappedToBuff + kMinDataLoadCycles + (streamInitAccs-1) * TP_DECIMATE_FACTOR *
    // m_kVOutSize / m_kStreamLoadVsize;
    constexpr unsigned int kParallelPhases = TP_DECIMATE_FACTOR;

    std::array<T_accDecAsym<TT_DATA, TT_COEFF>, kParallelPhases> acc;
    std::array<T_buff_256b<TT_COEFF>, kParallelPhases> coe;
    // std::array<T_buff_1024b<TT_DATA>*, kParallelPhases> ptr_delay;
    std::array<T_buff_1024b<TT_DATA>, kParallelPhases> sbuffArray;

// Read margin info back from stack
#pragma unroll(kParallelPhases)
    for (int phase = 0; phase < kParallelPhases; ++phase) {
        sbuffArray[phase] = *ptr_delay++;
    }
    static constexpr unsigned int kFirLenCeilCols =
        CEIL(CEIL(TP_FIR_RANGE_LEN, kParallelPhases) / kParallelPhases, m_kColumns);
    // static constexpr unsigned int  kDataMappedToPhaseOffset    = (kParallelPhases - ((m_kFirMargin -
    // m_kFirInitWinOffset)% kParallelPhases)) % kParallelPhases;
    static constexpr unsigned int kDataMappedToPhaseOffset =
        (kParallelPhases + TP_MODIFY_MARGIN_OFFSET) % kParallelPhases;
    static constexpr std::array<unsigned int, kParallelPhases> kDataMappedToPhasestartOffset =
        fnPhaseStartOffsets<kDataMappedToPhaseOffset, kParallelPhases>();
    // static constexpr int streamInitNullAccs           = getInitNullAccs<dataOffsetNthKernel, TP_DECIMATE_FACTOR,
    // m_kVOutSize>(); // Number of Null Mac Vectors sent as partial prouducts over cascade.

    int startDataLoads = marginLoadsMappedToBuff + streamInitAccs * m_kVOutSize / m_kStreamLoadVsize;
    int initDataLoads = marginLoadsMappedToBuff;

    // int startDataOffset = (marginLoadsMappedToBuff*m_kStreamLoadVsize - TP_FIR_RANGE_LEN /
    // kParallelPhases)%m_kSamplesInBuff;
    // data offset = index where data is initially loaded - number of coeffs still to compute, i.e. this and downstream
    // kernels TP_FIR_RANGE_LENs.
    int startDataOffset =
        ((marginLoadsMappedToBuff + streamInitAccs * m_kVOutSize / m_kStreamLoadVsize) * m_kStreamLoadVsize +
         TP_MODIFY_MARGIN_OFFSET - (TP_FIR_LEN - m_kFirRangeOffset) / kParallelPhases) %
        m_kSamplesInBuff;
    int initDataOffset = ((marginLoadsMappedToBuff)*m_kStreamLoadVsize + TP_MODIFY_MARGIN_OFFSET -
                          (TP_FIR_LEN - m_kFirRangeOffset) / kParallelPhases) %
                         m_kSamplesInBuff;

    // Init pre-loop to deal with m_kFirInitOffset. Only generate for cascaded designs
    // if constexpr (TP_CASC_LEN > 1) {
    if (doInit) {
        numDataLoads = initDataLoads;
        dataLoaded = 0;
        dataNeeded = kMinDataNeeded;

        for (unsigned i = 0; i < streamInitNullAccs; i++)
            chess_prepare_for_pipelining chess_loop_range(streamInitNullAccs, ) {
                acc[0] = readCascade(inInterface, acc[0]);
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[0]);
            }

        inDataLoadPhase = 0;
#pragma unroll(GUARD_ZERO(streamInitAccs))
        for (unsigned strobe = 0; strobe < (streamInitAccs); strobe++) {
#pragma unroll(kParallelPhases)
            for (int phase = 0; phase < kParallelPhases; ++phase) {
                T_buff_256b<TT_COEFF>* coeff =
                    ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)]);
                coe[phase] = *coeff;
            }

            dataNeeded += m_kVOutSize;
            if (dataNeeded > dataLoaded) {
                // Load 256-bits for each phase
                streamLoadAndDeinterleave<TP_CASC_IN, TT_DATA, TP_DUAL_IP, kParallelPhases>(
                    sbuffArray, inInterface, numDataLoads, kDataMappedToPhaseOffset);
                dataLoaded += m_kStreamLoadVsize;
                numDataLoads++;
            }

            // Read cascade input. Do nothing if cascade input not present.
            acc[0] = readCascade(inInterface, acc[0]);
            for (int phase = 0; phase < kParallelPhases; ++phase) {
                xstart = initDataOffset - CEIL(phase + TP_MODIFY_MARGIN_OFFSET, kParallelPhases) / kParallelPhases + 1 +
                         (streamInitNullAccs + strobe) * m_kVOutSize;
                acc[0] = sr_asym::macSrAsym<TT_DATA, TT_COEFF>(acc[0], sbuffArray[phase], xstart, coe[phase], 0);
            }
#pragma unroll(GUARD_ZERO(((kFirLenCeilCols / (m_kColumns)) - 1)))
            for (int op = m_kColumns; op < kFirLenCeilCols; op += m_kColumns) {
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    if ((op) % m_kCoeffRegVsize == 0) {
                        chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                            ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)] +
                             (op) / m_kCoeffRegVsize);
                        coe[phase] = *coeff;
                    }
                    // xstart = CEIL(m_kDataBuffXOffset, kParallelPhases)/kParallelPhases + op;
                    xstart = initDataOffset - CEIL(phase + TP_MODIFY_MARGIN_OFFSET, kParallelPhases) / kParallelPhases +
                             1 + (streamInitNullAccs + strobe) * m_kVOutSize + op;
                    // xstart = CEIL(m_kDataBuffXOffset, kParallelPhases)/kParallelPhases - phase +
                    // kDataMappedToPhasestartOffset[phase] + op;
                    acc[0] =
                        sr_asym::macSrAsym(acc[0], sbuffArray[phase], xstart, coe[phase], ((op) % m_kCoeffRegVsize));
                }
            }

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[0]);
            outVal = shiftAndSaturate(acc[0], TP_SHIFT);

            writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase++ % 2);
        }
        if
            constexpr(streamInitNullAccs == 0) { loopSize -= 1; }
        else {
            loopSize -= CEIL(streamInitNullAccs, streamRptFactor) / streamRptFactor;
        }
    }
    //}
    doInit = 0;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < loopSize; i++)
        chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(4)
            chess_loop_range((m_kLsize / streamRptFactor) - 1, (m_kLsize / streamRptFactor)) {
            numDataLoads = startDataLoads;
            dataOffset = startDataOffset;
            dataLoaded = 0;
            dataNeeded = 0;
            inDataLoadPhase = streamInitAccs;
// unroll streamRptFactor times
#pragma unroll(streamRptFactor)
            for (unsigned strobe = 0; strobe < streamRptFactor; strobe++) {
#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    T_buff_256b<TT_COEFF>* coeff =
                        ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)]);
                    coe[phase] = *coeff;
                }

                dataNeeded += m_kVOutSize;
                if (dataNeeded > dataLoaded) {
                    streamLoadAndDeinterleave<TP_CASC_IN, TT_DATA, TP_DUAL_IP, kParallelPhases>(
                        sbuffArray, inInterface, numDataLoads, kDataMappedToPhaseOffset);
                    dataLoaded += m_kStreamLoadVsize;
                    numDataLoads++;
                }

                // Read cascade input. Do nothing if cascade input not present.
                acc[0] = readCascade(inInterface, acc[0]);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                    xstart = dataOffset - CEIL(phase + TP_MODIFY_MARGIN_OFFSET, kParallelPhases) / kParallelPhases + 1 +
                             (streamInitNullAccs + strobe) * m_kVOutSize;
                    acc[0] = sr_asym::macSrAsym<TT_DATA, TT_COEFF>(acc[0], sbuffArray[phase], xstart, coe[phase], 0);
                }

#pragma unroll(GUARD_ZERO((kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < kFirLenCeilCols; op += m_kColumns) {
                    for (int phase = 0; phase < kParallelPhases; ++phase) {
                        if ((op) % m_kCoeffRegVsize == 0) {
                            chess_protect_access T_buff_256b<TT_COEFF>* coeff =
                                ((T_buff_256b<TT_COEFF>*)m_internalTaps2[fnCoeffPhase(phase, kParallelPhases)] +
                                 (op) / m_kCoeffRegVsize);
                            coe[phase] = *coeff;
                        }
                        xstart = dataOffset - CEIL(phase + TP_MODIFY_MARGIN_OFFSET, kParallelPhases) / kParallelPhases +
                                 1 + (streamInitNullAccs + strobe) * m_kVOutSize + op;
                        acc[0] = sr_asym::macSrAsym(acc[0], sbuffArray[phase], xstart, coe[phase],
                                                    ((op) % m_kCoeffRegVsize));
                    }
                }

                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc[0]);
#pragma unroll(kParallelPhases)
                for (int phase = 0; phase < kParallelPhases; ++phase) {
                }

#pragma unroll(kParallelPhases - 1)
                for (int phase = 1; phase < kParallelPhases; ++phase) {
                    // acc[0].val = acc[0].val + acc[phase].val;
                }
                outVal = shiftAndSaturateDecAsym(acc[0], TP_SHIFT);

                // Write to output
                writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, outDataPhase % 2);
                outDataPhase++;
            }
        }

    doInit = 0;

    ptr_delay = (T_buff_1024b<TT_DATA>*)delay;
// store sbuffs for next iteration
#pragma unroll(kParallelPhases)
    for (int phase = 0; phase < kParallelPhases; ++phase) {
        *ptr_delay++ = sbuffArray[phase];
    }
};

// Single kernel base specialization. Windowed. No cascade ports. Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_DECIMATE_FACTOR,
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

// Single kernel specialization. Windowed. No cascade ports. Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_DECIMATE_FACTOR,
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
    USE_WINDOW_API,
    TP_MODIFY_MARGIN_OFFSET,
    TP_COEFF_PHASE,
    TP_COEFF_PHASE_OFFSET,
    TP_COEFF_PHASES,
    TP_COEFF_PHASES_LEN,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_DECIMATE_FACTOR,
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
    USE_WINDOW_API,
    TP_MODIFY_MARGIN_OFFSET,
    TP_COEFF_PHASE,
    TP_COEFF_PHASE_OFFSET,
    TP_COEFF_PHASES,
    TP_COEFF_PHASES_LEN,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_DECIMATE_FACTOR,
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
    USE_WINDOW_API,
    TP_MODIFY_MARGIN_OFFSET,
    TP_COEFF_PHASE,
    TP_COEFF_PHASE_OFFSET,
    TP_COEFF_PHASES,
    TP_COEFF_PHASES_LEN,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                       input_stream_cacc48* inCascade,
                                       output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), no reload, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                       input_stream_cacc48* inCascade,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                       input_stream_cacc48* inCascade,
                                       output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                                       input_stream_cacc48* inCascade,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                             extents<inherited_extent>,
                                                             margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                       output_stream_cacc48* outCascade,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                             extents<inherited_extent>,
                                                             margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                       output_stream_cacc48* outCascade,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                       input_stream_cacc48* inCascade,
                                       output_stream_cacc48* outCascade,
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
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                       input_stream_cacc48* inCascade,
                                       output_stream_cacc48* outCascade,
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

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// Single kernel, Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for the first kernel in a cascade chain.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                       input_stream_cacc48* inCascade,
                                       output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for the first kernel in a cascade chain.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                       output_stream_cacc48* outCascade,
                                       const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// Single kernel, Static coefficients, dual input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       DUAL_IP_DUAL,
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

// Single kernel, Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       DUAL_IP_DUAL,
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

// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for the first kernel in a cascade chain.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
                                       input_stream<TT_DATA>* inStream2,
                                       output_stream_cacc48* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inStream = inStream;
    inInterface.inStream2 = inStream2;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       DUAL_IP_DUAL,
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
// This is a specialization of the main class for the final kernel in a cascade chain.
// Static coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for the final kernel in a cascade chain.
// Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       DUAL_IP_DUAL,
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for the first kernel in a cascade chain.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
void fir_decimate_asym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_STREAM_API,
                       TP_MODIFY_MARGIN_OFFSET,
                       TP_COEFF_PHASE,
                       TP_COEFF_PHASE_OFFSET,
                       TP_COEFF_PHASES,
                       TP_COEFF_PHASES_LEN,
                       TP_SAT>::filter(input_stream<TT_DATA>* inStream,
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
} // namespaces
