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
#ifndef _DSPLIB_matrix_vector_mul_TRAITS_HPP_
#define _DSPLIB_matrix_vector_mul_TRAITS_HPP_

#include "device_defs.h"
#include "fir_utils.hpp"
#define TRUNC(x, y) (((x) / y) * y)

#include "fir_utils.hpp"
/*
MATRIX_VECTOR_MUL traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

// Struct to get accum type based on input types
template <typename T_A, typename T_B>
#ifdef __SUPPORTS_ACC64__
// AIE_VARIANT=2
struct accType {
    using type = acc64;
};
template <>
struct accType<int16, int16> {
    using type = acc64;
};
template <>
struct accType<int16, cint16> {
    using type = cacc64;
};
template <>
struct accType<int16, cint32> {
    using type = cacc64;
};
template <>
struct accType<int16, int32> {
    using type = acc64;
};
template <>
struct accType<cint16, int16> {
    using type = cacc64;
};
template <>
struct accType<cint16, cint16> {
    using type = cacc64;
};
template <>
struct accType<cint16, int32> {
    using type = cacc64;
};
template <>
struct accType<cint16, cint32> {
    using type = cacc64;
};
template <>
struct accType<int32, int16> {
    using type = acc64;
};
template <>
struct accType<int32, cint16> {
    using type = cacc64;
};
template <>
struct accType<int32, int32> {
    using type = acc64;
};
template <>
struct accType<int32, cint32> {
    using type = cacc64;
};
template <>
struct accType<cint32, int16> {
    using type = cacc64;
};
template <>
struct accType<cint32, cint16> {
    using type = cacc64;
};
template <>
struct accType<cint32, int32> {
    using type = cacc64;
};
template <>
struct accType<cint32, cint32> {
    using type = cacc64;
};
template <>
struct accType<float, float> {
    using type = accfloat;
};
template <>
struct accType<cfloat, float> {
    using type = caccfloat;
};
template <>
struct accType<float, cfloat> {
    using type = caccfloat;
};
template <>
struct accType<cfloat, cfloat> {
    using type = caccfloat;
};
// AIE_VARIANT=1
#else
struct accType {
    using type = cacc48;
};
template <>
struct accType<int16, int16> {
    using type = acc48;
};
template <>
struct accType<int16, cint16> {
    using type = cacc48;
};
template <>
struct accType<int16, cint32> {
    using type = cacc80;
};
template <>
struct accType<int16, int32> {
    using type = acc80;
};
template <>
struct accType<cint16, int16> {
    using type = cacc48;
};
template <>
struct accType<cint16, cint16> {
    using type = cacc48;
};
template <>
struct accType<cint16, int32> {
    using type = cacc80;
};
template <>
struct accType<cint16, cint32> {
    using type = cacc80;
};
template <>
struct accType<int32, int16> {
    using type = acc80;
};
template <>
struct accType<int32, cint16> {
    using type = cacc80;
};
template <>
struct accType<int32, int32> {
    using type = acc80;
};
template <>
struct accType<int32, cint32> {
    using type = cacc80;
};
template <>
struct accType<cint32, int16> {
    using type = cacc80;
};
template <>
struct accType<cint32, cint16> {
    using type = cacc80;
};
template <>
struct accType<cint32, int32> {
    using type = cacc80;
};
template <>
struct accType<cint32, cint32> {
    using type = cacc80;
};
template <>
struct accType<float, float> {
    using type = accfloat;
};
template <>
struct accType<cfloat, float> {
    using type = caccfloat;
};
template <>
struct accType<float, cfloat> {
    using type = caccfloat;
};
template <>
struct accType<cfloat, cfloat> {
    using type = caccfloat;
};
#endif //__SUPPORTS_ACC64__
template <typename T_D_A, typename T_D_B>
using accType_t = typename accType<T_D_A, T_D_B>::type;

// Struct to get output type based on input types
template <typename T_A, typename T_B>
struct outType {
    using type = cint16;
};
template <>
struct outType<int16, int16> {
    using type = int16;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};
template <>
struct outType<cint16, cint16> {
    using type = cint16;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};
template <>
struct outType<float, float> {
    using type = float;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};
template <>
struct outType<float, cfloat> {
    using type = cfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};
template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

// Input Interfaces
template <typename T_A, typename T_B>
struct T_inputIF {
    T_A* __restrict inWindowA;
    T_B* __restrict inWindowB;
    input_stream<T_B>* __restrict inStreamB;
    input_stream<T_B>* __restrict inStreamB2;
    input_cascade<accType_t<T_A, T_B> >* inCascade = {};
};
// CASC_OUT_FALSE
template <typename T_A, typename T_B>
struct T_outputIF {
    outType_t<T_A, T_B>* __restrict outWindow;
    output_stream<outType_t<T_A, T_B> >* __restrict outStream;
    output_stream<outType_t<T_A, T_B> >* __restrict outStream2;
    output_cascade<accType_t<T_A, T_B> >* outCascade{};
};

// determine the number of lanes for A and B depending on DATA_TYPE combination
template <typename TT_A, typename TT_B>
struct mumLanesMatVec {
    unsigned lanesInA = 256 / 8 / sizeof(TT_A);
    unsigned lanesInB = 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
template <>
struct mumLanesMatVec<int16, cint32> { // 8 8
    unsigned lanesInA = 8;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 4;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
template <>
struct mumLanesMatVec<cint32, int16> { // 8 8
    unsigned lanesInA = 4;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 8;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
#ifndef __SUPPORTS_ACC64__
template <>
struct mumLanesMatVec<int16, int32> { // 8 8
    unsigned lanesInA = 8;            // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 8;            // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 8;          // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
template <>
struct mumLanesMatVec<int32, cint16> { // 8 8
    unsigned lanesInA = 4;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 4;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
template <>
struct mumLanesMatVec<cint16, int32> { // 8 8
    unsigned lanesInA = 4;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 4;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
#else
template <>
struct mumLanesMatVec<int32, cint16> { // 8 8
    unsigned lanesInA = 8;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 8;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
template <>
struct mumLanesMatVec<cint16, int32> { // 8 8
    unsigned lanesInA = 8;             // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
    unsigned lanesInB = 8;             // 256 / 8 / sizeof(TT_B);
    unsigned lanesInOut = 4;           // 256 / 8 / sizeof(outType_t<TT_A, TT_B>);
};
#endif //__SUPPORTS_ACC64__
}
}
}
}
}

#endif // _DSPLIB_matrix_vector_mul_TRAITS_HPP_
