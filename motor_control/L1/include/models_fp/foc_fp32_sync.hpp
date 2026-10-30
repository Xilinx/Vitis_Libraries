/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/
#ifndef _FOC_FP32_SYNC_HPP_
#define _FOC_FP32_SYNC_HPP_

#include "foc_golden.hpp"

#include <hls_stream.h>
#include <stdint.h>

namespace xf {
namespace motorcontrol {
namespace hls {

template <int VALUE_CPR, int NUM_MOTORS = 1, typename T_IO = float, typename T_RPM_THETA_FOC = int>
void hls_foc_strm_fp32(::hls::stream<T_IO>& Ia,
                       ::hls::stream<T_IO>& Ib,
                       ::hls::stream<T_IO>& Ic,
                       ::hls::stream<T_RPM_THETA_FOC>& FOC_RPM_THETA_m,
                       ::hls::stream<int>& FOC_MOTOR_ID,
                       ::hls::stream<T_IO>& Va_cmd,
                       ::hls::stream<T_IO>& Vb_cmd,
                       ::hls::stream<T_IO>& Vc_cmd,
                       ::hls::stream<int>& FOC_MOTOR_ID_out,
                       volatile int& ppr_args,
                       volatile int& control_mode_args,
                       volatile int& control_fixperiod_args,
                       volatile T_IO& flux_sp_args,
                       volatile T_IO& flux_kp_args,
                       volatile T_IO& flux_ki_args,
                       volatile T_IO& flux_kd_args,
                       volatile T_IO& torque_sp_args,
                       volatile T_IO& torque_kp_args,
                       volatile T_IO& torque_ki_args,
                       volatile T_IO& torque_kd_args,
                       volatile T_IO& speed_sp_args,
                       volatile T_IO& speed_kp_args,
                       volatile T_IO& speed_ki_args,
                       volatile T_IO& speed_kd_args,
                       volatile int& angle_sh_args,
                       volatile T_IO& vd_args,
                       volatile T_IO& vq_args,
                       volatile T_IO& fw_kp_args,
                       volatile T_IO& fw_ki_args,
                       volatile T_IO& id_stts,
                       volatile T_IO& flux_acc_stts,
                       volatile T_IO& flux_err_stts,
                       volatile T_IO& flux_out_stts,
                       volatile T_IO& iq_stts,
                       volatile T_IO& torque_acc_stts,
                       volatile T_IO& torque_err_stts,
                       volatile T_IO& torque_out_stts,
                       volatile T_IO& speed_stts,
                       volatile T_IO& speed_acc_stts,
                       volatile T_IO& speed_err_stts,
                       volatile T_IO& speed_out_stts,
                       volatile int& angle_stts,
                       volatile T_IO& Va_cmd_stts,
                       volatile T_IO& Vb_cmd_stts,
                       volatile T_IO& Vc_cmd_stts,
                       volatile T_IO& Ialpha_stts,
                       volatile T_IO& Ibeta_stts,
                       volatile T_IO& Ihomopolar_stts,
                       volatile int& fixed_angle_args,
                       volatile long& trip_cnt,
                       golden::FOCContext<T_IO> ctx[NUM_MOTORS]) {
LOOP_FOC_STRM_FP32:
    for (long i = 0; i < trip_cnt; i++) {
        //#ifdef __SYNTHESIS__
        //#pragma HLS PIPELINE
        //#endif
        T_IO ia_in = 0;
        T_IO ib_in = 0;
        T_IO ic_in = 0;
        T_RPM_THETA_FOC rpm_theta_in = 0;
        int motor_id_in = 0;

        if (!Ia.empty()) {
            ia_in = Ia.read();
            ib_in = Ib.read();
            ic_in = Ic.read();
        }
        if (!FOC_RPM_THETA_m.empty()) {
            rpm_theta_in = FOC_RPM_THETA_m.read();
        }
        if (!FOC_MOTOR_ID.empty()) {
            motor_id_in = FOC_MOTOR_ID.read();
        }

        int32_t rpm_theta_bits = static_cast<int32_t>(rpm_theta_in);
        short rpm_in = static_cast<short>(rpm_theta_bits & 0x0000FFFF);
        short angle_in = static_cast<short>((rpm_theta_bits >> 16) & 0x0000FFFF);

        golden::FOCConfig<T_IO> cfg;
        cfg.ppr = ppr_args;
        cfg.control_mode = control_mode_args;
        cfg.control_fixperiod = control_fixperiod_args;
        cfg.flux_sp = flux_sp_args;
        cfg.flux_kp = flux_kp_args;
        cfg.flux_ki = flux_ki_args;
        cfg.flux_kd = flux_kd_args;
        cfg.torque_sp = torque_sp_args;
        cfg.torque_kp = torque_kp_args;
        cfg.torque_ki = torque_ki_args;
        cfg.torque_kd = torque_kd_args;
        cfg.speed_sp = speed_sp_args;
        cfg.speed_kp = speed_kp_args;
        cfg.speed_ki = speed_ki_args;
        cfg.speed_kd = speed_kd_args;
        cfg.angle_shift = angle_sh_args;
        cfg.vd = vd_args;
        cfg.vq = vq_args;
        cfg.fw_kp = fw_kp_args;
        cfg.fw_ki = fw_ki_args;
        cfg.fixed_angle = fixed_angle_args;

        golden::FOCStatus<T_IO> status;
        T_IO va_out = 0;
        T_IO vb_out = 0;
        T_IO vc_out = 0;
        // FOC_MOTOR_ID uses natural binary encoding here. One-hot decode can be added later at the interface layer.
        const bool motor_id_valid = (motor_id_in >= 0) && (motor_id_in < NUM_MOTORS);
        if (motor_id_valid) {
            const int motor_idx = motor_id_in;
            golden::details::foc_core_golden<VALUE_CPR, T_IO>(ia_in, ib_in, ic_in, rpm_in, angle_in, va_out, vb_out,
                                                              vc_out, cfg, status, ctx[motor_idx]);
        }

        Va_cmd.write(va_out);
        Vb_cmd.write(vb_out);
        Vc_cmd.write(vc_out);
        FOC_MOTOR_ID_out.write(motor_id_in);

        id_stts = status.id;
        flux_acc_stts = status.flux_integral;
        flux_err_stts = status.flux_error;
        flux_out_stts = status.flux_output;
        iq_stts = status.iq;
        torque_acc_stts = status.torque_integral;
        torque_err_stts = status.torque_error;
        torque_out_stts = status.torque_output;
        speed_stts = status.speed;
        speed_acc_stts = status.speed_integral;
        speed_err_stts = status.speed_error;
        speed_out_stts = status.speed_output;
        angle_stts = status.angle;
        Va_cmd_stts = status.va_cmd;
        Vb_cmd_stts = status.vb_cmd;
        Vc_cmd_stts = status.vc_cmd;
        Ialpha_stts = status.ialpha;
        Ibeta_stts = status.ibeta;
        Ihomopolar_stts = status.ihomopolar;
    }
}

} // namespace hls
} // namespace motorcontrol
} // namespace xf

#endif
