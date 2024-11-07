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
kronecker kernal code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "kernel_api_utils.hpp"
#include "kronecker.hpp"
#include "kronecker_utils.hpp"

// #define _DSPLIB_KRONECKER_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

// Base specialization, used for static size window API configurations
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>

NOINLINE_DECL void
kronecker<TT_DATA_A,
          TT_DATA_B,
          TP_DIM_A_ROWS,
          TP_DIM_A_COLS,
          TP_DIM_B_ROWS,
          TP_DIM_B_COLS,
          TP_NUM_FRAMES,
          TP_API,
          TP_SHIFT,
          TP_RND,
          TP_SAT>::kronecker_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                  input_buffer<TT_DATA_B>& __restrict inWindowB,
                                  output_buffer<outTypeMult_t<TT_DATA_A, TT_DATA_B> >& __restrict outWindow) {
    using dataVectA_t = ::aie::vector<TT_DATA_A, kSamplesInVectA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kSamplesInVectB>;
    using dataVectOut_t = ::aie::vector<out_t, kSamplesInVectOut>;
    using dataVectTempOut_t = ::aie::vector<out_t, kSamplesInVectTempOut>;
    ::aie::accum<acc_t, kVecSampleNumAcc> acc;
    dataVectA_t* ptrInWindowA = (dataVectA_t*)inWindowA.data();
    dataVectB_t* ptrInWindowB = (dataVectB_t*)inWindowB.data();
    dataVectOut_t* ptrOutWindow = (dataVectOut_t*)outWindow.data();
    dataVectA_t inDataVecA;
    dataVectB_t inDataVecB;
    dataVectOut_t outDataVec;
    // dataAcc_t acc;
    dataVectTempOut_t outDataVecTemp;

    int indexMatB = 0;
    int indexMatA = 0;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frameIndex = 0; frameIndex < TP_NUM_FRAMES; frameIndex++) //{
        chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            //#pragma unroll(kMulCallsPerFrame)
            for (int j = 0; j < TP_DIM_A_COLS; j++) { // Mat A cols
                ptrInWindowA += indexMatA;
                for (int l = 0; l < TP_DIM_B_COLS; l++) { // Mat B cols
                    indexMatA = 0;
                    ptrInWindowB += indexMatB;
                    for (int numVecA = 0; numVecA < TP_DIM_A_ROWS / kSamplesInVectA;
                         numVecA++) { // Mat A: vectors per column
                        inDataVecA = *ptrInWindowA++;
                        indexMatA++;
                        for (int indexVecA = 0; indexVecA < kSamplesInVectA; indexVecA++) { // Mat A: elemets per vector
                            indexMatB = 0;
                            for (int numVecB = 0; numVecB < TP_DIM_B_ROWS / kSamplesInVectB;
                                 numVecB++) { // Mat B: vectors per column
                                inDataVecB = *ptrInWindowB++;
                                acc = ::aie::mul<acc_t>(inDataVecA[indexVecA], inDataVecB);
                                outDataVecTemp = acc.template to_vector<out_t>(TP_SHIFT);

#pragma unroll(kSamplesInVectTempOut / kSamplesInVectOut)
                                for (int n = 0; n < kSamplesInVectTempOut / kSamplesInVectOut; n++) {
                                    *ptrOutWindow++ = outDataVecTemp.template extract<kSamplesInVectOut>((n));
                                }
                                indexMatB++;
                            }
                            ptrInWindowB -= indexMatB; // this completes multiplication of each element of col j of
                                                       // matrix A with col l of matrix B
                        }
                    }
                    ptrInWindowA -= indexMatA; // this completes multiplication of each element of col j of matrix A
                                               // with col l of matrix B
                }
                ptrInWindowB -= (TP_DIM_B_ROWS / kSamplesInVectB) * TP_DIM_B_COLS; // reset Mat B address
            }
            ptrInWindowB += (TP_DIM_B_ROWS / kSamplesInVectB) * TP_DIM_B_COLS; // Mat B: set address to frame start
        }
};

// Specialization for iobuff in and stream out i/f
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          // unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void kronecker<TT_DATA_A,
                             TT_DATA_B,
                             TP_DIM_A_ROWS,
                             TP_DIM_A_COLS,
                             TP_DIM_B_ROWS,
                             TP_DIM_B_COLS,
                             TP_NUM_FRAMES,
                             1,
                             TP_SHIFT,
                             TP_RND,
                             TP_SAT>::kronecker_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                     input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                     output_stream<out_t>* __restrict outStream0) {
    using dataVectA_t = ::aie::vector<TT_DATA_A, kSamplesInVectA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kSamplesInVectB>;
    using dataVectOut_t = ::aie::vector<out_t, kSamplesInVectOut>;
    using dataVectTempOut_t = ::aie::vector<out_t, kSamplesInVectTempOut>;
    ::aie::accum<acc_t, kVecSampleNumAcc> acc;

    dataVectA_t* ptrInWindowA = (dataVectA_t*)inWindowA.data();
    dataVectB_t* ptrInWindowB = (dataVectB_t*)inWindowB.data();
    dataVectOut_t outDataVec;
    // dataAcc_t acc;
    dataVectA_t inDataVecA;
    dataVectB_t inDataVecB;
    dataVectTempOut_t outDataVecTemp;

    int indexMatB = 0;
    int indexMatA = 0;
    static constexpr unsigned int kMulCallsPerFrame = sizeMatA * sizeMatB;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frameIndex = 0; frameIndex < TP_NUM_FRAMES; frameIndex++) //{
        chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            for (int j = 0; j < TP_DIM_A_COLS; j++) { // Mat A cols
                ptrInWindowA += indexMatA;
                for (int l = 0; l < TP_DIM_B_COLS; l++) { // Mat B cols
                    indexMatA = 0;
                    ptrInWindowB += indexMatB;
                    for (int numVecA = 0; numVecA < TP_DIM_A_ROWS / kSamplesInVectA;
                         numVecA++) { // Mat A: vectors per column
                        inDataVecA = *ptrInWindowA++;
                        indexMatA++;
                        for (int indexVecA = 0; indexVecA < kSamplesInVectA; indexVecA++) { // Mat A: elemets per vector
                            indexMatB = 0;
                            for (int numVecB = 0; numVecB < TP_DIM_B_ROWS / kSamplesInVectB;
                                 numVecB++) { // Mat B: vectors per column
                                inDataVecB = *ptrInWindowB++;
                                acc = ::aie::mul<acc_t>(inDataVecA[indexVecA], inDataVecB);
                                outDataVecTemp = acc.template to_vector<out_t>(TP_SHIFT);
// writeincr<aie_stream_resource_out::a, out_t, kSamplesInVectOut>(outStream0, outDataVec);
#pragma unroll(kSamplesInVectTempOut / kSamplesInVectOut)
                                for (int n = 0; n < kSamplesInVectTempOut / kSamplesInVectOut; n++) {
                                    writeincr<aie_stream_resource_out::a, out_t, kSamplesInVectOut>(
                                        outStream0, outDataVecTemp.template extract<kSamplesInVectOut>(n));
                                }
                                indexMatB++;
                            }
                            // this completes multiplication of each element of col j of matrix A with col l of matrix B
                            ptrInWindowB -= indexMatB;
                        }
                    }
                    // this completes multiplication of each element of col j of matrix A with col l of matrix B
                    ptrInWindowA -= indexMatA;
                }
                ptrInWindowB -= (TP_DIM_B_ROWS / kSamplesInVectB) * TP_DIM_B_COLS; // reset Mat B address
            }
            ptrInWindowB += (TP_DIM_B_ROWS / kSamplesInVectB) * TP_DIM_B_COLS; // Mat B: set address to frame start
        }
};
}
}
}
}
