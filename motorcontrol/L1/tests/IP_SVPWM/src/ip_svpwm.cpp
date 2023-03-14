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

#include "ip_svpwm.hpp"

void hls_svpwm_duty(hls::stream<t_svpwm_cmd>& strm_Va_cmd,
                    hls::stream<t_svpwm_cmd>& strm_Vb_cmd,
                    hls::stream<t_svpwm_cmd>& strm_Vc_cmd,
                    hls::stream<t_svpwm_cmd>& strm_dc_link,
                    hls::stream<t_svpwm_ratio>& strm_duty_ratio_a,
                    hls::stream<t_svpwm_ratio>& strm_duty_ratio_b,
                    hls::stream<t_svpwm_ratio>& strm_duty_ratio_c,
                    volatile int& pwm_args_dc_link_ref,
                    volatile int& pwm_stt_cnt_iter,
                    volatile int& pwm_args_dc_src_mode,
                    volatile int& pwm_args_sample_ii,
                    volatile int& pwm_stt_Va_cmd,
                    volatile int& pwm_stt_Vb_cmd,
                    volatile int& pwm_stt_Vc_cmd) {
#pragma HLS interface axis port = strm_Va_cmd
#pragma HLS interface axis port = strm_Vb_cmd
#pragma HLS interface axis port = strm_Vc_cmd
#pragma HLS interface axis port = strm_dc_link
#pragma HLS interface axis port = strm_duty_ratio_a
#pragma HLS interface axis port = strm_duty_ratio_b
#pragma HLS interface axis port = strm_duty_ratio_c

#pragma HLS interface s_axilite port = pwm_args_dc_src_mode bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_dc_link_ref bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_sample_ii bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_cnt_iter bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_Va_cmd bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_Vb_cmd bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_Vc_cmd bundle = pwm_args
#pragma HLS interface s_axilite port = return bundle = pwm_args

// #pragma HLS interface ap_none port = pwm_args_dc_link_ref
// #pragma HLS interface ap_none port = pwm_args_dc_src_mode
// #pragma HLS interface ap_none port = pwm_args_sample_ii
// #pragma HLS interface ap_none port = pwm_stt_cnt_iter
// #pragma HLS interface ap_none port = pwm_stt_Va_cmd
// #pragma HLS interface ap_none port = pwm_stt_Vb_cmd
// #pragma HLS interface ap_none port = pwm_stt_Vc_cmd

#pragma HLS stable variable = pwm_args_dc_link_ref
#pragma HLS stable variable = pwm_args_dc_src_mode
#pragma HLS stable variable = pwm_args_sample_ii

    long pwm_args_cnt_trip = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    pwm_args_cnt_trip = TESTNUMBER;
#endif
    xf::motorcontrol::hls_svpwm_duty_axi<t_svpwm_cmd, t_svpwm_ratio>(
        strm_Va_cmd, strm_Vb_cmd, strm_Vc_cmd, strm_dc_link, strm_duty_ratio_a, strm_duty_ratio_b, strm_duty_ratio_c,
        pwm_args_dc_link_ref, pwm_stt_cnt_iter, pwm_args_dc_src_mode, pwm_args_sample_ii, pwm_args_cnt_trip,
        pwm_stt_Va_cmd, pwm_stt_Vb_cmd, pwm_stt_Vc_cmd);
}

void hls_pwm_gen(hls::stream<t_svpwm_ratio>& strm_duty_ratio_a,
                 hls::stream<t_svpwm_ratio>& strm_duty_ratio_b,
                 hls::stream<t_svpwm_ratio>& strm_duty_ratio_c,
                 hls::stream<ap_uint<1> >& strm_pwm_h_a,
                 hls::stream<ap_uint<1> >& strm_pwm_h_b,
                 hls::stream<ap_uint<1> >& strm_pwm_h_c,
                 hls::stream<ap_uint<1> >& strm_pwm_l_a,
                 hls::stream<ap_uint<1> >& strm_pwm_l_b,
                 hls::stream<ap_uint<1> >& strm_pwm_l_c,
                 hls::stream<ap_uint<1> >& strm_pwm_sync_a,
                 hls::stream<ap_uint<1> >& strm_pwm_sync_b,
                 hls::stream<ap_uint<1> >& strm_pwm_sync_c,
                 volatile int& pwm_args_pwm_freq,
                 volatile int& pwm_args_dead_cycles,
                 volatile int& pwm_args_phase_shift,
                 volatile int& pwm_stt_pwm_cycle,
                 volatile int& pwm_args_sample_ii,
                 volatile int& pwm_stt_duty_ratio_a,
                 volatile int& pwm_stt_duty_ratio_b,
                 volatile int& pwm_stt_duty_ratio_c) {
#pragma HLS interface axis port = strm_duty_ratio_a
#pragma HLS interface axis port = strm_duty_ratio_b
#pragma HLS interface axis port = strm_duty_ratio_c

#pragma HLS interface ap_fifo port = strm_pwm_h_a
#pragma HLS interface ap_fifo port = strm_pwm_l_a
#pragma HLS interface ap_fifo port = strm_pwm_sync_a
#pragma HLS interface ap_fifo port = strm_pwm_h_b
#pragma HLS interface ap_fifo port = strm_pwm_l_b
#pragma HLS interface ap_fifo port = strm_pwm_sync_b
#pragma HLS interface ap_fifo port = strm_pwm_h_c
#pragma HLS interface ap_fifo port = strm_pwm_l_c
#pragma HLS interface ap_fifo port = strm_pwm_sync_c

#pragma HLS interface s_axilite port = pwm_stt_pwm_cycle bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_pwm_freq bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_dead_cycles bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_phase_shift bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_args_sample_ii bundle = pwm_args

#pragma HLS interface s_axilite port = pwm_stt_duty_ratio_a bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_duty_ratio_b bundle = pwm_args
#pragma HLS interface s_axilite port = pwm_stt_duty_ratio_c bundle = pwm_args
#pragma HLS interface s_axilite port = return bundle = pwm_args

// #pragma HLS interface ap_none port = pwm_stt_pwm_cycle
// #pragma HLS interface ap_none port = pwm_args_pwm_freq
// #pragma HLS interface ap_none port = pwm_args_dead_cycles
// #pragma HLS interface ap_none port = pwm_args_phase_shift
// #pragma HLS interface ap_none port = pwm_args_sample_ii
// #pragma HLS interface ap_none port = pwm_stt_duty_ratio_a
// #pragma HLS interface ap_none port = pwm_stt_duty_ratio_b
// #pragma HLS interface ap_none port = pwm_stt_duty_ratio_c

#pragma HLS stable variable = pwm_args_pwm_freq
#pragma HLS stable variable = pwm_args_dead_cycles
#pragma HLS stable variable = pwm_args_phase_shift
#pragma HLS stable variable = pwm_args_sample_ii

    long pwm_args_cnt_trip = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    pwm_args_cnt_trip = TESTNUMBER;
#endif

    xf::motorcontrol::hls_pwm_gen_axi<t_svpwm_ratio>(
        strm_duty_ratio_a, strm_duty_ratio_b, strm_duty_ratio_c, /*strm_args,*/
        strm_pwm_h_a, strm_pwm_h_b, strm_pwm_h_c, strm_pwm_l_a, strm_pwm_l_b, strm_pwm_l_c, strm_pwm_sync_a,
        strm_pwm_sync_b, strm_pwm_sync_c, pwm_args_pwm_freq, pwm_args_dead_cycles, pwm_args_phase_shift,
        pwm_stt_pwm_cycle, pwm_args_cnt_trip, pwm_args_sample_ii, pwm_stt_duty_ratio_a, pwm_stt_duty_ratio_b,
        pwm_stt_duty_ratio_c);
}