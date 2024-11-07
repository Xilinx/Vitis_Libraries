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
#ifndef _DSPLIB_KRONECKER_REF_UTILS_HPP_
#define _DSPLIB_KRONECKER_REF_UTILS_HPP_

#include "device_defs.h"
#include "aie_api/utils.hpp" // for vector print function
#include "fir_ref_utils.hpp"
#include "single_mul_ref_out_types.hpp"
#include "single_mul_ref_acc_types.hpp"

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

// template <typename T_D>
// struct T_accRef {
//     int64_t real, imag;
// };

// Value accumulator type
template <typename T_A>
inline T_accRef<T_A> val_accRef(T_A DATA) {
    T_accRef<T_A> retVal;
    retVal.real = (int64_t)DATA.real;
    retVal.imag = (int64_t)DATA.imag;
    return retVal;
};
template <>
inline T_accRef<int32> val_accRef(int32 DATA) {
    T_accRef<int32> retVal;
    retVal.real = (int64_t)DATA;
    retVal.imag = (int64_t)0;
    return retVal;
};
template <>
inline T_accRef<int16> val_accRef(int16 DATA) {
    T_accRef<int16> retVal;
    retVal.real = (int64_t)DATA;
    retVal.imag = (int64_t)0;
    return retVal;
};
template <>
inline T_accRef<float> val_accRef(float DATA) {
    T_accRef<float> retVal;
    retVal.real = (float)DATA;
    return retVal;
};
template <>
inline T_accRef<cfloat> val_accRef(cfloat DATA) {
    T_accRef<cfloat> retVal;
    retVal.real = (float)DATA.real;
    retVal.imag = (float)DATA.imag;
    return retVal;
};

#endif