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
bitonic_sort kernel code.
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
#include "aie_api/utils.hpp"
#include "bitonic_sort.hpp"
#include "kernel_api_utils.hpp"
#include "bitonic_sort_utils.hpp"
#include "bitonic_sort_traits.hpp"
// #define _DSPLIB_BITONIC_SORT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

//--------------------------------------------------------------------
// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          unsigned int TP_CASC_IDX>
NOINLINE_DECL void
bitonic_sort<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, TP_CASC_IDX>::bitonic_sort_main(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    using dataVect_t = ::aie::vector<TT_DATA, kVecSampleNum>;

    dataVect_t* __restrict inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* __restrict outPtr = (dataVect_t*)outWindow.data();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            dataVect_t* pingPong[2] = {inPtr, outPtr};
            dataVect_t* __restrict readPtr = inPtr;
            dataVect_t* __restrict writePtr = outPtr;
            int ping = 1;

            // INITIAL STAGES WHICH ARE INTRA...
            constexpr int iEnd = MIN(kIEnd, fnLog2<kVecSampleNum>() - 1);
            ::aie::unroll_for<int, kIStart, iEnd + 1>([&](
                auto i) __attribute__((always_inline)) { // This is equivalent to for(int i = kIStart; i < kIEnd+1;
                                                         // i++){...} except i is a constant

                constexpr int jStart = getJIdx<i, kIStart, kJStart, 0>();
                constexpr int jEnd = getJIdx<i, kIEnd, kJEnd, i>();
                ::aie::unroll_for<int, jStart, jEnd + 1>([&](auto j) __attribute__((always_inline)) {

                    intrasort<TT_DATA, TP_DIM, TP_ASCENDING, i, j, kVecSampleNum, kUnrollMax>(readPtr, writePtr);
                    if
                        constexpr(getCurrStage<i, j, 0>() == kFirstStage) {
                            chess_memory_fence();
                        } // Required as sort of interference at low list sizes on the first stage.
                    swap<TT_DATA, kVecSampleNum>(readPtr, writePtr, pingPong, ping);
                });
            });

            if
                constexpr(TP_CASC_LEN == 1) {
                    // * If one kernel run this loop which has smaller microcode.
                    // * This is in essence what the cascade version does but the cascade code can start/end partly
                    // through the loop.
                    // * The ability to start/stop partly through a loop significantly bloats the microcode such that a
                    // single kernel
                    // * would run out of program memory, hence the necessity to run this loop. Cascaded kernels divide
                    // the program
                    // * memory between them, such that this is not a concern when running a cascaded design.

                    for (int i = fnLog2<kVecSampleNum>(); i < fnLog2<TP_DIM>(); i++)
                        chess_prepare_for_pipelining chess_loop_count(fnLog2<TP_DIM / kVecSampleNum>()) {
                            intersort_handover<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(readPtr,
                                                                                                         writePtr, i);

                            for (int j = 1; j < i + 1 - fnLog2<kVecSampleNum>(); j++) {
                                intersort<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(readPtr, writePtr,
                                                                                                    i, j);
                            }

                            ::aie::unroll_for<int, 1, fnLog2<kVecSampleNum>() + 1>([&](
                                auto j) __attribute__((always_inline)) {

                                intrasort<TT_DATA, TP_DIM, TP_ASCENDING, fnLog2<kVecSampleNum>(), j, kVecSampleNum,
                                          kUnrollMax>(readPtr, writePtr);
                                if
                                    constexpr(j == 1) {
                                        chess_memory_fence();
                                    } // Mitigates memory conflicts on first stage of intra..
                                swap<TT_DATA, kVecSampleNum>(readPtr, writePtr, pingPong, ping);
                            });
                        }
                }

            else { // * Cascaded design which allows start/stop part way through loops.
                // TODO: Code is convoluted. Consider whether it can be simplified.
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////

                constexpr int firstInterStage = getCurrStage<fnLog2<kVecSampleNum>(), 0, 0>();
                if
                    constexpr(firstInterStage <= kLastStage) {
                        constexpr int stage = MAX(firstInterStage, kFirstStage);
                        constexpr int iFirst = getOuterIdxForStage<stage>();
                        constexpr int jFirst = stage - getInitStageForOuterIdx<iFirst>();
                        int i = iFirst;
                        int j = jFirst;

                        // INTERSORT HANDOVER..
                        if
                            constexpr(jFirst == 0) {
                                intersort_handover<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(
                                    readPtr, writePtr, i);
                                j++;
                            }

                        // INTERSORT REGULAR..
                        constexpr int jStartInter = jFirst == 0 ? 1 : jFirst;
                        constexpr int jEndInterThreshold = iFirst - fnLog2<kVecSampleNum>();
                        constexpr int jIdx = getJIdx<iFirst, kIEnd, kJEnd, iFirst>();
                        constexpr int jEndInter = MIN(jIdx, jEndInterThreshold);

                        for (; j < jEndInter + 1; j++) // can re-use the j I declared earlier without redeclaring.
                            chess_prepare_for_pipelining chess_loop_count(MAX(0, jEndInter - jStartInter)) {
                                intersort<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(readPtr, writePtr,
                                                                                                    i, j);
                            }

                        // INTRASORT..
                        constexpr int jStartIntra = MAX(jEndInter + 1, jFirst);
                        constexpr int jEndIntra = getJIdx<iFirst, kIEnd, kJEnd, iFirst>();

                        ::aie::unroll_for<unsigned int, jStartIntra, jEndIntra + 1>([&](
                            auto j) __attribute__((always_inline)) {

                            intrasort<TT_DATA, TP_DIM, TP_ASCENDING, iFirst, j, kVecSampleNum, kUnrollMax>(readPtr,
                                                                                                           writePtr);
                            if
                                constexpr(j == jStartIntra) { chess_memory_fence(); }
                            swap<TT_DATA, kVecSampleNum>(readPtr, writePtr, pingPong, ping);
                        });

                        ///////////////////////////////////////////////////////////////////////////////////////////////////////

                        if
                            constexpr(getCurrStage<iFirst, jEndIntra, 0>() < kLastStage) {
                                for (int i = iFirst + 1; i < kIEnd; i++)
                                    chess_prepare_for_pipelining chess_loop_count(kIEnd - iFirst - 1) {
                                        intersort_handover<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(
                                            readPtr, writePtr, i);

                                        for (int j = 1; j < i + 1 - fnLog2<kVecSampleNum>(); j++) {
                                            intersort<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(
                                                readPtr, writePtr, i, j);
                                        }

                                        ::aie::unroll_for<int, 1, fnLog2<kVecSampleNum>() + 1>([&](
                                            auto j) __attribute__((always_inline)) {

                                            intrasort<TT_DATA, TP_DIM, TP_ASCENDING, fnLog2<kVecSampleNum>(), j,
                                                      kVecSampleNum, kUnrollMax>(readPtr, writePtr);
                                            // if constexpr(j == 1){ chess_memory_fence(); } // ! I do not know why but
                                            // this has the capability to break cascaded designs..
                                            chess_memory_fence(); // ! The fence needs to occur at every iteration for
                                                                  // this corruption to not occur (unlike everywhere
                                                                  // else)
                                            swap<TT_DATA, kVecSampleNum>(readPtr, writePtr, pingPong, ping);
                                        });
                                    }

                                ///////////////////////////////////////////////////////////////////////////////////////////////////

                                int i = kIEnd;

                                // INTERSORT HANDOVER..
                                intersort_handover<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(
                                    readPtr, writePtr, i);

                                // INTERSORT REGULAR..
                                constexpr int jEndLastInter = MIN(kJEnd, kIEnd - fnLog2<kVecSampleNum>());

                                for (int j = 1; j < jEndLastInter + 1; j++)
                                    chess_prepare_for_pipelining chess_loop_count(jEndLastInter) {
                                        intersort<TT_DATA, TP_DIM, TP_ASCENDING, kVecSampleNum, kUnrollMax>(
                                            readPtr, writePtr, i, j);
                                    }

                                // INTRASORT..
                                constexpr int jStartLastIntra = jEndLastInter + 1;

                                ::aie::unroll_for<unsigned int, jStartLastIntra, kJEnd + 1>([&](
                                    auto j) __attribute__((always_inline)) {

                                    intrasort<TT_DATA, TP_DIM, TP_ASCENDING, kIEnd, j, kVecSampleNum, kUnrollMax>(
                                        readPtr, writePtr);
                                    if
                                        constexpr(j == jStartLastIntra) { chess_memory_fence(); }
                                    swap<TT_DATA, kVecSampleNum>(readPtr, writePtr, pingPong, ping);
                                });
                            }
                    }
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
            // workaround for for aggressive compiler optimzation that results in functional mismatches in certain
            // cascaded designs
            chess_separator_scheduler();
            // If the final write was to the inPtr, then this will resolve to true regardless of inter or intra.
            if (writePtr == outPtr) {
                constexpr unsigned int kPragmaNum = MIN(kNumVecs, kUnrollMax); // How many unrolls of inner loop.
                constexpr unsigned int kChessNum = kNumVecs / kPragmaNum;      // How many pipelines of inner loop.

                int iter = 0;
                for (int k_chess = 0; k_chess < kChessNum; k_chess++)
                    chess_prepare_for_pipelining chess_loop_count(kChessNum) {
#pragma unroll(kPragmaNum)
                        for (int k_pragma = 0; k_pragma < kPragmaNum; k_pragma++) {
                            outPtr[iter] = inPtr[iter];
                            iter++;
                        }
                    }
            }

            inPtr += kNumVecs;
            outPtr += kNumVecs;
        }
};
}
}
}
}
