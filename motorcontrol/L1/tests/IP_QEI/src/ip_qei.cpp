/*
 * Copyright 2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
