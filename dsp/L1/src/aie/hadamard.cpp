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
hadamard kernel code.
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
#include "hadamard.hpp"
#include "kernel_api_utils.hpp"
#include "hadamard_traits.hpp"

//#define _DSPLIB_HADAMARD_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {

// Constructor for windowed (simple) config
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL
hadamard<TT_DATA_A, TT_DATA_B, TP_DIM, TP_NUM_FRAMES, TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT>::hadamard(){};

// Constructor for streaming config
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL hadamard<TT_DATA_A, TT_DATA_B, TP_DIM, TP_NUM_FRAMES, TP_SHIFT, 1, TP_SSR, TP_RND, TP_SAT>::hadamard(){};

// Base specialization, used for static size window API configurations
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
hadamard<TT_DATA_A, TT_DATA_B, TP_DIM, TP_NUM_FRAMES, TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT>::hadamard_main(
    input_buffer<TT_DATA_A>& __restrict inWindowA,
    input_buffer<TT_DATA_B>& __restrict inWindowB,
    output_buffer<out_t>& __restrict outWindow) {
    static constexpr unsigned int vecSampleNumA = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteA;
    static constexpr unsigned int vecSampleNumB = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteB;
    static constexpr unsigned int vecSampleNumOut = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteOut;

    static constexpr unsigned int vecSampleNumBuff = std::max(vecSampleNumA, vecSampleNumB);

    static constexpr unsigned int kCaptureDataA = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataA;
    static constexpr unsigned int kCaptureDataB = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataB;
    static constexpr unsigned int kCaptureDataOut = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataOut;

    using dataVectA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataVectOut_t = ::aie::vector<out_t, vecSampleNumOut>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    dataVectOut_t* outPtr = (dataVectOut_t*)outWindow.data();

    dataVectA_t vectA;
    dataVectB_t vectB;
    dataVectOut_t vectOut;

    ::aie::accum<acc_t, vecSampleNumBuff> acc;
    ::aie::vector<TT_DATA_A, vecSampleNumBuff> vectAbuff;
    ::aie::vector<TT_DATA_B, vecSampleNumBuff> vectBbuff;
    ::aie::vector<out_t, vecSampleNumBuff> vectOutbuff;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
#pragma unroll(kVecInFrame)
            for (int vect = 0; vect < kVecInFrame; vect++) {
                // CaptureAndMul<TT_DATA_A, TT_DATA_B>(inAPtr, inBPtr, outPtr, TP_SHIFT);

                for (int sampleA = 0; sampleA < kCaptureDataA; sampleA++) {
                    vectA = *inAPtr++;
                    vectAbuff.insert(sampleA, vectA);
                }

                for (int sampleB = 0; sampleB < kCaptureDataB; sampleB++) {
                    vectB = *inBPtr++;
                    vectBbuff.insert(sampleB, vectB);
                }

                acc = ::aie::mul<acc_t>(vectAbuff, vectBbuff);
                vectOutbuff = acc.template to_vector<out_t>(TP_SHIFT);
                for (int sampleOut = 0; sampleOut < kCaptureDataOut; sampleOut++) {
                    *outPtr++ = vectOutbuff.template extract<vecSampleNumOut>(sampleOut);
                }
            }
        }
};

// Specialization, for stream API configurations (1 stream per tile).
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
hadamard<TT_DATA_A, TT_DATA_B, TP_DIM, TP_NUM_FRAMES, TP_SHIFT, 1, TP_SSR, TP_RND, TP_SAT>::hadamard_main(
    input_stream<TT_DATA_A>* __restrict inStreamA,
    input_stream<TT_DATA_B>* __restrict inStreamB,
    output_stream<out_t>* __restrict outStream0) {
    using accVect_t = ::aie::accum<acc_t, kSamplesInVect>;
    using dataVectA_t = ::aie::vector<TT_DATA_A, kSamplesInVect>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kSamplesInVect>;
    using dataOutVect_t = ::aie::vector<out_t, kSamplesInVect>;
    dataOutVect_t outVect;
    dataVectA_t dataVectA;
    dataVectB_t dataVectB;
    accVect_t acc;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
#pragma unroll(kVecInFrame)
            for (int vect = 0; vect < kVecInFrame; vect++) {
                dataVectA = readincr_v<kSamplesInVect, aie_stream_resource_in::a>(inStreamA);
                dataVectB = readincr_v<kSamplesInVect, aie_stream_resource_in::b>(inStreamB);

                acc = ::aie::mul<acc_t>(dataVectA, dataVectB);
                outVect = acc.template to_vector<out_t>((TP_SHIFT));
                writeincr<aie_stream_resource_out::a, out_t, kSamplesInVect>(outStream0, outVect);
            }
        }
};
}
}
}
}
