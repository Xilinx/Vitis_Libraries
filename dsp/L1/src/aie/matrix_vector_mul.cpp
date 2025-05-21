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
MATRIX_VECTOR_MUL kernal code.
This file captures the body of run-time code for the kernal class.
Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include "device_defs.h"
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "matrix_vector_mul.hpp"
#include "kernel_api_utils.hpp"

#ifdef __X86SIM__
// #define _DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

// GEMV function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void kernelMatVecMulClass<TT_DATA_A,
                                      TT_DATA_B,
                                      TP_DIM_A,
                                      TP_DIM_B,
                                      TP_SHIFT,
                                      TP_RND,
                                      TP_SAT,
                                      TP_NUM_FRAMES,
                                      TP_CASC_LEN,
                                      TP_USE_MATRIX_RELOAD,
                                      TP_API,
                                      TP_DUAL_IP,
                                      TP_NUM_OUTPUTS,
                                      TP_KERNEL_POSITION,
                                      TP_CASC_IN,
                                      TP_CASC_OUT>::matVecMulKernel(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface,
                                                                    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface) {
    // select architecture and run FIR iteration
    matVecMulSelectArch(inInterface, outInterface);
};

//----------------------------------------------------------------------------------------------------------------------
// Select arch - matVecMulBasic (iobuffer B) or matVecMultStream (stream B)
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void kernelMatVecMulClass<TT_DATA_A,
                                      TT_DATA_B,
                                      TP_DIM_A,
                                      TP_DIM_B,
                                      TP_SHIFT,
                                      TP_RND,
                                      TP_SAT,
                                      TP_NUM_FRAMES,
                                      TP_CASC_LEN,
                                      TP_USE_MATRIX_RELOAD,
                                      TP_API,
                                      TP_DUAL_IP,
                                      TP_NUM_OUTPUTS,
                                      TP_KERNEL_POSITION,
                                      TP_CASC_IN,
                                      TP_CASC_OUT>::matVecMulSelectArch(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface,
                                                                        T_outputIF<TT_DATA_A, TT_DATA_B> outInterface) {
    if
        constexpr(TP_API == 0) { matVecMulBasic(inInterface, outInterface); }
    else {
        matVecMulStream(inInterface, outInterface);
    }
};
//----------------------------------------------------------------------------------------------------------------------
// matVecMulBasic
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void kernelMatVecMulClass<TT_DATA_A,
                                      TT_DATA_B,
                                      TP_DIM_A,
                                      TP_DIM_B,
                                      TP_SHIFT,
                                      TP_RND,
                                      TP_SAT,
                                      TP_NUM_FRAMES,
                                      TP_CASC_LEN,
                                      TP_USE_MATRIX_RELOAD,
                                      TP_API,
                                      TP_DUAL_IP,
                                      TP_NUM_OUTPUTS,
                                      TP_KERNEL_POSITION,
                                      TP_CASC_IN,
                                      TP_CASC_OUT>::matVecMulBasic(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface,
                                                                   T_outputIF<TT_DATA_A, TT_DATA_B> outInterface) {
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    using dataA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataAcc_t = ::aie::vector<TT_OUT, vecSampleNumA>;
    using dataOut_t = ::aie::vector<TT_OUT, vecSampleNumOut>;
    using accVect_t = ::aie::accum<typename accType<TT_DATA_A, TT_DATA_B>::type, vecSampleNumA>;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    dataA_t dataA;
    dataA_t* __restrict inPtrA;
    dataB_t dataB;
    dataB_t* __restrict inPtrB;

    accVect_t acc;
    dataAcc_t blankVect = ::aie::zeros<TT_OUT, vecSampleNumA>(); // to initialise acc
    dataAcc_t outVect;

    // Buffer or RTP matrix A ptr
    TT_DATA_A* __restrict inMatrixBuff = (TT_DATA_A*)inInterface.inWindowA;
    TT_DATA_A* __restrict inMatrixPtrRtp = m_inMatrixPtr;
    TT_DATA_A* __restrict matrixPtr = (TP_USE_MATRIX_RELOAD == 1) ? inMatrixPtrRtp : inMatrixBuff;

    dataA_t* __restrict matrixStartPtr = (dataA_t*)(matrixPtr);
    dataB_t* __restrict vectorStartPtr = (dataB_t*)inInterface.inWindowB;
    dataOut_t* __restrict outPtr = (dataOut_t*)outInterface.outWindow;

    // // Each frame contains a mutliplcation of one matrix-vector multiplication
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        // data loads per A column
        for (int idx = 0; idx < loadsPerColA; idx++) chess_prepare_for_pipelining chess_loop_count(loadsPerColA) {
                inPtrA = (matrixStartPtr) + (frame * loadsPerMatrix) + (idx);
                inPtrB = (vectorStartPtr) + (frame * loadsPerVectorB);

                if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                        acc = (accVect_t)readincr_v<vecSampleNumA>(inInterface.inCascade);
                    }
                else {
                    acc = (accVect_t)blankVect;
                }
                for (int vecInB = 0; vecInB < loadsPerVectorB; vecInB++) {
                    dataB = *inPtrB++;
#pragma unroll(vecSampleNumB)
                    for (int jdx = 0; jdx < vecSampleNumB; jdx++) {
                        dataA = *inPtrA;
                        inPtrA += loadsPerColA;
                        if
                            constexpr(castBtoA) { acc = ::aie::mac(acc, (TT_DATA_A)dataB[jdx], dataA); }
                        else {
                            acc = ::aie::mac(acc, dataB[jdx], dataA);
                        }
                    }
                }
                // output to outVect or to outCascade
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                        outVect = acc.template to_vector<TT_OUT>(TP_SHIFT);
#pragma unroll(vecSampleNumA / vecSampleNumOut)
                        for (int n = 0; n < vecSampleNumA / vecSampleNumOut; n++) {
                            *outPtr++ = outVect.template extract<vecSampleNumOut>(n);
                        }
                    }
                else {
                    writeincr(outInterface.outCascade, acc);
                }
            }
    }
}
//----------------------------------------------------------------------------------------------------------------------
// matVecMulStream
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void kernelMatVecMulClass<TT_DATA_A,
                                      TT_DATA_B,
                                      TP_DIM_A,
                                      TP_DIM_B,
                                      TP_SHIFT,
                                      TP_RND,
                                      TP_SAT,
                                      TP_NUM_FRAMES,
                                      TP_CASC_LEN,
                                      TP_USE_MATRIX_RELOAD,
                                      TP_API,
                                      TP_DUAL_IP,
                                      TP_NUM_OUTPUTS,
                                      TP_KERNEL_POSITION,
                                      TP_CASC_IN,
                                      TP_CASC_OUT>::matVecMulStream(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface,
                                                                    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface) {
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    using dataA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataAcc_t = ::aie::vector<TT_OUT, vecSampleNumA>;
    using dataOut_t = ::aie::vector<TT_OUT, vecSampleNumOut>;
    using accVect_t = ::aie::accum<typename accType<TT_DATA_A, TT_DATA_B>::type, vecSampleNumA>;

    using dataStreamB_t = ::aie::vector<TT_DATA_B, streamLoadSize>;
    using vectorBuff_t = ::aie::vector<TT_DATA_B, streamVectorBuffSize>;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    dataA_t dataA;
    dataA_t* __restrict inPtrA;
    dataB_t dataB;
    dataB_t* __restrict inPtrB;

    accVect_t acc;
    dataAcc_t blankVect = ::aie::zeros<TT_OUT, vecSampleNumA>(); // to initialise acc
    dataAcc_t outVect;

    // Buffer or RTP matrix A ptr
    TT_DATA_A* __restrict matrixPtr = m_inMatrixPtr;

    dataA_t* __restrict matrixStartPtr = (dataA_t*)(matrixPtr);
    dataOut_t* __restrict outPtr = (dataOut_t*)outInterface.outWindow;

    dataB_t vecB256;
    dataStreamB_t inVecStream, inVecStream2;
    vectorBuff_t vBuff = ::aie::zeros<TT_DATA_B, streamVectorBuffSize>();
    TT_DATA_B elem;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
#pragma unroll(TP_DIM_B / (streamLoadSize*(TP_DUAL_IP + 1)))
        for (int bdx = 0; bdx < ((TP_DIM_B / TP_CASC_LEN) / (streamLoadSize * (TP_DUAL_IP + 1))); bdx++) {
            // printf("bdx = %d\n", bdx);
            inVecStream = readincr_v<streamLoadSize, aie_stream_resource_in::a>(inInterface.inStreamB);
            vBuff.template insert((TP_DUAL_IP + 1) * bdx, inVecStream);
            if
                constexpr(TP_DUAL_IP) {
                    inVecStream2 = readincr_v<streamLoadSize, aie_stream_resource_in::b>(inInterface.inStreamB2);
                    vBuff.template insert((TP_DUAL_IP + 1) * bdx + 1, inVecStream2);
                }
        }

        for (int idx = 0; idx < loadsPerColA; idx++) chess_prepare_for_pipelining chess_loop_count(loadsPerColA) {
                inPtrA = (matrixStartPtr) + (frame * loadsPerMatrix) + idx;
                if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                        acc = (accVect_t)readincr_v<vecSampleNumA>(inInterface.inCascade);
                    }
                else {
                    acc = (accVect_t)blankVect;
                }

                for (int j = 0; j < (loadsPerVectorB); j++) {
                    dataB = vBuff.template extract<vecSampleNumB>(j);
#pragma unroll(vecSampleNumB)
                    for (int k = 0; k < vecSampleNumB; k++) {
                        dataA = *inPtrA;
                        inPtrA += loadsPerColA;
                        if
                            constexpr(castBtoA) { acc = ::aie::mac(acc, (TT_DATA_A)dataB[k], dataA); }
                        else {
                            acc = ::aie::mac(acc, dataB[k], dataA);
                        }
                    }
                }
                if
                    constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                        outVect = acc.template to_vector<TT_OUT>(TP_SHIFT);
                        // #pragma unroll(vecSampleNumA / (TP_NUM_OUTPUTS* streamWriteOutSize))
                        for (int n = 0; n < vecSampleNumA / ((TP_NUM_OUTPUTS * streamWriteOutSize)); n++) {
                            writeincr<aie_stream_resource_out::a, TT_OUT, streamWriteOutSize>(
                                outInterface.outStream,
                                outVect.template extract<streamWriteOutSize>((TP_NUM_OUTPUTS * n)));
                            if
                                constexpr(TP_NUM_OUTPUTS == 2) {
                                    writeincr<aie_stream_resource_out::b, TT_OUT, streamWriteOutSize>(
                                        outInterface.outStream2,
                                        outVect.template extract<streamWriteOutSize>((TP_NUM_OUTPUTS * n) + 1));
                                }
                        }
                    }
                else {
                    writeincr(outInterface.outCascade, acc);
                }
            }
    }
}
//----------------------------------------------------------------------------------------------------------------------
// iobuffer A, iobuffer B, iobuffer Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     TP_USE_MATRIX_RELOAD,
                                     TP_API,
                                     TP_DUAL_IP,
                                     TP_NUM_OUTPUTS,
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMul(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                             input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                             output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    outInterface.outWindow = outWindow.data();
    this->matVecMulKernel(inInterface, outInterface);
};

// iobuffer A, iobuffer B, iobuffer Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  TP_USE_MATRIX_RELOAD,
                  TP_API,
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirst(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                               input_buffer<TT_DATA_B>& __restrict inWindowB,
                                               output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    outInterface.outCascade = outCascade;
    this->matVecMulKernel(inInterface, outInterface);
};

// iobuffer A, iobuffer B, iobuffer Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  TP_USE_MATRIX_RELOAD,
                  TP_API,
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddle(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->matVecMulKernel(inInterface, outInterface);
};

// iobuffer A, iobuffer B, iobuffer Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  TP_USE_MATRIX_RELOAD,
                  TP_API,
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLast(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                              input_buffer<TT_DATA_B>& __restrict inWindowB,
                                              input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                              output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow.data();
    this->matVecMulKernel(inInterface, outInterface);
};
///////////////////////////////////////////////////// RTP //////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------
// RTP A, iobuffer B, iobuffer Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     1, // TP_USE_MATRIX_RELOAD
                                     0, // TP_API
                                     TP_DUAL_IP,
                                     TP_NUM_OUTPUTS,
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMulRtp(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                                input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                                output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowB = inWindowB.data();
    outInterface.outWindow = outWindow.data();
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, iobuffer B, iobuffer Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  0, // TP_API
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirstRtp(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                  input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                  output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowB = inWindowB.data();
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, iobuffer B, iobuffer Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  0, // TP_API
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddleRtp(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                   input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                   input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                   output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, iobuffer B, iobuffer Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  0, // TP_API
                  TP_DUAL_IP,
                  TP_NUM_OUTPUTS,
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLastRtp(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                 input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                 output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow.data();

    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};

///////////////////////////////////// RTP A,Stream B////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------
// RTP A, 1 stream B, 1 stream Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     1, // TP_USE_MATRIX_RELOAD
                                     1, // TP_API
                                     0, // TP_DUAL_IP
                                     1, // TP_NUM_OUTPUTS
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB,
                                                                      output_stream<TT_OUT>* __restrict outStream) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    outInterface.outStream = outStream;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 1 stream Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                        input_stream<TT_DATA_B>* __restrict inStreamB,
                                                        output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inStreamB = inStreamB;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 1 stream Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                         input_stream<TT_DATA_B>* __restrict inStreamB,
                                                         input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                         output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inStreamB = inStreamB;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 1 stream Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                       input_stream<TT_DATA_B>* __restrict inStreamB,
                                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                       output_stream<TT_OUT>* __restrict outStream) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;

    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};

//----------------------------------------------------------------------------------------------------------------------
// RTP A, 2 stream B, 2 stream Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     1, // TP_USE_MATRIX_RELOAD
                                     1, // TP_API
                                     1, // TP_DUAL_IP
                                     2, // TP_NUM_OUTPUTS
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB,
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                                      output_stream<TT_OUT>* __restrict outStream,
                                                                      output_stream<TT_OUT>* __restrict outStream2) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 2 stream Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                        input_stream<TT_DATA_B>* __restrict inStreamB,
                                                        input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                        output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 2 stream Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                         input_stream<TT_DATA_B>* __restrict inStreamB,
                                                         input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                         input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                         output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 2 stream Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                       input_stream<TT_DATA_B>* __restrict inStreamB,
                                                       input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                       output_stream<TT_OUT>* __restrict outStream,
                                                       output_stream<TT_OUT>* __restrict outStream2) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;

    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
//----------------------------------------------------------------------------------------------------------------------
// RTP A, 1 stream B, 2 stream Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     1, // TP_USE_MATRIX_RELOAD
                                     1, // TP_API
                                     0, // TP_DUAL_IP
                                     2, // TP_NUM_OUTPUTS
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB,
                                                                      output_stream<TT_OUT>* __restrict outStream,
                                                                      output_stream<TT_OUT>* __restrict outStream2) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 2 stream Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                        input_stream<TT_DATA_B>* __restrict inStreamB,
                                                        output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 2 stream Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                         input_stream<TT_DATA_B>* __restrict inStreamB,
                                                         input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                         output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inStreamB = inStreamB;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 1 stream B, 2 stream Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  0, // TP_DUAL_IP
                  2, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                       input_stream<TT_DATA_B>* __restrict inStreamB,
                                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                       output_stream<TT_OUT>* __restrict outStream,
                                                       output_stream<TT_OUT>* __restrict outStream2) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    outInterface.outStream2 = outStream2;

    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
//----------------------------------------------------------------------------------------------------------------------
// RTP A, 2 stream B, 1 stream Out, single
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_SAT,
                                     TP_NUM_FRAMES,
                                     TP_CASC_LEN,
                                     1, // TP_USE_MATRIX_RELOAD
                                     1, // TP_API
                                     1, // TP_DUAL_IP
                                     1, // TP_NUM_OUTPUTS
                                     TP_KERNEL_POSITION,
                                     TP_CASC_IN,
                                     TP_CASC_OUT>::matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB,
                                                                      input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                                      output_stream<TT_OUT>* __restrict outStream) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    outInterface.outStream = outStream;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 1 stream Out, first
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                        input_stream<TT_DATA_B>* __restrict inStreamB,
                                                        input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                        output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 1 stream Out, middle
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                         input_stream<TT_DATA_B>* __restrict inStreamB,
                                                         input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                         input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                         output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
// RTP A, 2 stream B, 1 stream Out, last
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void
matrix_vector_mul<TT_DATA_A,
                  TT_DATA_B,
                  TP_DIM_A,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_NUM_FRAMES,
                  TP_CASC_LEN,
                  1, // TP_USE_MATRIX_RELOAD
                  1, // TP_API
                  1, // TP_DUAL_IP
                  1, // TP_NUM_OUTPUTS
                  TP_KERNEL_POSITION,
                  TP_CASC_IN,
                  TP_CASC_OUT>::matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                                       input_stream<TT_DATA_B>* __restrict inStreamB,
                                                       input_stream<TT_DATA_B>* __restrict inStreamB2,
                                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                       output_stream<TT_OUT>* __restrict outStream) {
    T_inputIF<TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<TT_DATA_A, TT_DATA_B> outInterface;

    inInterface.inStreamB = inStreamB;
    inInterface.inStreamB2 = inStreamB2;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;

    this->m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    this->matVecMulKernel(inInterface, outInterface);
};
}
}
}
}
}
