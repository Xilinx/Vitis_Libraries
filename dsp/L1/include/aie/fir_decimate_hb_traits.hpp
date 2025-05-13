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
#ifndef _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_
#define _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_

#include "device_defs.h"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {
/*
    Halfband decimation FIR traits.
    This file contains sets of overloaded, templatized and specialized templatized functions which
    encapsulate properties of the intrinsics used by the main kernal class. Specifically,
    this file does not contain any vector types or intrinsics since it is required for construction
    and therefore must be suitable for the aie compiler graph-level compilation.
*/
enum eArchType {
    kArch1Buff,  // Forward and reverse data can fit into one 1024b register
    kArch2Buff,  // 2 registers required for forward and reverse data respectively
    kArchZigZag, // Performance optimized version of 2 buff.
    unknown
};

// Global constants,
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kUse128bitLoads = 8; // Flag to use 128-bit loads, instead of 256-bit default loads
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kDecimateFactor = 2;
static constexpr unsigned int kHbFactor = 2;
static constexpr unsigned int kFirRangeRound =
    kSymmetryFactor * kMaxColumns;            // Round cascaded FIR SYM kernels to multiples of 4,
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kUpdWToUpdVRatio = 256 / 128;
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

// Function to trap illegal types combinations particular to the halfband decimator
// The default, general case is a pass.
// Illegal combinations are called out by specialization, which takes priority over the general case.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnUnsupportedTypeComboFirDecHb() {
    return 1;
}; // default here is a legal combo
#if __HAS_SYM_PREADD__ == 1
template <>
INLINE_DECL constexpr unsigned int fnUnsupportedTypeComboFirDecHb<int16, int16>() {
    return 0;
};
#endif

// Function to determine how many bits to load each time data is fetched from the input window.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL constexpr unsigned int fnLoadSize() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<int16, int16, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<int16, int32, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint16, cint16, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint16, int32, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint16, cint32, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<int32, int32, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint32, cint16, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint32, int32, kArch2Buff>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSize<cint32, cint32, kArch2Buff>() {
    return 128;
};

// Function to determine how many bytes are in the X buff
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL constexpr unsigned int fnXBuffBSize() {
    return kBuffSize128Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<int16, int16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<int16, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint16, int16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint16, cint16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint16, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint16, cint32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<int32, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint32, cint16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint32, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
INLINE_DECL constexpr unsigned int fnXBuffBSize<cint32, cint32, kArch2Buff>() {
    return kBuffSize64Byte;
};

// Function to return the number of lanes in the each call to mul/mac, not the number of lanes in the instrinsic
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH = unknown>
INLINE_DECL constexpr unsigned int fnNumOpLanesDecHb() {
    return fnNumLanes384<TT_DATA, TT_COEFF>();
};

// Function to return the number of columns for a type combo
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH = unknown>
INLINE_DECL constexpr unsigned int fnNumColumnsDecHb() {
    return fnNumCols384<TT_DATA, TT_COEFF>();
};

// Function to return the number of passes of the main loop for a type combo
// Multiple passes create parallel accumulators and multiple outputs that can be send to 2 output streams.
// Multiple passes only required when 2 streams are in use and output vector is only 128-bit wide
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb() {
    return 1;
};
// template <> INLINE_DECL constexpr unsigned int fnNumPassesDecHb< int16,  int16>() {return 1;}; // unsopported
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<cint16, int16>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<cint16, cint16>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<cint16, int32>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<cint16, cint32>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<int32, int32>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnNumPassesDecHb<cint32, cint32>() {
    return 2;
};

// Generally, each operation requires data from 2 or 3 quarters of the xbuff, leaving the last quarter for pre-emptive
// loads.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg() {
    return 4;
};
// TODO. The following were exceptions when upd_v (128b loads) were always used.
template <>
INLINE_DECL constexpr unsigned int fnDataLoadsInReg<cint16, int16, kArch2Buff>() {
    return 2;
}; // iobuffer adoption - dual streams out failed because repeat factor was 1, so outputs only went to one phase
   // (stream). xbuff=1024 and reads are 256, for this data type

// ZigZag only developed for one type right now.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnSupportZigZag() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnSupportZigZag<cint16, int16>() {
    return 1;
};

// Function to return the output vector size for each type combo.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL constexpr unsigned int fnVOutSizeDecHb() {
    return fnNumLanes384<TT_DATA, TT_COEFF>();
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
#endif // _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_
