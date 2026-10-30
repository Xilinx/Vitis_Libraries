/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include "../../../../include/models_fp/foc_fp32_sync.hpp"

#ifdef FOC_FP32_MODEL_SIM
#define FOC_FP32_TESTNUMBER 3000
#else
#define FOC_FP32_TESTNUMBER 256
#endif
#ifndef FOC_FP32_NUM_MOTORS
#define FOC_FP32_NUM_MOTORS 1
#endif

static void call_foc_fp32_strm(::hls::stream<float>& Ia,
                               ::hls::stream<float>& Ib,
                               ::hls::stream<float>& Ic,
                               ::hls::stream<int>& FOC_RPM_THETA_m,
                               ::hls::stream<int>& FOC_MOTOR_ID,
                               ::hls::stream<float>& Va_cmd,
                               ::hls::stream<float>& Vb_cmd,
                               ::hls::stream<float>& Vc_cmd,
                               ::hls::stream<int>& FOC_MOTOR_ID_out,
                               volatile int& ppr_args,
                               volatile int& control_mode_args,
                               volatile int& control_fixperiod_args,
                               volatile float& flux_sp_args,
                               volatile float& flux_kp_args,
                               volatile float& flux_ki_args,
                               volatile float& flux_kd_args,
                               volatile float& torque_sp_args,
                               volatile float& torque_kp_args,
                               volatile float& torque_ki_args,
                               volatile float& torque_kd_args,
                               volatile float& speed_sp_args,
                               volatile float& speed_kp_args,
                               volatile float& speed_ki_args,
                               volatile float& speed_kd_args,
                               volatile int& angle_sh_args,
                               volatile float& vd_args,
                               volatile float& vq_args,
                               volatile float& fw_kp_args,
                               volatile float& fw_ki_args,
                               volatile float& id_stts,
                               volatile float& flux_acc_stts,
                               volatile float& flux_err_stts,
                               volatile float& flux_out_stts,
                               volatile float& iq_stts,
                               volatile float& torque_acc_stts,
                               volatile float& torque_err_stts,
                               volatile float& torque_out_stts,
                               volatile float& speed_stts,
                               volatile float& speed_acc_stts,
                               volatile float& speed_err_stts,
                               volatile float& speed_out_stts,
                               volatile int& angle_stts,
                               volatile float& Va_cmd_stts,
                               volatile float& Vb_cmd_stts,
                               volatile float& Vc_cmd_stts,
                               volatile float& Ialpha_stts,
                               volatile float& Ibeta_stts,
                               volatile float& Ihomopolar_stts,
                               volatile int& fixed_angle_args,
                               volatile long& trip_cnt,
                               xf::motorcontrol::golden::FOCContext<float> ctx[FOC_FP32_NUM_MOTORS]) {
    xf::motorcontrol::hls::hls_foc_strm_fp32<1000, FOC_FP32_NUM_MOTORS, float, int>(
        Ia, Ib, Ic, FOC_RPM_THETA_m, FOC_MOTOR_ID, Va_cmd, Vb_cmd, Vc_cmd, FOC_MOTOR_ID_out, ppr_args,
        control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args,
        torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args,
        speed_kd_args, angle_sh_args, vd_args, vq_args, fw_kp_args, fw_ki_args, id_stts, flux_acc_stts, flux_err_stts,
        flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts, torque_out_stts, speed_stts, speed_acc_stts,
        speed_err_stts, speed_out_stts, angle_stts, Va_cmd_stts, Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts,
        Ihomopolar_stts, fixed_angle_args, trip_cnt, ctx);
}

void hls_foc_periodic_fp32(::hls::stream<float>& Ia,
                           ::hls::stream<float>& Ib,
                           ::hls::stream<float>& Ic,
                           ::hls::stream<int>& FOC_RPM_THETA_m,
                           ::hls::stream<int>& FOC_MOTOR_ID,
                           ::hls::stream<float>& Va_cmd,
                           ::hls::stream<float>& Vb_cmd,
                           ::hls::stream<float>& Vc_cmd,
                           ::hls::stream<int>& FOC_MOTOR_ID_out,
                           volatile int& ppr_args,
                           volatile int& control_mode_args,
                           volatile int& control_fixperiod_args,
                           volatile float& flux_sp_args,
                           volatile float& flux_kp_args,
                           volatile float& flux_ki_args,
                           volatile float& flux_kd_args,
                           volatile float& torque_sp_args,
                           volatile float& torque_kp_args,
                           volatile float& torque_ki_args,
                           volatile float& torque_kd_args,
                           volatile float& speed_sp_args,
                           volatile float& speed_kp_args,
                           volatile float& speed_ki_args,
                           volatile float& speed_kd_args,
                           volatile int& angle_sh_args,
                           volatile float& vd_args,
                           volatile float& vq_args,
                           volatile float& fw_kp_args,
                           volatile float& fw_ki_args,
                           volatile float& id_stts,
                           volatile float& flux_acc_stts,
                           volatile float& flux_err_stts,
                           volatile float& flux_out_stts,
                           volatile float& iq_stts,
                           volatile float& torque_acc_stts,
                           volatile float& torque_err_stts,
                           volatile float& torque_out_stts,
                           volatile float& speed_stts,
                           volatile float& speed_acc_stts,
                           volatile float& speed_err_stts,
                           volatile float& speed_out_stts,
                           volatile int& angle_stts,
                           volatile float& Va_cmd_stts,
                           volatile float& Vb_cmd_stts,
                           volatile float& Vc_cmd_stts,
                           volatile float& Ialpha_stts,
                           volatile float& Ibeta_stts,
                           volatile float& Ihomopolar_stts,
                           volatile int& fixed_angle_args) {
#pragma HLS interface axis port = Ia
#pragma HLS interface axis port = Ib
#pragma HLS interface axis port = Ic
#pragma HLS interface axis port = FOC_RPM_THETA_m
#pragma HLS interface axis port = FOC_MOTOR_ID
#pragma HLS interface axis port = Va_cmd
#pragma HLS interface axis port = Vb_cmd
#pragma HLS interface axis port = Vc_cmd
#pragma HLS interface axis port = FOC_MOTOR_ID_out
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
#pragma HLS interface ap_ctrl_hs port = return
#pragma HLS interface s_axilite port = return bundle = foc_args

    static xf::motorcontrol::golden::FOCContext<float> ctx[FOC_FP32_NUM_MOTORS];
    long trip_cnt = 0x7fffffffffffffffL;
#ifdef SIM_FINITE
    trip_cnt = FOC_FP32_TESTNUMBER;
#endif
    call_foc_fp32_strm(Ia, Ib, Ic, FOC_RPM_THETA_m, FOC_MOTOR_ID, Va_cmd, Vb_cmd, Vc_cmd, FOC_MOTOR_ID_out, ppr_args,
                       control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args,
                       flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args,
                       speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args, fw_kp_args,
                       fw_ki_args, id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts,
                       torque_err_stts, torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts,
                       angle_stts, Va_cmd_stts, Vb_cmd_stts, Vc_cmd_stts, Ialpha_stts, Ibeta_stts, Ihomopolar_stts,
                       fixed_angle_args, trip_cnt, ctx);
}
