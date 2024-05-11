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

//-------- T_outputIF----------
// This interface is maximally sized (8) since it doesn't matter if some inputs are unused.
template <typename T_D, unsigned int T_IN_API>
struct T_inputIF {};
template <typename T_D>
struct T_inputIF<T_D, kWindowAPI> {
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
struct T_inputIF<T_D, kStreamAPI> {
    input_stream<T_D>* inStream0;
    input_stream<T_D>* inStream1;
};

// The following definitions extend the T_inputIF and T_outputIF as defined in traits.hpp
// CascStream
#ifdef __SUPPORTS_ACC64__
template <>
struct T_inputIF<int16, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<int16>* inStream1;
    input_stream<int16>* inStream2;
};
template <>
struct T_inputIF<cint16, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<cint16>* inStream1;
    input_stream<cint16>* inStream2;
};
template <>
struct T_inputIF<int32, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<int32>* inStream1;
    input_stream<int32>* inStream2;
};
template <>
struct T_inputIF<cint32, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<cint32>* inStream1;
    input_stream<cint32>* inStream2;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
struct T_inputIF<float, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<float>* inStream1;
    input_stream<float>* inStream2;
};
template <>
struct T_inputIF<cfloat, kCascStreamAPI> {
    input_cascade<cacc64>* inStream0;
    input_stream<cfloat>* inStream1;
    input_stream<cfloat>* inStream2;
};
#endif //__SUPPORTS_CFLOAT__ == 1
#endif //__SUPPORTS_ACC64__

// StreamCasc
#ifdef __SUPPORTS_ACC64__
template <>
struct T_inputIF<int16, kStreamCascAPI> {
    input_stream<int16>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<int16>* inStream2;
};
template <>
struct T_inputIF<cint16, kStreamCascAPI> {
    input_stream<cint16>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<cint16>* inStream2;
};
template <>
struct T_inputIF<int32, kStreamCascAPI> {
    input_stream<int32>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<int32>* inStream2;
};
template <>
struct T_inputIF<cint32, kStreamCascAPI> {
    input_stream<cint32>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<cint32>* inStream2;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
struct T_inputIF<float, kStreamCascAPI> {
    input_stream<float>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<float>* inStream2;
};
template <>
struct T_inputIF<cfloat, kStreamCascAPI> {
    input_stream<cfloat>* inStream0;
    input_cascade<cacc64>* inStream1;
    input_stream<cfloat>* inStream2;
};
#endif //__SUPPORTS_CFLOAT__ == 1
#endif //__SUPPORTS_ACC64__

//-------- T_outputIF----------
template <typename T_D, unsigned int T_out_API>
struct T_outputIF {};
template <typename T_D>
struct T_outputIF<T_D, kWindowAPI> {
    void* outWindow0;
    void* outWindow1;
    void* outWindow2;
    void* outWindow3;
};
template <typename T_D>
struct T_outputIF<T_D, kStreamAPI> {
    output_stream<T_D>* outStream0;
    output_stream<T_D>* outStream1;
};
#ifdef __SUPPORTS_ACC64__
// CascStreams
template <>
struct T_outputIF<int16, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<int16>* outStream1;
    output_stream<int16>* outStream2;
    output_stream<int16>* outStream3;
};
template <>
struct T_outputIF<cint16, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<cint16>* outStream1;
    output_stream<cint16>* outStream2;
    output_stream<cint16>* outStream3;
};
template <>
struct T_outputIF<int32, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<int32>* outStream1;
    output_stream<int32>* outStream2;
    output_stream<int32>* outStream3;
};
template <>
struct T_outputIF<cint32, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<cint32>* outStream1;
    output_stream<cint32>* outStream2;
    output_stream<cint32>* outStream3;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
struct T_outputIF<float, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<float>* outStream1;
    output_stream<float>* outStream2;
    output_stream<float>* outStream3;
};
template <>
struct T_outputIF<cfloat, kCascStreamAPI> {
    output_cascade<cacc64>* outStream0;
    output_stream<cfloat>* outStream1;
    output_stream<cfloat>* outStream2;
    output_stream<cfloat>* outStream3;
};
#endif //__SUPPORTS_CFLOAT__ == 1
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// StreamCascs
template <>
struct T_outputIF<int16, kStreamCascAPI> {
    output_stream<int16>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<int16>* outStream2;
    output_stream<int16>* outStream3;
};
template <>
struct T_outputIF<cint16, kStreamCascAPI> {
    output_stream<cint16>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<cint16>* outStream2;
    output_stream<cint16>* outStream3;
};
template <>
struct T_outputIF<int32, kStreamCascAPI> {
    output_stream<int32>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<int32>* outStream2;
    output_stream<int32>* outStream3;
};
template <>
struct T_outputIF<cint32, kStreamCascAPI> {
    output_stream<cint32>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<cint32>* outStream2;
    output_stream<cint32>* outStream3;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
struct T_outputIF<float, kStreamCascAPI> {
    output_stream<float>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<float>* outStream2;
    output_stream<float>* outStream3;
};
template <>
struct T_outputIF<cfloat, kStreamCascAPI> {
    output_stream<cfloat>* outStream0;
    output_cascade<cacc64>* outStream1;
    output_stream<cfloat>* outStream2;
    output_stream<cfloat>* outStream3;
};
#endif //__SUPPORTS_CFLOAT__ == 1
#endif //__SUPPORTS_ACC64__
}
}
}
}
}
#endif // _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_
