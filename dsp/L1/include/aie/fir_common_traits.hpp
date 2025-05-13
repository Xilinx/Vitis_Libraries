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
#ifndef _DSPLIB_FIR_COMMON_TRAITS_HPP_
#define _DSPLIB_FIR_COMMON_TRAITS_HPP_

/*
Common FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal classes. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
#if __MIN_REGSIZE__ == 128
template <typename T_D, typename T_C, int T_PORTS>
constexpr unsigned int getOptTapsPerKernelSrAsym() {
    return -1;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int16, int16, 1>() {
    return 16;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int16, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int16, int32, 1>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int16, int32, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, int16, 1>() {
    return 16;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, int32, 1>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, int32, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, cint16, 1>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, cint16, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, cint32, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint16, cint32, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int32, int16, 1>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int32, int16, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int32, int32, 1>() {
    return 8;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<int32, int32, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, int16, 1>() {
    return 8;
}; // user must also look at max in case max < opt
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, int16, 2>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, cint16, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, cint16, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, int32, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, int32, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, cint32, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cint32, cint32, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<float, float, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<float, float, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cfloat, float, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cfloat, float, 2>() {
    return 2;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cfloat, cfloat, 1>() {
    return 4;
};
template <>
constexpr unsigned int getOptTapsPerKernelSrAsym<cfloat, cfloat, 2>() {
    return 2;
};
#else
template <typename T_D, typename T_C, int T_PORTS>
constexpr unsigned int getOptTapsPerKernelSrAsym() {
    return 16;
};
#endif
}
}
}
}
#endif // _DSPLIB_FIR_COMMON_TRAITS_HPP_
