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
#ifndef _DSPLIB_MERGE_SORT_CPP_
#define _DSPLIB_MERGE_SORT_CPP_

#include <adf.h>

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#ifndef __AIE_API_USE_NATIVE_1024B_VECTOR__
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#endif
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
#include "kernel_api_utils.hpp"
#include "merge_sort_traits.hpp"
#include "merge_sort.hpp"

#ifdef __X86SIM__
// #define _DSPLIB_MERGE_SORT_HPP_DEBUG_
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

namespace aie = ::aie;

//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
INLINE_DECL void kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>::mergeSortKernel(
    T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface) {
    // select architecture and run FIR iteration
    mergeSortSelectArch(inInterface, outInterface);
};

//----------------------------------------------------------------------------------------------------------------------
// Select arch - matVecMulBasic (iobuffer B) or matVecMultStream (stream B)
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
INLINE_DECL void kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>::mergeSortSelectArch(
    T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface) {
    if
        constexpr(TP_IN_API == kWindowAPI) { mergeSortBufferIn(inInterface, outInterface); }
    else {
        mergeSortStreamIn(inInterface, outInterface);
    }
}
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
INLINE_DECL void kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>::mergeSortStreamIn(
    T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface) {
    using dataVectStream_t = aie::vector<TT_DATA, kSamplesInVec>;
    TT_DATA aVal, bVal, d_Out;

    dataVectStream_t v_A, v_B, v_Out;
    v_A = readincr_v<kSamplesInVec>(inInterface.inStream0);
    if
        constexpr(TP_IN_API == kStreamCascAPI) { v_B = readincr_v<kSamplesInVec>(inInterface.inCascade); }
    else {
        v_B = readincr_v<kSamplesInVec>(inInterface.inStream1);
    }
    unsigned int aIdx = 0;
    unsigned int bIdx = 0;
    unsigned int aLoadCount = 1;
    unsigned int bLoadCount = 1;
    for (int i = 0; i < (kVecInFrame); i++) {
#pragma unroll(kSamplesInVec)
        for (int j = 0; j < kSamplesInVec; j++) {
            if ((v_A[aIdx] <= v_B[bIdx]) == TP_ASCENDING) {
                v_Out[j] = v_A[aIdx++];
                // load new A when ready
                if (aIdx == kSamplesInVec) {
                    aIdx = 0;
                    if (aLoadCount < (kVecInFrame / 2)) {
                        aLoadCount++;
                        v_A = readincr_v<kSamplesInVec>(inInterface.inStream0);
                    } else {
                        v_A = ::aie::broadcast<TT_DATA, kSamplesInVec>(sortLimit);
                    }
                }
            } else {
                v_Out[j] = v_B[bIdx++];
                // load new B when ready
                if (bIdx == kSamplesInVec) {
                    bIdx = 0;
                    if (bLoadCount < (kVecInFrame / 2)) {
                        bLoadCount++;
                        if
                            constexpr(TP_IN_API == kStreamCascAPI) {
                                v_B = readincr_v<kSamplesInVec>(inInterface.inCascade);
                            }
                        else {
                            v_B = readincr_v<kSamplesInVec>(inInterface.inStream1);
                        }
                    } else {
                        v_B = ::aie::broadcast<TT_DATA, kSamplesInVec>(sortLimit);
                    }
                }
            }
        }
        if
            constexpr(TP_OUT_API == kCascAPI) { writeincr(outInterface.outCascade, v_Out); }
        else {
            writeincr(outInterface.outStream, v_Out);
        }
    }
};
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
INLINE_DECL void kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>::mergeSortBufferIn(
    T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface) {
    using dataVectStream_t = aie::vector<TT_DATA, kSamplesInVec>;
    using dataVectWindow_t = aie::vector<TT_DATA, kSamplesInWindowVec>;
    TT_DATA aVal, bVal, d_Out;

    dataVectStream_t v_Out;

    dataVectWindow_t v_A, v_B;
    dataVectWindow_t* inPtrA = (dataVectWindow_t*)inInterface.inWindow0;
    dataVectWindow_t* inPtrB = (dataVectWindow_t*)inInterface.inWindow1;
    v_A = *inPtrA++;
    v_B = *inPtrB++;

    unsigned int aIdx = 0;
    unsigned int bIdx = 0;
    unsigned int aLoadCount = 1;
    unsigned int bLoadCount = 1;
    for (int i = 0; i < (kVecInFrame); i++) chess_prepare_for_pipelining chess_loop_count(kVecInFrame) {
#pragma unroll(kSamplesInVec)
            for (int j = 0; j < kSamplesInVec; j++) {
                if ((v_A[aIdx] <= v_B[bIdx]) == TP_ASCENDING) {
                    v_Out[j] = v_A[aIdx++];
                    // load new A when ready
                    if (aIdx == kSamplesInWindowVec) {
                        aIdx = 0;
                        if (aLoadCount < kVecInWindow) {
                            aLoadCount++;
                            v_A = *inPtrA++;
                        } else {
                            v_A = ::aie::broadcast<TT_DATA, kSamplesInWindowVec>(sortLimit);
                        }
                    }
                } else {
                    v_Out[j] = v_B[bIdx++];
                    // load new B when ready
                    if (bIdx == kSamplesInWindowVec) {
                        bIdx = 0;
                        if (bLoadCount < kVecInWindow) {
                            bLoadCount++;
                            v_B = *inPtrB++;
                        } else {
                            v_B = ::aie::broadcast<TT_DATA, kSamplesInWindowVec>(sortLimit);
                        }
                    }
                }
            }
            if
                constexpr(TP_OUT_API == kCascAPI) { writeincr(outInterface.outCascade, v_Out); }
            else {
                writeincr(outInterface.outStream, v_Out);
            }
        }
};
//////////////////////////////////////////////////// dual stream in, stream out ////////////////////////////////////////
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
NOINLINE_DECL void merge_sort<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>::bitonic_merge_main(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_stream<TT_DATA>* __restrict outStream) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;

    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outStream = outStream;

    this->mergeSortKernel(inInterface, outInterface);
};
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
NOINLINE_DECL void merge_sort<TT_DATA, kStreamCascAPI, kStreamAPI, TP_DIM, TP_ASCENDING>::bitonic_merge_main(
    input_stream<TT_DATA>* __restrict inStream0,
    input_cascade<TT_DATA>* __restrict inCascade,
    output_stream<TT_DATA>* __restrict outStream) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;

    inInterface.inStream0 = inStream0;
    inInterface.inCascade = inCascade;
    outInterface.outStream = outStream;
    this->mergeSortKernel(inInterface, outInterface);
};
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
NOINLINE_DECL void merge_sort<TT_DATA, kStreamCascAPI, kCascAPI, TP_DIM, TP_ASCENDING>::bitonic_merge_main(
    input_stream<TT_DATA>* __restrict inStream0,
    input_cascade<TT_DATA>* __restrict inCascade,
    output_cascade<TT_DATA>* __restrict outCascade) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;

    inInterface.inStream0 = inStream0;
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    this->mergeSortKernel(inInterface, outInterface);
};
/////////////////////////////////////////////////////// iobuffer in ////////////////////////////////////////////////////
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
NOINLINE_DECL void merge_sort<TT_DATA, kWindowAPI, kStreamAPI, TP_DIM, TP_ASCENDING>::bitonic_merge_main(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    output_stream<TT_DATA>* __restrict outStream) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;

    inInterface.inWindow0 = inWindow0.data();
    inInterface.inWindow1 = inWindow1.data();
    outInterface.outStream = outStream;

    this->mergeSortKernel(inInterface, outInterface);
};
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
NOINLINE_DECL void merge_sort<TT_DATA, kWindowAPI, kCascAPI, TP_DIM, TP_ASCENDING>::bitonic_merge_main(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    output_cascade<TT_DATA>* __restrict outCascade) {
    T_inputIF<TT_DATA> inInterface;
    T_outputIF<TT_DATA> outInterface;

    inInterface.inWindow0 = inWindow0.data();
    inInterface.inWindow1 = inWindow1.data();
    outInterface.outCascade = outCascade;
    this->mergeSortKernel(inInterface, outInterface);
};
}
}
}
}

#endif
