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
#ifndef _DSPLIB_KRONECKER_UTILS_HPP_
#define _DSPLIB_KRONECKER_UTILS_HPP_

/*

This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
//#include "kernel_api_utils.hpp"
#include "kronecker_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

#ifdef __SUPPORTS_ACC48__

template <typename T_A, typename T_B>
struct kmpAcc {
    using type = acc48;
};

template <>
struct kmpAcc<int16, int16> {
    using type = acc48;
};
template <>
struct kmpAcc<cint16, int16> {
    using type = cacc48;
};
template <>
struct kmpAcc<int16, cint16> {
    using type = cacc48;
};
template <>
struct kmpAcc<cint16, cint16> {
    using type = cacc48;
};
template <>
struct kmpAcc<int32, int16> {
    using type = acc48;
};
template <>
struct kmpAcc<int32, int32> {
    using type = acc80;
};
template <>
struct kmpAcc<int32, cint16> {
    using type = cacc48;
};
template <>
struct kmpAcc<int32, cint32> {
    using type = cacc80;
};
template <>
struct kmpAcc<cint32, int16> {
    using type = cacc48;
};
template <>
struct kmpAcc<cint32, cint16> {
    using type = cacc48;
};
template <>
struct kmpAcc<cint32, int32> {
    using type = cacc80;
};
template <>
struct kmpAcc<cint32, cint32> {
    using type = cacc80;
};
template <>
struct kmpAcc<int16, int32> {
    using type = acc48;
};
template <>
struct kmpAcc<int16, cint32> {
    using type = cacc48;
};
template <>
struct kmpAcc<cint16, int32> {
    using type = cacc48;
};
template <>
struct kmpAcc<cint16, cint32> {
    using type = cacc48;
};
template <>
struct kmpAcc<float, float> {
    using type = accfloat;
};
template <>
struct kmpAcc<float, cfloat> {
    using type = caccfloat;
};
template <>
struct kmpAcc<cfloat, float> {
    using type = caccfloat;
};
template <>
struct kmpAcc<cfloat, cfloat> {
    using type = caccfloat;
};

#endif //__SUPPORTS_ACC48__

#ifdef __SUPPORTS_ACC64__

template <typename T_A, typename T_B>
struct kmpAcc {
    using type = acc64;
};

template <>
struct kmpAcc<int16, int16> {
    using type = acc64;
};
template <>
struct kmpAcc<int16, cint16> {
    using type = cacc64;
};
template <>
struct kmpAcc<int16, cint32> {
    using type = cacc64;
};
template <>
struct kmpAcc<int16, int32> {
    using type = acc64;
};
template <>
struct kmpAcc<cint16, int16> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint16, cint16> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint16, int32> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint16, cint32> {
    using type = cacc64;
};
template <>
struct kmpAcc<int32, int16> {
    using type = acc64;
};
template <>
struct kmpAcc<int32, cint16> {
    using type = cacc64;
};
template <>
struct kmpAcc<int32, int32> {
    using type = acc64;
};
template <>
struct kmpAcc<int32, cint32> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint32, int16> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint32, cint16> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint32, int32> {
    using type = cacc64;
};
template <>
struct kmpAcc<cint32, cint32> {
    using type = cacc64;
};

#endif //__SUPPORTS_ACC64__
}
}
}
}

#endif // _DSPLIB_KRONECKER_UTILS_HPP_
