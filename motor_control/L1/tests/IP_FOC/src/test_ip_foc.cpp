/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2025; Advanced Micro Devices; Inc.
SPDX-License-Identifier: X11

Permission is hereby granted; free of charge; to any person obtaining a copy
of this software and associated documentation files (the "Software"); to deal
in the Software without restriction; including without limitation the rights
to use; copy; modify; merge; publish; distribute; sublicense; and/or sell
copies of the Software; and to permit persons to whom the Software is
furnished to do so; subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS"; WITHOUT WARRANTY OF ANY KIND; EXPRESS OR
IMPLIED; INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY;
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM; DAMAGES OR OTHER LIABILITY;
WHETHER IN AN ACTION OF CONTRACT; TORT OR OTHERWISE; ARISING FROM;
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice; the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale;
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices; Inc.
*/
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <ncurses.h>

#include <iostream>
#include <utility>
#include <vector>
#include <numeric>
#include <fstream>
#include <sstream>

#include "ip_foc.hpp"

int main(void) {
    int angle_shift_out = 0;
    int control_mode_ = 0;
    int sp_speed = 0;
    int kp_speed = 0;
    int ki_speed = 0;
    int kd_speed = 0;
    int sp_torque = 0;
    int kp_torque = 0;
    int ki_torque = 0;
    int kd_torque = 0;
    int sp_flux = 0;
    int kp_flux = 0;
    int ki_flux = 0;
    int kd_flux = 0;
    int vd_ps = 0;
    int vq_ps = 0;
    int period_theta_ps;
    int increment_angle_open_loop;
    int id_ps;
    int iq_ps;
    int theta_ps;
    int filt_a_est_ps;
    int filt_b_est_ps;
    int phase_a_ps;
    int phase_b_ps;
    int phase_c_ps;
    int volt_mode = 0;
    int max_sym_interval = 0;
    int double_sym_interval = 0;
    int pwm_duty = 0;
    // In-out for parameters
    int ppr_args = 2;
    int control_mode_args = 2; // Torque
    int control_fixperiod_args = 0;
    int flux_sp_args = 0;
    int flux_kp_args = 65536; // 1
    int flux_ki_args = 0;
    int flux_kd_args = 0;       // Not used
    int torque_sp_args = 32768; // 0.5
    int torque_kp_args = 65536; // 1
    int torque_ki_args = 655;   // 0.1
    int torque_kd_args = 0;     // Not used
    int speed_sp_args = 0;
    int speed_kp_args = 0;
    int speed_ki_args = 0;
    int speed_kd_args = 0;
    int angle_sh_args = 178; // angle offset in CPR
    int vd_args = 0;
    int vq_args = 0;
    int id_args = 0;
    int iq_args = 0;
    int theta_args = 0;
    int fw_kp_args = 0;
    int fw_ki_args = 0;
    int increment_angle_open_loop_args = 0;
    int filt_a_est_args = 0;
    int filt_b_est_args = 0;
    int phase_a_ps_args = 0;
    int phase_b_ps_args = 0;
    int phase_c_ps_args = 0;
    int volt_mode_args = 0;
    int max_sym_interval_args = (24 << 16);
    int double_sym_interval_args = (24 << 16) << 1; // double
    int pwm_duty_args = 0;
    int Va_cmd = 5;
    int Vb_cmd = 12;
    int Vc_cmd = 3;
    //
    int id_stts;
    int flux_acc_stts;
    int flux_err_stts;
    int flux_out_stts;
    int iq_stts;
    int torque_acc_stts;
    int torque_err_stts;
    int torque_out_stts;
    int speed_stts;
    int speed_acc_stts;
    int speed_err_stts;
    int speed_out_stts;
    int angle_stts;
    int Va_cmd_stts;
    int Vb_cmd_stts;
    int Vc_cmd_stts;
    int Ialpha_stts;
    int Ibeta_stts;
    int Ihomopolar_stts;
    int fixed_angle_args;

    hls_foc_periodic_ap_fixed(
        angle_shift_out, control_mode_, sp_speed, kp_speed, ki_speed, kd_speed, sp_torque, kp_torque, ki_torque,
        kd_torque, sp_flux, kp_flux, ki_flux, kd_flux, vd_ps, vq_ps, period_theta_ps, increment_angle_open_loop, id_ps,
        iq_ps, theta_ps, filt_a_est_ps, filt_b_est_ps, phase_a_ps, phase_b_ps, phase_c_ps, volt_mode, max_sym_interval,
        double_sym_interval, pwm_duty,
        // FROM PL
        Va_cmd, Vb_cmd, Vc_cmd,
        // In-out for parameters
        ppr_args, control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args,
        torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args,
        speed_kd_args, angle_sh_args, vd_args, vq_args, id_args, iq_args, theta_args, fw_kp_args, fw_ki_args,
        increment_angle_open_loop_args, filt_a_est_args, filt_b_est_args, phase_a_ps_args, phase_b_ps_args,
        phase_c_ps_args, volt_mode_args, max_sym_interval_args, double_sym_interval_args, pwm_duty_args,
        //
        id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
        torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts,
        Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts);

    return 0;
}
