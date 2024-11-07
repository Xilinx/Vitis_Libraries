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
#ifndef _DSPLIB_MATRIX_MULT_TRAITS_HPP_
#define _DSPLIB_MATRIX_MULT_TRAITS_HPP_

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {
/*
Matrix Multiply traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

// The following struct returns the required accumulator for data combos.
// Also for devices supporting different acc types. AIE-ML - acc64, cacc64 etc. AIE1 - acc48, acc80 etc.
struct no_port {};
template <typename T_A, typename T_B>
#ifdef __SUPPORTS_ACC64__
struct accType {
    using type = cacc64;
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
#else  //__SUPPORTS_ACC48__
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

// The following struct returns the output type for a given input data type combination
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

// Input and Output Interfaces - T_inputIF and T_outputIF
template <bool T_CASC_IN, typename T_D_A, typename T_D_B>
struct T_inputIF {
    void* __restrict inWindowA;
    void* __restrict inWindowB;
    typename std::conditional<T_CASC_IN == CASC_IN_FALSE, no_port, input_stream<accType_t<T_D_A, T_D_B> > >::type*
        inCascade;
};

// IF output type
template <bool T_CASC_OUT, typename T_D_A, typename T_D_B>
struct T_outputIF {
    typename std::conditional<T_CASC_OUT == CASC_OUT_FALSE,
                              outType_t<T_D_A, T_D_B>,
                              output_stream<accType_t<T_D_A, T_D_B> > >::type* __restrict outWindow;
};

// The following is a set of type-specialized functions which return the number of accumulator registers
// available in the processor. Since these may be 384 or 768 bit registers the number could vary by type.
template <typename TT_DATA_A, typename TT_DATA_B>
unsigned int fnAccRegsMatMult() {
    return 0;
}; // default error trap
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<int16, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint16, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint16, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<int32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<int32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint32, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cint32, cint32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<float, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnAccRegsMatMult<cfloat, cfloat>() {
    return 4;
};

// function to return the number of lanes for a type combo
// The default is effectively an error trap, but adding an error message to a constexpr return results in a warning.
template <typename TT_DATA_A, typename TT_DATA_B>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<int16, int16>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint16, int16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint16, cint16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<int32, int16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<int32, int32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint32, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cint32, cint32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<float, float>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesMatMult<cfloat, cfloat>() {
    return 4;
};

// Function to return the lowest common multiple of two numbers
// A full implementation of this would entail prime factor decomposition, but here
// The maximum integer size is 16, so a simpler brute force method will do.
template <typename TT_DATA_A, typename TT_DATA_B, unsigned int TP_FACTOR>
INLINE_DECL constexpr unsigned int fnLCMMatMult() {
    return ((fnNumLanesMatMult<TT_DATA_A, TT_DATA_B>() == 2)
                ? ((TP_FACTOR % 2 == 0) ? TP_FACTOR : (TP_FACTOR * 2))
                : (fnNumLanesMatMult<TT_DATA_A, TT_DATA_B>() == 4)
                      ? ((TP_FACTOR % 4 == 0) ? TP_FACTOR : ((TP_FACTOR % 2 == 0) ? (TP_FACTOR * 2) : (TP_FACTOR * 4)))
                      : (fnNumLanesMatMult<TT_DATA_A, TT_DATA_B>() == 8)
                            ? ((TP_FACTOR % 8 == 0)
                                   ? TP_FACTOR
                                   : ((TP_FACTOR % 4 == 0) ? (TP_FACTOR * 2)
                                                           : ((TP_FACTOR % 2 == 0) ? (TP_FACTOR * 4) : TP_FACTOR * 8)))
                            : 0);
};

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA_A, typename TT_DATA_B>
INLINE_DECL constexpr unsigned int fnVOutSizeMatMult() {
    return fnNumLanesMatMult<TT_DATA_A, TT_DATA_B>();
};
}
}
}
}
}

#endif // _DSPLIB_MATRIX_MULT_TRAITS_HPP_
