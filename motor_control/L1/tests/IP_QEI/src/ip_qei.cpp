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

#include "ip_qei.hpp"

void hls_qei(hls::stream<t_bin_qei>& qei_A,
             hls::stream<t_bin_qei>& qei_B,
             hls::stream<t_bin_qei>& qei_I,
             hls::stream<ap_uint<32> >& qei_RPM_THETA_m,
             hls::stream<t_bin_qei>& qei_dir,
             hls::stream<t_err_qei>& qei_err,
             volatile int& qei_args_cpr,
             volatile int& qei_args_ctrl,
             volatile int& qei_stts_RPM_THETA_m,
             volatile int& qei_stts_dir,
             volatile int& qei_stts_err) {
#pragma HLS INTERFACE ap_fifo port = qei_A
#pragma HLS INTERFACE ap_fifo port = qei_B
#pragma HLS INTERFACE ap_fifo port = qei_I
#pragma HLS INTERFACE axis port = qei_RPM_THETA_m
#pragma HLS INTERFACE axis port = qei_dir
#pragma HLS INTERFACE axis port = qei_err
#pragma HLS INTERFACE s_axilite port = qei_args_cpr bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_args_ctrl bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_RPM_THETA_m bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_dir bundle = qei_args
#pragma HLS INTERFACE s_axilite port = qei_stts_err bundle = qei_args
#pragma HLS stable variable = qei_args_cpr
#pragma HLS stable variable = qei_args_ctrl
#pragma HLS interface s_axilite port = return bundle = qei_args

    long qei_args_cnt_trip = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    qei_args_cnt_trip = TESTNUMBER;
#endif
    xf::motorcontrol::hls_qei_axi<t_bin_qei, t_err_qei>(qei_A, qei_B, qei_I, qei_RPM_THETA_m, qei_dir, qei_err,
                                                        qei_args_cpr, qei_args_ctrl, qei_stts_RPM_THETA_m, qei_stts_dir,
                                                        qei_stts_err, qei_args_cnt_trip);
}
