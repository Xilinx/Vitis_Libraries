/*
Copyright (C) 2022-2022, Xilinx, Inc.
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
#ifndef _IP_SVPWM_HPP_
#define _IP_SVPWM_HPP_
#include "ap_int.h"
#include <hls_stream.h>
#include "svpwm.hpp"
using namespace hls;
#define TESTNUMBER (10)

// clang-format off
/*
 * @brief This module is to calculate the duty cycles of the switching bridges
 * @param Va_cmd in<strm>: Every pwm_args_sample_ii<~86> cycles, one output of FOC can be consumed
 * @param Vb_cmd in<strm>: Every pwm_args_sample_ii<~86> cycles, one output of FOC can be consumed
 * @param Vc_cmd in<strm>: Every pwm_args_sample_ii<~86> cycles, one output of FOC can be consumed
 * @param strm_h_pwm out<strm>: array of duty cycles on upper switches
 * @param strm_l_pwm out<strm>: array of duty cycles on lower switches
 * @param strm_pwm_sync out<strm>: array sync signals to sample the ADCs
 * @param pwm_stt_pwm_cycle in<reg>: how many clock cycles are there in a single PWM period
 * @param pwm_args_pwm_freq in<reg>: pwm cycle, the value in test is 100,000 Hz
 * @param pwm_args_dead_cycles 	in<reg>: Dead cycle, the value in test is pwm_args_dead_cycles cycles  
 * @param pwm_args_phase_shift in<reg>: 0 (0 degree Phase Shift for output), 1 (120 degree Phase Shift for output)
 * @param pwm_args_dc_link_ref in<reg>: Q15Q16 representation for dc_link_ref format, Eg.  0x180000: 24.00000(q15q16)
 * @param pwm_stt_cnt_iter in<reg>: How many pwm command sent
 * @param pwm_stt_cnt_read_foc in<reg>: How many pwm command received from FOC
 * @param pwm_stt_cnt_read_dc in<reg>: How many DC_LINK value received from system
 * @param pwm_args_dc_src_mode in<reg>: 0 (PWM voltage reference based on ADC measured DC link), 1 (PWM voltage reference uses static register value)
 * @param pwm_args_sample_ii in<reg>: demands the data streams sampling rate
 * @param pwm_args_cnt_trip in<reg>: the trip count of pwm
 */
// clang-format on
//#define _SVPWM_USING_INT_
#ifndef _SVPWM_USING_INT_
typedef t_glb_foc2pwm t_svpwm_cmd;
typedef ap_ufixed<16, 0> t_svpwm_ratio;
#else
typedef int t_svpwm_cmd;
typedef ap_uint<16> t_svpwm_ratio;
#endif
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
                    volatile int& pwm_stt_Vc_cmd);

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
                 volatile int& pwm_stt_duty_ratio_c);

#endif
