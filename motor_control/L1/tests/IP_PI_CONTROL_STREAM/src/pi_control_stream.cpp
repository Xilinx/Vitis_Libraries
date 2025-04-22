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
#include "pi_control.hpp"

using namespace xf::motorcontrol;

void PI_Control_stream_Inst(hls::stream<ap_int<BIT_WIDTH_DATA> >& s_axis,
                            hls::stream<ap_int<BIT_WIDTH_DATA> >& s_axis_2,
                            hls::stream<ap_int<BIT_WIDTH_DATA> >& m_axis,
                            ap_int<BIT_WIDTH_DATA> Sp,
                            ap_int<BIT_WIDTH_DATA> Kp,
                            ap_int<BIT_WIDTH_DATA> Ki,
                            ap_int<BIT_WIDTH_DATA> mode) {
#pragma HLS INTERFACE mode = axis port = s_axis
#pragma HLS INTERFACE mode = axis port = s_axis_2
#pragma HLS INTERFACE mode = axis port = m_axis

#pragma HLS INTERFACE mode = ap_none port = Sp
#pragma HLS INTERFACE mode = ap_none port = Kp
#pragma HLS INTERFACE mode = ap_none port = Ki
#pragma HLS INTERFACE mode = ap_none port = mode

#pragma HLS INTERFACE mode = ap_ctrl_none port = return

    PI_Control_stream<ap_uint<BIT_WIDTH_STREAM_FOC>, ap_uint<BIT_WIDTH_LOG_STREAM_FOC>, ap_int<BIT_WIDTH_DATA>,
                      ap_int<BIT_WIDTH_ACCUM> >(s_axis, s_axis_2, m_axis, Sp, Kp, Ki, mode);
}
