#ifndef _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_
#define _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_

/*
Single Rate Asymetrical FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {
enum eArchType { kArchBasic = 0, kArchIncLoads, kArchZigZag };

static constexpr unsigned int kMaxColumns = 2;
static constexpr unsigned int kUpdWSize = 32;         // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnNumLanesSrAsym() {
    return 0; // effectively an error trap, but adding an error message to a constexpr return results in a warning.
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<int16, int16>() {
    return 16;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint16, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint16, cint16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<int32, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<int32, int32>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint32, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cint32, cint32>() {
    return 2;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<float, float>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cfloat, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesSrAsym<cfloat, cfloat>() {
    return 4;
};

// function to return the number of columns for a type combo for the intrinsics used for this single rate asym FIR
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnNumColumnsSrAsym() {
    return sizeof(TT_COEFF) == 2 ? 2 : 1;
};
// specialize for any exceptions like this:
// template<> inline constexpr unsigned int fnNumColumnsDecHb< int16,  int16, K_ARCH_1BUFF>() { return 2;};

template <typename TT_DATA>
inline constexpr unsigned int fnDataLoadsInRegSrAsym() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    return 4;
}

// Parameter to constant resolution functions
template <typename TT_DATA>
inline constexpr unsigned int fnDataLoadVsizeSrAsym() {
    return (kUpdWSize / sizeof(TT_DATA));
}

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnVOutSizeSrAsym() {
    return fnNumLanesSrAsym<TT_DATA, TT_COEFF>();
};
}
}
}
}
}
#endif // _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_

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
