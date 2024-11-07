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
#ifndef _DSPLIB_DDS_MIXER_TRAITS_HPP_
#define _DSPLIB_DDS_MIXER_TRAITS_HPP_

#include "device_defs.h"

#define USE_INBUILT_SINCOS 0
#define USE_LUT_SINCOS 1

#define USE_PHASE_RELOAD_FALSE 0
#define USE_PHASE_RELOAD_TRUE 1

#define IOBUFFER_API 0
#define STREAM_API 1

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

template <typename TT_DATA, unsigned int tp_sincos_mode>
constexpr int ddsMulVecScalarLanes() {
    return 0;
} // effectively an error trap

template <>
constexpr int ddsMulVecScalarLanes<cint16, USE_INBUILT_SINCOS>() {
    return 8;
};
template <>
constexpr int ddsMulVecScalarLanes<cint32, USE_INBUILT_SINCOS>() {
    return 4;
};
template <>
constexpr int ddsMulVecScalarLanes<cfloat, USE_INBUILT_SINCOS>() {
    return 4;
};

#if __SUPPORTS_CFLOAT__ == 1
template <>
constexpr int ddsMulVecScalarLanes<cint16, USE_LUT_SINCOS>() {
    return 8;
};
template <>
constexpr int ddsMulVecScalarLanes<cint32, USE_LUT_SINCOS>() {
    return 4;
};
template <>
constexpr int ddsMulVecScalarLanes<cfloat, USE_LUT_SINCOS>() {
    return 2;
};
#else
template <>
constexpr int ddsMulVecScalarLanes<cint16, USE_LUT_SINCOS>() {
    return 8;
};
template <>
constexpr int ddsMulVecScalarLanes<cint32, USE_LUT_SINCOS>() {
    return 8;
};
template <>
constexpr int ddsMulVecScalarLanes<cfloat, USE_LUT_SINCOS>() {
    return 2;
};
#endif

/**

Base IO_API interface struct for shared functions across specialisations
*/
template <typename TT_DATA, typename PortType>
struct T_IFbase {
    template <unsigned int VECTOR_LEN>
    auto static INLINE_DECL port_readincr(PortType* in);

    template <typename OutDType>
    void static INLINE_DECL port_writeincr(PortType* out, OutDType data);
};
// mode 0 - no inputs, dds_only
template <typename TT_DATA, typename InPortType, unsigned int numMixerIn>
struct T_inputIF : T_IFbase<TT_DATA, InPortType> {};

// Mixer mode 1
template <typename TT_DATA, typename InPortType>
struct T_inputIF<TT_DATA, InPortType, 1> : T_IFbase<TT_DATA, InPortType> {
    InPortType* __restrict inPort;   // can be a window or a stream
    T_inputIF(){};                   // default constructor do nothing
    T_inputIF(InPortType* _inPort) { // todo - use references rather than pointers.
        inPort = _inPort;
    }
};

// Mixer mode 2
template <typename TT_DATA, typename InPortType>
struct T_inputIF<TT_DATA, InPortType, 2> : T_IFbase<TT_DATA, InPortType> {
    InPortType* __restrict inPortA;                         // can be a window or a stream
    InPortType* __restrict inPortB;                         // can be a window or a stream
    T_inputIF(){};                                          // default constructor do nothing
    T_inputIF(InPortType* _inPortA, InPortType* _inPortB) { // todo - use references rather than pointers.
        inPortA = _inPortA;
        inPortB = _inPortB;
    }
};

// mode 0,1,2 - single output (stream or window)
template <typename TT_DATA, typename OutPortType>
struct T_outputIF : T_IFbase<TT_DATA, OutPortType> { // inheritance

    OutPortType* __restrict outPort;
    T_outputIF(){};
    T_outputIF(OutPortType* _outPort) { outPort = _outPort; }
};

// Functions to support defensive checks
enum { enumUnknownType = 0, enumCint16, enumCint32, enumCfloat };
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnEnumType() {
    return enumUnknownType;
}; // returns 0 as default. This can be trapped as an error;
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint16>() {
    return enumCint16;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint32>() {
    return enumCint32;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cfloat>() {
    return enumCfloat;
};
#endif

enum IO_API { WINDOW = 0, STREAM = 1 };
}
}
}
}
}
#endif // _DSPLIB_DDS_MIXER_TRAITS_HPP_
