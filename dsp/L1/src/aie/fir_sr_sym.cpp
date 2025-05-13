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
Single Rate Symmetrical FIR kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/
#pragma once
#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include <adf.h>
#include "fir_sr_sym.hpp"
#include "kernel_api_utils.hpp"
#include "fir_sr_sym_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_sym {

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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // window broadcast is required for dual_ip streaming, but not for single input streaming.
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_SAT>::filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                            T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    constexpr int TP_CASC_API = TP_API == 0 ? 0 : (TP_DUAL_IP == DUAL_IP_DUAL ? 0 : 1);
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>(), TP_CASC_API>(inInterface,
                                                                                                      outInterface);

    m_coeffnEq = getRtpTrigger(); // 0 - equal, 1 - not equal

    sendRtpTrigger(m_coeffnEq, outInterface);
    if (m_coeffnEq) { // Coefficients have changed
        bufferReload<TT_DATA, TT_COEFF, (TP_FIR_LEN + 1) / kSymmetryFactor>(inInterface, m_oldInTaps, outInterface);
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // Two possible architectures depending on size of data/coef types & fir_len
    // Using a single data buffer for x and y (forward & reverse) or seperate
    if
        constexpr(m_kArch == kArch1Buff) { filterKernel1buff(inInterface, outInterface); }
    else {
        filterKernel2buff(inInterface, outInterface);
    }
    // windowRelease(inInterface);
};

// 1buff architecture.
// Used when data samples required for FIR calculation fit fully into 1024-bit input vector register.
// Architecture is characterized by the usage of single 1024-bit buffer
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_SAT>::filterKernel1buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0, coe1; // register for coeff values.
    T_buff_1024b<TT_DATA> sbuff;
    T_accSym<TT_DATA, TT_COEFF> acc;
    T_outValSym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readDataS;
    T_buff_256b<TT_DATA> readDataT;
    T_buff_128b<TT_DATA> readDataV;
    unsigned int sNumDataLoads = 0;
    unsigned int sDataNeeded = 0;
    unsigned int sDataLoaded = 0;
    unsigned int xstart = 0;
    unsigned int ystart = 0;

    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, kVsize>;

    auto inItr = ((TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) ||
                  ((TP_API == USE_STREAM_API) && (TP_DUAL_IP == DUAL_IP_DUAL) && (TP_KERNEL_POSITION != 0)))
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
    // Move only by  multiples of 128bit.
    inItr += m_kSDataLoadInitOffset;

#pragma unroll(m_kInitialLoads - 1)
    for (int initLoads = 0; initLoads < m_kInitialLoads - 1; ++initLoads) {
        upd_win_incr_256b<TT_DATA>(sbuff, initLoads, inItr);
    }
    coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
    coe0 = *coeff++;
    for (unsigned i = 0; i < m_kLsize / m_kIncrRepeatFactor; i++)
        // Allow optimizations in the kernel compilation for this loop
        chess_prepare_for_pipelining chess_loop_range(m_kLsize / m_kIncrRepeatFactor, ) {
            sNumDataLoads = 0;
            sDataNeeded = 0; // m_kDataLoadVsize samples still needed due to only performing m_kInitialLoads-1
            sDataLoaded = 0;
// unroll m_kDataLoadsInReg times
#pragma unroll(m_kIncrRepeatFactor)
            for (unsigned dataLoadPhase = 0; dataLoadPhase < m_kIncrRepeatFactor; dataLoadPhase++) {
                if (sDataNeeded >= sDataLoaded) {
                    // readDataS = window_readincr_256b<TT_DATA>(inWindow);
                    // sbuff.val = upd_w(sbuff.val, ((m_kInitialLoads-1) + sNumDataLoads) % m_kDataLoadsInReg,
                    // readDataS.val);
                    upd_win_incr_256b<TT_DATA>(sbuff, ((m_kInitialLoads - 1) + sNumDataLoads) % m_kDataLoadsInReg,
                                               inItr);
                    sNumDataLoads++;
                    sDataLoaded += m_kDataLoadVsize;
                }
                sDataNeeded += m_kVOutSize;
                xstart = dataLoadPhase * m_kVOutSize + m_kSBuffXOffset;
                ystart = dataLoadPhase * m_kVOutSize + m_kYstartInitOffset;

                coeff = ((T_buff_256b<TT_COEFF>*)m_internalTaps);
                coe0 = *coeff++;

                // Read cascade input. Do nothing if cascade input not present.
                acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
                // Init Vector operation. VMUL if cascade not present, otherwise VMAC
                if (m_kFirLenTruncCols != 0) {
                    acc = initMacSrSym<TT_DATA, TT_COEFF, TP_FIR_LEN>(acc, sbuff, xstart, ystart, coe0, 0);
                }

// The following loop is unrolled because this allows compile-time rather than run-time calculation
// of some of the variables within the loop hence increasing throughput.
#pragma unroll(GUARD_ZERO((m_kFirLenTruncCols / (m_kColumns) - 1)))
                // Operations loop. Op indicates the data index.
                for (int op = m_kColumns; op < m_kFirLenTruncCols; op += m_kColumns) {
                    // MAC operation.
                    if (op % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                    int xoffset = xstart + op;
                    int yoffset = ystart - (op % m_kDataRegVsize);
                    acc = macSrSym(acc, sbuff, xoffset, yoffset, coe0, (op % m_kCoeffRegVsize));
                }

                if (TP_FIR_RANGE_LEN % (kSymmetryFactor * m_kColumns) != 0) {
                    if (m_kFirLenTruncCols != 0 && m_kFirLenTruncCols % m_kCoeffRegVsize == 0) {
                        coe0 = *coeff++;
                    }
                }
                int xoffset = (dataLoadPhase * m_kVOutSize + m_kFirLenTruncCols + m_kSBuffXOffset);
                // Center tap vector operation.
                acc = macSrSymCT<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN % (kSymmetryFactor * m_kColumns)>(
                    acc, sbuff, xoffset, coe0, m_kFirLenTruncCols % m_kCoeffRegVsize);

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

// 2buff architecture.
// Used when data samples required for FIR calculation do not fit fully into 1024-bit input vector register.
// Architecture is characterized by the usage of 2 512-bit buffers, one for forward data and one for reverse data.
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
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
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_SAT>::filterKernel2buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    T_buff_256b<TT_COEFF>* __restrict coeff = (T_buff_256b<TT_COEFF>*)m_internalTaps;
    T_buff_256b<TT_COEFF> coe0; // register for coeff values.
    T_buff_512b<TT_DATA> sbuff = null_buff_512b<TT_DATA>();
    T_buff_512b<TT_DATA> tbuff = null_buff_512b<TT_DATA>();
    T_accSym<TT_DATA, TT_COEFF> acc;
    T_outValSym<TT_DATA, TT_COEFF> outVal;
    T_buff_256b<TT_DATA> readDataS;
    T_buff_256b<TT_DATA> readDataT;
    T_buff_128b<TT_DATA> readDataV;
    T_buff_128b<TT_DATA> readDataU;
    unsigned int sDataLoaded, sDataNeeded, sNumDataLoads, sVDataLoads = 0;
    unsigned int tDataLoaded, tDataNeeded, tNumDataLoads, tVDataLoads = 0;
    unsigned int sDataBuffSwap = 0;
    unsigned int dataLoadPhase = 0;

    constexpr int k128Vsize = 128 / 8 / sizeof(TT_DATA);
    using t_128vect = ::aie::vector<TT_DATA, k128Vsize>;
    t_128vect* read128Ptr;
    constexpr int k256Vsize = 256 / 8 / sizeof(TT_DATA);
    using t_256vect = ::aie::vector<TT_DATA, k256Vsize>;
    t_256vect* read256Ptr;

    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    auto inItr =
        ((TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) ||
         ((TP_API == USE_STREAM_API) && (TP_DUAL_IP == DUAL_IP_DUAL) && (TP_KERNEL_POSITION != 0)))
            ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
            : ::aie::begin_random_circular(
                  *(inInterface.inWindowCirc)); // sample granularity since outvector can be smaller than readvector.
    auto inItrcp = ((TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) ||
                    ((TP_API == USE_STREAM_API) && (TP_DUAL_IP == DUAL_IP_DUAL) && (TP_KERNEL_POSITION != 0)))
                       ? ::aie::begin_random_circular(*(inInterface.inWindowLin))
                       : ::aie::begin_random_circular(*(inInterface.inWindowCirc));
    if
        constexpr(!(TP_DUAL_IP == 0 || TP_API == 1 || TP_KERNEL_POSITION != 0)) {
            inItrcp = ::aie::begin_random_circular(*(inInterface.inWindowReverse));
        }
    constexpr bool hasOutWindow = (TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    constexpr bool hasOutWindow2 = (TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1);
    auto outItr = cond_begin_vector_random_or_scalar_circular<hasOutWindow, m_kVOutSize>(*outInterface.outWindow);
    auto outItr2 = cond_begin_vector_random_or_scalar_circular<hasOutWindow2, m_kVOutSize>(*(outInterface.outWindow2));

    // Move data pointer away from data consumed by previous cascades
    // Move only by  multiples of 128bit.
    inItr += m_kSDataLoadInitOffset; // Cascade phase remainder goes to m_kSBuffXOffset
    if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing128) {
        inItrcp += m_kTDataLoadInitOffset + m_kDataLoadVsize / 2;
    } else {
        inItrcp += m_kTDataLoadInitOffset;
    }

    for (unsigned i = 0; i < m_kLsize; i++)
        // Allow optimizations in the kernel compilation for this loop
        chess_prepare_for_pipelining chess_loop_range(m_kLsize, m_kLsize) {
            m_internalTapsCopy = chess_copy(m_internalTapsCopy);
            coeff = ((T_buff_256b<TT_COEFF>*)m_internalTapsCopy);
            coe0 = *coeff++;

            sNumDataLoads = 0;
            tNumDataLoads = 0;
            sDataLoaded = m_kSInitialLoads * m_kDataLoadVsize;
            tDataLoaded = m_kTInitialLoads * m_kDataLoadVsize;
            sVDataLoads = 0;
            tVDataLoads = 0;

            // Preamble, calculate and load data from window into register
            sDataNeeded = m_kSBuffXOffset + m_kVOutSize + m_kColumns - 1;
            tDataNeeded = m_kTBuffXOffset + m_kVOutSize + m_kColumns - 1;
            if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing128) {
#pragma unroll(2 * m_kSInitialLoads)
                for (int initLoads = 0; initLoads < 2 * m_kSInitialLoads; ++initLoads) {
                    read128Ptr = (t_128vect*)&*inItr;
                    inItr += k128Vsize;
                    readDataV.val = *read128Ptr;
                    sbuff.val = upd_v(sbuff.val, sVDataLoads, readDataV.val); // Update Sbuff
                    sVDataLoads++;
                }
#pragma unroll(2 * m_kTInitialLoads)
                for (int initLoads = 0; initLoads < 2 * m_kTInitialLoads; ++initLoads) {
                    read128Ptr = (t_128vect*)&*inItrcp;
                    inItrcp -= k128Vsize;
                    readDataU.val = *read128Ptr;
                    tbuff.val =
                        upd_v(tbuff.val, (2 * m_kDataLoadsInReg - 1 - tVDataLoads), readDataU.val); // Update Sbuff
                    tVDataLoads++;
                }
            } else {
#pragma unroll(m_kSInitialLoads)
                for (int initLoads = 0; initLoads < m_kSInitialLoads; ++initLoads) {
                    read256Ptr = (t_256vect*)&*inItr;
                    inItr += k256Vsize;
                    readDataS.val = *read256Ptr;
                    sbuff.val = upd_w(sbuff.val, sNumDataLoads % m_kDataLoadsInReg, readDataS.val);
                    sNumDataLoads++;
                }
#pragma unroll(m_kTInitialLoads)
                for (int initLoads = 0; initLoads < m_kTInitialLoads; ++initLoads) {
                    read256Ptr = (t_256vect*)&*inItrcp;
                    inItrcp -= k256Vsize;
                    readDataT.val = *read256Ptr;
                    tbuff.val =
                        upd_w(tbuff.val, (m_kDataLoadsInReg - 1 - tNumDataLoads) % m_kDataLoadsInReg, readDataT.val);
                    tNumDataLoads++;
                }
            }

            // Read cascade input. Do nothing if cascade input not present.

            acc = readCascade<TT_DATA, TT_COEFF>(inInterface, acc);
            // Init Vector operation. VMUL if cascade not present, otherwise VMAC
            if (m_kFirLenTruncCols != 0) {
                acc = initMacSrSym<TT_DATA, TT_COEFF, TP_FIR_LEN>(acc, sbuff, m_kSBuffXOffset, tbuff, m_kTStartOffset,
                                                                  coe0, 0);
            }

// The following loop is unrolled because this allows compile-time rather than run-time calculation
// of some of the variables within the loop hence increasing throughput.
#pragma unroll(GUARD_ZERO((m_kFirLenTruncCols / (m_kColumns) - 1)))
            // Operations loop. Op indicates the data index.
            for (int op = m_kColumns; op < m_kFirLenTruncCols; op += m_kColumns) {
                sDataNeeded += m_kColumns;
                tDataNeeded += m_kColumns;
                // indices track the amount of data loaded into registers and consumed
                // from those registers so that the need to load more can be determined.
                // sbuff is for forward direction data.
                // tbuff is for reverse direction data.

                if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing256) {
                    // kPreLoadUsing256 - uses upd_w - 256bit loads and  schedules load early, to avoid memory
                    // conflicts.
                    // To be used with data/coeff combo utilizing single column MUL/MAC intrinsics, i.e. coeff type
                    // greater than int16
                    // update sbuff with data read from memory bank.
                    if (sDataNeeded > sDataLoaded) {
                        read256Ptr = (t_256vect*)&*inItr;
                        inItr += k256Vsize;
                        readDataS.val = *read256Ptr;
                        sbuff.val = upd_w(sbuff.val, sNumDataLoads % m_kDataLoadsInReg, readDataS.val); // Update Sbuff
                        sDataLoaded += m_kDataLoadVsize;
                        sNumDataLoads++;
                    } else {
                        // update tbuff with data read from memory bank when not updating sbuff.
                        if (tDataNeeded + m_kColumns > tDataLoaded) {
                            read256Ptr = (t_256vect*)&*inItrcp;
                            inItrcp -= k256Vsize;
                            readDataT.val = *read256Ptr;
                            tbuff.val = upd_w(tbuff.val, (m_kDataLoadsInReg - 1 - (tNumDataLoads % m_kDataLoadsInReg)),
                                              readDataT.val); // Update Tbuff
                            tDataLoaded += m_kDataLoadVsize;
                            tNumDataLoads++;
                        }
                    }
                } else if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing128) {
                    // kPreLoadUsing128 - uses upd_v to update xbuff - 128bit loads and schedules load early, to avoid
                    // memory conflicts.
                    // To be used with data/coeff combo utilizing multi column MUL/MAC intrinsics, i.e. coeff type equal
                    // to int16
                    if (sDataNeeded > sDataLoaded) {
                        read128Ptr = (t_128vect*)&*inItr;
                        inItr += k128Vsize;
                        readDataV.val = *read128Ptr;
                        sbuff.val = upd_v(sbuff.val, (kUpdWToUpdVRatio * sNumDataLoads + sVDataLoads) %
                                                         (kUpdWToUpdVRatio * m_kDataLoadsInReg),
                                          readDataV.val); // Update Sbuff
                        sDataLoaded += m_kDataLoadVsize / kUpdWToUpdVRatio;
                        sVDataLoads++;
                    }
                    // update tbuff with data read from memory bank.
                    if (tDataNeeded > tDataLoaded) {
                        read128Ptr = (t_128vect*)&*inItrcp;
                        inItrcp -= k128Vsize;
                        readDataU.val = *read128Ptr;
                        tbuff.val =
                            upd_v(tbuff.val, (2 * m_kDataLoadsInReg - 1 - (tVDataLoads % (2 * m_kDataLoadsInReg))),
                                  readDataU.val); // Update Tbuff
                        tDataLoaded += m_kDataLoadVsize / kUpdWToUpdVRatio;
                        tVDataLoads++;
                    }

                } else { // (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kNoPreLoad)
                    // kNoPreLoad - default, uses upd_w - 256bit loads, no load sequence optimization.

                    // update sbuff with data read from memory bank.
                    if (sDataNeeded > sDataLoaded) {
                        read256Ptr = (t_256vect*)&*inItr;
                        inItr += k256Vsize;
                        readDataS.val = *read256Ptr;
                        sbuff.val = upd_w(sbuff.val, sNumDataLoads % m_kDataLoadsInReg, readDataS.val); // Update Sbuff
                        sDataLoaded += m_kDataLoadVsize;
                        sNumDataLoads++;
                    }
                    // update tbuff with data read from memory bank.
                    if (tDataNeeded > tDataLoaded) {
                        read256Ptr = (t_256vect*)&*inItrcp;
                        inItrcp -= k256Vsize;
                        readDataT.val = *read256Ptr;
                        tbuff.val = upd_w(tbuff.val, (m_kDataLoadsInReg - 1 - (tNumDataLoads % m_kDataLoadsInReg)),
                                          readDataT.val); // Update Tbuff
                        tDataLoaded += m_kDataLoadVsize;
                        tNumDataLoads++;
                    }
                }

                // The tracking of when to load a new splice of coefficients is simpler since it always starts at 0.
                if (op % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }

                int xstart = (op + m_kSBuffXOffset);
                int ystart = m_kTStartOffset - (op % m_kDataRegVsize);

                // MAC operation.
                acc = macSrSym(acc, sbuff, xstart, tbuff, ystart, coe0, (op % m_kCoeffRegVsize));
            }

            if (sDataNeeded + fnCTColumnsLeft(TP_FIR_RANGE_LEN, m_kColumns) > sDataLoaded) {
                if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing128) {
                    read128Ptr = (t_128vect*)&*inItr;
                    inItr += k128Vsize;
                    readDataV.val = *read128Ptr;
                    sbuff.val = upd_v(sbuff.val, (kUpdWToUpdVRatio * sNumDataLoads + sVDataLoads) %
                                                     (kUpdWToUpdVRatio * m_kDataLoadsInReg),
                                      readDataV.val); // Update Sbuff
                    sVDataLoads++;
                } else {
                    read256Ptr = (t_256vect*)&*inItr;
                    inItr += k256Vsize;
                    readDataS.val = *read256Ptr;
                    sbuff.val = upd_w(sbuff.val, sNumDataLoads % m_kDataLoadsInReg, readDataS.val);
                    sNumDataLoads++;
                }
            }
            if (tDataNeeded + (TP_FIR_RANGE_LEN % (2 * m_kColumns)) / 2 > tDataLoaded) {
                if (fnBufferUpdateScheme<TT_DATA, TT_COEFF>() == kPreLoadUsing128) {
                    read128Ptr = (t_128vect*)&*inItrcp;
                    inItrcp -= k128Vsize;
                    readDataU.val = *read128Ptr;
                    tbuff.val = upd_v(tbuff.val, (2 * m_kDataLoadsInReg - 1 - (tVDataLoads % (2 * m_kDataLoadsInReg))),
                                      readDataU.val); // Update Tbuff
                    tVDataLoads++;
                } else {
                    read256Ptr = (t_256vect*)&*inItrcp;
                    inItrcp -= k256Vsize;
                    readDataT.val = *read256Ptr;
                    tbuff.val =
                        upd_w(tbuff.val, (m_kDataLoadsInReg - 1 - tNumDataLoads) % m_kDataLoadsInReg, readDataT.val);
                    tNumDataLoads++;
                }
            }

            if ((TP_FIR_RANGE_LEN % (kSymmetryFactor * m_kColumns) != 0) || (TP_KERNEL_POSITION + 1 == TP_CASC_LEN)) {
                // Read coeffs for center tap operation.
                if (m_kFirLenTruncCols % m_kCoeffRegVsize == 0) {
                    coe0 = *coeff++;
                }
            }
            // Center tap vector operation.
            acc = macSrSymCT<TT_DATA, TT_COEFF, TP_FIR_RANGE_LEN % (kSymmetryFactor * m_kColumns)>(
                acc, sbuff, (m_kFirLenTruncCols + m_kSBuffXOffset), tbuff,
                ((m_kDataRegVsize + m_kTStartOffset - (m_kFirLenTruncCols) % m_kDataRegVsize) % m_kDataRegVsize),
                TP_FIR_RANGE_LEN % (kSymmetryFactor * m_kColumns), coe0, (m_kFirLenTruncCols % m_kCoeffRegVsize));

            // Write cascade. Do nothing if cascade not present.
            writeCascade<TT_DATA, TT_COEFF>(outInterface, acc);

            outVal = shiftAndSaturate(acc, TP_SHIFT);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2, outItr);
            if
                constexpr(TP_NUM_OUTPUTS == 2 && TP_API == 0 && TP_KERNEL_POSITION == TP_CASC_LEN - 1)
                    writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, dataLoadPhase % 2,
                                                                           outItr2);
            dataLoadPhase++;
            inItr -=
                (m_kDataLoadVsize * (kUpdWToUpdVRatio * sNumDataLoads + sVDataLoads) / kUpdWToUpdVRatio - m_kVOutSize);
            inItrcp +=
                (m_kDataLoadVsize * (kUpdWToUpdVRatio * tNumDataLoads + tVDataLoads) / kUpdWToUpdVRatio + m_kVOutSize);
        }
};

// FIR filter function overloaded with cascade interface variations
// This is the default specialization of the main class used  when there is only one kernel for the whole filter.
// (Window API), Static coefficients, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_circular_buffer<TT_DATA>& outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// This is the default specialization of the main class used  when there is only one kernel for the whole filter. Window
// API. Static coefficients, single output. Dual Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                1,
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_circular_buffer<TT_DATA>& outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API. Static
// coefficients, dual output. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_circular_buffer<TT_DATA>& outWindow,
                                output_circular_buffer<TT_DATA>& outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API. Static
// coefficients, dual output. Dual Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_circular_buffer<TT_DATA>& outWindow,
           output_circular_buffer<TT_DATA>& outWindow2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API.
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
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                DUAL_IP_SINGLE,
                USE_COEFF_RELOAD_TRUE,
                1,
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_circular_buffer<TT_DATA>& outWindow,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API.
// Reloadable coefficients, single output. Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                DUAL_IP_DUAL,
                USE_COEFF_RELOAD_TRUE,
                1,
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_circular_buffer<TT_DATA>& outWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API.
// Reloadable coefficients, dual output. Single Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                DUAL_IP_SINGLE,
                USE_COEFF_RELOAD_TRUE,
                2,
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_circular_buffer<TT_DATA>& outWindow,
                                output_circular_buffer<TT_DATA>& outWindow2,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Window API.
// Reloadable coefficients, dual output. Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                DUAL_IP_DUAL,
                USE_COEFF_RELOAD_TRUE,
                2,
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_circular_buffer<TT_DATA>& outWindow,
           output_circular_buffer<TT_DATA>& outWindow2,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain. Window API. Static coefficients,
// single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_circular_buffer<TT_DATA>& outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Window API. Static coefficients,
// dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_circular_buffer<TT_DATA>& outWindow,
                                output_circular_buffer<TT_DATA>& outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Window API. Static coefficients
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_cascade_cacc* outCascade,
                                output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Window API. Static coefficients.
// Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_cascade_cacc* outCascade,
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Window
// API. Static coefficients
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
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

// This is a specialization of the main class for the final kernel in a cascade chain. Window API. Reloadable
// coefficients, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_circular_buffer<TT_DATA>& outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Window API. Reloadable
// coefficients, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_circular_buffer<TT_DATA>& outWindow,
                                output_circular_buffer<TT_DATA>& outWindow2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&outWindow;
    outInterface.outWindow2 = (output_circular_buffer<TT_DATA>*)&outWindow2;
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Window API. Reloadable
// coefficients
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_WINDOW_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_cascade_cacc* outCascade,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor],
                                output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_SINGLE> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Window API. Reloadable
// coefficients, Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::
    filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindow,
           input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
               inWindowRev,
           output_cascade_cacc* outCascade,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor],
           output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inWindowReverse =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindowRev; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Window
// API. Reloadable coefficients
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
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

// FIR filter function overloaded with cascade interface variations
// This is the default specialization of the main class used  when there is only one kernel for the whole filter. Stream
// API. Static coefficients, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Stream API. Static
// coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter. Stream API.
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
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_DUAL_IP,
                USE_COEFF_RELOAD_TRUE,
                1,
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_stream<TT_DATA>* outStream,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface, inTaps);
};

// This is a specialization of the main class for when there is only one kernel for the whole filter. Stream API.
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
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_DUAL_IP,
                USE_COEFF_RELOAD_TRUE,
                2,
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Static coefficients,
// (single input), single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Static coefficients,
// dual input, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Static coefficients,
// (single input), dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Static coefficients,
// dual input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Stream API. Static coefficients.
// Single Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Stream API. Static coefficients.
// Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_cascade_cacc* outCascade,
                                output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Stream
// API. Static coefficients. Single Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                input_cascade_cacc* inCascade,
                                output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Stream
// API. Static coefficients. Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& inWindow,
                                input_cascade_cacc* inCascade,
                                output_cascade_cacc* outCascade,
                                output_async_buffer<TT_DATA>& broadcastWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow;
    inInterface.inCascade = inCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outCascade = outCascade;
    this->filterKernel(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Reloadable
// coefficients, (single input), single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Reloadable
// coefficients, dual input, single output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& __restrict inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Reloadable
// coefficients, (single input), dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// This is a specialization of the main class for the final kernel in a cascade chain. Stream API. Reloadable
// coefficients, dual input, dual output
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_async_buffer<TT_DATA>& __restrict inWindow,
                                input_cascade_cacc* inCascade,
                                output_stream<TT_DATA>* outStream,
                                output_stream<TT_DATA>* outStream2) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface;
    inInterface.inWindowLin = (input_async_buffer<TT_DATA>*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    outInterface.outWindow = (output_circular_buffer<TT_DATA>*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Stream API. Reloadable
// coefficients. Single Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                output_cascade_cacc* outCascade,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain. Stream API. Reloadable
// coefficients. Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                USE_STREAM_API,
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                                output_cascade_cacc* outCascade,
                                output_async_buffer<TT_DATA>& broadcastWindow,
                                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, DUAL_IP_DUAL> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    outInterface.outCascade = outCascade;
    outInterface.broadcastWindow = (output_async_buffer<TT_DATA>*)&broadcastWindow;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernel(inInterface, outInterface, inTaps);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Stream
// API. Reloadable coefficients. Single Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
                TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                      extents<inherited_extent>,
                                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                input_cascade_cacc* inCascade,
                                output_cascade_cacc* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface;
    inInterface.inWindowCirc =
        (input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >*)&inWindow; // 16 = kdummy
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    outInterface.outWindow = (input_circular_buffer<TT_DATA, extents<inherited_extent>,
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last. Stream
// API. Reloadable coefficients. Dual Input
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
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_SAT>
void fir_sr_sym<TT_DATA,
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
                TP_MODIFY_MARGIN_OFFSET,
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
                                                    margin<16> >*)&inWindow; // dummy to give iterator an anchor
    this->filterKernelRtp(inInterface, outInterface);
};
}
}
}
}
}
