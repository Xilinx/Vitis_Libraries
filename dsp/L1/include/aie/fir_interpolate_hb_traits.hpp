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
#ifndef _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_

#include "device_defs.h"

/*
Halfband interpolating FIR traits.
 This file contains sets of overloaded, templatized and specialized templatized
 functions which encapsulate properties of the intrinsics used by the main kernel
 class. Specifically, this file does not contain any vector types or intrinsics
 since it is required for construction and therefore must be suitable for the
 aie compiler graph-level compilation.
*/

#define NOT_SUPPORTED 0
#define SUPPORTED 1

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kUse128bitLoads = 8; // Flag to use 128-bit loads, instead of 256-bit default loads
// Global constants,
#if __HAS_SYM_PREADD__ == 1
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kInterpolateFactor = 2;
#else
static constexpr unsigned int kSymmetryFactor = 1;
static constexpr unsigned int kInterpolateFactor = 2;
#endif
static constexpr unsigned int kFirRangeRound =
    kSymmetryFactor * kMaxColumns;            // Round cascaded FIR SYM kernels to multiples of 4,
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kUpdWToUpdVRatio = 256 / 128;
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

enum eArchType {
    kArch1Buff = 0, // Forward and reverse data can fit into one 1024b register
    kArch2Buff,     // 2 registers required for forward and reverse data respectively
    kArch2BuffZigZag
};

// Function to return #columns in mul_sym intrinsic
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumSymColsIntHb() {
    return fnNumCols384<TT_DATA, TT_COEFF>();
};

// Function to return #lanes in mul_sym function - NOT intrinsic (see cint32/cint32)
#if __MIN_REGSIZE__ == 128
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API = 0, unsigned int TP_UPSHIFT_CT = 0>
INLINE_DECL constexpr unsigned int fnNumSymLanesIntHb() {
    return fnNumLanes384<TT_DATA, TT_COEFF>();
};
#else
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API = 0, unsigned int TP_UPSHIFT_CT = 0>
INLINE_DECL constexpr unsigned int fnNumSymLanesIntHb() {
    return TP_API == 0 ? fnNumLanes<TT_DATA, TT_COEFF>() : fnNumLanes384<TT_DATA, TT_COEFF>();
};
#endif

// Function to return #loads in xbuff register:
// 4 for 2 buff arch, 8 for 1 buff arch, when 128-bit loads are used, else 4.
// 128-bit loads are used when TBD
template <eArchType TP_ARCH, typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int16, int16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, int16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, cint16>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int16, int32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, int32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, cint32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int32, int32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, cint32>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, float, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cfloat, cfloat>() {
    return 4;
};

template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int16, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int16, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, cint32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, int16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, cint16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, int32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, cint32>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, float, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cfloat, cfloat>() {
    return 4;
};

// Return support for Upshift CT. Only available for 16-bit integer numbers.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport() {
    return NOT_SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<int16, int16>() {
    return SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<cint16, int16>() {
    return SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<cint16, cint16>() {
    return SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<int16, int32>() {
    return NOT_SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<cint16, int32>() {
    return NOT_SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnUpshiftCTSupport<cint16, cint32>() {
    return NOT_SUPPORTED;
};

// ZigZag only supports UCT mode with cint16 data and int16 coeffs right now.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT>
INLINE_DECL constexpr unsigned int fnSupportZigZag() {
    return NOT_SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnSupportZigZag<cint16, int16, 1>() {
    return SUPPORTED;
};

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
}
}
}
}
}
#endif // _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_
