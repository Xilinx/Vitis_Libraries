#ifndef _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_

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
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kInterpolateFactor = 2;
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
inline constexpr unsigned int fnNumSymColsIntHb() {
    return 0;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<int16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint16, cint16>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<int32, int16>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<int32, int32>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint32, int16>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint32, cint16>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint32, int32>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cint32, cint32>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<float, float>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cfloat, float>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumSymColsIntHb<cfloat, cfloat>() {
    return 1;
};

// Function to return #lanes in mul_sym function - NOT intrinsic (see cint32/cint32)
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT = 0>
inline constexpr unsigned int fnNumSymLanesIntHb() {
    return fnNumLanes384<TT_DATA, TT_COEFF>();
};
template <>
inline constexpr unsigned int fnNumSymLanesIntHb<int16, int16, 1>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumSymLanesIntHb<cint16, int16, 1>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumSymLanesIntHb<cint16, cint16, 1>() {
    return 4;
};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb< int32,  int16>(){  return 8;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb< int32,  int32>(){  return 4;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cint32,  int16>(){  return 4;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cint32, cint16>(){  return 4;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cint32,  int32>(){  return 4;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cint32, cint32>(){  return 2;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb< float,  float>(){  return 8;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cfloat,  float>(){  return 4;};
// template<> inline constexpr unsigned int fnNumSymLanesIntHb<cfloat, cfloat>(){  return 4;};

// Function to return #loads in xbuff register:
// 4 for 2 buff arch, 8 for 1 buff arch, when 128-bit loads are used, else 4.
// 128-bit loads are used when TBD
template <eArchType TP_ARCH, typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnDataLoadsInReg() {
    return 0;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int16, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint16, cint16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, int32, int32>() {
    return 8;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cint32, cint32>() {
    return 8;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, float, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cfloat, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch1Buff, cfloat, cfloat>() {
    return 4;
};

template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint16, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, int32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cint32, cint32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, float, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cfloat, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnDataLoadsInReg<kArch2Buff, cfloat, cfloat>() {
    return 4;
};

// Return support for Upshift CT. Only available for 16-bit integer numbers.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnUpshiftCTSupport() {
    return NOT_SUPPORTED;
};
template <>
inline constexpr unsigned int fnUpshiftCTSupport<int16, int16>() {
    return SUPPORTED;
};
template <>
inline constexpr unsigned int fnUpshiftCTSupport<cint16, int16>() {
    return SUPPORTED;
};
template <>
inline constexpr unsigned int fnUpshiftCTSupport<cint16, cint16>() {
    return SUPPORTED;
};

// ZigZag only supports UCT mode with cint16 data and int16 coeffs right now.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT>
inline constexpr unsigned int fnSupportZigZag() {
    return 0;
};
template <>
inline constexpr unsigned int fnSupportZigZag<cint16, int16, 1>() {
    return 1;
};

// Calculate SYM FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // make sure there's no runt filters ( lengths < 4)
    // make each cascade rounded to kFirRangeRound and only last in the chain possibly odd
    return fnFirRange<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeRemSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    return fnFirRangeRem<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}

// Calculate SYM FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeOffsetSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return fnFirRangeOffset<TP_FL, TP_CL, TP_KP, kFirRangeRound, kSymmetryFactor>();
}
}
}
}
}
}
#endif // _DSPLIB_FIR_INTERPOLATE_HB_TRAITS_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
