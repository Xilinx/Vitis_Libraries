/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
    using dataVectA_t = ::aie::vector<TT_DATA_A, kVecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kVecSampleNumB>;
    using dataVectBSub_t = ::aie::vector<TT_DATA_B, kVecSampleNumBSub>;
    using dataAcc_t = ::aie::accum<acc_t, kVecSampleNumBSub>;
    using dataVectOut_t = ::aie::vector<out_t, kVecSampleNumBSub>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    dataVectOut_t* outPtr = (dataVectOut_t*)outWindow.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            for (int i_chess = 0; i_chess < kChessNumI; i_chess++)
                chess_prepare_for_pipelining chess_loop_count(kChessNumI) {
                    int i_chess_add = i_chess * kUnrollsI;
#pragma unroll(kUnrollsI)
                    for (int i_pragma = 0; i_pragma < kUnrollsI; i_pragma++) {
                        int i = i_chess_add + i_pragma;

                        dataVectA_t vectA = inAPtr[i];

                        for (int j_chess = 0; j_chess < kChessNumJ; j_chess++)
                            chess_prepare_for_pipelining chess_loop_count(kChessNumJ) {
                                int j_chess_add = j_chess * kUnrollsJ;
#pragma unroll(kUnrollsJ)
                                for (int j_pragma = 0; j_pragma < kUnrollsJ; j_pragma++) {
                                    int j = j_chess_add + j_pragma;

                                    TT_DATA_A elemVectA = vectA[j];

                                    for (int k_chess = 0; k_chess < kChessNumK; k_chess++)
                                        chess_prepare_for_pipelining chess_loop_count(kChessNumK) {
                                            int k_chess_add = k_chess * kUnrollsK;
#pragma unroll(kUnrollsK)
                                            for (int k_pragma = 0; k_pragma < kUnrollsK; k_pragma++) {
                                                int k = k_chess_add + k_pragma;

                                                dataVectB_t vectB = inBPtr[k];

#pragma unroll(kNumExtractions)
                                                for (int n = 0; n < kNumExtractions; n++) {
                                                    dataVectBSub_t vectBSub =
                                                        vectB.template extract<kVecSampleNumBSub>(n);
                                                    dataAcc_t acc = ::aie::mul<acc_t>(elemVectA, vectBSub);
                                                    *outPtr++ = acc.template to_vector<out_t>(TP_SHIFT);
                                                }
                                            }
                                        }
                                }
                            }
                    }
                }
            inAPtr += kVecNumA;
            inBPtr += kVecNumB;
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
    using dataVectA_t = ::aie::vector<TT_DATA_A, kVecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kVecSampleNumB>;
    using dataVectBSub_t = ::aie::vector<TT_DATA_B, kVecSampleNumBSub>;
    using dataAcc_t = ::aie::accum<acc_t, kVecSampleNumBSub>;
    using dataVectOut_t = ::aie::vector<out_t, kVecSampleNumBSub>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            for (int i_chess = 0; i_chess < kChessNumI; i_chess++)
                chess_prepare_for_pipelining chess_loop_count(kChessNumI) {
                    int i_chess_add = i_chess * kUnrollsI;
#pragma unroll(kUnrollsI)
                    for (int i_pragma = 0; i_pragma < kUnrollsI; i_pragma++) {
                        int i = i_chess_add + i_pragma;

                        dataVectA_t vectA = inAPtr[i];

                        for (int j_chess = 0; j_chess < kChessNumJ; j_chess++)
                            chess_prepare_for_pipelining chess_loop_count(kChessNumJ) {
                                int j_chess_add = j_chess * kUnrollsJ;
#pragma unroll(kUnrollsJ)
                                for (int j_pragma = 0; j_pragma < kUnrollsJ; j_pragma++) {
                                    int j = j_chess_add + j_pragma;

                                    TT_DATA_A elemVectA = vectA[j];

                                    for (int k_chess = 0; k_chess < kChessNumK; k_chess++)
                                        chess_prepare_for_pipelining chess_loop_count(kChessNumK) {
                                            int k_chess_add = k_chess * kUnrollsK;
#pragma unroll(kUnrollsK)
                                            for (int k_pragma = 0; k_pragma < kUnrollsK; k_pragma++) {
                                                int k = k_chess_add + k_pragma;

                                                dataVectB_t vectB = inBPtr[k];

#pragma unroll(kNumExtractions)
                                                for (int n = 0; n < kNumExtractions; n++) {
                                                    dataVectBSub_t vectBSub =
                                                        vectB.template extract<kVecSampleNumBSub>(n);
                                                    dataAcc_t acc = ::aie::mul<acc_t>(elemVectA, vectBSub);
                                                    writeincr<aie_stream_resource_out::a, out_t, kVecSampleNumBSub>(
                                                        outStream0, acc.template to_vector<out_t>(TP_SHIFT));
                                                }
                                            }
                                        }
                                }
                            }
                    }
                }
            inAPtr += kVecNumA;
            inBPtr += kVecNumB;
        }
};
}
}
}
}
