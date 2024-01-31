/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_HADAMARD_TRAITS_HPP_
#define _DSPLIB_HADAMARD_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#include "fir_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {

static constexpr unsigned int kPointSizeMin = 16;
static constexpr unsigned int kPointSizeMax = 4096;

// //determine the output type depending on the input type combinations
template <typename T_A, typename T_B>
struct outTypeMult {
    using type = cint16;
};
template <>
struct outTypeMult<int16, int16> {
    using type = int16;
};
template <>
struct outTypeMult<int16, cint16> {
    using type = cint16;
};
template <>
struct outTypeMult<int16, cint32> {
    using type = cint32;
};
template <>
struct outTypeMult<int16, int32> {
    using type = int32;
};
template <>
struct outTypeMult<cint16, int16> {
    using type = cint16;
};
template <>
struct outTypeMult<cint16, cint16> {
    using type = cint16;
};
template <>
struct outTypeMult<cint16, int32> {
    using type = cint32;
};
template <>
struct outTypeMult<cint16, cint32> {
    using type = cint32;
};
template <>
struct outTypeMult<int32, int16> {
    using type = int32;
};
template <>
struct outTypeMult<int32, cint16> {
    using type = cint32;
};
template <>
struct outTypeMult<int32, int32> {
    using type = int32;
};
template <>
struct outTypeMult<int32, cint32> {
    using type = cint32;
};
template <>
struct outTypeMult<cint32, int16> {
    using type = cint32;
};
template <>
struct outTypeMult<cint32, cint16> {
    using type = cint32;
};
template <>
struct outTypeMult<cint32, int32> {
    using type = cint32;
};
template <>
struct outTypeMult<cint32, cint32> {
    using type = cint32;
};
template <>
struct outTypeMult<float, float> {
    using type = float;
};
template <>
struct outTypeMult<cfloat, float> {
    using type = cfloat;
};
template <>
struct outTypeMult<float, cfloat> {
    using type = cfloat;
};
template <>
struct outTypeMult<cfloat, cfloat> {
    using type = cfloat;
};

template <typename T_D_A, typename T_D_B>
using outTypeMult_t = typename outTypeMult<T_D_A, T_D_B>::type;

// determine the output type depending on the input type combinations
template <typename TT_A, typename TT_B>
struct vectByte {
    unsigned val_byteA = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteB = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteOut = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteBuffWin = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteBuffStream = ::std::min(sizeof(TT_A), sizeof(TT_B));
    unsigned int kCaptureDataA = 1;
    unsigned int kCaptureDataB = 1;
    unsigned int kCaptureDataOut = 1;
};
template <>
struct vectByte<int16, cint32> {
    unsigned int val_byteA = 4;
    unsigned int val_byteB = 8;
    unsigned int val_byteOut = 8;
    unsigned int val_byteBuffWin = 4;
    unsigned int val_byteBuffStream = 2;
    unsigned int kCaptureDataA = 1;
    unsigned int kCaptureDataB = 2;
    unsigned int kCaptureDataOut = 2;
};

template <>
struct vectByte<cint32, int16> {
    unsigned int val_byteA = 8;
    unsigned int val_byteB = 4;
    unsigned int val_byteOut = 8;
    unsigned int val_byteBuffWin = 4;
    unsigned int val_byteBuffStream = 2;
    unsigned int kCaptureDataA = 2;
    unsigned int kCaptureDataB = 1;
    unsigned int kCaptureDataOut = 2;
};
}
}
}
}
#endif // _DSPLIB_HADAMARD_TRAITS_HPP_
