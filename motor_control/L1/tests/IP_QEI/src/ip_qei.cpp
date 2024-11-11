/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "qei.hpp"
#include "ip_qei.hpp"

typedef ap_uint<1> bit;

void hls_qei(hls::stream<bit>& strm_qei_A,
             hls::stream<bit>& strm_qei_B,
             hls::stream<bit>& strm_qei_I,
             hls::stream<ap_uint<32> >& strm_qei_RPM_THETA_m,
             hls::stream<ap_uint<256> >& logger,
             volatile int& qei_args_cpr,
             volatile int& qei_args_ctrl,
             volatile int& qei_stts_RPM_THETA_m,
             volatile int& qei_stts_dir,
             volatile int& qei_stts_err,
             volatile int& qei_args_flt_size,
             volatile int& qei_args_cnt_trip,
             volatile int& qei_debug_rpm,
             volatile int& qei_count_mode,
             volatile int& qei_args_flt_size_i) {
#pragma HLS INTERFACE ap_fifo port = strm_qei_A
#pragma HLS INTERFACE ap_fifo port = strm_qei_B
#pragma HLS INTERFACE ap_fifo port = strm_qei_I
#pragma HLS INTERFACE ap_fifo port = logger
#pragma HLS INTERFACE axis port = strm_qei_RPM_THETA_m
#pragma HLS INTERFACE s_axilite port = qei_args_cpr offset = 0x10 bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_args_ctrl offset = 0x20 bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_RPM_THETA_m offset = 0x28 bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_dir offset = 0x38 bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_err offset = 0x48 bundle = qei_args
#pragma HLS interface s_axilite port = qei_args_flt_size bundle = qei_args
#pragma HLS interface s_axilite port = qei_args_cnt_trip bundle = qei_args
#pragma HLS interface s_axilite port = qei_debug_rpm bundle = qei_args
#pragma HLS interface s_axilite port = qei_args_flt_size_i bundle = qei_args
#pragma HLS interface s_axilite port = qei_count_mode bundle = qei_args
#pragma HLS interface s_axilite port = return bundle = qei_args
    xf::motorcontrol::hls_qei_axi<bit>(strm_qei_A, strm_qei_B, strm_qei_I, strm_qei_RPM_THETA_m, logger, qei_args_cpr,
                                       qei_args_ctrl, qei_stts_RPM_THETA_m, qei_stts_dir, qei_stts_err,
                                       qei_args_flt_size, qei_args_cnt_trip, qei_debug_rpm, qei_count_mode,
                                       qei_args_flt_size_i);
}
