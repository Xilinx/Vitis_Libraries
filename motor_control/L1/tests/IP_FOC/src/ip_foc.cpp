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

// clang-format off
void hls_foc_periodic_ap_fixed(
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
}
// clang-format on

void hls_foc_periodic_int( // used for testing synthesizability of hls_foc_strm_int
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
}
