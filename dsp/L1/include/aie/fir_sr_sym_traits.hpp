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
#ifndef _DSPLIB_FIR_SR_SYM_TRAITS_HPP_
#define _DSPLIB_FIR_SR_SYM_TRAITS_HPP_

#include "device_defs.h"

/*
Single Rate Symmetrical FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_sym {
// Global constants,
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kFirRangeRound =
    kSymmetryFactor * kMaxColumns;            // Round cascaded FIR SYM kernels to multiples of 8,
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kUpdWToUpdVRatio = 256 / 128;
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

enum eArchType { kArch1Buff = 0, kArch2Buff };

enum eBuffVariant {
    // kNoPreLoad - default, uses upd_w - 256bit loads, no load sequence optimization
    kNoPreLoad = 0,
    // kPreLoadUsing128 - uses upd_v to update xbuff - 128bit loads and schedules load early, to avoid memory conflicts.
    // To be used with data/coeff combo utilizing multi column MUL/MAC intrinsics, i.e. coeff type equal to int16
    kPreLoadUsing128,
    // kPreLoadUsing256 - uses upd_w - 256bit loads and  schedules load early, to avoid memory conflicts.
    // To be used with data/coeff combo utilizing single column MUL/MAC intrinsics, i.e. coeff type greater than int16
    kPreLoadUsing256,
};

// Function returns window access size, to be used when discarding FIR function's initial offset.
// This allows better xbuff and ybuff buffers alignment and reduces the amount of data loads.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize() {
    return 32;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<cint16, int16>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<cint16, cint16>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<int32, int32>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<int16, int16>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<int16, int32>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<cint16, int32>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize<cint16, cint32>() {
    return 16;
};

template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadsInRegSrSym() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    return 4;
}

// Parameter to constant resolution functions
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadVsizeSrSym() {
    return (kUpdWSize / sizeof(TT_DATA));
}

// Calculate SYM FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
INLINE_DECL constexpr int fnFirRangeSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // make sure there's no runt filters ( lengths < 4)
    // make each cascade rounded to kFirRangeRound and only last in the chain possibly odd
    return fnFirRange<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
INLINE_DECL constexpr int fnFirRangeRemSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    return fnFirRangeRem<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}

// Calculate SYM FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
INLINE_DECL constexpr int fnFirRangeOffsetSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return fnFirRangeOffset<TP_FL, TP_CL, TP_KP, kFirRangeRound, kSymmetryFactor>();
}

// Function returns number of columns used by MUL/MACs in Sym FIR for a specific data/coeff combo.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumColumnsSym() {
    return fnNumCols384<TT_DATA, TT_COEFF>();
};

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanesSym() {
    return fnNumLanes384<TT_DATA, TT_COEFF>();
};

// function to return info about x & y buffer update scheme
// Update scheme scheduled load in such way that some memory conflicts are avoided.
// As a result, higher performance is obtained when single input buffer is used.
// No effect when DUAL_IP, i.e. 2 input buffers are used, since memory conflicts are avoided altogether.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme() {
    return kNoPreLoad;
};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme< int16,  int16>() { return kPreLoadUsing128;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint16,  int16>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint16, cint16>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme< int32,  int16>() { return kPreLoadUsing128;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme< int32,  int32>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint32,  int16>() { return kNoPreLoad;};        //
// combo not fit for pre-load optimization,
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint32, cint16>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint32,  int32>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cint32, cint32>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme< float,  float>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cfloat,  float>() { return kPreLoadUsing256;};
// template<> INLINE_DECL constexpr eBuffVariant fnBufferUpdateScheme<cfloat, cfloat>() { return kPreLoadUsing256;};

// function anwsers whether 1 Buff architecture is supported for a given data/coeff type combinations.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr bool fn1BuffArchSupported() {
    // all, no exceptions
    return true;
};
}
}
}
}
}
#endif // _DSPLIB_FIR_SR_SYM_TRAITS_HPP_
