/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

#include "ip_foc.hpp"
#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>

// clang-format off
void hls_foc_periodic_ap_fixed(
    // TO PL
    volatile int& angle_shift_out,
    volatile int& control_mode_,
    volatile int& sp_speed,
    volatile int& kp_speed,
    volatile int& ki_speed,
    volatile int& kd_speed,
    volatile int& sp_torque,
    volatile int& kp_torque,
    volatile int& ki_torque,
    volatile int& kd_torque,
    volatile int& sp_flux,
    volatile int& kp_flux,
    volatile int& ki_flux,
    volatile int& kd_flux,
    volatile int& vd_ps,
    volatile int& vq_ps,
    volatile int& period_theta_ps,
    volatile int& increment_angle_open_loop,
    volatile int& id_ps,
    volatile int& iq_ps,
    volatile int& theta_ps,
    volatile int& filt_a_est_ps,
    volatile int& filt_b_est_ps,
    volatile int& phase_a_ps,
    volatile int& phase_b_ps,
    volatile int& phase_c_ps,
    volatile int& volt_mode,
    volatile int& max_sym_interval,
    volatile int& double_sym_interval,
    volatile int& duty_cycle_pwm,
    // FROM PL
    volatile int& Va_cmd,
    volatile int& Vb_cmd,
    volatile int& Vc_cmd,
    // In-out for parameters
    volatile int& ppr_args,
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& id_args,
    volatile int& iq_args,
    volatile int& theta_args,
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    volatile int& increment_angle_open_loop_args,
    volatile int& filt_a_est_args,
    volatile int& filt_b_est_args,
    volatile int& phase_a_ps_args,
    volatile int& phase_b_ps_args,
    volatile int& phase_c_ps_args,
    volatile int& volt_mode_args,
    volatile int& max_sym_interval_args,
    volatile int& double_sym_interval_args,
    volatile int& duty_cycle_pwm_args,
    //
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Va_cmd_stts,
    volatile int& Vb_cmd_stts,
    volatile int& Vc_cmd_stts,
    volatile int& Ialpha_stts, 
    volatile int& Ibeta_stts, 
    volatile int& Ihomopolar_stts) {
    
    #pragma HLS interface ap_none port = angle_shift_out
    #pragma HLS interface ap_none port = control_mode_
    #pragma HLS interface ap_none port = sp_speed
    #pragma HLS interface ap_none port = kp_speed
    #pragma HLS interface ap_none port = ki_speed
    #pragma HLS interface ap_none port = kd_speed
    #pragma HLS interface ap_none port = sp_torque
    #pragma HLS interface ap_none port = kp_torque
    #pragma HLS interface ap_none port = ki_torque
    #pragma HLS interface ap_none port = kd_torque
    #pragma HLS interface ap_none port = sp_flux
    #pragma HLS interface ap_none port = kp_flux
    #pragma HLS interface ap_none port = ki_flux
    #pragma HLS interface ap_none port = kd_flux
    #pragma HLS interface ap_none port = vq_ps
    #pragma HLS interface ap_none port = vd_ps
    #pragma HLS interface ap_none port = period_theta_ps
    #pragma HLS interface ap_none port = increment_angle_open_loop
    #pragma HLS interface ap_none port = iq_ps
    #pragma HLS interface ap_none port = id_ps
    #pragma HLS interface ap_none port = theta_ps
    #pragma HLS interface ap_none port = filt_a_est_ps
    #pragma HLS interface ap_none port = filt_b_est_ps
    #pragma HLS interface ap_none port = phase_a_ps
    #pragma HLS interface ap_none port = phase_b_ps
    #pragma HLS interface ap_none port = phase_c_ps
    #pragma HLS interface ap_none port = volt_mode
    #pragma HLS interface ap_none port = max_sym_interval
    #pragma HLS interface ap_none port = double_sym_interval
    #pragma HLS interface ap_none port = duty_cycle_pwm
    #pragma HLS interface ap_none port = Va_cmd
    #pragma HLS interface ap_none port = Vb_cmd
    #pragma HLS interface ap_none port = Vc_cmd

    #pragma HLS interface s_axilite port = ppr_args bundle = foc_args offset=0x010
    #pragma HLS interface s_axilite port = control_mode_args bundle = foc_args offset=0x018
    #pragma HLS interface s_axilite port = control_fixperiod_args bundle = foc_args offset=0x020
    #pragma HLS interface s_axilite port = flux_sp_args bundle = foc_args offset=0x028
    #pragma HLS interface s_axilite port = flux_kp_args bundle = foc_args offset=0x030
    #pragma HLS interface s_axilite port = flux_ki_args bundle = foc_args offset=0x038
    #pragma HLS interface s_axilite port = flux_kd_args bundle = foc_args offset=0x040
    #pragma HLS interface s_axilite port = torque_sp_args bundle = foc_args offset=0x048
    #pragma HLS interface s_axilite port = torque_kp_args bundle = foc_args offset=0x050
    #pragma HLS interface s_axilite port = torque_ki_args bundle = foc_args offset=0x058
    #pragma HLS interface s_axilite port = torque_kd_args bundle = foc_args offset=0x060
    #pragma HLS interface s_axilite port = speed_sp_args bundle = foc_args offset=0x068
    #pragma HLS interface s_axilite port = speed_kp_args bundle = foc_args offset=0x070
    #pragma HLS interface s_axilite port = speed_ki_args bundle = foc_args offset=0x078
    #pragma HLS interface s_axilite port = speed_kd_args bundle = foc_args offset=0x080
    #pragma HLS interface s_axilite port = angle_sh_args bundle = foc_args offset=0x088
    #pragma HLS interface s_axilite port = vd_args bundle = foc_args offset=0x090
    #pragma HLS interface s_axilite port = vq_args bundle = foc_args offset=0x098
    #pragma HLS interface s_axilite port = fw_kp_args bundle = foc_args offset=0x0A0
    #pragma HLS interface s_axilite port = fw_ki_args bundle = foc_args offset=0x0A8
    #pragma HLS interface s_axilite port = id_stts bundle = foc_args offset=0x0B0
    #pragma HLS interface s_axilite port = flux_acc_stts bundle = foc_args offset=0x0C0
    #pragma HLS interface s_axilite port = flux_err_stts bundle = foc_args offset=0x0D0
    #pragma HLS interface s_axilite port = flux_out_stts bundle = foc_args offset=0x0E0
    #pragma HLS interface s_axilite port = iq_stts bundle = foc_args offset=0x0F0
    #pragma HLS interface s_axilite port = torque_acc_stts bundle = foc_args offset=0x100
    #pragma HLS interface s_axilite port = torque_err_stts bundle = foc_args offset=0x110
    #pragma HLS interface s_axilite port = torque_out_stts bundle = foc_args offset=0x120
    #pragma HLS interface s_axilite port = speed_stts bundle = foc_args offset=0x130
    #pragma HLS interface s_axilite port = speed_acc_stts bundle = foc_args offset=0x140
    #pragma HLS interface s_axilite port = speed_err_stts bundle = foc_args offset=0x150
    #pragma HLS interface s_axilite port = speed_out_stts bundle = foc_args offset=0x160
    #pragma HLS interface s_axilite port = angle_stts bundle = foc_args offset=0x170
    #pragma HLS interface s_axilite port = Va_cmd_stts bundle = foc_args offset=0x180
    #pragma HLS interface s_axilite port = Vb_cmd_stts bundle = foc_args offset=0x190
    #pragma HLS interface s_axilite port = Vc_cmd_stts bundle = foc_args offset=0x1A0
    #pragma HLS interface s_axilite port = Ialpha_stts     bundle = foc_args offset=0x1B0
    #pragma HLS interface s_axilite port = Ibeta_stts      bundle = foc_args offset=0x1C0
    #pragma HLS interface s_axilite port = Ihomopolar_stts bundle = foc_args offset=0x1D0
    // #pragma HLS interface s_axilite port = fixed_angle_args bundle = foc_args offset=0x1E0
    
    #pragma HLS interface s_axilite port = increment_angle_open_loop_args bundle = foc_args offset=0x300
    #pragma HLS interface s_axilite port = id_args bundle = foc_args offset=0x308
    #pragma HLS interface s_axilite port = iq_args bundle = foc_args offset=0x310
    #pragma HLS interface s_axilite port = theta_args bundle = foc_args offset=0x318
    #pragma HLS interface s_axilite port = filt_a_est_args bundle = foc_args offset=0x320
    #pragma HLS interface s_axilite port = filt_b_est_args bundle = foc_args offset=0x328
    #pragma HLS interface s_axilite port = phase_a_ps_args bundle = foc_args offset=0x330
    #pragma HLS interface s_axilite port = phase_b_ps_args bundle = foc_args offset=0x338
    #pragma HLS interface s_axilite port = phase_c_ps_args bundle = foc_args offset=0x340
    #pragma HLS interface s_axilite port = volt_mode_args bundle = foc_args offset=0x348
    #pragma HLS interface s_axilite port = max_sym_interval_args bundle = foc_args offset=0x350
    #pragma HLS interface s_axilite port = double_sym_interval_args bundle = foc_args offset=0x358
    #pragma HLS interface s_axilite port = duty_cycle_pwm_args bundle = foc_args offset=0x360
    #pragma HLS interface s_axilite port = return bundle = foc_args
    
    long trip_cnt = 0x7fffffffffffffffL;
	ap_uint< 512 > logger_var;
	ap_uint< 128 > packet_out;
	hls::stream< ap_uint< 512 >, 16 > logger_stream;
	hls::stream< ap_uint< 128 >, 16 > output_stream_filter;
	hls::stream< ap_uint< 128 >, 16 > output_clarke_direct, output_park_direct, from_muxer, output_park_inverse, output_clarke_inverse;
	hls::stream< int32_t > to_PI_speed, to_PI_torque, to_PI_flux, to_muxer;
	hls::stream< int32_t > from_PI_speed, from_PI_torque, from_PI_flux;
	ap_uint< 32 > angle_vel = 0;
	
	while(1){
		 // TO PL
                angle_shift_out = angle_sh_args;
                control_mode_ = control_mode_args;
                sp_speed = speed_sp_args;
                kp_speed = speed_kp_args;
                ki_speed = speed_ki_args;
                kd_speed = speed_kd_args;
                sp_torque = torque_sp_args;
                kp_torque = torque_kp_args;
                ki_torque = torque_ki_args;
                kd_torque = torque_kd_args;
                sp_flux = flux_sp_args;
                kp_flux = flux_kp_args;
                ki_flux = flux_ki_args;
                kd_flux = flux_kd_args;
                vd_ps = vd_args;
                vq_ps = vq_args;
                period_theta_ps = control_fixperiod_args;
                increment_angle_open_loop = increment_angle_open_loop_args;
                id_ps = id_args;
                iq_ps = iq_args;
                theta_ps = theta_args;
                filt_a_est_ps = filt_a_est_args;
                filt_b_est_ps = filt_b_est_args;
                phase_a_ps = phase_a_ps_args;
                phase_b_ps = phase_b_ps_args;
                phase_c_ps = phase_c_ps_args;
                volt_mode = volt_mode_args;
                max_sym_interval = max_sym_interval_args;
                double_sym_interval = double_sym_interval_args;
                duty_cycle_pwm = duty_cycle_pwm_args;
                
                // FROM PL
                Va_cmd_stts = Va_cmd;
                Vb_cmd_stts = Vb_cmd;
                Vc_cmd_stts = Vc_cmd;
                
#ifndef __SYNTHESIS__
		#include <iostream>	
		std::cout << "PARAMATERS RECEIVED FROM PS" << std::endl << \
		"ANGLE_SHIFT: " << angle_shift_out << std::endl << \
		"CONTROL_MODE: " << control_mode_ << std::endl << \
		"SP SPEED: " << sp_speed << std::endl << \
		"KP SPEED: " << kp_speed << std::endl << \
		"KI SPEED: " << ki_speed << std::endl << \
		"KD SPEED: " << kd_speed << std::endl << \
		"SP_TORQUE: " << sp_torque << std::endl << \
		"KP_TORQUE: " << kp_torque << std::endl << \
		"KI_TORQUE: " << ki_torque << std::endl << \
		"KD_TORQUE: " << kd_torque << std::endl << \
		"SP_FLUX: " << sp_flux << std::endl << \
		"KP_FLUX: " << kp_flux << std::endl << \
		"KI_FLUX: " << ki_flux << std::endl << \
		"KD_FLUX: " << kd_flux << std::endl  << \
		"VD OPEN LOOP: " << vd_ps << std::endl << \
		"VQ OPEN LOOP: " << vq_ps << std::endl << \
		"FIXPERIOD OPEN LOOP: " << period_theta_ps << std::endl << \
		"INCREMENT ANGLE OPEN LOOP: " << increment_angle_open_loop << std::endl << \
		"ID OPEN LOOP: " << id_ps << std::endl << \
		"IQ OPEN LOOP: " << iq_ps << std::endl << \
		"THETA FROM PS: " << theta_ps << std::endl << \
		"A COEFF FILTER: " << filt_a_est_ps << std::endl << \
		"B COEFF FILTER: " << filt_b_est_ps << std::endl << \
		"PHASE A PWM SET: " << phase_a_ps << std::endl << \
		"PHASE B PWM SET: " << phase_b_ps << std::endl << \
		"PHASE C PWM SET: " << phase_c_ps << std::endl \
		<< std::endl;
		break;
#endif 
    }
        
}

// clang-format on
