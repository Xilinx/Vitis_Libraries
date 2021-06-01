#ifndef _DSPLIB_FIR_DECIMATE_SYM_TRAITS_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_TRAITS_HPP_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {
/*
Symmetrical Decimation FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#define NOT_SUPPORTED 0
#define SUPPORTED 1
// Trigger static_assert with a reference.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnFirDecSym1buffSupport() {
    return SUPPORTED;
};

// Defensive checks

// Trigger static_assert with a reference.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnFirDecSymMultiColumn() {
    return SUPPORTED;
};

// Trigger static_assert with a reference.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnFirDecSymmertySupported() {
    return SUPPORTED;
};

#ifndef _DSPLIB_FIR_AIE_LLI_API_DEBUG_
// Unsupported types trigger static_assert. Only affected when AIE API in use
// template<>inline constexpr unsigned int fnFirDecSymMultiColumn<cint16,  int16>() {return NOT_SUPPORTED;}; //
// template<>inline constexpr unsigned int fnFirDecSymMultiColumn<cint16, cint16>() {return NOT_SUPPORTED;}; //
// template<>inline constexpr unsigned int fnFirDecSymMultiColumn< int32,  int32>() {return NOT_SUPPORTED;}; //

// FIR element support type combination with AIE API
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnFirDecSymTypeSupport() {
    return fnUnsupportedTypeCombo<TT_DATA, TT_COEFF>(); // supported combinations, apart from int16 data & int16 coeffs
                                                        // - due to HW restriction (xsquare)
};

template <>
inline constexpr unsigned int fnFirDecSymmertySupported<int32, int16>() {
    return NOT_SUPPORTED;
}; // unsupported. Data register size insufficient to cater for symmetric intrinsic requirements.
template <>
inline constexpr unsigned int fnFirDecSymmertySupported<cint32, int16>() {
    return NOT_SUPPORTED;
}; // unsupported. Data register size insufficient to cater for symmetric intrinsic requirements.
template <>
inline constexpr unsigned int fnFirDecSymmertySupported<float, float>() {
    return NOT_SUPPORTED;
}; // unsupported. No sym intrinsic.
template <>
inline constexpr unsigned int fnFirDecSymmertySupported<cfloat, float>() {
    return NOT_SUPPORTED;
}; // unsupported. No sym intrinsic.
template <>
inline constexpr unsigned int fnFirDecSymmertySupported<cfloat, cfloat>() {
    return NOT_SUPPORTED;
}; // unsupported. No sym intrinsic.

#endif // _DSPLIB_FIR_AIE_LLI_API_DEBUG_

// Forward and reverse data can fit into one 1024b register
// 2 registers required for forward and reverse data respectively
// Only low Decimation factor implementations exist because higher decimation factors require data ranges greater than
// buffer capacity
enum { kArch1BuffLowDFBasic, kArch1BuffLowDFIncrStrobe, kArch2BuffLowDFBasic, kArch2BuffLowDFZigZagStrobe };

static constexpr unsigned int kMaxDecimateFactor = 3;
static constexpr unsigned int kSymmetryFactor = 2;
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kUpdWSize = 32;         // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnMaxDecimateFactor() {
    return 3; //
};
template <>
inline constexpr unsigned int fnMaxDecimateFactor<cint32, cint16>() {
    return 2;
};
template <>
inline constexpr unsigned int fnMaxDecimateFactor<cint32, int32>() {
    return 2;
};

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnNumLanesDecSym() {
    return fnNumLanes384<TT_DATA, TT_COEFF>(); //
};

// function to return the number of columns for a type combo for the intrinsics used for this single rate asym FIR
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnNumColumnsDecSym() {
    return fnNumCols384<TT_DATA, TT_COEFF>();
};

#ifndef _DSPLIB_FIR_AIE_LLI_API_DEBUG_

// Once CRVO-3442 is fixed, revisit the code, to switch Cint32 Int32 type combo to the alternative intrinsic (2x2).
// template <> inline constexpr unsigned int fnNumLanesDecSym<  cint32,  int32>() {return 2;};
// template <> inline constexpr unsigned int fnNumColumnsDecSym<  cint32,  int32>() {return 2;};

#endif // _DSPLIB_FIR_AIE_LLI_API_DEBUG_

// Function to determine how many bits to load each time data is fetched from the input window.
template <typename TT_DATA, typename TT_COEFF, unsigned int T_ARCH>
inline constexpr unsigned int fnLoadSizeDecSym() {
    return 256;
}; // 256 - 1buff for all type combos with DF2 & DF3
template <>
inline constexpr unsigned int fnLoadSizeDecSym<cint16, int16, kArch2BuffLowDFBasic>() {
    return 128;
}; // 256 - DF2, 128 DF3
template <>
inline constexpr unsigned int fnLoadSizeDecSym<cint16, cint16, kArch2BuffLowDFBasic>() {
    return 128;
}; // 256 - DF2, 128 DF3
template <>
inline constexpr unsigned int fnLoadSizeDecSym<int32, int32, kArch2BuffLowDFBasic>() {
    return 128;
}; // 256 - DF2, 128 DF3
template <>
inline constexpr unsigned int fnLoadSizeDecSym<cint32, cint16, kArch2BuffLowDFBasic>() {
    return 128;
}; // 128 - DF2, unsupported - DF3
template <>
inline constexpr unsigned int fnLoadSizeDecSym<cint32, int32, kArch2BuffLowDFBasic>() {
    return 128;
}; // 128 - DF2, unsupported - DF3
// template <> inline constexpr unsigned int fnLoadSizeDecSym< cint32, cint32, kArch2BuffLowDFBasic>() {return 128;}; //
// 256 - DF2, 256 - DF3

template <typename TT_DATA>
inline constexpr unsigned int fnDataLoadsInRegDecSym() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    return 4;
}

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnVOutSizeDecSym() {
    return fnNumLanesDecSym<TT_DATA, TT_COEFF>();
};

// Function to return Lsize - the number of interations required to cover the input window.
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DECIMATE_FACTOR, unsigned int m_kVOutSize>
inline constexpr unsigned int fnLsize() {
    return (TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR) / m_kVOutSize;
}

// Calculate SYM FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeSym() {
    // TP_FL - FIR Length, TP_DF - Decimate Factor, TP_CL - Cascade Length, TP_KP - Kernel Position
    // make sure there's no runt filters ( lengths < 4)
    // make each cascade rounded to kFirRangeRound and only last in the chain possibly odd
    constexpr unsigned int kFirRangeRound = kSymmetryFactor * kMaxColumns;
    return fnFirRange<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeRemSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    constexpr unsigned int kFirRangeRound = kSymmetryFactor * kMaxColumns;
    return fnFirRangeRem<TP_FL, TP_CL, TP_KP, kFirRangeRound>();
}

// Calculate SYM FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP>
inline constexpr int fnFirRangeOffsetSym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    constexpr unsigned int kFirRangeRound = kSymmetryFactor * kMaxColumns;
    return fnFirRangeOffset<TP_FL, TP_CL, TP_KP, kFirRangeRound, kSymmetryFactor>();
}
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_SYM_TRAITS_HPP_

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
