/*
 * Copyright (C) 2024-2024, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * Except as contained in this notice, the name of Advanced Micro Devices
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization
 * from Advanced Micro Devices, Inc.
 * */
#include "voltage_modulation.hpp"

void voltage_modulation_inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                             hls::stream<ap_int<BIT_WIDTH_DATA> >& voltage_in,
                             hls::stream<ap_uint<96> >& output_s,
                             hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                             hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                             volatile unsigned int& mode,
                             volatile int& max_sym_interval,
                             volatile int& double_interval,
                             volatile int& scaling_interval_pwm,
                             volatile int& phase_a,
                             volatile int& phase_b,
                             volatile int& phase_c) {
#pragma HLS INTERFACE mode = axis port = s_axis
#pragma HLS INTERFACE mode = axis port = voltage_in
// #pragma HLS INTERFACE mode = axis port = Va_out
// #pragma HLS INTERFACE mode = axis port = Vb_out
// #pragma HLS INTERFACE mode = axis port = Vc_out
#pragma HLS INTERFACE mode = axis port = output_s
#pragma HLS INTERFACE mode = axis port = logger_stream_in
#pragma HLS INTERFACE mode = axis port = logger_stream_out

#pragma HLS INTERFACE mode = ap_none port = mode
#pragma HLS INTERFACE mode = ap_none port = max_sym_interval
#pragma HLS INTERFACE mode = ap_none port = double_interval
#pragma HLS INTERFACE mode = ap_none port = scaling_interval_pwm
#pragma HLS INTERFACE mode = ap_none port = phase_a
#pragma HLS INTERFACE mode = ap_none port = phase_b
#pragma HLS INTERFACE mode = ap_none port = phase_c

#pragma HLS INTERFACE mode = ap_ctrl_none port = return

    voltage_modulation<ap_uint<BIT_WIDTH_STREAM_FOC>, ap_uint<BIT_WIDTH_LOG_STREAM_FOC>, ap_int<BIT_WIDTH_DATA>,
                       ap_int<BIT_WIDTH_ACCUM>, ap_uint<PWM_DATA_TYPE_>, ap_uint<PWM_DATA_TYPE_3_PHASE> >(
        s_axis, voltage_in, /*Va_out, Vb_out, Vc_out,*/ output_s, logger_stream_in, logger_stream_out, mode,
        max_sym_interval, double_interval, scaling_interval_pwm, phase_a, phase_b, phase_c);
}
