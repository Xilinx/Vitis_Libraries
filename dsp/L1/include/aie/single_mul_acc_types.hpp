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

#pragma once

namespace xf {
namespace dsp {
namespace aie {

// determine the acc type depending on the input type combinations
template <typename TT_DATA_A, typename TT_DATA_B>
#ifdef __SUPPORTS_ACC64__
struct acc_mul_t {
    using type = cacc64;
};
template <>
struct acc_mul_t<int16, int16> {
    using type = acc32;
};
template <>
struct acc_mul_t<int16, cint16> {
    using type = cacc32;
};
template <>
struct acc_mul_t<int16, cint32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<int16, int32> {
    using type = acc64;
};
template <>
struct acc_mul_t<cint16, int16> {
    using type = cacc32;
};
template <>
struct acc_mul_t<cint16, cint16> {
    using type = cacc32;
};
template <>
struct acc_mul_t<cint16, int32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<cint16, cint32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<int32, int16> {
    using type = acc64;
};
template <>
struct acc_mul_t<int32, cint16> {
    using type = cacc64;
};
template <>
struct acc_mul_t<int32, int32> {
    using type = acc64;
};
template <>
struct acc_mul_t<int32, cint32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<cint32, int16> {
    using type = cacc64;
};
template <>
struct acc_mul_t<cint32, cint16> {
    using type = cacc64;
};
template <>
struct acc_mul_t<cint32, int32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<cint32, cint32> {
    using type = cacc64;
};
template <>
struct acc_mul_t<float, float> {
    using type = accfloat;
};
template <>
struct acc_mul_t<float, cfloat> {
    using type = caccfloat;
};
template <>
struct acc_mul_t<cfloat, float> {
    using type = caccfloat;
};
template <>
struct acc_mul_t<cfloat, cfloat> {
    using type = caccfloat;
};
#else //__SUPPORTS_ACC48__
struct acc_mul_t {
    using type = acc48;
};
template <>
struct acc_mul_t<int16, int16> {
    using type = acc48;
};
template <>
struct acc_mul_t<cint16, int16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<int16, cint16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<cint16, cint16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<int32, int16> {
    using type = acc48;
};
template <>
struct acc_mul_t<int32, int32> {
    using type = acc80;
};
template <>
struct acc_mul_t<int32, cint16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<int32, cint32> {
    using type = cacc80;
};
template <>
struct acc_mul_t<cint32, int16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<cint32, cint16> {
    using type = cacc48;
};
template <>
struct acc_mul_t<cint32, int32> {
    using type = cacc80;
};
template <>
struct acc_mul_t<cint32, cint32> {
    using type = cacc80;
};
template <>
struct acc_mul_t<int16, int32> {
    using type = acc48;
};
template <>
struct acc_mul_t<int16, cint32> {
    using type = cacc48;
};
template <>
struct acc_mul_t<cint16, int32> {
    using type = cacc48;
};
template <>
struct acc_mul_t<cint16, cint32> {
    using type = cacc48;
};
template <>
struct acc_mul_t<float, float> {
    using type = accfloat;
};
template <>
struct acc_mul_t<float, cfloat> {
    using type = caccfloat;
};
template <>
struct acc_mul_t<cfloat, float> {
    using type = caccfloat;
};
template <>
struct acc_mul_t<cfloat, cfloat> {
    using type = caccfloat;
};

#endif //__SUPPORTS_ACC64__

template <typename T_D_A, typename T_D_B>
using accTypeMult_t = typename acc_mul_t<T_D_A, T_D_B>::type;
}
}
}