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
outer_tensor kernel code.
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
#include "outer_tensor.hpp"
#include "kernel_api_utils.hpp"
#include "outer_tensor_utils.hpp"
#include "outer_tensor_traits.hpp"
//#define _DSPLIB_OUTER_TENSOR_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {

//--------------------------------------------------------------------
// Base specialization, used for static size window API configurations
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
outer_tensor<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT>::
    outer_tensor_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                      input_buffer<TT_DATA_B>& __restrict inWindowB,
                      output_buffer<out_t>& __restrict outWindow) {
    using dataVectA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataAcc_t = ::aie::accum<acc_t, vecSampleNumAcc>;
    using dataVectTempOut_t = ::aie::vector<out_t, vecSampleNumTempOut>;
    using dataVectOut_t = ::aie::vector<out_t, vecSampleNumOut>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    dataVectOut_t* outPtr = (dataVectOut_t*)outWindow.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    dataVectA_t vectABuff;
    dataVectB_t vectBBuff;
    dataAcc_t acc;
    dataVectTempOut_t vectTempBuff;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            for (int i = 0; i < vecNumA; i++) chess_prepare_for_pipelining chess_loop_count(vecNumA) {
                    vectABuff = inAPtr[i];
#pragma unroll(vecSampleNumA)
                    for (int j = 0; j < vecSampleNumA; j++) {
#pragma unroll(vecNumB)
                        for (int k = 0; k < vecNumB; k++) {
                            vectBBuff = inBPtr[k];
                            acc = ::aie::mul<acc_t>(vectABuff[j], vectBBuff);
                            vectTempBuff = acc.template to_vector<outTypeMult_t<TT_DATA_A, TT_DATA_B> >(TP_SHIFT);
#pragma unroll(vecSampleNumTempOut / vecSampleNumOut)
                            for (int n = 0; n < vecSampleNumTempOut / vecSampleNumOut; n++) {
                                *outPtr++ = vectTempBuff.template extract<vecSampleNumOut>(n);
                            }
                        }
                    }
                }
            inAPtr += vecNumA;
            inBPtr += vecNumB;
        }
};

//--------------------------------------------------------------------
// Stream specialization for i/o buffer in and stream out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          // unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
outer_tensor<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SHIFT, 1, TP_SSR, TP_RND, TP_SAT>::
    outer_tensor_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                      input_buffer<TT_DATA_B>& __restrict inWindowB,
                      output_stream<out_t>* __restrict outStream0) {
    using dataVectA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataAcc_t = ::aie::accum<acc_t, vecSampleNumAcc>;
    using dataVectTempOut_t = ::aie::vector<out_t, vecSampleNumTempOut>;
    using dataVectOut_t = ::aie::vector<out_t, vecSampleNumOut>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    dataVectA_t vectABuff;
    dataVectB_t vectBBuff;
    dataAcc_t acc;
    dataVectTempOut_t vectTempBuff;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            for (int i = 0; i < vecNumA; i++) chess_prepare_for_pipelining chess_loop_count(vecNumA) {
                    vectABuff = inAPtr[i];
                    for (int j = 0; j < vecSampleNumA; j++) {
                        for (int k = 0; k < vecNumB; k++) chess_prepare_for_pipelining chess_loop_count(vecNumB) {
                                vectBBuff = inBPtr[k];
                                acc = ::aie::mul<acc_t>(vectABuff[j], vectBBuff);
                                vectTempBuff = acc.template to_vector<outTypeMult_t<TT_DATA_A, TT_DATA_B> >(TP_SHIFT);
#pragma unroll(vecSampleNumTempOut / vecSampleNumOut)
                                for (int n = 0; n < vecSampleNumTempOut / vecSampleNumOut; n++) {
                                    writeincr<aie_stream_resource_out::a, out_t, vecSampleNumOut>(
                                        outStream0, vectTempBuff.template extract<vecSampleNumOut>(n));
                                }
                            }
                    }
                }
            inAPtr += vecNumA;
            inBPtr += vecNumB;
        }
};
}
}
}
}
