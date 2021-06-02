#ifndef _DSPLIB_L1_UTILS_HPP_
#define _DSPLIB_L1_UTILS_HPP_

/*
L1 testbench utilities.
This file contains sets of overloaded functions for use by the L1 testbench.
*/

#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
// Update int16 window with samples starting from offset value
void update_window(
    int offset, int16_t* samples, input_window_int16* input, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    int16 unit;
    int real, imag;
    window_incr(input, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        unit = samples[i + firMargin];
        window_writeincr((output_window_int16*)input, unit);
    }
}
// Update int32 window with samples starting from offset value
void update_window(
    int offset, int32_t* samples, input_window_int32* input, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    int32 unit;
    int real, imag;
    window_incr(input, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        unit = samples[i + firMargin];
        window_writeincr((output_window_int32*)input, unit);
    }
}
// Update cint16 window with samples starting from offset value
void update_window(
    int offset, cint16_t* samples, input_window_cint16* cinput, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    cint16 cunit;
    int real, imag;
    window_incr(cinput, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        cunit.real = samples[i + firMargin].real;
        cunit.imag = samples[i + firMargin].imag;
        window_writeincr((output_window_cint16*)cinput, cunit);
    }
}
// Update cint32 window with samples starting from offset value
void update_window(
    int offset, cint32_t* samples, input_window_cint32* cinput, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    cint32 cunit;
    int real, imag;
    window_incr(cinput, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        cunit.real = samples[i + firMargin].real;
        cunit.imag = samples[i + firMargin].imag;
        window_writeincr((output_window_cint32*)cinput, cunit);
    }
}
// Update float window with samples starting from offset value
void update_window(
    int offset, float* samples, input_window_float* input, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    float unit;
    window_incr(input, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        unit = samples[i + firMargin];
        window_writeincr((output_window_float*)input, unit);
    }
}
// Update cfloat window with samples starting from offset value
void update_window(
    int offset, cfloat* samples, input_window_cfloat* cinput, int firLen, int firMargin, int SAMPLES_IN_WINDOW) {
    cfloat cunit;
    float real, imag;
    window_incr(cinput, firMargin);
    for (int i = offset; i < offset + SAMPLES_IN_WINDOW; i++) {
        cunit.real = samples[i + firMargin].real;
        cunit.imag = samples[i + firMargin].imag;
        window_writeincr((output_window_cfloat*)cinput, cunit);
    }
}
}
}
}
#endif // _DSPLIB_L1_UTILS_HPP_

/*  (c) Copyright 2019 Xilinx, Inc. All rights reserved.

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
