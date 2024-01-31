/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_
#define _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/
#include "adf.h"
#include "device_defs.h"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

// TP_API values
static constexpr unsigned int kWindowAPI = 0;
static constexpr unsigned int kStreamAPI = 1;
static constexpr unsigned int kCascStreamAPI = 2;
static constexpr unsigned int kStreamCascAPI = 3;

// TP_PATTERN values
static constexpr unsigned int kDefaultIntlv = 0; // dual streams interleave (128bits granularity).
static constexpr unsigned int kSampleIntlv = 1;  // card dealing, sample granularity
static constexpr unsigned int kSplit = 2;        // deck cutting

// This interface is maximally sized (3) since it doesn't matter if some inputs are unused.
template <typename T_D, unsigned int T_IN_API>
struct T_inputIF {};
template <typename T_D>
struct T_inputIF<T_D, 0> {
    void* inWindow0;
    void* inWindow1;
    void* inWindow2;
    void* inWindow3;
    void* inWindow4;
    void* inWindow5;
    void* inWindow6;
    void* inWindow7;
};

template <typename T_D>
struct T_inputIF<T_D, 1> {
    input_stream<T_D>* inStream0;
    input_stream<T_D>* inStream1;
};

template <typename T_D, unsigned int T_out_API>
struct T_outputIF {};
template <typename T_D>
struct T_outputIF<T_D, 0> {
    void* outWindow0;
    void* outWindow1;
    void* outWindow2;
    void* outWindow3;
};
template <typename T_D>
struct T_outputIF<T_D, 1> {
    output_stream<T_D>* outStream0;
    output_stream<T_D>* outStream1;
};
}
}
}
}
}
#endif // _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_
