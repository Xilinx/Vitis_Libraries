#ifndef _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_
#define _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_

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
    kArch1Buff, // Forward and reverse data can fit into one 1024b register
    kArch2Buff, // 2 registers required for forward and reverse data respectively
    kArchZigZag // Performance optimized version of 2 buff.

};

// Global constants,
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kUse128bitLoads = 8; // Flag to use 128-bit loads, instead of 256-bit default loads
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kDecimateFactor = 2;
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
inline constexpr unsigned int fnUnsupportedTypeComboFirDecHb() {
    return 1;
}; // default here is a legal combo
template <>
inline constexpr unsigned int fnUnsupportedTypeComboFirDecHb<int16, int16>() {
    return 0;
};

// Function to determine how many bits to load each time data is fetched from the input window.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnLoadSize() {
    return 256;
};
template <>
inline constexpr unsigned int fnLoadSize<int16, int16, kArch2Buff>() {
    return 128;
};
template <>
inline constexpr unsigned int fnLoadSize<cint16, cint16, kArch2Buff>() {
    return 128;
};
template <>
inline constexpr unsigned int fnLoadSize<int32, int32, kArch2Buff>() {
    return 128;
};
template <>
inline constexpr unsigned int fnLoadSize<cint32, cint16, kArch2Buff>() {
    return 128;
};
template <>
inline constexpr unsigned int fnLoadSize<cint32, int32, kArch2Buff>() {
    return 128;
};
template <>
inline constexpr unsigned int fnLoadSize<cint32, cint32, kArch2Buff>() {
    return 128;
};

// Function to determine how many bytes are in the X buff
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnXBuffBSize() {
    return kBuffSize128Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<int16, int16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<cint16, int16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<cint16, cint16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<int32, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<cint32, cint16, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<cint32, int32, kArch2Buff>() {
    return kBuffSize64Byte;
};
template <>
inline constexpr unsigned int fnXBuffBSize<cint32, cint32, kArch2Buff>() {
    return kBuffSize64Byte;
};

// Function to return the number of lanes in the intrinsic of choice
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnNumIntrLanesDecHb() {
    return 0;
}; // effectively an error trap, but adding an error message to a constexpr return results in a warning.
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int16, int16, kArch1Buff>() {
    return 16;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint16, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint16, cint16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int32, int16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int32, int32, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, cint16, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, int32, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, cint32, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<float, float, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cfloat, float, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cfloat, cfloat, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int16, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint16, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint16, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int32, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<int32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cint32, cint32, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<float, float, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cfloat, float, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumIntrLanesDecHb<cfloat, cfloat, kArch2Buff>() {
    return 4;
};

// Function to return the number of lanes in the each call to mul/mac, not the number of lanes in the instrinsic
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnNumOpLanesDecHb() {
    return 0;
}; // effectively an error trap, but adding an error message to a constexpr return results in a warning.
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int16, int16, kArch1Buff>() {
    return 16;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint16, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint16, cint16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int32, int16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int32, int32, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, int16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, cint16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, int32, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, cint32, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<float, float, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cfloat, float, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cfloat, cfloat, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int16, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint16, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint16, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int32, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<int32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cint32, cint32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<float, float, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cfloat, float, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumOpLanesDecHb<cfloat, cfloat, kArch2Buff>() {
    return 4;
};

// Function to return the number of columns for a type combo
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnNumColumnsDecHb() {
    return 0;
}; // effectively an error trap, but adding an error message to a constexpr return results in a warning.
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int16, int16, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint16, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint16, cint16, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int32, int16, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int32, int32, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, int16, kArch1Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, cint16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, int32, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, cint32, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<float, float, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cfloat, float, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cfloat, cfloat, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int16, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint16, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint16, cint16, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int32, int16, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<int32, int32, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, int16, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, cint16, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, int32, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cint32, cint32, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<float, float, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cfloat, float, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumColumnsDecHb<cfloat, cfloat, kArch2Buff>() {
    return 1;
};

// Function to return the number of passes of the main loop for a type combo
// Multiple passes create parallel accumulators, which are operated on sequentially,
// re-using data in xbuff/ybuff and therefore, reducing the amount of data loads
// required to process complete window of data samples.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnNumPassesDecHb() {
    return 0;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<int16, int16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint16, int16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint16, cint16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<int32, int16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<int32, int32, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, int16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, cint16, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, int32, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, cint32, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<float, float, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cfloat, float, kArch1Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cfloat, cfloat, kArch1Buff>() {
    return 1;
};

template <>
inline constexpr unsigned int fnNumPassesDecHb<int16, int16, kArch2Buff>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint16, int16, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint16, cint16, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<int32, int16, kArch2Buff>() {
    return 1;
}; // see note wrt Input Vector register size at top of fir_decimate_hb_utils.hpp
template <>
inline constexpr unsigned int fnNumPassesDecHb<int32, int32, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, int16, kArch2Buff>() {
    return 1;
}; // see note wrt Input Vector register size at top of fir_decimate_hb_utils.hpp
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, cint16, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, int32, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cint32, cint32, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<float, float, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cfloat, float, kArch2Buff>() {
    return 1;
};
template <>
inline constexpr unsigned int fnNumPassesDecHb<cfloat, cfloat, kArch2Buff>() {
    return 1;
};

// Generally, each operation requires data from 2 or 3 quarters of the xbuff, leaving the last quarter for pre-emptive
// loads.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnDataLoadsInReg() {
    return 4;
};
// TODO. The following were exceptions when upd_v (128b loads) were always used.
template <>
inline constexpr unsigned int fnDataLoadsInReg<cint16, int16, kArch2Buff>() {
    return 2;
};

// ZigZag only developed for one type right now.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnSupportZigZag() {
    return 0;
};
template <>
inline constexpr unsigned int fnSupportZigZag<cint16, int16>() {
    return 1;
};

// Function to return the output vector size for each type combo.
// Follows closely fnNumIntrLanesDecHb, but differs when possible to use multiple
// accs, in order to craete a longer output vector.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
inline constexpr unsigned int fnVOutSizeDecHb() {
    return 0;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int16, int16, kArch1Buff>() {
    return 16;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint16, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint16, cint16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int32, int16, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int32, int32, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, int16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, cint16, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, int32, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, cint32, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<float, float, kArch1Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cfloat, float, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cfloat, cfloat, kArch1Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int16, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint16, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint16, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int32, int16, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<int32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, int16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, cint16, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, int32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cint32, cint32, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<float, float, kArch2Buff>() {
    return 8;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cfloat, float, kArch2Buff>() {
    return 4;
};
template <>
inline constexpr unsigned int fnVOutSizeDecHb<cfloat, cfloat, kArch2Buff>() {
    return 4;
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
#endif // _DSPLIB_FIR_DECIMATE_HB_TRAITS_HPP_

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
