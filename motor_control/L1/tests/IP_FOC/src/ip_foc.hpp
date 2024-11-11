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

#ifndef _IP_FOC_HPP_
#define _IP_FOC_HPP_

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>

// for regression test
#define TESTNUMBER (3000)

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
    volatile int& fixed_angle_args);
#endif // _IP_FOC_HPP_
