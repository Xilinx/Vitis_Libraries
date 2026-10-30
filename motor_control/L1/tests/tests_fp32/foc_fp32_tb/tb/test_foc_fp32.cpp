/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include "../../../../include/models_fp/foc_golden.hpp"

#include <hls_stream.h>

#include <cmath>
#include <cstdio>

#ifndef FOC_FP32_NUM_MOTORS
#define FOC_FP32_NUM_MOTORS 1
#endif

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
                           volatile int& fixed_angle_args);

static int pack_speed_theta(short rpm, short theta) {
    return ((static_cast<int>(theta) & 0xFFFF) << 16) | (static_cast<int>(rpm) & 0xFFFF);
}

static bool close_enough(float actual, float expected, float tolerance) {
    return std::fabs(actual - expected) <= tolerance;
}

int main() {
    constexpr int kCpr = 1000;
    constexpr int kSamples = 256;
    constexpr int kNumMotors = FOC_FP32_NUM_MOTORS;

    xf::motorcontrol::golden::FOCConfig<float> cfg;
    cfg.ppr = 2;
    cfg.control_mode = xf::motorcontrol::golden::MOD_TORQUE_WITHOUT_SPEED;
    cfg.control_fixperiod = 2;
    cfg.flux_sp = 0.0f;
    cfg.flux_kp = 0.5f;
    cfg.flux_ki = 0.02f;
    cfg.torque_sp = 0.25f;
    cfg.torque_kp = 0.6f;
    cfg.torque_ki = 0.03f;
    cfg.speed_kp = 0.0f;
    cfg.speed_ki = 0.0f;
    cfg.vd = 0.0f;
    cfg.vq = 0.0f;
    cfg.fw_kp = 0.0f;
    cfg.fw_ki = 0.0f;

    float ia[kSamples];
    float ib[kSamples];
    float ic[kSamples];
    short rpm[kSamples];
    short theta[kSamples];
    int motor_id[kSamples];
    float expected_va[kSamples];
    float expected_vb[kSamples];
    float expected_vc[kSamples];

    xf::motorcontrol::golden::FOCContext<float> expected_ctx[kNumMotors];
    xf::motorcontrol::golden::FOCStatus<float> expected_status;
    for (int i = 0; i < kSamples; ++i) {
        ia[i] = 0.10f + 0.001f * static_cast<float>(i % 17);
        ib[i] = -0.05f - 0.001f * static_cast<float>(i % 11);
        ic[i] = -(ia[i] + ib[i]);
        rpm[i] = static_cast<short>(1000 + (i % 8) * 25);
        theta[i] = static_cast<short>((i * 7) % kCpr);
        motor_id[i] = (kNumMotors == 1) ? 0 : ((i * 13) % kNumMotors);
        if ((i % 53) == 0) {
            motor_id[i] = kNumMotors;
        }
        if ((i % 79) == 0) {
            motor_id[i] = -1;
        }
        expected_va[i] = 0.0f;
        expected_vb[i] = 0.0f;
        expected_vc[i] = 0.0f;

        const bool motor_id_valid = (motor_id[i] >= 0) && (motor_id[i] < kNumMotors);
        if (motor_id_valid) {
            const int motor_idx = motor_id[i];
            xf::motorcontrol::golden::details::foc_core_golden<kCpr, float>(
                ia[i], ib[i], ic[i], rpm[i], theta[i], expected_va[i], expected_vb[i], expected_vc[i], cfg,
                expected_status, expected_ctx[motor_idx]);
        }
    }

    ::hls::stream<float> ia_stream;
    ::hls::stream<float> ib_stream;
    ::hls::stream<float> ic_stream;
    ::hls::stream<int> speed_theta_stream;
    ::hls::stream<int> motor_id_stream;
    ::hls::stream<float> va_stream;
    ::hls::stream<float> vb_stream;
    ::hls::stream<float> vc_stream;
    ::hls::stream<int> motor_id_out_stream;
    for (int i = 0; i < kSamples; ++i) {
        ia_stream.write(ia[i]);
        ib_stream.write(ib[i]);
        ic_stream.write(ic[i]);
        speed_theta_stream.write(pack_speed_theta(rpm[i], theta[i]));
        motor_id_stream.write(motor_id[i]);
    }

    volatile int ppr_args = cfg.ppr;
    volatile int control_mode_args = cfg.control_mode;
    volatile int control_fixperiod_args = cfg.control_fixperiod;
    volatile float flux_sp_args = cfg.flux_sp;
    volatile float flux_kp_args = cfg.flux_kp;
    volatile float flux_ki_args = cfg.flux_ki;
    volatile float flux_kd_args = cfg.flux_kd;
    volatile float torque_sp_args = cfg.torque_sp;
    volatile float torque_kp_args = cfg.torque_kp;
    volatile float torque_ki_args = cfg.torque_ki;
    volatile float torque_kd_args = cfg.torque_kd;
    volatile float speed_sp_args = cfg.speed_sp;
    volatile float speed_kp_args = cfg.speed_kp;
    volatile float speed_ki_args = cfg.speed_ki;
    volatile float speed_kd_args = cfg.speed_kd;
    volatile int angle_sh_args = cfg.angle_shift;
    volatile float vd_args = cfg.vd;
    volatile float vq_args = cfg.vq;
    volatile float fw_kp_args = cfg.fw_kp;
    volatile float fw_ki_args = cfg.fw_ki;
    volatile float id_stts = 0.0f;
    volatile float flux_acc_stts = 0.0f;
    volatile float flux_err_stts = 0.0f;
    volatile float flux_out_stts = 0.0f;
    volatile float iq_stts = 0.0f;
    volatile float torque_acc_stts = 0.0f;
    volatile float torque_err_stts = 0.0f;
    volatile float torque_out_stts = 0.0f;
    volatile float speed_stts = 0.0f;
    volatile float speed_acc_stts = 0.0f;
    volatile float speed_err_stts = 0.0f;
    volatile float speed_out_stts = 0.0f;
    volatile int angle_stts = 0;
    volatile float va_cmd_stts = 0.0f;
    volatile float vb_cmd_stts = 0.0f;
    volatile float vc_cmd_stts = 0.0f;
    volatile float ialpha_stts = 0.0f;
    volatile float ibeta_stts = 0.0f;
    volatile float ihomopolar_stts = 0.0f;
    volatile int fixed_angle_args = cfg.fixed_angle;

    hls_foc_periodic_fp32(ia_stream, ib_stream, ic_stream, speed_theta_stream, motor_id_stream, va_stream, vb_stream,
                          vc_stream, motor_id_out_stream, ppr_args, control_mode_args, control_fixperiod_args,
                          flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args,
                          torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args,
                          angle_sh_args, vd_args, vq_args, fw_kp_args, fw_ki_args, id_stts, flux_acc_stts,
                          flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts, torque_out_stts,
                          speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, va_cmd_stts,
                          vb_cmd_stts, vc_cmd_stts, ialpha_stts, ibeta_stts, ihomopolar_stts, fixed_angle_args);

    int errors = 0;
    for (int i = 0; i < kSamples; ++i) {
        const float va = va_stream.read();
        const float vb = vb_stream.read();
        const float vc = vc_stream.read();
        const int motor_out = motor_id_out_stream.read();
        if (!close_enough(va, expected_va[i], 1e-5f) || !close_enough(vb, expected_vb[i], 1e-5f) ||
            !close_enough(vc, expected_vc[i], 1e-5f) || motor_out != motor_id[i]) {
            if (errors < 8) {
                std::printf("Mismatch at sample %d: dut=(%f,%f,%f,id=%d) expected=(%f,%f,%f,id=%d)\n", i, va, vb, vc,
                            motor_out, expected_va[i], expected_vb[i], expected_vc[i], motor_id[i]);
            }
            ++errors;
        }
    }

    if (errors == 0) {
        std::printf("FOC FP32 periodic stream csim passed.\n");
    }
    return errors;
}
