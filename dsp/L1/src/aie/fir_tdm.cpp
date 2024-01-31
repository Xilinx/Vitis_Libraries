/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifdef __X86SIM__
#define _DSPLIB_FIR_TDM_HPP_DEBUG_
#endif
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "fir_tdm.hpp"
#include "kernel_api_utils.hpp"
#include "fir_tdm_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

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
          unsigned int TP_TDM_CHANNELS,
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
                                   TP_TDM_CHANNELS,
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
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>(), TP_API,
                    TP_DUAL_IP, m_kKernelPosEnum>(inInterface, outInterface);

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
          unsigned int TP_TDM_CHANNELS,
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
                                   TP_TDM_CHANNELS,
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
    filterBasic(inInterface, outInterface);
};
// #undef _DSPLIB_FIR_TDM_HPP_DEBUG_

// ----------------------------------------------------- Basic ----------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
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
                                   TP_TDM_CHANNELS,
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
    TT_COEFF* __restrict coeffPtr = &m_internalTaps[0];
    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVectData>;
    using outDataVect_t = ::aie::vector<TT_DATA, kSamplesInVectAcc>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVectCoeff>;

    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVectAcc>;
    // using mul = ::aie::mul<typename tAccBaseType<TT_DATA, TT_COEFF>::type, dataVect_t, coeffVect_t>;

    dataVect_t dataVect;
    dataVect_t* inPointer;
    outDataVect_t outVect, outVect2;
    coeffVect_t* coeffVectPtr;

    coeffVect_t coeffVect, coeffVect2;
    accVect_t acc, acc2;
    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
    outDataVect_t* outPtr = (outDataVect_t*)outInterface.outWindowPtr;
    // #undef _DSPLIB_FIR_TDM_HPP_DEBUG_

    // inPointer = frameStart;
    // Loop through each frame
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            // pointer to a vector of coefficients
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            //             coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
            coeffVectPtr = (coeffVect_t*)(coeffPtr);
            for (unsigned int tdm = 0; tdm < TP_TDM_CHANNELS / m_kVOutSize; tdm++) {
                // FIR legnth

                // always load data.
                inPointer = ((dataVect_t*)frameStart) + tdm + (frame * TP_TDM_CHANNELS / kSamplesInVectAcc) +
                            (0 * TP_TDM_CHANNELS / kSamplesInVectAcc);
                dataVect = *inPointer;

                // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                // Each load is 256 bit? so schedule load every single or every second iteration.
                // Alternatively, cover the difference with different loads, e.g. 256-bit load and a 128-bit load and
                // schedule both on each iteration.
                if (0 % (dataToCoeffSizeRatio) == 0) {
                    coeffVect = *coeffVectPtr++;
                }
                // acc = ::aie::mul(dataVect, coeffVect);
                acc = mulTdm(acc, dataVect, coeffVect);

#pragma unroll(TP_FIR_RANGE_LEN / dataToCoeffSizeRatio)
                for (unsigned int j = 1; j < (TP_FIR_RANGE_LEN / dataToCoeffSizeRatio); j++) {
                    // always load 256-bits of data.
                    inPointer = ((dataVect_t*)frameStart) + tdm + (frame * TP_TDM_CHANNELS / kSamplesInVectAcc) +
                                (j * TP_TDM_CHANNELS / kSamplesInVectAcc);
                    dataVect = *inPointer;
                    // or load twice
                    // if dataToCoeffSizeRatio == 2 {
                    //     inPointer = ((dataVect_t*)frameStart) + tdm +
                    //         (frame * TP_TDM_CHANNELS/kSamplesInVectAcc) + (j * TP_TDM_CHANNELS/kSamplesInVectAcc);
                    //     dataVect = *inPointer;
                    // }

                    // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                    // Each load is 256 bit? so schedule load every single or every second iteration.
                    // Alternatively, cover the difference with different loads, e.g. 256-bit load and a 128-bit load
                    // and schedule both on each iteration.
                    coeffVect = *coeffVectPtr++;
                    // if ( j % (dataToCoeffSizeRatio) == 0) {
                    //     coeffVect = *coeffVectPtr++;
                    // }
                    // acc = ::aie::mac(acc, dataVect, coeffVect);
                    acc = macTdm(acc, dataVect, coeffVect);
                }

                // output to outVect or to outCascade
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(std::is_same<TT_COEFF, cfloat>()) {
                                outVect = acc.template to_vector<TT_DATA>();
                                // outVect2 = acc2.template to_vector<TT_DATA>();
                            }
                        else {
                            outVect = acc.template to_vector<TT_DATA>(0);
                            // outVect2 = acc2.template to_vector<TT_DATA>(shift);
                        }
                        *outPtr++ = outVect;
                        // *outPtr++ = outVect2;
                    }
                else {
                    writeincr(outInterface.outCascade, acc);
                    // writeincr(outInterface.outCascade, acc2);
                }
                // Jump to data of next frame
                inPointer -= ((TP_TDM_CHANNELS * TP_FIR_LEN) - m_kVOutSize);
            }
        }
};

// // ----------------------------------------------------- Basic -----------------------------------------------------
// //
// template <typename TT_DATA,
//           typename TT_COEFF,
//           unsigned int TP_FIR_LEN,
//           unsigned int TP_SHIFT,
//           unsigned int TP_RND,
//           unsigned int TP_INPUT_WINDOW_VSIZE,
//           unsigned int TP_TDM_CHANNELS,
//           bool TP_CASC_IN,
//           bool TP_CASC_OUT,
//           unsigned int TP_FIR_RANGE_LEN,
//           unsigned int TP_KERNEL_POSITION,
//           unsigned int TP_CASC_LEN,
//           unsigned int TP_USE_COEFF_RELOAD,
//           unsigned int TP_NUM_OUTPUTS,
//           unsigned int TP_DUAL_IP,
//           unsigned int TP_API,
//           int TP_MODIFY_MARGIN_OFFSET,
//           unsigned int TP_COEFF_PHASE,
//           unsigned int TP_COEFF_PHASE_OFFSET,
//           unsigned int TP_COEFF_PHASES,
//           unsigned int TP_COEFF_PHASES_LEN,
//           unsigned int TP_SAT>
// INLINE_DECL void kernelFilterClass<TT_DATA,
//                                    TT_COEFF,
//                                    TP_FIR_LEN,
//                                    TP_SHIFT,
//                                    TP_RND,
//                                    TP_INPUT_WINDOW_VSIZE,
//                                    TP_TDM_CHANNELS,
//                                    TP_CASC_IN,
//                                    TP_CASC_OUT,
//                                    TP_FIR_RANGE_LEN,
//                                    TP_KERNEL_POSITION,
//                                    TP_CASC_LEN,
//                                    TP_USE_COEFF_RELOAD,
//                                    TP_NUM_OUTPUTS,
//                                    TP_DUAL_IP,
//                                    TP_API,
//                                    TP_MODIFY_MARGIN_OFFSET,
//                                    TP_COEFF_PHASE,
//                                    TP_COEFF_PHASE_OFFSET,
//                                    TP_COEFF_PHASES,
//                                    TP_COEFF_PHASES_LEN,
//                                    TP_SAT>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
//                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
//     set_rnd_mode<TP_RND>();
//     set_sat_mode<TP_SAT>();
//     T_buff_256b<TT_COEFF>* __restrict coeff;
//     coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
//     T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
//     T_buff_1024b<TT_DATA> sbuff; // input data value cache.
//     T_acc<TT_DATA, TT_COEFF> acc;
//     T_outVal<TT_DATA, TT_COEFF> outVal;
//     unsigned int dataLoaded, numDataLoads;
//     unsigned int dataNeeded;
//     unsigned int dataLoadPhase = 0;

//     auto inItr = (TP_API == USE_WINDOW_API && TP_CASC_IN == true && (TP_KERNEL_POSITION != 0))
//                      ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
//                      : ::aie::begin_random_circular(*(inInterface.inWindowCirc)); // output size can be less than
//                      data
//                                                                                   // load size, so this iterator
//                                                                                   needs
//                                                                                   // greater precision to reach finer
//                                                                                   // addressed.

//     constexpr bool hasOutWindow = (TP_API == 0 && TP_CASC_OUT == false);
//     constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_CASC_OUT == false);
//     auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
//     auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

//     // Move data pointer away from data consumed by previous cascades
//     // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
//     // window_incr(inInterface.inWindow,(TRUNC((m_kFirInitOffset),(16/sizeof(TT_DATA)))));
//     inItr += TRUNC((m_kFirInitOffset), m_kOffsetResolution);

//     TT_COEFF* m_internalTapsCopy = m_internalTaps;

//     // This loop creates the output window data. In each iteration a vector of samples is output
//     for (unsigned i = 0; i < m_kLsize; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize, ) {
//             m_internalTapsCopy = chess_copy(m_internalTapsCopy);
//             coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);

//             coe0 = *coeff;
//             coeff++;

//             numDataLoads = 0;
//             dataLoaded = 0;

//             // Preamble, calculate and load data from window into register
//             dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;

// #pragma unroll(m_kInitialLoads)
//             for (int initLoads = 0; initLoads < m_kInitialLoads; ++initLoads) {
//                 upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads,
//                                            inItr); // Update sbuff with data from input window. 00++|____|____|____
//                 dataLoaded += m_kDataLoadVsize;
//                 numDataLoads++;
//             }
//             // Read cascade input. Do nothing if cascade input not present.
//             acc = readCascade(inInterface, acc);
//             // Init Vector operation. VMUL if cascade not present, otherwise VMAC
//             acc = initMacSrAsym<TT_DATA, TT_COEFF>(inInterface, acc, sbuff, m_kDataBuffXOffset, coe0, 0);
// #pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
//             for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
//                 dataNeeded += m_kColumns;
//                 if (dataNeeded > dataLoaded) {
//                     upd_win_incr_256b<TT_DATA>(sbuff, numDataLoads % m_kDataLoadsInReg, inItr);
//                     dataLoaded += m_kDataLoadVsize;
//                     numDataLoads++;
//                 }
//                 if (op % m_kCoeffRegVsize == 0) {
//                     coe0 = *coeff++;
//                 }
//                 acc = macSrAsym(acc, sbuff, (op + m_kDataBuffXOffset), coe0, (op % m_kCoeffRegVsize));
//             }

//             inItr -= m_kDataLoadVsize * numDataLoads - m_kVOutSize;

//             // Write cascade. Do nothing if cascade not present.
//             writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

//             outVal = shiftAndSaturate(acc, TP_SHIFT);
//             // Write to output window
//             writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
//             outItr);
//             if
//                 constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
//                     writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase++ % 2,
//                                                                            outItr2);
//         }
// };

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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
           output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    constexpr int kdummy = 16;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindowPtr = outWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    constexpr int kdummy = 16;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindowPtr = outWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindowPtr = outWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    filterFirstKernel(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindowPtr = outWindow.data();
    this->filterKernel(inInterface, outInterface);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
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
          unsigned int TP_TDM_CHANNELS,
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
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
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
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade) {
    constexpr int kdummy = 16;
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
}
}
}
}
}
}
