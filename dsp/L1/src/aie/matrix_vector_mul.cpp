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
namespace matrix_vector_mul {

// MATRIX_VECTOR_MUL single channel function - base of specialization .
//-----------------------------------------------------------------------------------------------------
//// TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES, TP_CASC_LEN
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
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
                     TP_NUM_FRAMES,
                     TP_CASC_LEN,
                     TP_KERNEL_POSITION,
                     TP_CASC_IN,
                     TP_CASC_OUT>::kernelMatVecMul(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                                                   T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B> outInterface) {
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    using dataA_t = ::aie::vector<TT_DATA_A, kSamplesDataA>;
    using dataB_t = ::aie::vector<TT_DATA_B, kSamplesDataB>;
    using dataOut_t = ::aie::vector<TT_OUT, kSamplesDataOut>;
    using accVect_t = ::aie::detail::accum<fnAccClass<TT_OUT>(),              // int, cint, FP or CFP
                                           fnAccSize<TT_DATA_A, TT_DATA_B>(), // acc80 or acc48
                                           kSamplesDataOut>;
    set_rnd(rnd_pos_inf);
    set_sat(); // do saturate.

    dataA_t dataA;
    dataA_t* inPtrA;
    dataB_t dataB;
    dataB_t* inPtrB;

    accVect_t acc;

    dataOut_t blankVect = ::aie::zeros<TT_OUT, kSamplesDataOut>(); // to initialise acc

    dataOut_t outVect;

    dataB_t* vectorStartPtr = (dataB_t*)inInterface.inWindowB;
    dataA_t* matrixStartPtr = (dataA_t*)inInterface.inWindowA;
    dataOut_t* outPtr = (dataOut_t*)outInterface.outWindow;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        inPtrA = matrixStartPtr;
        for (int accIndex = 0; accIndex < TP_DIM_A / kSamplesDataA; accIndex++)
            chess_prepare_for_pipelining chess_loop_count(TP_DIM_A / kSamplesDataA) {
                // printf("accIndex = %d\n", accIndex);
                inPtrA = matrixStartPtr++;
                inPtrB = vectorStartPtr;
                acc = blankVect;

#pragma unroll kSamplesDataB
                for (int elemB = 0; elemB < TP_DIM_B; elemB++) {
                    // printf("elemB = %d", elemB);
                    // printf("elemB % kSamplesDataB = %d\n", (elemB % kSamplesDataB));

                    if (elemB % kSamplesDataB == 0) {
                        dataB = *inPtrB++;
                    }
                    dataA = *inPtrA;
                    inPtrA += (TP_DIM_A / kSamplesDataA);

                    acc = ::aie::mac(acc, dataA, dataB[elemB % kSamplesDataB]);
                }
                // matrixStartPtr++;
                outVect = acc.template to_vector<TT_OUT>(shift);

                *outPtr++ = outVect;
            }
        vectorStartPtr = inPtrB;
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
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void matrix_vector_mul<TT_DATA_A,
                                     TT_DATA_B,
                                     TP_DIM_A,
                                     TP_DIM_B,
                                     TP_SHIFT,
                                     TP_RND,
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
}
}
}
}
