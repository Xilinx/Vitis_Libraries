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
Symmetric Decimation FIR kernal code.
This file captures the body of run-time code for the kernal class and a higher wrapping 'cascade' layer which has
specializations for
 combinations of inputs and outputs. That is, in a chain of kernels, the first will have an input window, and a cascade
out stream.
 The next, potentially multiple, kernel(s) will each have an input window and cascade stream and will output a cascade
steam. The final kernel
 will have an input window and cascade stream and an output window only.
 The cascade layer class is called fir_interpolate_hb with the kernel-layer (operational) class called
kernelFilterClass.
 The fir_interpolate_hb class has a member of the kernelFilterClass.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#ifndef L_BUFFER
#define L_BUFFER xa
#endif
#ifndef R_BUFFER
#define R_BUFFER xb
#endif
#ifndef Y_BUFFER
#define Y_BUFFER ya
#endif
#ifndef Z_BUFFER
#define Z_BUFFER wc0
#endif

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
#include "kernel_api_utils.hpp"
#include "fir_decimate_sym.hpp"
#include "fir_decimate_sym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {

// Symmetrical Decimation FIR run-time function
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
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

// Symmetrical Decimation FIR run-time function.
// According to template parameter the input may be a window, or window and cascade input.
// Similarly the output interface may be a window or a cascade output.
// RTP read from input port.
// Only used in RTP designs within kernel[0]

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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
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
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                                                         const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
                                                                                                      outInterface);
    m_coeffnEq = rtpCompare(inTaps, m_oldInTaps);

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload(inTaps, m_oldInTaps, outInterface);
        firReload(inTaps);
    }
    filterSelectArch(inInterface, outInterface);
};

// Symmetrical Decimation FIR run-time function
// According to template parameter the input may be a window, or window and cascade input.
// Similarly the output interface may be a window or a cascade output.
// RTP read from cascade input.
// Only used in RTP designs within cascade chain - for kernels [1] to [TP_CASC_LEN-1].
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
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
        bufferReload<TT_DATA, TT_COEFF, (TP_FIR_LEN + 1) / kSymmetryFactor, TP_DUAL_IP>(inInterface, m_oldInTaps,
                                                                                        outInterface);
        firReload(m_oldInTaps);
    }
    filterSelectArch(inInterface, outInterface);
};

// Symmetrical Decimation FIR run-time function
// Select architecture and execute FIR function.
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
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
                                   TP_DUAL_IP,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // windowAcquire(inInterface);
    if
        constexpr(m_kArch == kArch1BuffLowDFBasic) { filter1BuffLowDFBasic(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArch1BuffLowDFIncrStrobe) { filter1BuffLowDFIncrStrobe(inInterface, outInterface); }
    else if
        constexpr(m_kArch == kArch2BuffLowDFBasic) { filter2BuffLowDFBasic(inInterface, outInterface); }
    // windowRelease(inInterface);
};

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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void
kernelFilterClass<TT_DATA,
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
                  TP_DUAL_IP,
                  TP_USE_COEFF_RELOAD,
                  TP_NUM_OUTPUTS,
                  TP_API,
                  TP_SAT>::filter1BuffLowDFBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    static constexpr unsigned int m_kInitialLoads1Buff =
        CEIL(m_kDataBuffXOffset + m_kArchFirLen + TP_DECIMATE_FACTOR * (m_kLanes - 1), m_kDataLoadVsize) /
        m_kDataLoadVsize;

    T_buff_256b<TT_COEFF>* __restrict coeff;
    coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> xbuff; // input data value cache.
    T_accDecSym<TT_DATA, TT_COEFF> acc;
    T_outValDecSym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for xbuff
    unsigned int initLoadPtr, dataLoadPtr, dataLoadPtrOp;
    unsigned int dataLoaded, dataNeeded;
    unsigned int xstart, ystart;
    unsigned int lastXstart, lastYstart;
    unsigned int ct;
    using buf_type = typename T_buff_1024b<TT_DATA>::v_type;
    buf_type chess_storage(Y_BUFFER) xbuffTmp;

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
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += (TRUNC((m_kFirInitOffset), (16 / sizeof(TT_DATA))));

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / 2; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize / 2, ) {
// The total loop size is unrolled by a factor of 2. This is useful for performance when TP_API=1 and TP_NUM_OUTPUTS=2.
// The loop index - DataLoadPhase is used as a parameter to the writeOutput function,
// to index the output stream to which we write the output data.
#pragma unroll(2)
            for (unsigned dataLoadPhase = 0; dataLoadPhase < 2; dataLoadPhase++) {
                initLoadPtr = 0;
                //#pragma unroll (m_kInitialLoads1Buff)
                for (int initLoads = 0; initLoads < m_kInitialLoads1Buff; ++initLoads) {
                    read256Ptr = (t_256vect*)&*inItr;
                    inItr += k256Vsize;
                    readData.val = *read256Ptr;
                    xbuffTmp = upd_w(xbuff.val, initLoadPtr % m_kDataLoadsInReg,
                                     readData.val); // Update xbuff with data from input window. 00++|____|____|____
                    xbuff.val = xbuffTmp;
                    dataLoaded += m_kDataLoadVsize;
                    initLoadPtr++;
                }

                // Preamble, calc number of samples for first mul.
                dataLoadPtr = m_kInitialLoads1Buff % m_kDataLoadsInReg;
                dataLoaded = 0;
                dataNeeded = m_kDataBuffXOffset + m_kVOutSize + m_kColumns - 1;

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff;
                coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                xstart = m_kDataBuffXOffset;
                ystart = m_kDataBuffXOffset + (TP_FIR_LEN - 1) - 2 * m_kFirRangeOffset;
                if (m_kNumOps == 1 and TP_FIR_RANGE_LEN % 2 == 1) {
                    lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    acc = initMacDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN, TP_DECIMATE_FACTOR, TP_DUAL_IP>(
                        inInterface, acc, xbuff, lastXstart, lastYstart, (m_kColumns - 1), coe0, 0, m_kDecimateOffsets);
                } else {
                    acc = initMacDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN, TP_DECIMATE_FACTOR, TP_DUAL_IP>(
                        inInterface, acc, xbuff, xstart, ystart, coe0, 0, m_kDecimateOffsets);
                }
                dataLoadPtrOp = dataLoadPtr; // dataLoadPtr;
                dataLoadPtr = (dataLoadPtr + 1) % m_kDataLoadsInReg;
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    xstart += m_kColumns;
                    ystart -= m_kColumns;
                    if (op == m_kFirLenCeilCols - m_kColumns && TP_FIR_RANGE_LEN % 2 == 1) {
                        lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        acc = macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, lastXstart, lastYstart, (m_kColumns - 1), coe0, (op % m_kCoeffRegVsize),
                            m_kDecimateOffsets);
                    } else {
                        acc = macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, xstart, ystart, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets);
                    }
                }
                // Go back by the number of input samples loaded minus  (i.e forward) by the number of samples consumed
                inItr -= (m_kDataLoadVsize * m_kInitialLoads1Buff -
                          m_kVOutSize * TP_DECIMATE_FACTOR); // return read pointer to start of next chunk of window.
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturateDecSym(acc, TP_SHIFT);
                // Write to output window
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2,
                                                                               outItr2);
                    }

                // writeWindow<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal);
            }
        } // for i
};

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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void
kernelFilterClass<TT_DATA,
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
                  TP_DUAL_IP,
                  TP_USE_COEFF_RELOAD,
                  TP_NUM_OUTPUTS,
                  TP_API,
                  TP_SAT>::filter1BuffLowDFIncrStrobe(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // In the incremental implementation, use is made of the fact that the data samples required for one set of outputs
    // overlap with the samples required
    // for the next set of outputs. e.g, the first set of data required could be D(0) to D(FIR_LEN-1+numLanes). The
    // second could be D(numLanes) to D(FIR_LEN-1 +2*numLanes)
    // so only samples from D(FIR_LEN-1 +numLanes+1) to D(FIR_LEN-1 +2*numLanes) need be loaded.
    // So as to allow the strobe loop to be unrolled, the number of data samples loaded initially must be the same as
    // the useful samples left at the end of the strobe loop.
    // otherwise the first time round the strobe loop will differ from subsequent rounds and the loop will not unroll as
    // desired.
    static constexpr unsigned int m_kLoadSize = fnLoadSizeDecSym<TT_DATA, TT_COEFF, m_kArch>();
    static constexpr unsigned int m_kInitDataNeeded =
        m_kDataBuffXOffset + TP_FIR_LEN - 2 * m_kFirRangeOffset + TP_DECIMATE_FACTOR * (m_kLanes - 1);
    static constexpr unsigned int m_kInitialLoads1Buff =
        CEIL(m_kInitDataNeeded - (TP_DECIMATE_FACTOR * m_kVOutSize), m_kDataLoadVsize) / m_kDataLoadVsize; //

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0;  // register for coeff values.
    T_buff_1024b<TT_DATA> xbuff; // input data value cache.
    T_accDecSym<TT_DATA, TT_COEFF> acc;
    T_outValDecSym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for xbuff
    unsigned int initLoadPtr, dataLoadPtr;
    unsigned int initDataLoaded, initDataNeeded;
    unsigned int dataLoaded, dataNeeded;
    unsigned int xstart, ystart;
    unsigned int lastXstart, lastYstart;
    unsigned int ct;

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
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += (TRUNC((m_kFirInitOffset), (16 / sizeof(TT_DATA))));

    initLoadPtr = 0;
    // Preamble, calc number of samples for first mul.
    dataLoadPtr = m_kInitialLoads1Buff % m_kDataLoadsInReg;
    initDataLoaded = 0;
    initDataNeeded = m_kInitDataNeeded - TP_DECIMATE_FACTOR * m_kVOutSize;
#pragma unroll(m_kInitialLoads1Buff)
    for (int initLoads = 0; initLoads < m_kInitialLoads1Buff; ++initLoads) {
        fnLoadXIpData<TT_DATA, TT_COEFF, m_kLoadSize>(xbuff, initLoadPtr, inItr);
        initDataLoaded += m_kDataLoadVsize;
        initLoadPtr++;
    }
    dataLoadPtr = initLoadPtr;

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / m_kRepeatFactor; i++)
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kRepeatFactor, ) {
            dataNeeded = initDataNeeded;
            dataLoaded = initDataLoaded;
            dataLoadPtr = initLoadPtr;
// The strobe loop is the number of iterations of Lsize required for conditions to return to the same state (typically 4
// due to the ratio of
// load size(256b) to xbuff size(1024b))
#pragma unroll(m_kRepeatFactor)
            for (int strobe = 0; strobe < m_kRepeatFactor; strobe++) {
                dataNeeded += TP_DECIMATE_FACTOR * m_kVOutSize;
                // it might take more than one load to top up the buffer of input data to satisfy the need for the next
                // vector of outputs
                if (dataNeeded > dataLoaded) {
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kLoadSize>(xbuff, dataLoadPtr, inItr);
                    dataLoaded += m_kDataLoadVsize;
                    dataLoadPtr = (dataLoadPtr + 1) % m_kDataLoadsInReg;
                    if (dataNeeded > dataLoaded) {
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kLoadSize>(xbuff, dataLoadPtr, inItr);
                        dataLoaded += m_kDataLoadVsize;
                        dataLoadPtr = (dataLoadPtr + 1) % m_kDataLoadsInReg;
                    }
                }

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff;
                coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF, TP_DUAL_IP>(inInterface, acc);
                xstart = (m_kDataBuffXOffset + strobe * TP_DECIMATE_FACTOR * m_kLanes) % m_kSamplesInDataBuff;
                ystart = (m_kDataBuffXOffset + strobe * TP_DECIMATE_FACTOR * m_kLanes + (TP_FIR_LEN - 1) -
                          2 * m_kFirRangeOffset) %
                         m_kSamplesInDataBuff;
                if (m_kNumOps == 1 and TP_FIR_RANGE_LEN % 2 == 1) {
                    // Condition only met for cascaded designs. Single kernel designs quickly grow beyond m_kNumOps=1.
                    lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    acc = initMacDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN, TP_DECIMATE_FACTOR>(
                        inInterface, acc, xbuff, lastXstart, lastYstart, (m_kColumns - 1), coe0, 0, m_kDecimateOffsets);
                } else {
                    acc = initMacDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN, TP_DECIMATE_FACTOR>(
                        inInterface, acc, xbuff, xstart, ystart, coe0, 0, m_kDecimateOffsets);
                }
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    xstart += m_kColumns;
                    ystart -= m_kColumns;
                    if (op == m_kFirLenCeilCols - m_kColumns && TP_FIR_RANGE_LEN % 2 == 1) {
                        lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        acc = macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, lastXstart, lastYstart, (m_kColumns - 1), coe0, (op % m_kCoeffRegVsize),
                            m_kDecimateOffsets);
                    } else {
                        acc = macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, xstart, ystart, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets);
                    }
                }
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturateDecSym(acc, TP_SHIFT);
                // Write to output window
                // writeWindow<TT_DATA,TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, strobe % 2, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, strobe % 2,
                                                                               outItr2);
                    }

            } // for strobe
        }     // for i
};

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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
INLINE_DECL void
kernelFilterClass<TT_DATA,
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
                  TP_DUAL_IP,
                  TP_USE_COEFF_RELOAD,
                  TP_NUM_OUTPUTS,
                  TP_API,
                  TP_SAT>::filter2BuffLowDFBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    static constexpr unsigned int m_kDataReadGranByte = 16; // Data reads occur on 16 byte (128 bit boundary)
    static constexpr unsigned int m_kDataWindowOffset =
        TRUNC((m_kFirInitOffset), (m_kDataReadGranByte / sizeof(TT_DATA)));
    // static constexpr unsigned int m_kFirstYdata        = (m_kFirInitOffset + (TP_FIR_LEN-1)-(m_kColumns-1));
    static constexpr unsigned int m_kFirstYdata =
        (m_kFirMarginOffset + (TP_FIR_LEN - 1) - (m_kColumns - 1)) - m_kFirRangeOffset;
    static constexpr unsigned int m_kySpliceStart =
        ((m_kFirstYdata / (m_kDataReadGranByte / sizeof(TT_DATA)))) * (m_kDataReadGranByte / sizeof(TT_DATA));
    static constexpr unsigned int m_kFirstYdataOffset = m_kFirstYdata - m_kySpliceStart;
    static constexpr unsigned int m_kyStart = m_kFirstYdata + (m_kColumns - 1) - m_kySpliceStart;
    static constexpr unsigned int m_kLoadSize = fnLoadSizeDecSym<TT_DATA, TT_COEFF, m_kArch>();
    static constexpr unsigned int m_kInitialLoadsX =
        CEIL(m_kDataBuffXOffset + TP_DECIMATE_FACTOR * (m_kLanes - 1) + 1 + m_kColumns - 1, m_kDataLoadVsize) /
        m_kDataLoadVsize;
    static constexpr unsigned int m_kYBuffCeil =
        m_kLanes > m_kDataLoadVsize ? m_kLanes : m_kDataLoadVsize; // ceil to bigger of the two
    static constexpr unsigned int m_kInitialLoadsY =
        CEIL(m_kyStart + TP_DECIMATE_FACTOR * (m_kLanes - 1), m_kYBuffCeil) / m_kDataLoadVsize;

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_512b<TT_DATA> xbuff; // input data value cache.
    T_buff_512b<TT_DATA> ybuff; // input data value cache.
    T_accDecSym<TT_DATA, TT_COEFF> acc;
    T_outValDecSym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readData; // input data read from window, bound for xbuff, or ybuff
    unsigned int initLoadsX, initLoadsY;
    unsigned int dataLoadPtrX, dataLoadPtrOpX;
    unsigned int dataLoadPtrY, dataLoadPtrOpY;
    unsigned int dataLoadedX, dataNeededX;
    unsigned int dataLoadedY, dataNeededY;
    unsigned int dataLoadsX, dataLoadsY; // a tally to help rewind the windows.
    unsigned int xstart, ystart;
    unsigned int lastXstart, lastYstart;
    unsigned int ct; // centre tap position (which column)

    TT_COEFF* m_internalTapsCopy = m_internalTaps;
    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    auto inItr = fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                     ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                     : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    auto yinItr = (TP_DUAL_IP == 0 || TP_API == 1)
                      ? fnBuffIsLin<IS_SYM, TP_DUAL_IP, TP_API, TP_KERNEL_POSITION>()
                            ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                            : ::aie::begin_random_circular(*(inInterface.inWindowCirc))
                      : ::aie::begin_random_circular(*(inInterface.inWindowReverse));

    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*outInterface.outWindow2);

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit. Cascade phase remainder goes to m_kDataBuffXOffset
    inItr += m_kDataWindowOffset;
    yinItr += m_kySpliceStart +
              m_kDataLoadVsize * (m_kInitialLoadsY - 1); // Fast forward to 128b boundary containing first Y data

    // This loop creates the output window data. In each iteration a vector of samples is output
    for (unsigned i = 0; i < m_kLsize / 2; i++) chess_prepare_for_pipelining chess_loop_range(m_kLsize / 2, ) {
// The total loop size is unrolled by a factor of 2. This is useful for performance when TP_API=1 and TP_NUM_OUTPUTS=2.
// The loop index - DataLoadPhase is used as a parameter to the writeOutput function,
// to index the output stream to which we write the output data.
#pragma unroll(2)
            for (unsigned dataLoadPhase = 0; dataLoadPhase < 2; dataLoadPhase++) {
#pragma unroll(m_kInitialLoadsX)
                for (unsigned int initLoadsX = 0; initLoadsX < m_kInitialLoadsX; ++initLoadsX) {
                    fnLoadXIpData<TT_DATA, TT_COEFF, m_kLoadSize>(xbuff, initLoadsX, inItr);
                }
// preload ydata from window into ybuff register. Loading backwards initially leaves window pointer in the correct place
// without adjustment
#pragma unroll(m_kInitialLoadsY)
                for (int initLoadsY = m_kInitialLoadsY - 1; initLoadsY >= 0; --initLoadsY) {
                    fnLoadYIpData<TT_DATA, TT_COEFF, m_kLoadSize>(ybuff, initLoadsY, yinItr);
                }

                // Preamble, calc number of samples for first mul.
                dataLoadPtrX = m_kInitialLoadsX % m_kDataLoadsInReg;
                dataLoadPtrY = m_kDataLoadsInReg - 1; // Y loads go backwards, from 0, so loop round.
                dataLoadedX = m_kInitialLoadsX * m_kDataLoadVsize;
                dataLoadedY = m_kFirstYdata - m_kySpliceStart;
                dataNeededX = m_kDataBuffXOffset + ((m_kLanes - 1) * TP_DECIMATE_FACTOR) + 1 + m_kColumns - 1;
                dataNeededY = 0;
                dataLoadsX = m_kInitialLoadsX;
                dataLoadsY = m_kInitialLoadsY;

                m_internalTapsCopy = chess_copy(m_internalTapsCopy);
                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
                coe0 = *coeff;
                coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF, TP_DUAL_IP>(inInterface, acc);
                // Short FIR with only 1 operation with CT may be the last kernel in the cascaded design
                if (m_kNumOps == 1 and TP_FIR_RANGE_LEN % 2 == 1) {
                    xstart = m_kDataBuffXOffset;
                    ystart = m_kyStart;
                    lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    ct = m_kColumns - 1;
                    acc = initMacDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR, TP_DUAL_IP>(
                        inInterface, acc, xbuff, lastXstart, ybuff, lastYstart, ct, coe0, 0, m_kDecimateOffsets);
                } else {
                    xstart = m_kDataBuffXOffset;
                    ystart = m_kyStart;
                    acc = initMacDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR, TP_DUAL_IP>(
                        inInterface, acc, xbuff, xstart, ybuff, ystart, coe0, 0, m_kDecimateOffsets);
                }
#pragma unroll(GUARD_ZERO((m_kFirLenCeilCols / (m_kColumns) - 1)))
                for (int op = m_kColumns; op < m_kFirLenCeilCols; op += m_kColumns) {
                    dataNeededX += m_kColumns;
                    dataNeededY += m_kColumns;
                    if (dataNeededX > dataLoadedX) {
                        fnLoadXIpData<TT_DATA, TT_COEFF, m_kLoadSize>(xbuff, dataLoadPtrX, inItr);
                        dataLoadedX += m_kDataLoadVsize;
                        dataLoadPtrX = (dataLoadPtrX + 1) % m_kDataLoadsInReg;
                        dataLoadsX++;
                    }
                    if (dataNeededY > dataLoadedY) {
                        fnLoadYIpData<TT_DATA, TT_COEFF, m_kLoadSize>(ybuff, dataLoadPtrY, yinItr);
                        dataLoadedY += m_kDataLoadVsize;
                        dataLoadPtrY = (dataLoadPtrY + m_kDataLoadsInReg - 1) % m_kDataLoadsInReg;
                        dataLoadsY++;
                    }
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    xstart = (xstart + m_kColumns + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    ystart = (ystart - m_kColumns + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                    if (op == m_kFirLenCeilCols - m_kColumns && TP_FIR_RANGE_LEN % 2 == 1) {
                        lastXstart = (xstart - m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        lastYstart = (ystart + m_kFinalOpSkew + m_kSamplesInDataBuff) % m_kSamplesInDataBuff;
                        ct = m_kColumns - 1;
                        acc = macDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, lastXstart, ybuff, lastYstart, ct, coe0, (op % m_kCoeffRegVsize),
                            m_kDecimateOffsets);
                    } else {
                        acc = macDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(
                            acc, xbuff, xstart, ybuff, ystart, coe0, (op % m_kCoeffRegVsize), m_kDecimateOffsets);
                    }
                }
                // Go back by the number of input samples loaded minus  (i.e forward) by the number of samples consumed
                inItr -= (m_kDataLoadVsize * dataLoadsX -
                          m_kVOutSize * TP_DECIMATE_FACTOR); // return read pointer to start of next chunk of window.
                yinItr += (m_kDataLoadVsize * dataLoadsY +
                           m_kVOutSize * TP_DECIMATE_FACTOR); // return read pointer to start of next chunk of window.
                // Write cascade. Do nothing if cascade not present.
                writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

                outVal = shiftAndSaturateDecSym(acc, TP_SHIFT);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2, outItr);
                if
                    constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                        writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2,
                                                                               outItr2);
                    }

            } // for strobe
        }     // for i
};

//---------------------------------------------------------------------------
// Cascade layer class and specializations

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0. TP_NUM_OUTPUTS = 1
// Standalone kernel specialization with no cascade ports, Windowed. a single input, no reload, single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<
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

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0. TP_NUM_OUTPUTS = 2
// Single kernel specialization. No cascade ports, Windowed. single input, with static coefficients, dual outputs
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for when there is only one kernel for the whole filter, but with single
// input and reload, single output
// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS=1
// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_circular_buffer<TT_DATA>& outWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS=2
// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_circular_buffer<TT_DATA>& outWindow,
           output_circular_buffer<TT_DATA>& outWindow2,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter, but with dual
// inputs and no reload, single output.
// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=1
// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=2
// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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

// This is a specialization of the main class for when
// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS= 1
// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS= 2
// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
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
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowReverse;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=1
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and no coefficient reloads, dual output
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=2
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    2,
                                    USE_WINDOW_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
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

/*
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=1
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
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_FALSE, 1, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>* __restrict inWindow,
       input_window<TT_DATA>* __restrict inWindowReverse,
       input_cascade_cacc*   inCascade,
       output_window<TT_DATA>* __restrict outWindow
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outWindow = outWindow;
this->filterKernel(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=2
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
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_FALSE, 2, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>* __restrict inWindow,
       input_window<TT_DATA>* __restrict inWindowReverse,
       input_cascade_cacc*   inCascade,
       output_window<TT_DATA>* __restrict outWindow,
       output_window<TT_DATA>* __restrict outWindow2
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outWindow = outWindow;
outInterface.outWindow2 = outWindow2;
this->filterKernel(inInterface, outInterface);
};
*/

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 1
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 2
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, dual output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    2,
                                    USE_WINDOW_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
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
// This is a specialization of the main class for the first kernel in a cascade chain, single input and no coefficient
// reloads..
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = 0,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, no reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_WINDOW_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the first kernel in a cascade chain with dual inputs and no
// coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = 0,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, no reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_WINDOW_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
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

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain, single input and coefficient
// reloads..
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = 0,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, with reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
                                    USE_WINDOW_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for the first kernel in a cascade chain with dual inputs and no
// coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = 0,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, with reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
                                    USE_WINDOW_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowReverse,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
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

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last, single
// input and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = neither first nor last,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual  input, no
// reload
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
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
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

/*
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last, dual
inputs and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = neither first nor last,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0
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
unsigned int TP_NUM_OUTPUTS,
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>  *inWindow,
       input_window<TT_DATA>  *inWindowReverse,
       input_cascade_cacc    *inCascade,
       output_cascade_cacc   *outCascade,
       output_window<TT_DATA> *broadcastWindow
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outCascade = outCascade;
outInterface.broadcastWindow = broadcastWindow;
this->filterKernel(inInterface, outInterface);
};
*/

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last,
// single/dual input and coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = neither first nor last,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
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
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};

/*
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 1
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
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_TRUE, 1, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>* __restrict inWindow,
       input_window<TT_DATA>* __restrict inWindowReverse,
       input_cascade_cacc    *inCascade,
       output_window<TT_DATA>* __restrict outWindow
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outWindow = outWindow;
this->filterKernelRtp(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 2
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
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_TRUE, 2, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>* __restrict inWindow,
       input_window<TT_DATA>* __restrict inWindowReverse,
       input_cascade_cacc*   inCascade,
       output_window<TT_DATA>* __restrict outWindow,
       output_window<TT_DATA>* __restrict outWindow2
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outWindow = outWindow;
outInterface.outWindow2 = outWindow2;
this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last, dual
inputs and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = neither first nor last,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 1
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
unsigned int TP_NUM_OUTPUTS,
unsigned int TP_SAT>
NOINLINE_DECL
void fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION,
            TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>::filter
      (input_window<TT_DATA>  *inWindow,
       input_window<TT_DATA>  *inWindowReverse,
       input_cascade_cacc    *inCascade,
       output_cascade_cacc   *outCascade,
       output_window<TT_DATA> *broadcastWindow
      )
{
T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
inInterface.inWindow = inWindow;
inInterface.inWindowReverse = inWindowReverse;
inInterface.inCascade = inCascade;
outInterface.outCascade = outCascade;
outInterface.broadcastWindow = broadcastWindow;
this->filterKernelRtp(inInterface, outInterface);
};
*/

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Stream
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
                                    TP_SHIFT,
                                    TP_RND,
                                    TP_INPUT_WINDOW_VSIZE,
                                    CASC_IN_FALSE,
                                    CASC_OUT_FALSE,
                                    TP_FIR_LEN,
                                    0,
                                    1,
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_stream<TT_DATA>* restrict outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0. TP_NUM_OUTPUTS = 2
// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Dual Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
                                    TP_SHIFT,
                                    TP_RND,
                                    TP_INPUT_WINDOW_VSIZE,
                                    CASC_IN_FALSE,
                                    CASC_OUT_FALSE,
                                    TP_FIR_LEN,
                                    0,
                                    1,
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_stream<TT_DATA>* restrict outStream,
           output_stream<TT_DATA>* restrict outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter, but with single
// input and reload, single output
// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS=1
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
                                    TP_SHIFT,
                                    TP_RND,
                                    TP_INPUT_WINDOW_VSIZE,
                                    CASC_IN_FALSE,
                                    CASC_OUT_FALSE,
                                    TP_FIR_LEN,
                                    0,
                                    1,
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_stream<TT_DATA>* restrict outStream,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Special: TP_CASC_LEN = 1, TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS=2
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
                                    TT_COEFF,
                                    TP_FIR_LEN,
                                    TP_DECIMATE_FACTOR,
                                    TP_SHIFT,
                                    TP_RND,
                                    TP_INPUT_WINDOW_VSIZE,
                                    CASC_IN_FALSE,
                                    CASC_OUT_FALSE,
                                    TP_FIR_LEN,
                                    0,
                                    1,
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_stream<TT_DATA>* restrict outStream,
           output_stream<TT_DATA>* restrict outStream2,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=1
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Single Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_stream<TT_DATA>* restrict outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain with
// dual input and no coefficient reloads.
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=1
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Single Output
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
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_FALSE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_stream<TT_DATA>* restrict outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain with
// single input and no coefficient reloads, dual output
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=2
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Dual Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_FALSE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_stream<TT_DATA>* restrict outStream,
           output_stream<TT_DATA>* restrict outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain with
// dual input and no coefficient reloads, dual output
// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 1, TP_USE_COEFF_RELOAD = 0, TP_NUM_OUTPUTS=2
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Dual Output
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
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_FALSE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_stream<TT_DATA>* restrict outStream,
                                                    output_stream<TT_DATA>* restrict outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.outWindow =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // dummy
    this->filterKernel(inInterface, outInterface);
};

// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Streaming, Middle, no reload, dual input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_FALSE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_cascade_cacc* outCascade,
                                                    output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 1
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Single Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_stream<TT_DATA>* restrict outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 1
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. dual input. Single Output
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
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_TRUE,
                                    1,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_stream<TT_DATA>* restrict outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 2
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Dual Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    TP_DUAL_IP,
                                    USE_COEFF_RELOAD_TRUE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_stream<TT_DATA>* restrict outStream,
           output_stream<TT_DATA>* restrict outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// Special: TP_CASC_LEN = >1, TP_KERNEL_POSITION = TP_CASC_LEN-1,
//          TP_DUAL_IP = 0, TP_USE_COEFF_RELOAD = 1, TP_NUM_OUTPUTS = 2
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Dual Output
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
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_TRUE,
                                    2,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_stream<TT_DATA>* restrict outStream,
                                                    output_stream<TT_DATA>* restrict outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           output_cascade_cacc* outCascade,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>

NOINLINE_DECL void fir_decimate_sym<
    TT_DATA,
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
    DUAL_IP_DUAL,
    USE_COEFF_RELOAD_TRUE,
    TP_NUM_OUTPUTS,
    USE_STREAM_API,
    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                    output_cascade_cacc* outCascade,
                    output_async_buffer<TT_DATA>& broadcastWindow,
                    const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface, inTaps);
};

// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_SINGLE,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_cascade_cacc* inCascade,
           output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc = (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};

// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
NOINLINE_DECL void fir_decimate_sym<TT_DATA,
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
                                    DUAL_IP_DUAL,
                                    USE_COEFF_RELOAD_TRUE,
                                    TP_NUM_OUTPUTS,
                                    USE_STREAM_API,
                                    TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                                    input_cascade_cacc* inCascade,
                                                    output_cascade_cacc* outCascade,
                                                    output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give outItr a hook
    this->filterKernelRtp(inInterface, outInterface);
};
}
}
}
}
}
// Placeholder for middle kernel, single inputs, reload
// placeholder for middle kernel, dual inputs, reload
