#ifndef _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_
#define _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_

/*
FFT traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
//-------------------------------------
// app-specific constants
static constexpr unsigned int kPointSizeMin = 16;
static constexpr unsigned int kPointSizeMax = 4096;
static constexpr unsigned int kMaxColumns = 2;
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kMaxPointSize = 4096;
static constexpr unsigned int kMaxPointLog = 12;

//-------------------------------
// I/O types
template <typename T_D>
struct T_inputIF {};
template <>
struct T_inputIF<int16> {
    input_window<int16>* inWindow;
};
template <>
struct T_inputIF<int32> {
    input_window<int32>* inWindow;
};
template <>
struct T_inputIF<cint16> {
    input_window<cint16>* inWindow;
};
template <>
struct T_inputIF<cint32> {
    input_window<cint32>* inWindow;
};
template <>
struct T_inputIF<float> {
    input_window<float>* inWindow;
};
template <>
struct T_inputIF<cfloat> {
    input_window<cfloat>* inWindow;
};

template <typename T_D>
struct T_outputIF {};
template <>
struct T_outputIF<int16> {
    output_window<int16>* outWindow;
};
template <>
struct T_outputIF<int32> {
    output_window<int32>* outWindow;
};
template <>
struct T_outputIF<cint16> {
    output_window<cint16>* outWindow;
};
template <>
struct T_outputIF<cint32> {
    output_window<cint32>* outWindow;
};
template <>
struct T_outputIF<float> {
    output_window<float>* outWindow;
};
template <>
struct T_outputIF<cfloat> {
    output_window<cfloat>* outWindow;
};

//---------------------------------------
// Configuration Defensive check functions
template <typename TT_DATA>
inline constexpr bool fnCheckDataType() {
    return false;
};
template <>
inline constexpr bool fnCheckDataType<cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataType<cint32>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataType<cfloat>() {
    return true;
};

template <typename TT_IN_DATA, typename TT_OUT_DATA>
inline constexpr bool fnCheckDataIOType() {
    return false;
};
template <>
inline constexpr bool fnCheckDataIOType<cint16, cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataIOType<cint16, cint32>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataIOType<cint32, cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataIOType<cint32, cint32>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataIOType<cfloat, cfloat>() {
    return true;
};

template <typename TT_TWIDDLE>
inline constexpr bool fnCheckTwiddleType() {
    return false;
};
template <>
inline constexpr bool fnCheckTwiddleType<cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckTwiddleType<cfloat>() {
    return true;
};

template <typename TT_DATA, typename TT_TWIDDLE>
inline constexpr bool fnCheckDataTwiddleType() {
    return false;
};
template <>
inline constexpr bool fnCheckDataTwiddleType<cint16, cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataTwiddleType<cint32, cint16>() {
    return true;
};
template <>
inline constexpr bool fnCheckDataTwiddleType<cfloat, cfloat>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
inline constexpr bool fnCheckPointSize() {
    return (TP_POINT_SIZE == 16 || TP_POINT_SIZE == 32 || TP_POINT_SIZE == 64 || TP_POINT_SIZE == 128 ||
            TP_POINT_SIZE == 256 || TP_POINT_SIZE == 512 || TP_POINT_SIZE == 1024 || TP_POINT_SIZE == 2048 ||
            TP_POINT_SIZE == 4096);
};

template <unsigned int TP_SHIFT>
inline constexpr bool fnCheckShift() {
    return (TP_SHIFT >= 0) && (TP_SHIFT <= 60);
};

template <typename TT_DATA, unsigned int TP_SHIFT>
inline constexpr bool fnCheckShiftFloat() {
    return !(std::is_same<TT_DATA, cfloat>::value) || // This check traps shift != 0 when data = cfloat
           (TP_SHIFT == 0);
};

template <typename TT_DATA, unsigned int RANKS, unsigned int TP_CASC_LEN>
inline constexpr bool fnCheckCascLen() {
    // equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
    return (TP_CASC_LEN > 0) &&
           (std::is_same<TT_DATA, cfloat>::value ? (TP_CASC_LEN <= RANKS) : (TP_CASC_LEN <= (RANKS + 1) / 2));
}

template <typename TT_DATA, unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN>
inline constexpr bool fnCheckCascLen2() {
    return (TP_CASC_LEN == 1) || (!std::is_same<TT_DATA, cfloat>::value) || (TP_POINT_SIZE != 16);
}

// End of Defensive check functions

//---------------------------------------------------
// Functions

// To reduce Data Memory required, the input window can be re-used as a temporary buffer of samples,
// but only when the internal type size is the same as the input type size
template <typename TT_DATA>
inline constexpr bool fnUsePingPongIntBuffer() {
    return false;
}; // only cint16 requires second internal buffer
template <>
inline constexpr bool fnUsePingPongIntBuffer<cint16>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
inline constexpr int fnPointSizePower() {
    return 0;
};
template <>
inline constexpr int fnPointSizePower<16>() {
    return 4;
}
template <>
inline constexpr int fnPointSizePower<32>() {
    return 5;
}
template <>
inline constexpr int fnPointSizePower<64>() {
    return 6;
}
template <>
inline constexpr int fnPointSizePower<128>() {
    return 7;
}
template <>
inline constexpr int fnPointSizePower<256>() {
    return 8;
}
template <>
inline constexpr int fnPointSizePower<512>() {
    return 9;
}
template <>
inline constexpr int fnPointSizePower<1024>() {
    return 10;
}
template <>
inline constexpr int fnPointSizePower<2048>() {
    return 11;
}
template <>
inline constexpr int fnPointSizePower<4096>() {
    return 12;
}

template <unsigned int TP_POINT_SIZE>
inline constexpr int fnOddPower() {
    return 0;
};
template <>
inline constexpr int fnOddPower<32>() {
    return 1;
}
template <>
inline constexpr int fnOddPower<128>() {
    return 1;
}
template <>
inline constexpr int fnOddPower<512>() {
    return 1;
}
template <>
inline constexpr int fnOddPower<2048>() {
    return 1;
}

//----------------------------------------------------------------------
// nullElem
template <typename T_RDATA>
inline T_RDATA nullElem() {
    return 0;
};

// Null cint16_t element
template <>
inline cint16_t nullElem() {
    cint16_t d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null cint32 element
template <>
inline cint32 nullElem() {
    cint32 d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null float element
template <>
inline float nullElem() {
    return 0.0;
};

// Null cint32 element
template <>
inline cfloat nullElem() {
    cfloat retVal;

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
}
}
}
}
}

#endif // _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_

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
