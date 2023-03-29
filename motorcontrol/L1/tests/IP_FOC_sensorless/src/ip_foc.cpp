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

#include "ip_foc.hpp"

// clang-format off
void hls_foc_periodic_ap_fixed(
    // Input
    hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_foc2pwm>& Va_smo,
    hls::stream<t_glb_foc2pwm>& Vb_smo,
    hls::stream<t_glb_foc2pwm>& Vc_smo,
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
    volatile int& Ihomopolar_stts) {
    #pragma HLS interface axis port = Ia
    #pragma HLS interface axis port = Ib
    #pragma HLS interface axis port = Ic
    #pragma HLS interface axis port = SPEED_THETA_m
    #pragma HLS interface axis port = Va_cmd
    #pragma HLS interface axis port = Vb_cmd
    #pragma HLS interface axis port = Vc_cmd
    #pragma HLS interface axis port = Va_smo
    #pragma HLS interface axis port = Vb_smo
    #pragma HLS interface axis port = Vc_smo

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

    #pragma HLS interface s_axilite port = return bundle = foc_args
    long trip_cnt = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    trip_cnt = TESTNUMBER;
#endif
        xf::motorcontrol::hls_foc_strm_ap_fixed_sensorless<COMM_MACRO_CPR, t_glb_foc2pwm,  MAX_VAL_PWM, COMM_W, COMM_I, t_glb_speed_theta>(
            Ia, Ib, Ic, Va_smo, Vb_smo, Vc_smo, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args,
            control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args,
            torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args,
            vq_args, fw_kp_args, fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts,
            Va_cmd_stts, Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, trip_cnt);
}
// clang-format on

void hls_foc_periodic_int(
    // Input
    hls::stream<int>& Ia,
    hls::stream<int>& Ib,
    hls::stream<int>& Ic,
    hls::stream<int>& Va_smo,
    hls::stream<int>& Vb_smo,
    hls::stream<int>& Vc_smo,
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
    volatile int& Ihomopolar_stts) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = SPEED_THETA_m
#pragma HLS interface axis port = Va_cmd
#pragma HLS interface axis port = Vb_cmd
#pragma HLS interface axis port = Vc_cmd
#pragma HLS interface axis port = Va_smo
#pragma HLS interface axis port = Vb_smo
#pragma HLS interface axis port = Vc_smo

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

#pragma HLS interface s_axilite port = return bundle = foc_args

    long trip_cnt = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    trip_cnt = TESTNUMBER;
#endif
    xf::motorcontrol::hls_foc_strm_int_sensorless<COMM_MACRO_CPR, MAX_VAL_PWM, COMM_W, COMM_I, t_glb_speed_theta>(
        Ia, Ib, Ic, Va_smo, Vb_smo, Vc_smo, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args,
        control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args,
        torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args,
        vd_args, vq_args, fw_kp_args, fw_ki_args,
        //
        id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
        torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts,
        Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, trip_cnt);
}

// hls_foc_oneSample_top_ap_fixed is mainly used for generating testing files for cosim
void hls_foc_oneSample_ap_fixed(
    // Input
    hls::stream<t_glb_foc2pwm>& Ia,
    hls::stream<t_glb_foc2pwm>& Ib,
    hls::stream<t_glb_foc2pwm>& Ic,
    hls::stream<t_glb_foc2pwm>& Va_smo,
    hls::stream<t_glb_foc2pwm>& Vb_smo,
    hls::stream<t_glb_foc2pwm>& Vc_smo,
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
    volatile int& Ihomopolar_stts) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = SPEED_THETA_m
#pragma HLS interface axis port = Va_cmd
#pragma HLS interface axis port = Vb_cmd
#pragma HLS interface axis port = Vc_cmd
#pragma HLS interface axis port = Va_smo
#pragma HLS interface axis port = Vb_smo
#pragma HLS interface axis port = Vc_smo

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

#pragma HLS interface s_axilite port = return bundle = foc_args
    long trip_cnt = 1;
    xf::motorcontrol::hls_foc_strm_ap_fixed_sensorless<COMM_MACRO_CPR, t_glb_foc2pwm, MAX_VAL_PWM, PWM_AP_FIXED_PARA_W2,
                                                       PWM_AP_FIXED_PARA_I2, t_glb_speed_theta>(
        Ia, Ib, Ic, Va_smo, Vb_smo, Vc_smo, SPEED_THETA_m, Va_cmd, Vb_cmd, Vc_cmd, ppr_args, control_mode_args,
        control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args,
        torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args,
        vd_args, vq_args, fw_kp_args, fw_ki_args,
        //
        id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
        torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts,
        Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts, trip_cnt);
}
