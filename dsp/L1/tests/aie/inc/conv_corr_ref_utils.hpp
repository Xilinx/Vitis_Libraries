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
#ifndef _DSPLIB_CONV_CORR_REF_UTILS_HPP_
#define _DSPLIB_CONV_CORR_REF_UTILS_HPP_

/*
    CONV_CORR reference model utility helpers
*/

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <type_traits>
#include <adf.h>
#include "device_defs.h"
#include "fir_ref_utils.hpp"

using namespace adf;
using namespace std;

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#define SHIFT_MAX 62
#define SHIFT_MIN 0
#define SAT_MODE_MAX 3
#define SAT_MODE_MIN 0

// conv/corr compute modes
#define FULL_MODE 0
#define SAME_MODE 1
#define VALID_MODE 2

// Function_Type
#define CONV 1
#define CORR 0

// RTP Port related flags
#define USE_RTP_VECTOR_LENGTHS_TRUE 1
#define USE_RTP_VECTOR_LENGTHS_FALSE 0

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

static constexpr unsigned int kMaxBitsRegY = 1024;                            // Max number of bits on Y Register.
static constexpr unsigned int kMaxBitsRegX = 512;                             // Max number of bits on X Register.
static constexpr unsigned int kMaxBitsRegW = 256;                             // Max number of bits on W Register.
static constexpr unsigned int kMaxBitsRegV = 128;                             // Max number of bits on V Register.
static constexpr unsigned int kMaxBitsLoadOnAie = 256;                        // Max number of bits Load on AIE
static constexpr unsigned int kMaxBytesLoadOnAie = 32;                        // Max number of bytes (256/8) Load on AIE
static constexpr unsigned int kMaxBufferLenOnAieInBytes = __DATA_MEM_BYTES__; // Max buffer Length
static constexpr unsigned int kBuffSize16Byte = 16;                           // 128-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;                           // 256-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;                           // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize128Byte = 128;                         // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffIndx1 = 1;                                 // updW index to update the vector
static constexpr unsigned int kBuffIndx2 = 2;                                 // updW index to update the vector

//**************************************************************************************/
//****   STREAM Related traits which were used when TP_API=1 and AIE_VARIANT=1      ****/
//**************************************************************************************/
static constexpr unsigned int kMaxNumOfPhases =
    16; // Max number of Phases supported by the design of stream processing conv/corr
static constexpr unsigned int kDataBuffLenFactor = 4; // DataBuffLenFactor for stream based conv/corr on AIE-1
static constexpr unsigned int kMinDataBuffLen = 16;   // MinDataBuffLen for stream based conv/corr on AIE-1
static constexpr unsigned int kShiftFactor2 = 2;      // MulFactor2
static constexpr unsigned int kMaxNumOfStreams = 2;   // Maximum Number Of Streams is 2 on AIE-1 for conv/corr
static constexpr unsigned int kStreamSupportsOnlyValidMode =
    2; // Stream Processing supports on AIE-1 only with valid(2) mode only.
static constexpr unsigned int kMaxSamplesInShuffleVec = 16; // Max number of samples to use shuffle() operation is 16
static constexpr unsigned int kMinSamplesInShuffleVec = 4;  // Min number of samples to use shuffle() operation is 4
static constexpr unsigned int kStreamPhaseOffset = 0x3210u; // Base offset of Phase is 0x3210U
static constexpr unsigned int kMaxIndexOf32ByteVector = 4;  // |4|3|2|1| ===> 32byte vector -- Max index number is 4
static constexpr unsigned int kMaxIndexOf16ByteVector = 2;  // |2|1|     ===> 16byte vector -- Max index number is 2
static constexpr unsigned int kNumPoints2 = 2;              // Num of points is 2 for stream processing.
static constexpr unsigned int kNumPoints4 = 4;              // Num of points is 4 for stream processing
static constexpr int kXstepOfMac4Rot = -4;               // xstep is -4 for mac4_rot() intrinsic as per current design.
static constexpr unsigned int kMinLenOfGforStream = 8;   // Min Len of G should be 8 while doing stream processing
static constexpr unsigned int kMaxLenOfGforStream = 256; // Max Len of G should be 8 while doing stream processing

template <typename TT_DATA_F, typename TT_DATA_G>
struct t_CC_RefAccType {
    using type = cint64;
};

template <typename TT_DATA_F, typename TT_DATA_G>
struct outType {
    using type = cint16;
};

// int8 x int8
template <>
struct t_CC_RefAccType<int8, int8> {
    using type = int64;
};
template <>
struct outType<int8, int8> {
    using type = int16;
};

// int16 x int8
template <>
struct t_CC_RefAccType<int16, int8> {
    using type = int64;
};
template <>
struct outType<int16, int8> {
    using type = int16;
};

// int16
template <>
struct t_CC_RefAccType<int16, int16> {
    using type = int64;
};
template <>
struct outType<int16, int16> {
    using type = int32;
};

template <>
struct t_CC_RefAccType<int16, int32> {
    using type = int64;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};

template <>
struct t_CC_RefAccType<int16, cint16> {
    using type = cint64;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};

template <>
struct t_CC_RefAccType<int16, cint32> {
    using type = cint64;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};

// int32
template <>
struct t_CC_RefAccType<int32, int16> {
    using type = int64;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};

template <>
struct t_CC_RefAccType<int32, int32> {
    using type = int64;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};

template <>
struct t_CC_RefAccType<int32, cint16> {
    using type = cint64;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<int32, cint32> {
    using type = cint64;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};

// float
template <>
struct t_CC_RefAccType<float, float> {
    using type = float;
};
template <>
struct outType<float, float> {
    using type = float;
};

// cint16
template <>
struct t_CC_RefAccType<cint16, int16> {
    using type = cint64;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};

template <>
struct t_CC_RefAccType<cint16, int32> {
    using type = cint64;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<cint16, cint16> {
    using type = cint64;
};
template <>
struct outType<cint16, cint16> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<cint16, cint32> {
    using type = cint64;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};

// cint32
template <>
struct t_CC_RefAccType<cint32, int16> {
    using type = cint64;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<cint32, cint16> {
    using type = cint64;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<cint32, int32> {
    using type = cint64;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};

template <>
struct t_CC_RefAccType<cint32, cint32> {
    using type = cint64;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};

// cfloat
template <>
struct t_CC_RefAccType<cfloat, float> {
    using type = cfloat;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};

template <>
struct t_CC_RefAccType<cfloat, cfloat> {
    using type = cfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};

template <typename T_D_A, typename T_D_B>
using t_CC_RefAccType_t = typename t_CC_RefAccType<T_D_A, T_D_B>::type;

template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

template <typename TT_DATA_F>
INLINE_DECL constexpr bool fnCheckDType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDType<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDType<cfloat>() {
    return true;
};

// Function to return the Lanes of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnNumOfLanesForMac4Rot() {
    return 4;
}; // defualt Lanes are 4

template <>
INLINE_DECL constexpr unsigned int fnNumOfLanesForMac4Rot<cint16>() {
    return 4;
};

// Function to return the Points of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnNumOfPointsForMac4Rot() {
    return 2;
}; // defualt Points are 2

template <>
INLINE_DECL constexpr unsigned int fnNumOfPointsForMac4Rot<cint16>() {
    return 2;
};

// ************************************** ************ //
// ****      END of stream related traits      ******* //
// ************************************** ************ //

// Lanes, Muls/Macs and data load
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnRefNumLanes() {
    return 32; // defualt return for AIE2
};
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnRefNumMuls() {
    return 256; // defualt return for AIE2
};
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF() {
    return 8; // defualt sample size is 8
};

template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples() {
    return 32; // defualt multiple factor is 32 for both AIE-1 and AIE-2
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
// AIE-1
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int8, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint32, int16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cfloat, cfloat>() {
    return 4;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int8, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int16, int8>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int16, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cfloat, cfloat>() {
    return 4;
}; //

#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
// AIE-2
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int8, int8>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, int32>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumLanes<cint32, cint16>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int8, int8>() {
    return 256;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int16, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<int32, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, int32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint16, cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint32, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumMuls<cint32, cint16>() {
    return 8;
}; //

#endif

// function to return number of samples for given data type of F Sig
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<cfloat>() {
    return 64;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamplesizeF<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<cfloat>() {
    return 64;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnRefNumOfSamples<bfloat16>() {
    return 16;
}; //
#endif

// Conjugate Function
template <typename T_F>
INLINE_DECL T_F conjugate(T_F& inData) {
    return inData;
}; //

// int8
template <>
INLINE_DECL int8 conjugate(int8& inData) {
    inData = inData;
    return inData;
}; //

// int16
template <>
INLINE_DECL int16 conjugate(int16& inData) {
    inData = inData;
    return inData;
}; //

// int32
template <>
INLINE_DECL int32 conjugate(int32& inData) {
    inData = inData;
    return inData;
}; //

// cint16
template <>
INLINE_DECL cint16 conjugate(cint16& inData) {
    inData.real = inData.real;
    inData.imag = (-1) * inData.imag;
    return inData;
}; //

// cint32
template <>
INLINE_DECL cint32 conjugate(cint32& inData) {
    inData.real = inData.real;
    inData.imag = (-1) * inData.imag;
    return inData;
}; //

// float32
template <>
INLINE_DECL float conjugate(float& inData) {
    inData = inData;
    return inData;
}; //

// bfloat16
template <>
INLINE_DECL bfloat16 conjugate(bfloat16& inData) {
    inData = inData;
    return inData;
}; //

// MULTIPLY_ACCUM Function
template <typename T_F, typename T_G, typename T_OUT>
void multiplyAccum(T_accRef<T_OUT>& accum, T_F& inDataF, T_G& inDataG){};

// int8xint8 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<int16>& accum, int8& inDataF, int8& inDataG) {
    accum.real += (int64_t)inDataF * inDataG;
};

// int16xint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<int32>& accum, int16& inDataF, int16& inDataG) {
    accum.real += (int64_t)inDataF * inDataG;
};

// int32xint32 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<int32>& accum, int32& inDataF, int16& inDataG) {
    accum.real += (int64_t)inDataF * inDataG;
};

// cint16xint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint32>& accum, cint16& inDataF, int16& inDataG) {
    accum.real += (int64_t)inDataF.real * inDataG;
    accum.imag += (int64_t)inDataF.imag * inDataG;
};

// cint16xint16 - cint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint16>& accum, cint16& inDataF, int16& inDataG) {
    accum.real += (int32)inDataF.real * inDataG;
    accum.imag += (int32)inDataF.imag * inDataG;
};

// cint16xint32 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint32>& accum, cint16& inDataF, int32& inDataG) {
    accum.real += (int64_t)inDataF.real * (int64_t)inDataG;
    accum.imag += (int64_t)inDataF.imag * (int64_t)inDataG;
};

// cint16xcint16- cint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint16>& accum, cint16& inDataF, cint16& inDataG) {
    accum.real += (int32)inDataG.real * (int32)inDataF.real - (int32)inDataG.imag * (int32)inDataF.imag;
    accum.imag += (int32)inDataG.real * (int32)inDataF.imag + (int32)inDataG.imag * (int32)inDataF.real;
};

// cint16xcint16 - cint32 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint32>& accum, cint16& inDataF, cint16& inDataG) {
    accum.real += (int64_t)inDataG.real * (int64_t)inDataF.real - (int64_t)inDataG.imag * (int64_t)inDataF.imag;
    accum.imag += (int64_t)inDataG.real * (int64_t)inDataF.imag + (int64_t)inDataG.imag * (int64_t)inDataF.real;
};

// cint32xint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint32>& accum, cint32& inDataF, int16& inDataG) {
    accum.real += (int64_t)inDataF.real * inDataG;
    accum.imag += (int64_t)inDataF.imag * inDataG;
};

// cint32xcint16 - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cint32>& accum, cint32& inDataF, cint16& inDataG) {
    accum.real += (int64_t)inDataG.real * (int64_t)inDataF.real - (int64_t)inDataG.imag * (int64_t)inDataF.imag;
    accum.imag += (int64_t)inDataG.real * (int64_t)inDataF.imag + (int64_t)inDataG.imag * (int64_t)inDataF.real;
};

// floatxfloat - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<float>& accum, float& inDataF, float& inDataG) {
    accum.real += (float)inDataF * inDataG;
};

// bfloat16xbfloat16 - multiplyAccum
#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL void multiplyAccum(T_accRef<float>& accum, bfloat16& inDataF, bfloat16& inDataG) {
    short* dataF = reinterpret_cast<short*>(&inDataF);
    short* dataG = reinterpret_cast<short*>(&inDataG);
    int data_F = (int)((*dataF) << 16);
    int data_G = (int)((*dataG) << 16);
    float* spfp32_F = reinterpret_cast<float*>(&data_F);
    float* spfp32_G = reinterpret_cast<float*>(&data_G);
    accum.real += (float)*spfp32_F * *spfp32_G;
};
#endif

// cfloatxfloat - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cfloat>& accum, cfloat& inDataF, float& inDataG) {
    accum.real += (float)inDataF.real * inDataG;
    accum.imag += (float)inDataF.imag * inDataG;
};

// cfloatxcfloat - multiplyAccum
template <>
INLINE_DECL void multiplyAccum(T_accRef<cfloat>& accum, cfloat& inDataF, cfloat& inDataG) {
    accum.real += (float)inDataG.real * (float)inDataF.real - (float)inDataG.imag * (float)inDataF.imag;
    accum.imag += (float)inDataG.real * (float)inDataF.imag + (float)inDataG.imag * (float)inDataF.real;
};

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return (((kMaxBufferLenOnAieInBytes >> kShiftFactor2) / sizeof(TT_DATA)));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return (((kMaxBitsLoadOnAie << 1) / fnRefNumOfSamples<TT_DATA>()));
};

// Function to return true or false by checking given length is in range or not
template <typename TT_DATA, unsigned int sigLen, unsigned int TP_API>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (kMaxBitsLoadOnAie / fnRefNumOfSamples<TT_DATA>());
    bool checkLen = false;

    if ((sigLen >= getMinLen<TT_DATA>()) && (sigLen <= getMaxLen<TT_DATA>())) {
        checkLen = (((sigLen % minDataLoad) == 0) ? true : false);
    }
    return (TP_API == USE_STREAM_API) ? true : checkLen;
};

// Function to return the LoopCount based on given data length and compute mode
template <unsigned int computeMode, unsigned int dataLenF, unsigned int dataLenG>
INLINE_DECL constexpr unsigned int getRefLoopCount() {
    unsigned int loopCount = 0;
    if (computeMode == FULL_MODE) // Full
    {
        loopCount = ((dataLenF + dataLenG) - 1);
    } else if (computeMode == SAME_MODE) // Same
    {
        loopCount = dataLenF;

    } else if (computeMode == VALID_MODE) // Valid
    {
        loopCount = ((dataLenF - dataLenG) + 1);
    } else {
        loopCount = ((dataLenF + dataLenG) - 1); // default Full mode
    }

    return loopCount;
};

// Function to return the refPaddedLength based on given data length and compute mode
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int computeMode,
          unsigned int dataLenF,
          unsigned int dataLenG>
INLINE_DECL constexpr unsigned int getRefPaddedLength() {
    unsigned int refPaddedLength = 0;
    unsigned int refLanes = fnRefNumLanes<TT_DATA_F, TT_DATA_G>();
    unsigned int refDataSamples = fnRefNumOfSamplesizeF<TT_DATA_F>();
    unsigned int refDataLoad = kMaxBitsLoadOnAie / refDataSamples;

    if (computeMode == FULL_MODE) // Full
    {
        refPaddedLength = ((dataLenG - 1) + dataLenF + (dataLenG - 1));
        refPaddedLength = CEIL(refPaddedLength, refDataLoad);
    } else if (computeMode == SAME_MODE) // Same
    {
        refPaddedLength = (((dataLenG >> 1) - 1) + dataLenF + ((dataLenG >> 1) - 1));
        refPaddedLength = CEIL(refPaddedLength, refDataLoad);
    } else if (computeMode == VALID_MODE) {
        if (refLanes > refDataLoad) // Valid
        {
            refPaddedLength = (dataLenF + (refDataLoad << 1));
            refPaddedLength = CEIL(refPaddedLength, refDataLoad);
        } else {
            refPaddedLength = (dataLenF + refDataLoad);
            refPaddedLength = CEIL(refPaddedLength, refDataLoad);
        }
    } else {
        // do nothing
    }

    return refPaddedLength;
};

// 8x8 16x8 16x16 32x16 c16x16 c16x32 c16xc16 c32x16 c32xc16 f32xf32 bf16xbf16
// Configuration Defensive check functions
//----------------------------------------------------------------------

// Check Output data type whether its complex if any input is complex
template <typename TT_DATA_F, typename TT_DATA_G, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataOutType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int8, int8, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int16, int8, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int16, int16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int32, int16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, cfloat>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, cfloat>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, float>() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, bfloat16, float>() {
    return true;
};
#endif

// Configuration Defensive check function for input data types whether supported or not
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer() {
    return false;
};

template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int8, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, cfloat>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, int16>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, cint16>() {
    return true;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int8, int8>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<bfloat16, bfloat16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, cfloat>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, int16>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, cint16>() {
    return false;
};
#endif

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckInputDataTypes() {
    return ((TP_API == USE_WINDOW_API) ? fnCheckInDataTypesOfIoBuffer<TT_DATA_F, TT_DATA_G>()
                                       : fnCheckInDataTypesOfStream<TT_DATA_F, TT_DATA_G>());
}

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataTypeOfOutput() {
    return (fnCheckDataOutType<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>() ? true : false);
}

// Function to check Len of G is in valid range or not while doing stream processing
template <unsigned int gLen>
INLINE_DECL constexpr bool fnCheckLenOfStreamforSigG() {
    return ((gLen >= kMinLenOfGforStream) && (gLen <= kMaxLenOfGforStream));
};

// Configuration Defensive check function to check Length of F Signal and G Signal
template <typename TT_DATA, unsigned int sigLen, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckLenOfData() {
    return (isLenInRange<TT_DATA, sigLen, TP_API>() ? true : false);
}

// Configuration Defensive check function to check whether strem process supported by AIE-1 or AIE-2
template <unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsStreamSupportedbyArch() {
    if (TP_API == USE_STREAM_API) {
#if (__HAS_ACCUM_PERMUTES__ == 1)
        return true;
#elif (__HAS_ACCUM_PERMUTES__ == 0)
        return false;
#endif
    } else {
        return true;
    }
};

// Function which return true or false if Given Number is power of 2 or not
template <unsigned int inData>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((inData) && !(inData & (inData - 1))) ? true : false);
};

// Configuration Defensive check function to check numPhases which should be power of 2
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int gLen,
          unsigned int cascLen,
          unsigned int numPhases,
          unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckPhases() {
    bool isphasesValid = false;
    unsigned int muls = fnRefNumMuls<TT_DATA_F, TT_DATA_G>();
    unsigned int numOfCores4 =
        (gLen / muls); // muls are 8 Complex MACS for eg: 32(G_Len)/8(CMACS) = 4 is the Max CascLen (4 cores)
    if (TP_API == USE_STREAM_API && numPhases <= kMaxNumOfPhases && isPowerOfTwo<numPhases>()) {
        isphasesValid = ((cascLen == numOfCores4) && (numPhases > 1)) ? true : ((numPhases == 1) ? true : false);
    } else {
        isphasesValid = ((TP_API == USE_WINDOW_API) && (numPhases == 1)) ? true : false;
    }
    return isphasesValid;
};

// Configuration Defensive check function to check whether computeMode is VALID or not for stream processing
template <unsigned int computeMode, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsComputeModeValid() {
    return (((TP_API == USE_STREAM_API) && (computeMode != kStreamSupportsOnlyValidMode))
                ? false
                : ((computeMode > kStreamSupportsOnlyValidMode) ? false : true));
};

// Configuration Defensive check function to check Length of G Signal should be multiple
// of ((phases*lanes)*(Points/streamsPerCore)) when stream processing happening
template <typename TT_DATA_F, unsigned int fLen, unsigned int gLen, unsigned int numPhases, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckGLen() {
    bool isLenOfGvalid = true;
    bool isGlenGreaterThanFlen = (gLen > fLen) ? true : false;

    if (TP_API == USE_STREAM_API) {
        if ((isGlenGreaterThanFlen) && (!(fnCheckLenOfStreamforSigG<gLen>()))) {
            isLenOfGvalid = false;

        } else {
            if ((isGlenGreaterThanFlen) && (gLen < (numPhases << kShiftFactor2))) {
                isLenOfGvalid = false;

            } else if (gLen > (numPhases << kShiftFactor2)) {
                if ((isGlenGreaterThanFlen) && (gLen % (numPhases << (kShiftFactor2 + 1)) != 0)) {
                    isLenOfGvalid = false;
                }
            }
        }
    } else // Check for gLen when TP_API = USE_WINDOW_API (I/O BUFFER)
    {
        if (isGlenGreaterThanFlen) // gLen <= fLen
        {
            isLenOfGvalid = false;
        }
    }
    return isLenOfGvalid;
};

//  Configuration Defensive check function to check whether Glen/casc_len should be multiple of lanes*points
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int gLen,
          unsigned int cascLen,
          unsigned int numPhases,
          unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckCascLen() {
    bool isCascLenValid = false;
    unsigned int muls = fnRefNumMuls<TT_DATA_F, TT_DATA_G>(); // Num of CMACS are 8 per core on AIE-1.

    // CASCLEN is the parameter which will tell that how many cores are required to achieve maximum throughput i.e.,
    // 1GSPS
    unsigned int numOfCores4 =
        (gLen / muls); // muls are 8 Complex MACS for eg: 32(G_Len)/8(CMACS) = 4 is the Max CascLen (4 cores)
    unsigned int numOfCores2 =
        (gLen / (muls << 1)); // muls are 8 Complex MACS for eg: 32(G_Len)/16(CMACS) = 2 is the CascLen (2 cores)
    unsigned int numOfCores1 =
        (gLen /
         (muls << kShiftFactor2)); // muls are 8 Complex MACS for eg: 32(G_Len)/32(CMACS) = 1 is the CascLen (1 core)
    isCascLenValid =
        (TP_API == USE_STREAM_API)
            ? (((cascLen == (numOfCores4)) || (cascLen == (numOfCores2)) || (cascLen == (numOfCores1))) ? true : false)
            : true;

    return isCascLenValid;
};

} // End of namespace conv_corr {
} // End of namespace aie {
} // End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_REF_UTILS_HPP_