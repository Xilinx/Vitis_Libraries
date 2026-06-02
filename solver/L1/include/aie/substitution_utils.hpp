/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_SUBSTITUTION_UTILS_HPP_
#define _DSPLIB_SUBSTITUTION_UTILS_HPP_

// #define _DSPLIB_CHOLESKY_RUNTIME_DEBUG_

/*
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "substitution_traits.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

//-----------------------------------------------------------------
// fnMyInv: templatized function to return reciprocal of input value
template <typename T_D>
INLINE_DECL tFbsubBaseType<T_D> fnMyInv(T_D& inVal){
    // empty default
};

template <>
INLINE_DECL tFbsubBaseType<float> fnMyInv<float>(float& inVal) {
#ifdef _SUPPORTS_HW_INVERSE_
    return ::aie::inv(inVal); // supported in scalar processor
#else
    return ::aie::inv(inVal);      // emulated - may replace with LUT-based method later
#endif //_SUPPORTS_HW_INVERSE_
};

template <>
INLINE_DECL tFbsubBaseType<cfloat> fnMyInv<cfloat>(cfloat& inVal) {
#ifdef _SUPPORTS_HW_INVERSE_
    return ::aie::inv(inVal.real); // supported in scalar processor
#else
    return ::aie::inv(inVal.real); // emulated - may replace with LUT-based method later
#endif //_SUPPORTS_HW_INVERSE_
};

//----------------------
// NullElem
template <typename T_D>
INLINE_DECL T_D nullElem(){
    // empty default
};
template <>
INLINE_DECL float nullElem<float>() {
    return 0.0f;
};
template <>
INLINE_DECL cfloat nullElem<cfloat>() {
    cfloat retVal;
    retVal.real = 0.0f;
    retVal.imag = 0.0f;
    return retVal;
};
}
}
}
}

#endif // _DSPLIB_SUBSTITUTION_UTILS_HPP_
