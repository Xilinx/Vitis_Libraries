/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#ifndef _IP_FOC_MULTIPLE_HPP_
#define _IP_FOC_MULTIPLE_HPP_

#include "foc.hpp"

#ifndef IP_FOC_NUM_MOTORS
#define IP_FOC_NUM_MOTORS 1
#endif

#ifdef IP_FOC_MODEL_SIM
#define TESTNUMBER (3000)
#else
#define TESTNUMBER (256)
#endif

void hls_foc_periodic_ap_fixed_multiple(hls::stream<t_glb_foc2pwm>& Ia,
                                        hls::stream<t_glb_foc2pwm>& Ib,
                                        hls::stream<t_glb_foc2pwm>& Ic,
                                        hls::stream<t_glb_speed_theta>& SPEED_THETA_m,
                                        hls::stream<int>& FOC_MOTOR_ID,
                                        hls::stream<t_glb_foc2pwm>& Va_cmd,
                                        hls::stream<t_glb_foc2pwm>& Vb_cmd,
                                        hls::stream<t_glb_foc2pwm>& Vc_cmd,
                                        hls::stream<int>& FOC_MOTOR_ID_out,
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

void hls_foc_oneSample_ap_fixed_multiple(hls::stream<t_glb_foc2pwm>& Ia,
                                         hls::stream<t_glb_foc2pwm>& Ib,
                                         hls::stream<t_glb_foc2pwm>& Ic,
                                         hls::stream<t_glb_speed_theta>& SPEED_THETA_m,
                                         hls::stream<int>& FOC_MOTOR_ID,
                                         hls::stream<t_glb_foc2pwm>& Va_cmd,
                                         hls::stream<t_glb_foc2pwm>& Vb_cmd,
                                         hls::stream<t_glb_foc2pwm>& Vc_cmd,
                                         hls::stream<int>& FOC_MOTOR_ID_out,
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

#endif // _IP_FOC_MULTIPLE_HPP_
