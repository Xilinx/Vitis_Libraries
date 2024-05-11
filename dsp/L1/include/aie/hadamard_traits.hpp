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
#include "mul_out_type.hpp"
#include "mul_acc_type.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {

static constexpr unsigned int kPointSizeMin = 16;

// determine the output type depending on the input type combinations
template <typename TT_A, typename TT_B>
struct vectByte {
    unsigned val_byteA = sizeof(out_mul_type<TT_A, TT_B>);
    unsigned val_byteB = sizeof(out_mul_type<TT_A, TT_B>);
    unsigned val_byteOut = sizeof(out_mul_type<TT_A, TT_B>);
    unsigned val_byteBuffWin = sizeof(out_mul_type<TT_A, TT_B>);
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
