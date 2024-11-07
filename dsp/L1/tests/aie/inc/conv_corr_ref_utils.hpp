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

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

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

// Function to return the size of Y Reg
static constexpr unsigned MAX_WIDTH_Y_REG() {
    return 1024;
};

// Function to return the size of X Reg
static constexpr unsigned MAX_WIDTH_X_REG() {
    return 512;
};

// Function to return the size of bits to load on AIE
static constexpr unsigned maxBitsLoadOnAie() {
    return 256;
};

// Function to return the size of bits to load on AIE
static constexpr unsigned MAX_BUFFER_LEN_ON_AIE_IN_BITS() {
    return 65536;
}; // 8192bytes --> (8192*8) bits

//**************************************************************************************//
//****   STREAM Related traits which were used when TP_API=1 and AIE_VARIANT=1      ****//
//**************************************************************************************//

// Function to return the minNumOfPhases
static constexpr unsigned minNumOfPhases() {
    return 1;
};

// Function to return the maxNumOfPhases
static constexpr unsigned maxNumOfPhases() {
    return 16;
};

// Function to return the dataBuffLenFactor for stream based conv/corr on AIE-1
static constexpr unsigned dataBuffLenFactor() {
    return 4;
};

// Function to return the minDataBuffLen for stream based conv/corr on AIE-1
static constexpr unsigned minDataBuffLen() {
    return 16;
};

// Function to return the mulFactor2
static constexpr unsigned mulFactor2() {
    return 2;
};

// Function to return the memAlignmentBy32
static constexpr unsigned memAlignmentBy32() {
    return 32;
};

// Function to return the TP_COMPUTE_MODE_IS_VALID_MODE
static constexpr unsigned TP_COMPUTE_MODE_IS_VALID_MODE() {
    return 2;
};

// Function to return the TP_API_IS_ONE
static constexpr unsigned TP_API_IS_ONE() {
    return 1;
};

// Function to return the min. Length of G_Sig when Stream based processing happens
static constexpr unsigned minLenOfG_Stream() {
    return 8;
};

// Function to return the max. Length of G_Sig when Stream based processing happens
static constexpr unsigned maxLenOfG_Stream() {
    return 256;
};

// Function to return the maximum supported streams by AIE-1 is 2
static constexpr unsigned maxNumOfStreams() {
    return 2;
};

// Function to return the Lanes of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int getLanesOfMac4RotIntrinsic() {
    return 4;
}; // defualt Lanes are 4

template <>
INLINE_DECL constexpr unsigned int getLanesOfMac4RotIntrinsic<cint16>() {
    return 4;
};

// Function to return the Points of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int getPointsOfMac4RotIntrinsic() {
    return 2;
}; // defualt Points are 2

template <>
INLINE_DECL constexpr unsigned int getPointsOfMac4RotIntrinsic<cint16>() {
    return 2;
};

// ************************************** ************ //
// ****      END of stream related traits      ******* //
// ************************************** ************ //

// Lanes, Muls/Macs and data load
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes() {
    return 32; // defualt return for AIE2
};
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls() {
    return 256; // defualt return for AIE2
};
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size() {
    return 8; // defualt sample size is 8
};

template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int ref_CC_Size() {
    return 32; // defualt multiple factor is 32 for both AIE-1 and AIE-2
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
// AIE-1
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int8, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint32, int16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cfloat, cfloat>() {
    return 4;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int8, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int16, int8>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int16, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cfloat, cfloat>() {
    return 4;
}; //

#endif

// AIE-2
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int8, int8>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, int32>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumLanes<cint32, cint16>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int8, int8>() {
    return 256;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int16, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<int32, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, int32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint16, cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint32, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_NumMuls<cint32, cint16>() {
    return 8;
}; //

#endif

// The below params for common for both AIE-1 and AIE-2
// function to return number of samples for given data type of F Sig
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<cfloat>() {
    return 64;
}; //
#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL constexpr unsigned int ref_CC_Fsample_Size<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<cfloat>() {
    return 64;
}; //
#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL constexpr unsigned int ref_CC_Size<bfloat16>() {
    return 16;
}; //
#endif

// Conjugate Function
template <typename T_F>
INLINE_DECL T_F conjugate(T_F inData) {
    return inData;
};

// int8
template <>
INLINE_DECL int8_t conjugate(int8_t inData) {
    inData = inData;
    return inData;
};

// int16
template <>
INLINE_DECL int16_t conjugate(int16_t inData) {
    inData = inData;
    return inData;
};

// int32
template <>
INLINE_DECL int32_t conjugate(int32_t inData) {
    inData = inData;
    return inData;
};

// cint16
template <>
INLINE_DECL cint16_t conjugate(cint16_t inData) {
    inData.real = inData.real;
    inData.imag = (-1) * inData.imag;
    return inData;
};

// cint32
template <>
INLINE_DECL cint32_t conjugate(cint32_t inData) {
    inData.real = inData.real;
    inData.imag = (-1) * inData.imag;
    return inData;
};

// float32
template <>
INLINE_DECL float conjugate(float inData) {
    inData = inData;
    return inData;
};

// bfloat16
template <>
INLINE_DECL bfloat16 conjugate(bfloat16 inData) {
    inData = inData;
    return inData;
};

// MULTIPLY_ACCUM Function
template <typename T_F, typename T_G, typename T_OUT>
void multiplyAccum(T_accRef<T_OUT>& accum, T_F inData_F, T_G inData_G){};

// int8xint8 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<int16_t>& accum, int8_t inData_F, int8_t inData_G) {
    accum.real += (int64_t)inData_F * inData_G;
};

// int16xint16 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<int32_t>& accum, int16_t inData_F, int16_t inData_G) {
    accum.real += (int64_t)inData_F * inData_G;
};

// int32xint32 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<int32_t>& accum, int32_t inData_F, int16_t inData_G) {
    accum.real += (int64_t)inData_F * inData_G;
};

// cint16xint16 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint32_t>& accum, cint16_t inData_F, int16_t inData_G) {
    accum.real += (int64_t)inData_F.real * inData_G;
    accum.imag += (int64_t)inData_F.imag * inData_G;
};

// cint16xint32 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint32_t>& accum, cint16_t inData_F, int32_t inData_G) {
    accum.real += (int64_t)inData_F.real * (int64_t)inData_G;
    accum.imag += (int64_t)inData_F.imag * (int64_t)inData_G;
};

// cint16xcint16- cint16 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint16_t>& accum, cint16_t inData_F, cint16_t inData_G) {
    accum.real += (int32_t)inData_G.real * (int32_t)inData_F.real - (int32_t)inData_G.imag * (int32_t)inData_F.imag;
    accum.imag += (int32_t)inData_G.real * (int32_t)inData_F.imag + (int32_t)inData_G.imag * (int32_t)inData_F.real;
};

// cint16xcint16 - cint32 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint32_t>& accum, cint16_t inData_F, cint16_t inData_G) {
    accum.real += (int64_t)inData_G.real * (int64_t)inData_F.real - (int64_t)inData_G.imag * (int64_t)inData_F.imag;
    accum.imag += (int64_t)inData_G.real * (int64_t)inData_F.imag + (int64_t)inData_G.imag * (int64_t)inData_F.real;
};

// cint32xint16 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint32_t>& accum, cint32_t inData_F, int16_t inData_G) {
    accum.real += (int64_t)inData_F.real * inData_G;
    accum.imag += (int64_t)inData_F.imag * inData_G;
};

// cint32xcint16 - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cint32_t>& accum, cint32_t inData_F, cint16_t inData_G) {
    accum.real += (int64_t)inData_G.real * (int64_t)inData_F.real - (int64_t)inData_G.imag * (int64_t)inData_F.imag;
    accum.imag += (int64_t)inData_G.real * (int64_t)inData_F.imag + (int64_t)inData_G.imag * (int64_t)inData_F.real;
};

// floatxfloat - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<float>& accum, float inData_F, float inData_G) {
    accum.real += (float)inData_F * inData_G;
};

// bfloat16xbfloat16 - multiplyAccum
#ifdef _SUPPORTS_BFLOAT16_
template <>
inline void multiplyAccum(T_accRef<float>& accum, bfloat16 inData_F, bfloat16 inData_G) {
    short* dataF = reinterpret_cast<short*>(&inData_F);
    short* dataG = reinterpret_cast<short*>(&inData_G);
    int data_F = (int)((*dataF) << 16);
    int data_G = (int)((*dataG) << 16);
    float* spfp32_F = reinterpret_cast<float*>(&data_F);
    float* spfp32_G = reinterpret_cast<float*>(&data_G);
    accum.real += (float)*spfp32_F * *spfp32_G;
};
#endif

// cfloatxfloat - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cfloat>& accum, cfloat inData_F, float inData_G) {
    accum.real += (float)inData_F.real * inData_G;
    accum.imag += (float)inData_F.imag * inData_G;
};

// cfloatxcfloat - multiplyAccum
template <>
inline void multiplyAccum(T_accRef<cfloat>& accum, cfloat inData_F, cfloat inData_G) {
    accum.real += (float)inData_G.real * (float)inData_F.real - (float)inData_G.imag * (float)inData_F.imag;
    accum.imag += (float)inData_G.real * (float)inData_F.imag + (float)inData_G.imag * (float)inData_F.real;
};

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return ((MAX_BUFFER_LEN_ON_AIE_IN_BITS() / ref_CC_Size<TT_DATA>()));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return (((maxBitsLoadOnAie() << 1) / ref_CC_Size<TT_DATA>()));
};

// Function to return true or false by checking given length is in range or not
template <typename T_DATA, unsigned int TP_SIG_LEN>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (maxBitsLoadOnAie() / ref_CC_Size<T_DATA>());
    bool check_len = 0;

    if ((TP_SIG_LEN >= getMinLen<T_DATA>()) && (TP_SIG_LEN <= getMaxLen<T_DATA>())) {
        check_len = (((TP_SIG_LEN % minDataLoad) == 0) ? 1 : 0);
    }
    return check_len;
};

// Function to return the loopcount based on given data length and compute mode
template <unsigned int compute_mode, unsigned int TP_F_LEN, unsigned int TP_G_LEN>
INLINE_DECL constexpr unsigned int getLoopCount() {
    unsigned int LoopCount = 0;
    if (compute_mode == 0) // Full
    {
        LoopCount = ((TP_F_LEN + TP_G_LEN) - 1);
    } else if (compute_mode == 1) // Same
    {
        LoopCount = TP_F_LEN;
    } else if (compute_mode == 2) // Valid
    {
        LoopCount = ((TP_F_LEN - TP_G_LEN) + 1);
    } else {
        LoopCount = ((TP_F_LEN + TP_G_LEN) - 1); // default Full mode
    }
    return LoopCount;
};

// Function to return the ref_PaddedLength based on given data length and compute mode
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int compute_mode,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN>
INLINE_DECL constexpr unsigned int getRefPaddedLength() {
    unsigned int ref_PaddedLength = 0;
    unsigned int ref_lanes = ref_CC_NumLanes<TT_DATA_F, TT_DATA_G>();
    unsigned int ref_Data_Samples = ref_CC_Fsample_Size<TT_DATA_F>();
    unsigned int ref_Data_Load = maxBitsLoadOnAie() / ref_Data_Samples;

    if (compute_mode == 0) // Full
    {
        ref_PaddedLength = ((TP_G_LEN - 1) + TP_F_LEN + (TP_G_LEN - 1));
        ref_PaddedLength = CEIL(ref_PaddedLength, ref_Data_Load);
    } else if (compute_mode == 1) // Same
    {
        ref_PaddedLength = (((TP_G_LEN >> 1) - 1) + TP_F_LEN + ((TP_G_LEN >> 1) - 1));
        ref_PaddedLength = CEIL(ref_PaddedLength, ref_Data_Load);
    } else if (compute_mode == 2) {
        if (ref_lanes > ref_Data_Load) // Valid
        {
            ref_PaddedLength = (TP_F_LEN + (ref_Data_Load << 1));
            ref_PaddedLength = CEIL(ref_PaddedLength, ref_Data_Load);
        } else {
            ref_PaddedLength = (TP_F_LEN + ref_Data_Load);
            ref_PaddedLength = CEIL(ref_PaddedLength, ref_Data_Load);
        }
    } else {
        // do nothing
    }

    return ref_PaddedLength;
};

// 8x8 16x8 16x16 32x16 c16x16 c16x32 c16xc16 c32x16 c32xc16
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
    return true;
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
#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, bfloat16, float>() {
    return true;
};
#endif

// Configuration Defensive check function for input data types whether supported or not
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int8, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cfloat, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cfloat, cfloat>() {
    return true;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int8, int8>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16, bfloat16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32, cint16>() {
    return true;
};
#endif

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckInputDataTypes() {
    bool isDataTypeSupported = false;
    if (TP_API == 1) {
        if ((std::is_same<TT_DATA_F, cint16>::value) && (std::is_same<TT_DATA_G, cint16>::value)) {
            isDataTypeSupported = true;
        }
    } else {
        isDataTypeSupported = (fnCheckDataTypesOfInputs<TT_DATA_F, TT_DATA_G>() ? 1 : 0);
    }
    return isDataTypeSupported;
}

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataTypeOfOutput() {
    return (fnCheckDataOutType<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>() ? 1 : 0);
}

// Configuration Defensive check function to check Length of F Signal and G Signal
template <typename TT_DATA, unsigned int TP_SIG_LEN, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckLenOfData() {
    if (TP_API == 1) {
        return true;
    } else {
        return (isLenInRange<TT_DATA, TP_SIG_LEN>() ? 1 : 0);
    }
}

// Configuration Defensive check function to check whether strem process supported by AIE-1 or AIE-2
template <unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsStreamSupportedbyArch() {
    if (TP_API == 1) {
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
template <unsigned int TP_DATA>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((TP_DATA) && !(TP_DATA & (TP_DATA - 1))) ? 1 : 0);
};

// Configuration Defensive check function to check Num_Phases which should be power of 2
template <unsigned int G_Len, unsigned int Casc_Len, unsigned int Num_Phases, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckPhases() {
    bool isphasesValid = 0;
    if (TP_API == 1 && Num_Phases <= maxNumOfPhases() && isPowerOfTwo<Num_Phases>()) {
        if ((Casc_Len == (G_Len >> (mulFactor2() + 1))) && (Num_Phases > 1)) {
            isphasesValid = 1;
        } else {
            if (Num_Phases == 1) {
                isphasesValid = 1;
            }
        }

    } else {
        if (TP_API == 0) {
            if (Num_Phases == 1) {
                isphasesValid = 1;
            }
        }
    }
    return isphasesValid;
};

// Configuration Defensive check function to check whether COMPUTE_MODE is VALID or not for stream processing
template <unsigned int compute_mode, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsComputeModeValid() {
    bool isItValid = 1;

    if ((TP_API == 1) && (compute_mode != TP_COMPUTE_MODE_IS_VALID_MODE())) {
        isItValid = 0;
    } else {
        if (compute_mode > TP_COMPUTE_MODE_IS_VALID_MODE()) {
            isItValid = 0;
        }
    }
    return isItValid;
};

// Configuration Defensive check function to check Length of G Signal should be multiple
// of ((phases*lanes)*(Points/streams_per_core)) when stream processing happening
template <typename TT_DATA_F, unsigned int F_Len, unsigned int G_Len, unsigned int Num_Phases, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckGLen() {
    bool isLenOfGvalid = 1;
    unsigned int lanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>();
    unsigned int points = getPointsOfMac4RotIntrinsic<TT_DATA_F>();
    unsigned int muls = (lanes * points);
    unsigned int streampercore_var = ((muls * Num_Phases) >> 1);
    unsigned int streams_per_core = (G_Len > streampercore_var) ? 1 : maxNumOfStreams();
    unsigned int value = (Num_Phases * lanes * (points / streams_per_core));

    if (G_Len > F_Len) {
        isLenOfGvalid = 0;
    }

    if (TP_API == 1) {
        if (!(G_Len >= minLenOfG_Stream() && G_Len <= maxLenOfG_Stream())) {
            isLenOfGvalid = 0;

        } else {
            if (G_Len < (Num_Phases << mulFactor2())) {
                isLenOfGvalid = 0;

            } else if (G_Len > (Num_Phases << mulFactor2())) {
                if (G_Len % (Num_Phases << (mulFactor2() + 1)) != 0) {
                    isLenOfGvalid = 0;
                }
            }
        }
    }

    return isLenOfGvalid;
};

//  Configuration Defensive check function to check whether Glen/casc_len should be multiple of lanes*points
template <unsigned int G_Len, unsigned int Casc_Len, unsigned int Num_Phases, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckCascLen() {
    bool isCascLenValid = 0;
    unsigned int gLendivideby8 = (G_Len >> (mulFactor2() + 1));
    unsigned int gLendivideby16 = (G_Len >> (mulFactor2() + mulFactor2()));
    unsigned int gLendivideby32 = (G_Len >> (mulFactor2() + mulFactor2() + 1));

    if (TP_API == 1) {
        if ((Casc_Len == gLendivideby8) || (Casc_Len == gLendivideby16) || (Casc_Len == gLendivideby32)) {
            isCascLenValid = 1;
        }
    } else {
        isCascLenValid = 1;
    }

    return isCascLenValid;
};

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_REF_UTILS_HPP_