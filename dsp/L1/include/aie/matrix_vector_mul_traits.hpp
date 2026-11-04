/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#include "promotions_mac.hpp"
#define TRUNC(x, y) (((x) / y) * y)

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

using namespace ::xf::dsp::aie::promotions_mac;

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
