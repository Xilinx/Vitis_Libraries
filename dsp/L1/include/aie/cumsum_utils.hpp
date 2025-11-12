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
#ifndef _DSPLIB_CUMSUM_UTILS_HPP_
#define _DSPLIB_CUMSUM_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "cumsum.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {

template <typename T_D>
struct tcumsumAccBaseType {
    using type = acc48;
};

#ifdef __SUPPORTS_ACC48__
template <>
struct tcumsumAccBaseType<int16> {
    using type = acc48;
};
template <>
struct tcumsumAccBaseType<int32> {
    using type = acc48;
};
template <>
struct tcumsumAccBaseType<cint16> {
    using type = cacc48;
};
template <>
struct tcumsumAccBaseType<cint32> {
    using type = cacc80;
};
#else
template <>
struct tcumsumAccBaseType<int16> {
    using type = acc32;
};
template <>
struct tcumsumAccBaseType<int32> {
    using type = acc64;
};
template <>
struct tcumsumAccBaseType<cint16> {
    using type = cacc64; // cacc32;
};
template <>
struct tcumsumAccBaseType<cint32> {
    using type = cacc64;
};
#endif
template <>
struct tcumsumAccBaseType<float> {
    using type = accfloat;
};
template <>
struct tcumsumAccBaseType<cfloat> {
    using type = caccfloat;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
struct tcumsumAccBaseType<bfloat16> {
    using type = accfloat;
};
#endif //_SUPPORTS_BFLOAT16_
#ifdef _SUPPORTS_CBFLOAT16_
template <>
struct tcumsumAccBaseType<cbfloat16> {
    using type = caccfloat;
};
#endif //_SUPPORTS_CBFLOAT16_

//---------------------------------------------
// coeff type - this is used simply because to add using a sliding mul, a trivial coeff is required, but it must be type
// appropriate.

template <typename T_D>
struct tcumsumCoeffBaseType {
    using type = int16;
};
template <>
struct tcumsumCoeffBaseType<cint32> {
    using type = int32;
};

#ifdef _SUPPORTS_BFLOAT16_
template <>
struct tcumsumCoeffBaseType<bfloat16> {
    using type = bfloat16;
};
#endif
#ifdef _SUPPORTS_CBFLOAT16_
template <>
struct tcumsumCoeffBaseType<cbfloat16> {
    using type = bfloat16;
};
#endif
template <>
struct tcumsumCoeffBaseType<float> {
    using type = float;
};
#ifdef __SUPPORTS_CFLOAT__
template <>
struct tcumsumCoeffBaseType<cfloat> {
    using type = float;
};
#endif

//--------------------------------------------
template <typename T_RDATA>
INLINE_DECL T_RDATA nullElem() {
    return 0;
    //    return ::aie::zero<T_RDATA>();
};

// Null cint16_t element
template <>
INLINE_DECL cint16_t nullElem() {
    cint16_t d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null cint32 element
template <>
INLINE_DECL cint32 nullElem() {
    cint32 d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null float element
template <>
INLINE_DECL float nullElem() {
    return 0.0;
};

// Null cfloat element
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL cfloat nullElem() {
    cfloat retVal = {0.0, 0.0};

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
#endif
#ifdef _SUPPORTS_BFLOAT16_
// null bfloat16
template <>
INLINE_DECL bfloat16 nullElem() {
    return 0.0;
};
#endif //_SUPPORTS_BFLOAT16_

// Null cint32 element
#ifdef _SUPPORTS_CBFLOAT16_
template <>
INLINE_DECL cbfloat16 nullElem() {
    cbfloat16 retVal;
    retVal = {0.0, 0.0};

    return retVal;
};
#endif //_SUPPORTS_CBFLOAT16_

//-------------------------------------
// Unity - return a 1 in the templated type
template <typename T_D>
INLINE_DECL T_D cumsumUnity() {
    return 1;
};

template <>
INLINE_DECL cint16 cumsumUnity() {
    cint16 retVal;
    retVal.real = 1.0;
    retVal.imag = 0.0;
    return retVal;
};

template <>
INLINE_DECL cint32 cumsumUnity() {
    cint32 retVal;
    retVal.real = 1.0;
    retVal.imag = 0.0;
    return retVal;
};

template <>
INLINE_DECL float cumsumUnity() {
    return 1.0;
};

template <>
INLINE_DECL cfloat cumsumUnity() {
    cfloat retVal;
    retVal.real = 1.0;
    retVal.imag = 0.0;
    return retVal;
};

#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL bfloat16 cumsumUnity() {
    return 1.0;
};
template <>
INLINE_DECL cbfloat16 cumsumUnity() {
    cbfloat16 retVal;
    retVal = {1.0, 0.0};
    return retVal;
};
#endif //_SUPPORTS_BFLOAT16_
}
}
}
}

#endif // _DSPLIB_CUMSUM_UTILS_HPP_
