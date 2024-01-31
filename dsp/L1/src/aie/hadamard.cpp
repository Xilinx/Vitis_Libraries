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
#include "hadamard_utils.hpp"

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
    output_buffer<TT_OUT>& __restrict outWindow) {
    static constexpr unsigned int vecSampleNumA = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteA;
    static constexpr unsigned int vecSampleNumB = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteB;
    static constexpr unsigned int vecSampleNumOut = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteOut;

    static constexpr unsigned int vecSampleNumBuff = std::max(vecSampleNumA, vecSampleNumB);

    static constexpr unsigned int kCaptureDataA = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataA;
    static constexpr unsigned int kCaptureDataB = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataB;
    static constexpr unsigned int kCaptureDataOut = vectByte<TT_DATA_A, TT_DATA_B>().kCaptureDataOut;

    using dataVectA_t = ::aie::vector<TT_DATA_A, vecSampleNumA>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, vecSampleNumB>;
    using dataVectOut_t = ::aie::vector<TT_OUT, vecSampleNumOut>;

    dataVectA_t* inAPtr = (dataVectA_t*)inWindowA.data();
    dataVectB_t* inBPtr = (dataVectB_t*)inWindowB.data();
    dataVectOut_t* outPtr = (dataVectOut_t*)outWindow.data();

    dataVectA_t vectA;
    dataVectB_t vectB;
    dataVectOut_t vectOut;

    ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type, vecSampleNumBuff> acc;
    ::aie::vector<TT_DATA_A, vecSampleNumBuff> vectAbuff;
    ::aie::vector<TT_DATA_B, vecSampleNumBuff> vectBbuff;
    ::aie::vector<TT_OUT, vecSampleNumBuff> vectOutbuff;

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

                acc = mul_samples_win<TT_DATA_A, TT_DATA_B>(vectAbuff, vectBbuff);
                vectOutbuff = acc.template to_vector<TT_OUT>(TP_SHIFT);

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
    output_stream<TT_OUT>* __restrict outStream0) {
    using accVect_t = ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type, kSamplesInVect>;
    using dataVectA_t = ::aie::vector<TT_DATA_A, kSamplesInVect>;
    using dataVectB_t = ::aie::vector<TT_DATA_B, kSamplesInVect>;
    using dataOutVect_t = ::aie::vector<TT_OUT, kSamplesInVect>;
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
                acc = mul_samples_stream<TT_DATA_A, TT_DATA_B>(dataVectA, dataVectB);
                outVect = acc.template to_vector<TT_OUT>((TP_SHIFT));
                writeincr<aie_stream_resource_out::a, TT_OUT, kSamplesInVect>(outStream0, outVect);
            }
        }
};
}
}
}
}
/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */