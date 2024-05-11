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
MATRIX_VECTOR_MUL kernal code.
This file captures the body of run-time code for the kernal class.
Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/
//#include <adf.h>
#include <stdio.h>
using namespace std;
#include "device_defs.h"
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "matrix_vector_mul.hpp"
#include "kernel_api_utils.hpp"

// #define _DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

// MATRIX_VECTOR_MUL - base of specialization .
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void
kernelMatVecMulClass<TT_DATA_A,
                     TT_DATA_B,
                     TP_DIM_A,
                     TP_DIM_B,
                     TP_SHIFT,
                     TP_RND,
                     TP_SAT,
                     TP_NUM_FRAMES,
                     TP_CASC_LEN,
                     TP_KERNEL_POSITION,
                     TP_CASC_IN,
                     TP_CASC_OUT>::kernelMatVecMul(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                                                   T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B> outInterface) {
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

    dataA_t* matrixStartPtr = (dataA_t*)inInterface.inWindowA;
    dataB_t* vectorStartPtr = (dataB_t*)inInterface.inWindowB;
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
                        if
                            constexpr(std::is_same<TT_OUT, cfloat>() || std::is_same<TT_OUT, float>()) {
                                outVect = acc.template to_vector<TT_OUT>();
                            }
                        else {
                            outVect = acc.template to_vector<TT_OUT>(shift);
                        }
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
//-----------------------------------------------------------------------------------------------------
// For a single kernel - iobuffer in and out, no cascades
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
void matrix_vector_mul<TT_DATA_A,
                       TT_DATA_B,
                       TP_DIM_A,
                       TP_DIM_B,
                       TP_SHIFT,
                       TP_RND,
                       TP_SAT,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_KERNEL_POSITION,
                       TP_CASC_IN,
                       TP_CASC_OUT>::matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                   input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                   output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    outInterface.outWindow = outWindow.data();
    m_mat_vec_mulKernel.kernelMatVecMul(inInterface, outInterface);
};

//-----------------------------------------------------------------------------------------------------
// For a multiple kernels - first kernel - iobuffer in, cascade out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
void matrix_vector_mul<TT_DATA_A,
                       TT_DATA_B,
                       TP_DIM_A,
                       TP_DIM_B,
                       TP_SHIFT,
                       TP_RND,
                       TP_SAT,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_KERNEL_POSITION,
                       CASC_IN_FALSE,
                       CASC_OUT_TRUE>::matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                     input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                     output_stream<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    outInterface.outCascade = outCascade;
    m_mat_vec_mulKernel.kernelMatVecMul(inInterface, outInterface);
};
//-----------------------------------------------------------------------------------------------------
// For a multiple kernels - middle kernel - iobuffer in, cascade in AND out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
void matrix_vector_mul<TT_DATA_A,
                       TT_DATA_B,
                       TP_DIM_A,
                       TP_DIM_B,
                       TP_SHIFT,
                       TP_RND,
                       TP_SAT,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_TRUE>::matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                     input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                     input_stream<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                     output_stream<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    m_mat_vec_mulKernel.kernelMatVecMul(inInterface, outInterface);
};
//-----------------------------------------------------------------------------------------------------
// For a multiple kernels - last kernel - iobuffer in, cascade out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
void matrix_vector_mul<TT_DATA_A,
                       TT_DATA_B,
                       TP_DIM_A,
                       TP_DIM_B,
                       TP_SHIFT,
                       TP_RND,
                       TP_SAT,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_FALSE>::matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                      input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                      input_stream<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                                      output_buffer<TT_OUT>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA_A, TT_DATA_B> outInterface;
    inInterface.inWindowA = inWindowA.data();
    inInterface.inWindowB = inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow.data();
    m_mat_vec_mulKernel.kernelMatVecMul(inInterface, outInterface);
};
}
}
}
}
}
