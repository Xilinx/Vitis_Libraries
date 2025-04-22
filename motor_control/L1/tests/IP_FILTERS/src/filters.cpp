/*
Copyright (C) 2024-2024, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#include "filters.hpp"

using namespace xf::motorcontrol;

void ps_iir_filter_inst(

    hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_speed_theta>& SPEED_THETA_m, // RPM & Theta_m

    hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& output_stream,
    hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream,

    int32_t& filt_a,
    int32_t& filt_b,
    int32_t& angle_shift) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = SPEED_THETA_m
#pragma HLS interface axis port = output_stream
#pragma HLS interface axis port = logger_stream
#pragma HLS INTERFACE ap_none port = filt_a
#pragma HLS INTERFACE ap_none port = filt_b
#pragma HLS INTERFACE ap_none port = angle_shift

#pragma HLS interface ap_ctrl_none port = return

    ps_iir_filter<ap_uint<BIT_WIDTH_STREAM_FOC>, ap_uint<BIT_WIDTH_LOG_STREAM_FOC>, ap_int<BIT_WIDTH_DATA>,
                  ap_int<BIT_WIDTH_ACCUM>, t_glb_foc2pwm, t_glb_speed_theta>(
        Ia, Ib, Ic, SPEED_THETA_m, output_stream, logger_stream, filt_a, filt_b, angle_shift);
}
