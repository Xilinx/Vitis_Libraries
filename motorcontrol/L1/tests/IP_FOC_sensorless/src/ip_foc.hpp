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

#ifndef _IP_FOC_HPP_
#define _IP_FOC_HPP_

// not infer BRAM v1
//#include "sin_cos_table.h"

// not infer BRAM v2
// static short sin_table[1000];
// static short cos_table[1000];

// clang-format on

// infer BRAM v3

#include "foc_sensorless.hpp"

// for regression test
#define TESTNUMBER (300000)

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
    volatile int& Ihomopolar_stts);
// clang-format on

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
    volatile int& Ihomopolar_stts);

#endif // _IP_FOC_HPP_
