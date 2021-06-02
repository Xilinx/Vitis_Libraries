/*
Widget API Cast reference model
*/

#include "widget_real2complex_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {

// Widget real2complex - default/base 'specialization'
// int16 to cint16
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE>::convertData(
    input_window<TT_DATA>* inWindow0, output_window<TT_OUT_DATA>* outWindow0) {
    TT_DATA d_in;
    TT_OUT_DATA d_out;
    d_out.imag = 0;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out.real = d_in;
        window_writeincr(outWindow0, d_out);
    }
};

// int32 to cint32
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<int32, cint32, TP_WINDOW_VSIZE>::convertData(input_window<int32>* inWindow0,
                                                                          output_window<cint32>* outWindow0) {
    int32 d_in;
    cint32 d_out;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out.real = d_in;
        window_writeincr(outWindow0, d_out);
    }
};

// float to cfloat
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<float, cfloat, TP_WINDOW_VSIZE>::convertData(input_window<float>* inWindow0,
                                                                          output_window<cfloat>* outWindow0) {
    float d_in;
    cfloat d_out;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out.real = d_in;
        window_writeincr(outWindow0, d_out);
    }
};

// cint16 to int16
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cint16, int16, TP_WINDOW_VSIZE>::convertData(input_window<cint16>* inWindow0,
                                                                          output_window<int16>* outWindow0) {
    cint16 d_in;
    int16 d_out;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out = d_in.real;
        window_writeincr(outWindow0, d_out);
    }
};

// cint32 to int32
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cint32, int32, TP_WINDOW_VSIZE>::convertData(input_window<cint32>* inWindow0,
                                                                          output_window<int32>* outWindow0) {
    cint32 d_in;
    int32 d_out;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out = d_in.real;
        window_writeincr(outWindow0, d_out);
    }
};

// cfloat to float
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cfloat, float, TP_WINDOW_VSIZE>::convertData(input_window<cfloat>* inWindow0,
                                                                          output_window<float>* outWindow0) {
    cfloat d_in;
    float d_out;

#ifdef _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        d_out = d_in.real;
        window_writeincr(outWindow0, d_out);
    }
};
}
}
}
}
}

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
