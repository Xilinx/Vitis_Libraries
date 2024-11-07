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
#include "single_mul_ref_out_types.hpp"
#include "single_mul_ref_acc_types.hpp"

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