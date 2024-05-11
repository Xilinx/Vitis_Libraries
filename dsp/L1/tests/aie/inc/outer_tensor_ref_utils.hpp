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
#ifndef _DSPLIB_OUTER_TENSOR_REF_UTILS_HPP_
#define _DSPLIB_OUTER_TENSOR_REF_UTILS_HPP_

#include "device_defs.h"
#include "aie_api/utils.hpp" // for vector print function

// //determine the output type depending on the input type combinations
template <typename TT_A, typename TT_B>
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