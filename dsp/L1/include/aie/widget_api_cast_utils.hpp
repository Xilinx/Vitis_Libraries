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
#ifndef _DSPLIB_WIDGET_API_CAST_UTILS_HPP_
#define _DSPLIB_WIDGET_API_CAST_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "widget_api_cast.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

// function to return the accumulator sample width in AIE-ML (Cascade for parallel interconnect)
template <typename TT_DATA>
INLINE_DECL constexpr int fnFFTAccWidthCasc() {
    return -1; // will cause error by default
}
template <>
INLINE_DECL constexpr int fnFFTAccWidthCasc<cint16>() {
    return 32;
};
template <>
INLINE_DECL constexpr int fnFFTAccWidthCasc<cint32>() {
    return 64;
};

// function to return the accumulator vector in samples in AIE-ML (Cascade for parallel interconnect)
template <typename TT_DATA>
INLINE_DECL constexpr int fnFFTCascVWidth() {
    return -1; // will cause error by default
}
template <>
INLINE_DECL constexpr int fnFFTCascVWidth<cint16>() {
    return 8;
};
template <>
INLINE_DECL constexpr int fnFFTCascVWidth<cint32>() {
    return 4;
};
}
}
}
}
}

#endif // _DSPLIB_WIDGET_API_CAST_UTILS_HPP_
