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
#ifndef _DSPLIB_BITONIC_SORT_UTILS_HPP_
#define _DSPLIB_BITONIC_SORT_UTILS_HPP_

// #define _DSPLIB_BITONIC_SORT_UTILS_DEBUG_

/*
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "bitonic_sort_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

void printMask(const char* preamble, uint32 myMask) {
    printf("%s = ", preamble);
    for (int i = 32; i > 0; i--) {
        printf("%d", (myMask >> (i - 1)) & 1);
    }
    printf("\n");
};

template <unsigned int i, unsigned int iEdgeCase, unsigned int jEdgeTrue, unsigned int jEdgeFalse>
INLINE_DECL constexpr unsigned int getJIdx() {
    if
        constexpr(i == iEdgeCase) { return jEdgeTrue; }
    return jEdgeFalse;
};

template <typename TT, unsigned int vecSize, unsigned int row, unsigned int col>
INLINE_DECL::aie::vector<TT, vecSize> transposeSpecial(typename ::aie::vector<TT, vecSize>& vectInput) {
    // * Special version of transpose which will interpret the datatype of the input such that it satisfies the number
    // * of rows and columns. It will then perform the transpose and reinterpret_cast the vector to the original data
    // * type for return.

    if
        constexpr(row > 1) {
            using vectCasted_t = ::aie::vector<transposeType_t<row * col>, row * col>;
            vectCasted_t vectCasted = ::aie::transpose(*reinterpret_cast<vectCasted_t*>(&vectInput), row, col);
            return *reinterpret_cast< ::aie::vector<TT, vecSize>*>(&vectCasted);
        }
    return vectInput;
};

#if (__SUPPORTS_COMPREHENSIVE_SHUFFLES__ == 1)
template <typename TT>
INLINE_DECL::aie::vector<TT, 64 / sizeof(TT)> shuffle(typename ::aie::vector<TT, 64 / sizeof(TT)>& vectBuff,
                                                      const int64& top,
                                                      const int64& bot) {
    return fpshuffle16(vectBuff, 0, top, bot);
}
template <>
INLINE_DECL::aie::vector<int32, 64 / sizeof(int32)> shuffle(typename ::aie::vector<int32, 64 / sizeof(int32)>& vectBuff,
                                                            const int64& top,
                                                            const int64& bot) {
    return shuffle16(vectBuff, 0, top, bot);
}
// template <>
// INLINE_DECL ::aie::vector<int16, 64/sizeof(int16)> shuffle(typename ::aie::vector<int16, 64/sizeof(int16)>& vectBuff,
//                                                     const int64& top, const int64& bot){
//     return ::aie::concat(shuffle32(vectBuff, 0, top, bot, 0x3210),
//                         shuffle32(vectBuff, 16, top, bot, 0x3210));
// }
#endif // __SUPPORTS_COMPREHENSIVE_SHUFFLES__

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_ASCEND,
          unsigned int i,
          unsigned int j,
          unsigned int vecSize,
          unsigned int kUnrollMax>
INLINE_DECL void intrasort(::aie::vector<TT_DATA, vecSize>* __restrict(&readPtr),
                           ::aie::vector<TT_DATA, vecSize>* __restrict(&writePtr)) {
    using dataVectHalf_t = ::aie::vector<TT_DATA, vecSize / 2>;
    using dataVect_t = ::aie::vector<TT_DATA, vecSize>;
    using pairVectHalf_t = ::std::pair<dataVectHalf_t, dataVectHalf_t>;
    using pairVect_t = ::std::pair<dataVect_t, dataVect_t>;
    using dataVectDouble_t = ::aie::vector<TT_DATA, 2 * vecSize>;

    constexpr unsigned int kNumVecs = TP_DIM / vecSize;

    if
        constexpr((sizeof(TT_DATA) == 4) && (__SUPPORTS_COMPREHENSIVE_SHUFFLES__ == 1)) {
#if (__SUPPORTS_COMPREHENSIVE_SHUFFLES__ == 1)

            constexpr unsigned int kPragmaNum = MIN(kNumVecs / 2, kUnrollMax); // How many unrolls of inner loop.
            constexpr unsigned int kChessNum = kNumVecs / (2 * kPragmaNum);    // How many pipelines of inner loop.

            constexpr uint64 top_in = getShuffleIn<i, j, TP_ASCEND, 16, 0, 0>();
            constexpr uint64 bot_in = getShuffleIn<i, j, 1 - TP_ASCEND, 16, 0, 0>();
            constexpr std::array<uint64, 2> shufOut =
                getShuffleOut<top_in, bot_in, 0, 0, 8, 0>(); // ! This only works with kVecSize of 8.
            constexpr uint64 top_out = shufOut[0];
            constexpr uint64 bot_out = shufOut[1];

            for (int k_chess = 0; k_chess < kChessNum; k_chess++)
                chess_prepare_for_pipelining chess_loop_count(kChessNum) {
                    int k_chess_add = k_chess * kPragmaNum;
#pragma unroll(kPragmaNum)
                    for (int k_pragma = 0; k_pragma < kPragmaNum; k_pragma++) {
                        int k = k_chess_add + k_pragma;

                        dataVectDouble_t vectBuff = ::aie::concat(readPtr[2 * k], readPtr[2 * k + 1]);
                        vectBuff = shuffle<TT_DATA>(vectBuff, top_in, bot_in);
                        dataVect_t v_top = vectBuff.template extract<vecSize>(0);
                        dataVect_t v_bot = vectBuff.template extract<vecSize>(1);
                        dataVect_t vectMin = ::aie::min(v_top, v_bot);
                        dataVect_t vectMax = ::aie::max(v_top, v_bot);
                        vectBuff = ::aie::concat(vectMin, vectMax);
                        vectBuff = shuffle<TT_DATA>(vectBuff, top_out, bot_out);
                        writePtr[2 * k] = vectBuff.template extract<vecSize>(0);
                        writePtr[2 * k + 1] = vectBuff.template extract<vecSize>(1);
                    }
                }

#endif // __SUPPORTS_COMPREHENSIVE_SHUFFLES__
        }
    else {
        constexpr unsigned int kPragmaNum = MIN(kNumVecs, kUnrollMax); // How many unrolls of inner loop.
        constexpr unsigned int kChessNum = kNumVecs / kPragmaNum;      // How many pipelines of inner loop.

        constexpr uint32 mask_xor = fnGetInterleave<fnPwr2<i - j>()>() ^ fnGetInterleave<fnPwr2<i + 1>()>();
        constexpr uint32 mask_descend = TP_ASCEND == 1 ? 0 : -1;
        constexpr uint32 mask_final = mask_xor ^ mask_descend;
        ::aie::mask<vecSize> myMask = ::aie::mask<vecSize>::from_uint32(
            mask_final); // get_submask not required. Will automatically select first vecSize bits.

        for (int k_chess = 0; k_chess < kChessNum; k_chess++) chess_prepare_for_pipelining chess_loop_count(kChessNum) {
                int k_chess_add = k_chess * kPragmaNum;
#pragma unroll(kPragmaNum)
                for (int k_pragma = 0; k_pragma < kPragmaNum; k_pragma++) {
                    int k = k_chess_add + k_pragma;

                    if
                        constexpr((fnPwr2<i + 1>() > vecSize) && (__MIN_REGSIZE__ == 128)) {
                            dataVect_t vectBuff =
                                transposeSpecial<TT_DATA, vecSize, vecSize / fnPwr2<i - j + 1>(), 2>(readPtr[k]);
                            dataVectHalf_t evens = vectBuff.template extract<vecSize / 2>(0);
                            dataVectHalf_t odds = vectBuff.template extract<vecSize / 2>(1);
                            dataVectHalf_t vectMin = ::aie::min(evens, odds);
                            dataVectHalf_t vectMax = ::aie::max(evens, odds);
                            if
                                constexpr(TP_ASCEND) {
                                    writePtr[k] =
                                        ::aie::concat(::aie::interleave_zip(vectMin, vectMax, fnPwr2<i - j>()));
                                }
                            else {
                                writePtr[k] = ::aie::concat(::aie::interleave_zip(vectMax, vectMin, fnPwr2<i - j>()));
                            }
                        }
                    else {
                        dataVect_t vectBuff = transposeSpecial<TT_DATA, vecSize, vecSize / fnPwr2<i - j + 1>(), 2>(
                            readPtr[k]); // Separates the evens and odds in one vector operation.
                        pairVect_t evenOdd = ::aie::interleave_zip(
                            vectBuff, vectBuff, fnPwr2<i - j>()); // ! Necessary to throw away half the vector with
                                                                  // select() as the patterns are not interleave
                                                                  // zippable (e.g. 00111100)
                        dataVect_t vectMin = ::aie::min(evenOdd.first, evenOdd.second);
                        dataVect_t vectMax = ::aie::max(evenOdd.first, evenOdd.second);
                        writePtr[k] = ::aie::select(vectMin, vectMax, myMask);
                    }
                }
            }
        if
            constexpr((sizeof(TT_DATA) == 2) && (TP_DIM == 64)) {
                chess_memory_fence();
            } // TODO: This may be mitigated with working ping-pong buffer.
    };
}

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_ASCEND, unsigned int vecSize, unsigned int kUnrollMax>
INLINE_DECL void intersort(::aie::vector<TT_DATA, vecSize>* __restrict(&readPtr),
                           ::aie::vector<TT_DATA, vecSize>* __restrict(&writePtr),
                           int& i,
                           int& j) {
    constexpr unsigned int kNumVecs = TP_DIM / vecSize;
    constexpr unsigned int kPragmaNum = MIN(kNumVecs / 2, kUnrollMax); // How many unrolls of inner loop.
    constexpr unsigned int kChessNum = kNumVecs / (2 * kPragmaNum);    // How many pipelines of inner loop.

    using dataVect_t = ::aie::vector<TT_DATA, vecSize>;

    int iScaled = i - fnLog2<vecSize>();
    int k = 0;
    for (int k_chess = 0; k_chess < kChessNum; k_chess++) chess_prepare_for_pipelining chess_loop_count(kChessNum) {
#pragma unroll(kPragmaNum)
            for (int k_pragma = 0; k_pragma < kPragmaNum; k_pragma++) {
                int minIdx = getMinMaxIdxRegRuntime<TP_ASCEND>(iScaled, j, k);
                int maxIdx = getMinMaxIdxRegRuntime<1 - TP_ASCEND>(iScaled, j, k);
                dataVect_t vectBuff0 = readPtr[minIdx];
                dataVect_t vectBuff1 = readPtr[maxIdx];
                readPtr[minIdx] = ::aie::min(vectBuff0, vectBuff1);
                readPtr[maxIdx] = ::aie::max(vectBuff0, vectBuff1);
                k++;
            }
        }
};

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_ASCEND, unsigned int vecSize, unsigned int kUnrollMax>
INLINE_DECL void intersort_handover(::aie::vector<TT_DATA, vecSize>* __restrict(&readPtr),
                                    ::aie::vector<TT_DATA, vecSize>* __restrict(&writePtr),
                                    int& i) {
    constexpr unsigned int kNumVecs = TP_DIM / vecSize;
    constexpr unsigned int kPragmaNum = MIN(kNumVecs / 2, kUnrollMax); // How many unrolls of inner loop.
    constexpr unsigned int kChessNum = kNumVecs / (2 * kPragmaNum);    // How many pipelines of inner loop.

    using dataVect_t = ::aie::vector<TT_DATA, vecSize>;

    int iScaled = i - fnLog2<vecSize>();
    int k = 0;
    for (int k_chess = 0; k_chess < kChessNum; k_chess++) chess_prepare_for_pipelining chess_loop_count(kChessNum) {
#pragma unroll(kPragmaNum)
            for (int k_pragma = 0; k_pragma < kPragmaNum; k_pragma++) {
                int minIdx = getMinMaxIdxHandRuntime<TP_ASCEND>(iScaled, k);
                int maxIdx = getMinMaxIdxHandRuntime<1 - TP_ASCEND>(iScaled, k);
                dataVect_t vectBuff0 = readPtr[minIdx];
                dataVect_t vectBuff1 = ::aie::reverse(readPtr[maxIdx]);
                readPtr[minIdx] = ::aie::min(vectBuff0, vectBuff1);
                readPtr[maxIdx] = ::aie::max(vectBuff0, vectBuff1);
                k++;
            }
        }
};

template <typename TT, unsigned int vecSize>
INLINE_DECL void swap(::aie::vector<TT, vecSize>* __restrict(&pingPtr),
                      ::aie::vector<TT, vecSize>* __restrict(&pongPtr),
                      ::aie::vector<TT, vecSize>* __restrict (&pingPong)[2],
                      int& ping) {
    pingPtr = pingPong[ping];
    pongPtr = pingPong[1 - ping];
    ping = 1 - ping;
}
}
}
}
}

#endif // _DSPLIB_BITONIC_SORT_UTILS_HPP_
