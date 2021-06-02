#ifndef _DSPLIB_FIR_INTERPOLATE_ASYM_TRAITS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_ASYM_TRAITS_HPP_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {
/*
Asymmetrical Interpolation FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

// The following is a set of type-specialized functions which return the number of accumulator registers
// available in the processor. Since these may be 384 or 768 bit registers the number could vary by type.
template <typename TT_DATA, typename TT_COEFF>
unsigned int fnAccRegsIntAsym() {
    return 0;
}; // default error trap
template <>
inline constexpr unsigned int fnAccRegsIntAsym<int16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint16, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint16, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<int32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<int32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint32, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cint32, cint32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<float, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cfloat, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnAccRegsIntAsym<cfloat, cfloat>() {
    return 4;
};

// function to return the number of lanes for a type combo
// The default is effectively an error trap, but adding an error message to a constexpr return results in a warning.
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnNumLanesIntAsym() {
    return 0;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<int16, int16>() {
    return 16;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint16, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint16, cint16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<int32, int16>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<int32, int32>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint32, int16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint32, cint16>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint32, int32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cint32, cint32>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<float, float>() {
    return 8;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cfloat, float>() {
    return 4;
};
template <>
inline constexpr unsigned int fnNumLanesIntAsym<cfloat, cfloat>() {
    return 4;
};

// Function to return the lowest common multiple of two numbers
// A full implementation of this would entail prime factor decomposition, but here
// The maximum integer size is 16, so a simpler brute force method will do.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FACTOR>
inline constexpr unsigned int fnLCMIntAsym() {
    return ((fnNumLanesIntAsym<TT_DATA, TT_COEFF>() == 2)
                ? ((TP_FACTOR % 2 == 0) ? TP_FACTOR : (TP_FACTOR * 2))
                : (fnNumLanesIntAsym<TT_DATA, TT_COEFF>() == 4)
                      ? ((TP_FACTOR % 4 == 0) ? TP_FACTOR : ((TP_FACTOR % 2 == 0) ? (TP_FACTOR * 2) : (TP_FACTOR * 4)))
                      : (fnNumLanesIntAsym<TT_DATA, TT_COEFF>() == 8)
                            ? ((TP_FACTOR % 8 == 0)
                                   ? TP_FACTOR
                                   : ((TP_FACTOR % 4 == 0) ? (TP_FACTOR * 2)
                                                           : ((TP_FACTOR % 2 == 0) ? (TP_FACTOR * 4) : TP_FACTOR * 8)))
                            : 0);
};

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnVOutSizeIntAsym() {
    return fnNumLanesIntAsym<TT_DATA, TT_COEFF>();
};
}
}
}
}
} // namespaces
#endif // _DSPLIB_FIR_INTERPOLATE_ASYM_TRAITS_HPP_

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
