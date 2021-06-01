#ifndef _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_
#define _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_

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
namespace widget {
namespace api_cast {

static constexpr unsigned int kWindowAPI = 0;
static constexpr unsigned int kStreamAPI = 1;

// This interface is maximally sized (3) since it doesn't matter if some inputs are unused.
template <typename T_D, unsigned int T_IN_API>
struct T_inputIF {};
template <>
struct T_inputIF<int16, 0> {
    input_window<int16>* inWindow0;
    input_window<int16>* inWindow1;
    input_window<int16>* inWindow2;
};
template <>
struct T_inputIF<cint16, 0> {
    input_window<cint16>* inWindow0;
    input_window<cint16>* inWindow1;
    input_window<cint16>* inWindow2;
};
template <>
struct T_inputIF<int32, 0> {
    input_window<int32>* inWindow0;
    input_window<int32>* inWindow1;
    input_window<int32>* inWindow2;
};
template <>
struct T_inputIF<cint32, 0> {
    input_window<cint32>* inWindow0;
    input_window<cint32>* inWindow1;
    input_window<cint32>* inWindow2;
};
template <>
struct T_inputIF<float, 0> {
    input_window<float>* inWindow0;
    input_window<float>* inWindow1;
    input_window<float>* inWindow2;
};
template <>
struct T_inputIF<cfloat, 0> {
    input_window<cfloat>* inWindow0;
    input_window<cfloat>* inWindow1;
    input_window<cfloat>* inWindow2;
};

template <>
struct T_inputIF<int16, 1> {
    input_stream<int16>* inStream0;
    input_stream<int16>* inStream1;
    input_stream<int16>* inStream2;
};
template <>
struct T_inputIF<cint16, 1> {
    input_stream<cint16>* inStream0;
    input_stream<cint16>* inStream1;
    input_stream<cint16>* inStream2;
};
template <>
struct T_inputIF<int32, 1> {
    input_stream<int32>* inStream0;
    input_stream<int32>* inStream1;
    input_stream<int32>* inStream2;
};
template <>
struct T_inputIF<cint32, 1> {
    input_stream<cint32>* inStream0;
    input_stream<cint32>* inStream1;
    input_stream<cint32>* inStream2;
};
template <>
struct T_inputIF<float, 1> {
    input_stream<float>* inStream0;
    input_stream<float>* inStream1;
    input_stream<float>* inStream2;
};
template <>
struct T_inputIF<cfloat, 1> {
    input_stream<cfloat>* inStream0;
    input_stream<cfloat>* inStream1;
    input_stream<cfloat>* inStream2;
};

template <typename T_D, unsigned int T_out_API>
struct T_outputIF {};
template <>
struct T_outputIF<int16, 0> {
    output_window<int16>* outWindow0;
    output_window<int16>* outWindow1;
    output_window<int16>* outWindow2;
    output_window<int16>* outWindow3;
};
template <>
struct T_outputIF<cint16, 0> {
    output_window<cint16>* outWindow0;
    output_window<cint16>* outWindow1;
    output_window<cint16>* outWindow2;
    output_window<cint16>* outWindow3;
};
template <>
struct T_outputIF<int32, 0> {
    output_window<int32>* outWindow0;
    output_window<int32>* outWindow1;
    output_window<int32>* outWindow2;
    output_window<int32>* outWindow3;
};
template <>
struct T_outputIF<cint32, 0> {
    output_window<cint32>* outWindow0;
    output_window<cint32>* outWindow1;
    output_window<cint32>* outWindow2;
    output_window<cint32>* outWindow3;
};
template <>
struct T_outputIF<float, 0> {
    output_window<float>* outWindow0;
    output_window<float>* outWindow1;
    output_window<float>* outWindow2;
    output_window<float>* outWindow3;
};
template <>
struct T_outputIF<cfloat, 0> {
    output_window<cfloat>* outWindow0;
    output_window<cfloat>* outWindow1;
    output_window<cfloat>* outWindow2;
    output_window<cfloat>* outWindow3;
};

template <>
struct T_outputIF<int16, 1> {
    output_stream<int16>* outStream0;
    output_stream<int16>* outStream1;
    output_stream<int16>* outStream2;
    output_stream<int16>* outStream3;
};
template <>
struct T_outputIF<cint16, 1> {
    output_stream<cint16>* outStream0;
    output_stream<cint16>* outStream1;
    output_stream<cint16>* outStream2;
    output_stream<cint16>* outStream3;
};
template <>
struct T_outputIF<int32, 1> {
    output_stream<int32>* outStream0;
    output_stream<int32>* outStream1;
    output_stream<int32>* outStream2;
    output_stream<int32>* outStream3;
};
template <>
struct T_outputIF<cint32, 1> {
    output_stream<cint32>* outStream0;
    output_stream<cint32>* outStream1;
    output_stream<cint32>* outStream2;
    output_stream<cint32>* outStream3;
};
template <>
struct T_outputIF<float, 1> {
    output_stream<float>* outStream0;
    output_stream<float>* outStream1;
    output_stream<float>* outStream2;
    output_stream<float>* outStream3;
};
template <>
struct T_outputIF<cfloat, 1> {
    output_stream<cfloat>* outStream0;
    output_stream<cfloat>* outStream1;
    output_stream<cfloat>* outStream2;
    output_stream<cfloat>* outStream3;
};
}
}
}
}
}
#endif // _DSPLIB_WIDGET_API_CAST_TRAITS_HPP_

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
