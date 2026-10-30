/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include "ip_foc_multiple.hpp"

#include <cstdio>

#ifndef __SYNTHESIS__
RangeTracer ranger;
#endif

static int q15q16(float value) {
    t_glb_q15q16 fixed_value = value;
    return fixed_value.range(31, 0);
}

static t_glb_speed_theta pack_speed_theta(short rpm, short theta) {
    t_glb_speed_theta speed_theta = 0;
    speed_theta(31, 16) = theta;
    speed_theta(15, 0) = rpm;
    return speed_theta;
}

int main() {
    const int kSamples = TESTNUMBER;
    const int kNumMotors = IP_FOC_NUM_MOTORS;

    t_glb_foc2pwm ia[kSamples];
    t_glb_foc2pwm ib[kSamples];
    t_glb_foc2pwm ic[kSamples];
    t_glb_speed_theta speed_theta[kSamples];
    int motor_id[kSamples];
    t_glb_foc2pwm expected_va[kSamples];
    t_glb_foc2pwm expected_vb[kSamples];
    t_glb_foc2pwm expected_vc[kSamples];

    volatile int ppr_args = COMM_MACRO_PPR;
    volatile int control_mode_args = xf::motorcontrol::FOC_Mode::MOD_TORQUE_WITHOUT_SPEED;
    volatile int control_fixperiod_args = 4;
    volatile int flux_sp_args = q15q16(0.0f);
    volatile int flux_kp_args = q15q16(0.5f);
    volatile int flux_ki_args = q15q16(0.02f);
    volatile int flux_kd_args = q15q16(0.0f);
    volatile int torque_sp_args = q15q16(0.25f);
    volatile int torque_kp_args = q15q16(0.6f);
    volatile int torque_ki_args = q15q16(0.03f);
    volatile int torque_kd_args = q15q16(0.0f);
    volatile int speed_sp_args = q15q16(0.0f);
    volatile int speed_kp_args = q15q16(0.0f);
    volatile int speed_ki_args = q15q16(0.0f);
    volatile int speed_kd_args = q15q16(0.0f);
    volatile int angle_sh_args = q15q16(0.0f);
    volatile int vd_args = q15q16(0.0f);
    volatile int vq_args = q15q16(0.0f);
    volatile int fw_kp_args = q15q16(0.0f);
    volatile int fw_ki_args = q15q16(0.0f);
    volatile int fixed_angle_args = q15q16(0.0f);

    volatile int id_stts = 0;
    volatile int flux_acc_stts = 0;
    volatile int flux_err_stts = 0;
    volatile int flux_out_stts = 0;
    volatile int iq_stts = 0;
    volatile int torque_acc_stts = 0;
    volatile int torque_err_stts = 0;
    volatile int torque_out_stts = 0;
    volatile int speed_stts = 0;
    volatile int speed_acc_stts = 0;
    volatile int speed_err_stts = 0;
    volatile int speed_out_stts = 0;
    volatile int angle_stts = 0;
    volatile int va_cmd_stts = 0;
    volatile int vb_cmd_stts = 0;
    volatile int vc_cmd_stts = 0;
    volatile int ialpha_stts = 0;
    volatile int ibeta_stts = 0;
    volatile int ihomopolar_stts = 0;

    xf::motorcontrol::FocContext_ap_fixed<t_glb_foc2pwm, COMM_W, COMM_I> expected_ctx[IP_FOC_NUM_MOTORS];
    short cpr_div_ppr = COMM_MACRO_CPR / ppr_args;
    unsigned int tab_map_factor = ((COMM_MACRO_TLB_LENTH * (unsigned int)ppr_args) << 16) / COMM_MACRO_CPR;
    const t_glb_foc2pwm rpm_factor = 60.0 / 2 / 3.1415926535;
    t_glb_foc2pwm rpm_to_speed = (t_glb_q15q16)ppr_args / rpm_factor;

    for (int i = 0; i < kSamples; ++i) {
        ia[i] = 0.10f + 0.001f * (i % 17);
        ib[i] = -0.05f - 0.001f * (i % 11);
        ic[i] = 0 - ia[i] - ib[i];
        short rpm = 1000 + (i % 8) * 25;
        short theta = (i * 7) % COMM_MACRO_CPR;
        speed_theta[i] = pack_speed_theta(rpm, theta);
        motor_id[i] = (kNumMotors == 1) ? 0 : ((i * 17 + 3) % kNumMotors);
        if ((i % 41) == 7) {
            motor_id[i] = kNumMotors;
        }
        if ((i % 67) == 11) {
            motor_id[i] = -5;
        }

        expected_va[i] = 0;
        expected_vb[i] = 0;
        expected_vc[i] = 0;
        int trigger_args = 0;
        int control2_args = 0;
        const bool motor_id_valid = (motor_id[i] >= 0) && (motor_id[i] < kNumMotors);
        if (motor_id_valid) {
            xf::motorcontrol::details::foc_core_ap_fixed_context<COMM_MACRO_CPR, t_glb_foc2pwm, MAX_VAL_PWM, COMM_W,
                                                                 COMM_I>(
                ia[i], ib[i], ic[i], rpm, theta, expected_va[i], expected_vb[i], expected_vc[i], tab_map_factor,
                cpr_div_ppr, rpm_to_speed, control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args,
                flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args,
                speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args,
                trigger_args, control2_args, fw_kp_args, fw_ki_args, id_stts, flux_acc_stts, flux_err_stts,
                flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts, torque_out_stts, speed_stts, speed_acc_stts,
                speed_err_stts, speed_out_stts, angle_stts, ialpha_stts, ibeta_stts, ihomopolar_stts, fixed_angle_args,
                expected_ctx[motor_id[i]]);
        }
    }

    hls::stream<t_glb_foc2pwm> ia_stream;
    hls::stream<t_glb_foc2pwm> ib_stream;
    hls::stream<t_glb_foc2pwm> ic_stream;
    hls::stream<t_glb_speed_theta> speed_theta_stream;
    hls::stream<int> motor_id_stream;
    hls::stream<t_glb_foc2pwm> va_stream;
    hls::stream<t_glb_foc2pwm> vb_stream;
    hls::stream<t_glb_foc2pwm> vc_stream;
    hls::stream<int> motor_id_out_stream;

    for (int i = 0; i < kSamples; ++i) {
        ia_stream.write(ia[i]);
        ib_stream.write(ib[i]);
        ic_stream.write(ic[i]);
        speed_theta_stream.write(speed_theta[i]);
        motor_id_stream.write(motor_id[i]);
    }

    hls_foc_periodic_ap_fixed_multiple(
        ia_stream, ib_stream, ic_stream, speed_theta_stream, motor_id_stream, va_stream, vb_stream, vc_stream,
        motor_id_out_stream, ppr_args, control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args,
        flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args,
        speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args, fw_kp_args, fw_ki_args, id_stts,
        flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts, torque_out_stts,
        speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, va_cmd_stts, vb_cmd_stts, vc_cmd_stts,
        ialpha_stts, ibeta_stts, ihomopolar_stts, fixed_angle_args);

    int errors = 0;
    int invalid_count = 0;
    for (int i = 0; i < kSamples; ++i) {
        const t_glb_foc2pwm va = va_stream.read();
        const t_glb_foc2pwm vb = vb_stream.read();
        const t_glb_foc2pwm vc = vc_stream.read();
        const int id_out = motor_id_out_stream.read();
        const bool motor_id_valid = (motor_id[i] >= 0) && (motor_id[i] < kNumMotors);
        if (!motor_id_valid) {
            ++invalid_count;
        }
        if (va != expected_va[i] || vb != expected_vb[i] || vc != expected_vc[i] || id_out != motor_id[i]) {
            if (errors < 8) {
                std::printf("Mismatch sample %d id=%d out_id=%d dut=(%f,%f,%f) expected=(%f,%f,%f)\n", i, motor_id[i],
                            id_out, va.to_float(), vb.to_float(), vc.to_float(), expected_va[i].to_float(),
                            expected_vb[i].to_float(), expected_vc[i].to_float());
            }
            ++errors;
        }
    }

    if (invalid_count == 0) {
        std::printf("Test did not exercise invalid motor IDs.\n");
        ++errors;
    }
    if (kNumMotors > 1 && motor_id[0] == motor_id[1]) {
        std::printf("Test did not exercise non-sequential motor IDs.\n");
        ++errors;
    }
    if (errors == 0) {
        std::printf("IP_FOC_Multiple ap_fixed csim passed for NUM_MOTORS=%d with %d invalid IDs.\n", kNumMotors,
                    invalid_count);
    }
    return errors;
}
