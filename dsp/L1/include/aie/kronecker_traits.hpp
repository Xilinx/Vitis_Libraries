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
#ifndef _DSPLIB_KRONECKER_TRAITS_HPP_
#define _DSPLIB_KRONECKER_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "device_defs.h"
#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

template <typename T_A, typename T_B>
struct vectByte {
    unsigned val_byteA = sizeof(outTypeMult_t<T_A, T_B>);
    unsigned val_byteB = sizeof(outTypeMult_t<T_A, T_B>);
    unsigned val_byteAcc = sizeof(outTypeMult_t<T_A, T_B>);
    unsigned val_byteOut = sizeof(outTypeMult_t<T_A, T_B>);
};

template <>
struct vectByte<int16, cint32> {
    unsigned val_byteA = 2;
    unsigned val_byteB = 8;
    unsigned val_byteAcc = 8;
    unsigned val_byteOut = 8;
};

template <>
struct vectByte<cint32, int16> {
    unsigned val_byteA = 8;
    unsigned val_byteB = 4;
    unsigned val_byteAcc = 4;
    unsigned val_byteOut = 8;
};

template <>
struct vectByte<int16, int16> {
    unsigned val_byteA = 4;
    unsigned val_byteB = 4;
    unsigned val_byteAcc = 4;
    unsigned val_byteOut = 4;
};

template <typename T_A, typename T_B>
struct vecSampleNum {
    unsigned int A = 256 / 8 / vectByte<T_A, T_B>().val_byteA;
    unsigned int B = 256 / 8 / vectByte<T_A, T_B>().val_byteB;
    unsigned int Acc = 256 / 8 / vectByte<T_A, T_B>().val_byteAcc;
    unsigned int TempOut = 256 / 8 / vectByte<T_A, T_B>().val_byteOut;
    unsigned int Out = 256 / 8 / vectByte<T_A, T_B>().val_byteOut;
};
template <>
struct vecSampleNum<cint32, int16> {
    unsigned int A = 256 / 8 / vectByte<cint32, int16>().val_byteA;
    unsigned int B = 256 / 8 / vectByte<cint32, int16>().val_byteB;
    unsigned int Acc = 256 / 8 / vectByte<cint32, int16>().val_byteAcc;
    unsigned int TempOut = 2 * 256 / 8 / vectByte<cint32, int16>().val_byteOut; // 4 loops to push data out
    unsigned int Out = 256 / 8 / vectByte<cint32, int16>().val_byteOut;
};
}
}
}
}
#endif // _DSPLIB_KRONECKER_TRAITS_HPP_
