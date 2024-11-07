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
#ifndef _DSPLIB_BITONIC_SORT_TRAITS_HPP_
#define _DSPLIB_BITONIC_SORT_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

template <unsigned int vecSize>
struct transposeType {
    static_assert((vecSize == 4) || (vecSize == 8) || (vecSize == 16) || (vecSize == 32),
                  "ERROR: vecSize must be 4, 8, 16 or 32.");
};
template <>
struct transposeType<4> {
    using type = cint32;
};
template <>
struct transposeType<8> {
    using type = int32;
};
template <>
struct transposeType<16> {
    using type = int16;
};
template <>
struct transposeType<32> {
    using type = int8;
};
template <int vecSize>
using transposeType_t = typename transposeType<vecSize>::type;

template <unsigned int powerOf2>
INLINE_DECL constexpr unsigned int fnLog2() {
    if
        constexpr(powerOf2 == 32768) { return 15; }
    else if
        constexpr(powerOf2 == 16384) { return 14; }
    else if
        constexpr(powerOf2 == 8192) { return 13; }
    else if
        constexpr(powerOf2 == 4096) { return 12; }
    else if
        constexpr(powerOf2 == 2048) { return 11; }
    else if
        constexpr(powerOf2 == 1024) { return 10; }
    else if
        constexpr(powerOf2 == 512) { return 9; }
    else if
        constexpr(powerOf2 == 256) { return 8; }
    else if
        constexpr(powerOf2 == 128) { return 7; }
    else if
        constexpr(powerOf2 == 64) { return 6; }
    else if
        constexpr(powerOf2 == 32) { return 5; }
    else if
        constexpr(powerOf2 == 16) { return 4; }
    else if
        constexpr(powerOf2 == 8) { return 3; }
    else if
        constexpr(powerOf2 == 4) { return 2; }
    else if
        constexpr(powerOf2 == 2) { return 1; }
    else if
        constexpr(powerOf2 == 1) { return 0; }
    else {
        return -1;
    }
};

template <unsigned int logBase2>
INLINE_DECL constexpr unsigned int fnPwr2() {
    if
        constexpr(logBase2 == 15) { return 32768; }
    else if
        constexpr(logBase2 == 14) { return 16384; }
    else if
        constexpr(logBase2 == 13) { return 8192; }
    else if
        constexpr(logBase2 == 12) { return 4096; }
    else if
        constexpr(logBase2 == 11) { return 2048; }
    else if
        constexpr(logBase2 == 10) { return 1024; }
    else if
        constexpr(logBase2 == 9) { return 512; }
    else if
        constexpr(logBase2 == 8) { return 256; }
    else if
        constexpr(logBase2 == 7) { return 128; }
    else if
        constexpr(logBase2 == 6) { return 64; }
    else if
        constexpr(logBase2 == 5) { return 32; }
    else if
        constexpr(logBase2 == 4) { return 16; }
    else if
        constexpr(logBase2 == 3) { return 8; }
    else if
        constexpr(logBase2 == 2) { return 4; }
    else if
        constexpr(logBase2 == 1) { return 2; }
    else if
        constexpr(logBase2 == 0) { return 1; }
    else {
        return -1;
    }
};

template <unsigned int logBase16>
INLINE_DECL constexpr uint64 fnPwr16() {
    if
        constexpr(logBase16 == 15) { return 0x1000000000000000u; }
    else if
        constexpr(logBase16 == 14) { return 0x100000000000000u; }
    else if
        constexpr(logBase16 == 13) { return 0x10000000000000u; }
    else if
        constexpr(logBase16 == 12) { return 0x1000000000000u; }
    else if
        constexpr(logBase16 == 11) { return 0x100000000000u; }
    else if
        constexpr(logBase16 == 10) { return 0x10000000000u; }
    else if
        constexpr(logBase16 == 9) { return 0x1000000000u; }
    else if
        constexpr(logBase16 == 8) { return 0x100000000u; }
    else if
        constexpr(logBase16 == 7) { return 0x10000000u; }
    else if
        constexpr(logBase16 == 6) { return 0x1000000u; }
    else if
        constexpr(logBase16 == 5) { return 0x100000u; }
    else if
        constexpr(logBase16 == 4) { return 0x10000u; }
    else if
        constexpr(logBase16 == 3) { return 0x1000u; }
    else if
        constexpr(logBase16 == 2) { return 0x100u; }
    else if
        constexpr(logBase16 == 1) { return 0x10u; }
    else if
        constexpr(logBase16 == 0) { return 0x1u; }
    else {
        return -1;
    }
};

template <int stride>
INLINE_DECL constexpr uint32 fnGetInterleave() {
    if
        constexpr(stride == 1) { return 0b01010101010101010101010101010101; }
    else if
        constexpr(stride == 2) { return 0b00110011001100110011001100110011; }
    else if
        constexpr(stride == 4) { return 0b00001111000011110000111100001111; }
    else if
        constexpr(stride == 8) { return 0b00000000111111110000000011111111; }
    else if
        constexpr(stride == 16) { return 0b00000000000000001111111111111111; }
    else {
        return 0b11111111111111111111111111111111;
    } // ? Reaching this condition results in non-symmetric mask,
};    // ? revealing from_uint32 reads from LSB to MSB.

template <unsigned int stage>
INLINE_DECL constexpr unsigned int getOuterIdxForStage() {
    if
        constexpr(stage < 1) { return 0; }
    else if
        constexpr(stage < 3) { return 1; }
    else if
        constexpr(stage < 6) { return 2; }
    else if
        constexpr(stage < 10) { return 3; }
    else if
        constexpr(stage < 15) { return 4; }
    else if
        constexpr(stage < 21) { return 5; }
    else if
        constexpr(stage < 28) { return 6; }
    else if
        constexpr(stage < 36) { return 7; }
    else if
        constexpr(stage < 45) { return 8; }
    else if
        constexpr(stage < 55) { return 9; }
    else if
        constexpr(stage < 66) { return 10; }
    else if
        constexpr(stage < 78) { return 11; }
    else if
        constexpr(stage < 91) { return 12; }
    else if
        constexpr(stage < 105) { return 13; }
    else if
        constexpr(stage < 120) { return 14; }
    else if
        constexpr(stage < 136) { return 15; }
};
template <unsigned int outerIdx>
INLINE_DECL constexpr unsigned int getInitStageForOuterIdx() {
    if
        constexpr(outerIdx == 0) { return 0; }
    else if
        constexpr(outerIdx == 1) { return 1; }
    else if
        constexpr(outerIdx == 2) { return 3; }
    else if
        constexpr(outerIdx == 3) { return 6; }
    else if
        constexpr(outerIdx == 4) { return 10; }
    else if
        constexpr(outerIdx == 5) { return 15; }
    else if
        constexpr(outerIdx == 6) { return 21; }
    else if
        constexpr(outerIdx == 7) { return 28; }
    else if
        constexpr(outerIdx == 8) { return 36; }
    else if
        constexpr(outerIdx == 9) { return 45; }
    else if
        constexpr(outerIdx == 10) { return 55; }
    else if
        constexpr(outerIdx == 11) { return 66; }
    else if
        constexpr(outerIdx == 12) { return 78; }
    else if
        constexpr(outerIdx == 13) { return 91; }
    else if
        constexpr(outerIdx == 14) { return 105; }
    else if
        constexpr(outerIdx == 15) { return 120; }
};

template <unsigned int TP_DIM>
INLINE_DECL constexpr unsigned int getNumStages() {
    return (fnLog2<TP_DIM>() + 1) * fnLog2<TP_DIM>() / 2;
};

template <int i, int j, int idx>
INLINE_DECL constexpr unsigned int getCurrStage() {
    // Compile time function which recursively walks back through loops to get current stage index.
    if
        constexpr(j <= 0) {
            if
                constexpr(i <= 0) { return idx; }
            else {
                return getCurrStage<i - 1, i - 1, idx + 1>();
            }
        }
    else {
        return getCurrStage<i, j - 1, idx + 1>();
    }
};

INLINE_DECL constexpr unsigned int getCurrStage(int i, int j, int idx) {
    // Run time function which recursively walks back through loops to get current stage index.
    if (j <= 0) {
        if (i <= 0) {
            return idx;
        } else {
            return getCurrStage(i - 1, j - 1, idx + 1);
        }
    } else {
        return getCurrStage(i, j - 1, idx + 1);
    }
};

template <int i, int j, int k, unsigned int TP_ASCEND>
INLINE_DECL constexpr uint64 getMinMaxIdx() {
    constexpr uint64 stride = k & (-fnPwr2<i - j>());

    if
        constexpr((j == 0) && (1 - TP_ASCEND)) {
            constexpr uint64 addend = k & (fnPwr2<i>() - 1);
            return 2 * stride + fnPwr2<i + 1>() - (addend + 1);
        }
    else {
        return stride + k + (1 - TP_ASCEND) * fnPwr2<i - j>();
    }
};

// TODO: Optimize this
template <unsigned int TP_ASCEND>
INLINE_DECL int getMinMaxIdxRegRuntime(int& i, int& j, int& k) {
    uint64 stride = k & (-(1 << (i - j)));

    return stride + k + (1 - TP_ASCEND) * (1 << (i - j));
};

// TODO: Optimize this
template <unsigned int TP_ASCEND>
INLINE_DECL int getMinMaxIdxHandRuntime(int& i, int& k) {
    uint64 stride = k & (-(1 << i));

    if
        constexpr(1 - TP_ASCEND) {
            uint64 addend = k & ((1 << i) - 1);
            return 2 * stride + (1 << (i + 1)) - (addend + 1);
        }
    else {
        return stride + k + (1 - TP_ASCEND) * (1 << i);
    }
};

template <int i, int j, bool TOP, unsigned int vecSize, int iter, uint64 shuffle>
INLINE_DECL constexpr uint64 getShuffleIn() {
    if
        constexpr(iter < vecSize) {
            return getShuffleIn<i, j, TOP, vecSize, iter + 1,
                                shuffle + getMinMaxIdx<i, j, iter, TOP>() * fnPwr16<iter>()>();
        }
    return shuffle;
};
template <uint64 shufTopIn, uint64 shufBotIn, uint64 shufTopOut, uint64 shufBotOut, uint32 vecSize, uint32 iter>
INLINE_DECL constexpr std::array<uint64, 2> getShuffleOut() {
    if
        constexpr(iter >= 2 * vecSize) {
            constexpr std::array<uint64, 2> shufOut = {shufTopOut, shufBotOut >> 32};
            return shufOut;
        }
    else {
        constexpr uint64 numerator = iter < vecSize ? shufTopIn : shufBotIn;
        constexpr uint64 denominator = iter < vecSize ? fnPwr16<iter>() : fnPwr16<iter - vecSize>();
        constexpr uint32 idx = (numerator / denominator) & 0xF;

        constexpr uint64 addend = fnPwr16<idx>() * iter;
        constexpr uint64 condition = idx < vecSize ? 1 : 0;
        constexpr uint64 shufTopOutNext = shufTopOut + addend * condition;
        constexpr uint64 shufBotOutNext = shufBotOut + addend * (1 - condition);

        return getShuffleOut<shufTopIn, shufBotIn, shufTopOutNext, shufBotOutNext, vecSize, iter + 1>();
    }
};
}
}
}
} // closing namespaces
#endif // _DSPLIB_BITONIC_SORT_TRAITS_HPP_
