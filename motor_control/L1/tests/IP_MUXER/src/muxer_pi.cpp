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
#include "muxer_pi.hpp"

using namespace xf::motorcontrol;

void muxer_pi_inst(hls::stream<ap_int<BIT_WIDTH_DATA> >& from_torque,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_flux,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_demux,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_gen,
                   hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                   hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                   hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                   volatile int& control_mode,
                   volatile int& Vd_ps,
                   volatile int& Vq_ps) {
#pragma HLS INTERFACE mode = axis port = from_torque
#pragma HLS INTERFACE mode = axis port = logger_stream_in
#pragma HLS INTERFACE mode = axis port = logger_stream_out
#pragma HLS INTERFACE mode = axis port = from_flux
#pragma HLS INTERFACE mode = axis port = from_demux
#pragma HLS INTERFACE mode = axis port = from_gen
#pragma HLS INTERFACE mode = axis port = m_axis

#pragma HLS INTERFACE mode = ap_none port = control_mode
#pragma HLS INTERFACE mode = ap_none port = Vd_ps
#pragma HLS INTERFACE mode = ap_none port = Vq_ps

#pragma HLS INTERFACE mode = ap_ctrl_none port = return

    muxer_pi<ap_uint<BIT_WIDTH_STREAM_FOC>, ap_uint<BIT_WIDTH_LOG_STREAM_FOC>, ap_int<BIT_WIDTH_DATA>,
             ap_int<BIT_WIDTH_ACCUM> >(from_torque, from_flux, from_demux, from_gen, m_axis, logger_stream_in,
                                       logger_stream_out, control_mode, Vd_ps, Vq_ps);
}
