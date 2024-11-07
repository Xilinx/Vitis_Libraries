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

// determine the output type depending on the input type combinations
template <typename T_A, typename T_B>
struct out_mul_t {
    using type = cint16;
};
template <>
struct out_mul_t<int16, int16> {
    using type = int16;
};
template <>
struct out_mul_t<int16, cint16> {
    using type = cint16;
};
template <>
struct out_mul_t<int16, cint32> {
    using type = cint32;
};
template <>
struct out_mul_t<int16, int32> {
    using type = int32;
};
template <>
struct out_mul_t<cint16, int16> {
    using type = cint16;
};
template <>
struct out_mul_t<cint16, cint16> {
    using type = cint16;
};
template <>
struct out_mul_t<cint16, int32> {
    using type = cint32;
};
template <>
struct out_mul_t<cint16, cint32> {
    using type = cint32;
};
template <>
struct out_mul_t<int32, int16> {
    using type = int32;
};
template <>
struct out_mul_t<int32, cint16> {
    using type = cint32;
};
template <>
struct out_mul_t<int32, int32> {
    using type = int32;
};
template <>
struct out_mul_t<int32, cint32> {
    using type = cint32;
};
template <>
struct out_mul_t<cint32, int16> {
    using type = cint32;
};
template <>
struct out_mul_t<cint32, cint16> {
    using type = cint32;
};
template <>
struct out_mul_t<cint32, int32> {
    using type = cint32;
};
template <>
struct out_mul_t<cint32, cint32> {
    using type = cint32;
};
template <>
struct out_mul_t<float, float> {
    using type = float;
};
template <>
struct out_mul_t<cfloat, float> {
    using type = cfloat;
};
template <>
struct out_mul_t<float, cfloat> {
    using type = cfloat;
};
template <>
struct out_mul_t<cfloat, cfloat> {
    using type = cfloat;
};

template <typename T_D_A, typename T_D_B>
using outTypeMult_t = typename out_mul_t<T_D_A, T_D_B>::type;
}
}
}