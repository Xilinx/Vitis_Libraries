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

#include "ip_foc.hpp"
#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>

// clang-format off
/*void hls_foc_periodic_ap_fixed(
    // Input
    hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_speed_theta>& SPEED_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<t_glb_foc2pwm>& Va_cmd,
    hls::stream<t_glb_foc2pwm>& Vb_cmd,
    hls::stream<t_glb_foc2pwm>& Vc_cmd,
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
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
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
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args) {
    #pragma HLS interface axis port = Ia
    #pragma HLS interface axis port = Ib
    #pragma HLS interface axis port = Ic
    #pragma HLS interface axis port = SPEED_THETA_m
    #pragma HLS interface axis port = Va_cmd
    #pragma HLS interface axis port = Vb_cmd
    #pragma HLS interface axis port = Vc_cmd

    #pragma HLS interface s_axilite port = ppr_args bundle = foc_args
    #pragma HLS interface s_axilite port = control_mode_args bundle = foc_args
    #pragma HLS interface s_axilite port = control_fixperiod_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = angle_sh_args bundle = foc_args
    #pragma HLS interface s_axilite port = vd_args bundle = foc_args
    #pragma HLS interface s_axilite port = vq_args bundle = foc_args
    #pragma HLS interface s_axilite port = fw_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = fw_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = id_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = iq_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = angle_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Va_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Vb_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Vc_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Ialpha_stts     bundle = foc_args
    #pragma HLS interface s_axilite port = Ibeta_stts      bundle = foc_args
    #pragma HLS interface s_axilite port = Ihomopolar_stts bundle = foc_args
    #pragma HLS interface s_axilite port = fixed_angle_args bundle = foc_args

    #pragma HLS interface s_axilite port = return bundle = foc_args
    long trip_cnt = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    trip_cnt = TESTNUMBER;
#endif
        xf::motorcontrol::hls_foc_strm_ap_fixed<COMM_MACRO_CPR, t_glb_foc2pwm,  MAX_VAL_PWM, COMM_W, COMM_I, t_glb_speed_theta>(
            Ia, Ib, Ic, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args,
            control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args,
            torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args,
            vq_args, fw_kp_args, fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts,
            Va_cmd_stts, Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, fixed_angle_args, trip_cnt);
}*/

void hls_foc_periodic_ap_fixed(
    // Input
    /*hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_speed_theta>& SPEED_THETA_m, // RPM & Theta_m
    // Output
    hls::stream< ap_uint< 512 > >& logger,
    hls::stream<t_glb_foc2pwm>& Va_cmd,
    hls::stream<t_glb_foc2pwm>& Vb_cmd,
    hls::stream<t_glb_foc2pwm>& Vc_cmd,*/
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
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args) {
    
    //#pragma HLS interface axis port = Ia
    //#pragma HLS interface axis port = Ib
    //#pragma HLS interface axis port = Ic
    //#pragma HLS interface axis port = SPEED_THETA_m
    //#pragma HLS interface ap_fifo port = logger
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
    //#pragma HLS interface axis port = Va_cmd
    //#pragma HLS interface axis port = Vb_cmd
    //#pragma HLS interface axis port = Vc_cmd

    #pragma HLS interface s_axilite port = ppr_args bundle = foc_args
    #pragma HLS interface s_axilite port = control_mode_args bundle = foc_args
    #pragma HLS interface s_axilite port = control_fixperiod_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = flux_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = torque_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_sp_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = speed_kd_args bundle = foc_args
    #pragma HLS interface s_axilite port = angle_sh_args bundle = foc_args
    #pragma HLS interface s_axilite port = vd_args bundle = foc_args
    #pragma HLS interface s_axilite port = vq_args bundle = foc_args
    #pragma HLS interface s_axilite port = increment_angle_open_loop_args bundle = foc_args
    #pragma HLS interface s_axilite port = id_args bundle = foc_args
    #pragma HLS interface s_axilite port = iq_args bundle = foc_args
    #pragma HLS interface s_axilite port = theta_args bundle = foc_args
    #pragma HLS interface s_axilite port = filt_a_est_args bundle = foc_args
    #pragma HLS interface s_axilite port = filt_b_est_args bundle = foc_args
    #pragma HLS interface s_axilite port = phase_a_ps_args bundle = foc_args
    #pragma HLS interface s_axilite port = phase_b_ps_args bundle = foc_args
    #pragma HLS interface s_axilite port = phase_c_ps_args bundle = foc_args
    #pragma HLS interface s_axilite port = fw_kp_args bundle = foc_args
    #pragma HLS interface s_axilite port = fw_ki_args bundle = foc_args
    #pragma HLS interface s_axilite port = id_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = flux_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = iq_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = torque_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_acc_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_err_stts bundle = foc_args
    #pragma HLS interface s_axilite port = speed_out_stts bundle = foc_args
    #pragma HLS interface s_axilite port = angle_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Va_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Vb_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Vc_cmd_stts bundle = foc_args
    #pragma HLS interface s_axilite port = Ialpha_stts     bundle = foc_args
    #pragma HLS interface s_axilite port = Ibeta_stts      bundle = foc_args
    #pragma HLS interface s_axilite port = Ihomopolar_stts bundle = foc_args
    #pragma HLS interface s_axilite port = fixed_angle_args bundle = foc_args

    #pragma HLS interface s_axilite port = return bundle = foc_args
    long trip_cnt = 0x7fffffffffffffffL;
//#ifdef SIM_FINITE
//    trip_cnt = TESTNUMBER;
//#endif
        // VARIABLES
	ap_uint< 512 > logger_var;
	ap_uint< 128 > packet_out;
	hls::stream< ap_uint< 512 >, 16 > logger_stream;
	hls::stream< ap_uint< 128 >, 16 > output_stream_filter;
	hls::stream< ap_uint< 128 >, 16 > output_clarke_direct, output_park_direct, from_muxer, output_park_inverse, output_clarke_inverse;
	hls::stream< int32_t > to_PI_speed, to_PI_torque, to_PI_flux, to_muxer;
	hls::stream< int32_t > from_PI_speed, from_PI_torque, from_PI_flux;
	ap_uint< 32 > angle_vel = 0;
	
	// t_glb_foc2pwm Ia_, Ib_, Ic_;
	
	while(1){
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
                
		// FOC
		// FILTERING INPUT
		//Filters(Ia, Ib, Ic, SPEED_THETA_m, output_stream_filter, logger_var, angle_sh_args);
		//CLARKE DIRECT
		//Clarke_Direct(output_stream_filter, output_clarke_direct, logger_var);
		//PARK DIRECT
		/*Park_Direct(output_clarke_direct, output_park_direct, logger_var);
		//DEMUX
		demuxer_pi(output_park_direct, logger_var, to_PI_torque, to_PI_flux, to_PI_speed, to_muxer);
		//PI CONTROL
		PI_Control(to_PI_torque, from_PI_torque, torque_sp_args, torque_kp_args, torque_ki_args, control_mode_args);
		//PI CONTROL
		PI_Control(to_PI_flux, from_PI_flux, flux_sp_args, flux_kp_args, flux_ki_args, control_mode_args);
		//MUXER
		muxer_pi(from_PI_torque, from_PI_flux, to_muxer, from_muxer, logger_var);
		//PARK INVERSE
		Park_Inverse(from_muxer, output_park_inverse, logger_var);
		//CLARKE INVERSE
		Clarke_Inverse(output_park_inverse, output_clarke_inverse, logger_var); */
		
		//OUTPUT
		//packet_out = output_clarke_direct.read(); //sink for module	
		//logger
		//logger_var = logger_stream.read();
		//logger.write_nb(logger_var);
		//temp out
		//Va_cmd.write(0);
		//Vb_cmd.write(0);
		//Vc_cmd.write(0);
		/*Va_cmd.write(packet_out.range(95, 64));
		Vb_cmd.write(packet_out.range(63, 32));
		Vc_cmd.write(packet_out.range(31, 0));*/
	}

        //logger
        //packet_out = output_stream_filter.read(); //fake sink for module
        //logger.write_nb(logger_var);
        
}

// clang-format on

/*void hls_foc_periodic_int( // used for testing synthesizability of hls_foc_strm_int
    // Input
    hls::stream<int>& Ia,
    hls::stream<int>& Ib,
    hls::stream<int>& Ic,
    hls::stream<t_glb_speed_theta>& SPEED_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<int>& Va_cmd,
    hls::stream<int>& Vb_cmd,
    hls::stream<int>& Vc_cmd,
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
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
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
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = SPEED_THETA_m
#pragma HLS interface axis port = Va_cmd
#pragma HLS interface axis port = Vb_cmd
#pragma HLS interface axis port = Vc_cmd

#pragma HLS interface s_axilite port = ppr_args bundle = foc_args
#pragma HLS interface s_axilite port = control_mode_args bundle = foc_args
#pragma HLS interface s_axilite port = control_fixperiod_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = angle_sh_args bundle = foc_args
#pragma HLS interface s_axilite port = vd_args bundle = foc_args
#pragma HLS interface s_axilite port = vq_args bundle = foc_args
#pragma HLS interface s_axilite port = fw_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = fw_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = id_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = iq_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = angle_stts bundle = foc_args
#pragma HLS interface s_axilite port = Va_cmd_stts bundle = foc_args
#pragma HLS interface s_axilite port = Vb_cmd_stts bundle = foc_args
#pragma HLS interface s_axilite port = Vc_cmd_stts bundle = foc_args

#pragma HLS interface s_axilite port = Ialpha_stts bundle = foc_args
#pragma HLS interface s_axilite port = Ibeta_stts bundle = foc_args
#pragma HLS interface s_axilite port = Ihomopolar_stts bundle = foc_args
#pragma HLS interface s_axilite port = fixed_angle_args bundle = foc_args

#pragma HLS interface s_axilite port = return bundle = foc_args

    long trip_cnt = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    trip_cnt = TESTNUMBER;
#endif
    xf::motorcontrol::hls_foc_strm_int<COMM_MACRO_CPR, MAX_VAL_PWM, COMM_W, COMM_I, t_glb_speed_theta>(
        Ia, Ib, Ic, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args, control_fixperiod_args,
        flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args,
        torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args,
        fw_kp_args, fw_ki_args,
        //
        id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
        torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts,
        Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, fixed_angle_args, trip_cnt);
}

// hls_foc_oneSample_ap_fixed is mainly used for generating testing files for cosim
void hls_foc_oneSample_ap_fixed(
    // Input
    hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_speed_theta>& SPEED_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<t_glb_foc2pwm>& Va_cmd,
    hls::stream<t_glb_foc2pwm>& Vb_cmd,
    hls::stream<t_glb_foc2pwm>& Vc_cmd,
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
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
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
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = SPEED_THETA_m
#pragma HLS interface axis port = Va_cmd
#pragma HLS interface axis port = Vb_cmd
#pragma HLS interface axis port = Vc_cmd

#pragma HLS interface s_axilite port = ppr_args bundle = foc_args
#pragma HLS interface s_axilite port = control_mode_args bundle = foc_args
#pragma HLS interface s_axilite port = control_fixperiod_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = flux_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = torque_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_sp_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = speed_kd_args bundle = foc_args
#pragma HLS interface s_axilite port = angle_sh_args bundle = foc_args
#pragma HLS interface s_axilite port = vd_args bundle = foc_args
#pragma HLS interface s_axilite port = vq_args bundle = foc_args
#pragma HLS interface s_axilite port = fw_kp_args bundle = foc_args
#pragma HLS interface s_axilite port = fw_ki_args bundle = foc_args
#pragma HLS interface s_axilite port = id_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = flux_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = iq_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = torque_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_acc_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_err_stts bundle = foc_args
#pragma HLS interface s_axilite port = speed_out_stts bundle = foc_args
#pragma HLS interface s_axilite port = angle_stts bundle = foc_args
#pragma HLS interface s_axilite port = Va_cmd_stts bundle = foc_args
#pragma HLS interface s_axilite port = Vb_cmd_stts bundle = foc_args
#pragma HLS interface s_axilite port = Vc_cmd_stts bundle = foc_args
#pragma HLS interface s_axilite port = Ialpha_stts bundle = foc_args
#pragma HLS interface s_axilite port = Ibeta_stts bundle = foc_args
#pragma HLS interface s_axilite port = Ihomopolar_stts bundle = foc_args
#pragma HLS interface s_axilite port = fixed_angle_args bundle = foc_args

// #pragma HLS interface ap_none port = ppr_args
// #pragma HLS interface ap_none port = control_mode_args
// #pragma HLS interface ap_none port = control_fixperiod_args
// #pragma HLS interface ap_none port = flux_sp_args
// #pragma HLS interface ap_none port = flux_kp_args
// #pragma HLS interface ap_none port = flux_ki_args
// #pragma HLS interface ap_none port = flux_kd_args
// #pragma HLS interface ap_none port = torque_sp_args
// #pragma HLS interface ap_none port = torque_kp_args
// #pragma HLS interface ap_none port = torque_ki_args
// #pragma HLS interface ap_none port = torque_kd_args
// #pragma HLS interface ap_none port = speed_sp_args
// #pragma HLS interface ap_none port = speed_kp_args
// #pragma HLS interface ap_none port = speed_ki_args
// #pragma HLS interface ap_none port = speed_kd_args
// #pragma HLS interface ap_none port = angle_sh_args
// #pragma HLS interface ap_none port = vd_args
// #pragma HLS interface ap_none port = vq_args
// #pragma HLS interface ap_none port = fw_kp_args
// #pragma HLS interface ap_none port = fw_ki_args
// #pragma HLS interface ap_none port = id_stts
// #pragma HLS interface ap_none port = flux_acc_stts
// #pragma HLS interface ap_none port = flux_err_stts
// #pragma HLS interface ap_none port = flux_out_stts
// #pragma HLS interface ap_none port = iq_stts
// #pragma HLS interface ap_none port = torque_acc_stts
// #pragma HLS interface ap_none port = torque_err_stts
// #pragma HLS interface ap_none port = torque_out_stts
// #pragma HLS interface ap_none port = speed_stts
// #pragma HLS interface ap_none port = speed_acc_stts
// #pragma HLS interface ap_none port = speed_err_stts
// #pragma HLS interface ap_none port = speed_out_stts
// #pragma HLS interface ap_none port = angle_stts
// #pragma HLS interface ap_none port = Va_cmd_stts
// #pragma HLS interface ap_none port = Vb_cmd_stts
// #pragma HLS interface ap_none port = Vc_cmd_stts
// #pragma HLS interface ap_none port = Ialpha_stts
// #pragma HLS interface ap_none port = Ibeta_stts
// #pragma HLS interface ap_none port = Ihomopolar_stts

#pragma HLS interface s_axilite port = return bundle = foc_args
    long trip_cnt = 1;
    xf::motorcontrol::hls_foc_strm_ap_fixed<COMM_MACRO_CPR, t_glb_foc2pwm, MAX_VAL_PWM, PWM_AP_FIXED_PARA_W2,
                                            PWM_AP_FIXED_PARA_I2, t_glb_speed_theta>(
        Ia, Ib, Ic, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args, control_fixperiod_args,
        flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args,
        torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args,
        fw_kp_args, fw_ki_args,
        //
        id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
        torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts,
        Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, fixed_angle_args, trip_cnt);
}*/
