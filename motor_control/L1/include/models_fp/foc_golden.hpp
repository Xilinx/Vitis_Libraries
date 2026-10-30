/*
Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/
#ifndef _FOC_GOLDEN_HPP_
#define _FOC_GOLDEN_HPP_

#include "clarke_direct.hpp"
#include "clarke_inverse.hpp"
#include "park_direct.hpp"
#include "park_inverse.hpp"
#include "pi_control.hpp"

#include <cmath>

namespace xf {
namespace motorcontrol {
namespace golden {

enum FOCMode {
    MOD_STOPPED = 0,
    MOD_SPEED_WITH_TORQUE,
    MOD_TORQUE_WITHOUT_SPEED,
    MOD_FLUX,
    MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED,
    MOD_MANUAL_TORQUE_FLUX,
    MOD_MANUAL_TORQUE,
    MOD_MANUAL_FLUX,
    MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE,
    MOD_TOTAL_NUM
};

template <typename T = float>
struct FOCConfig {
    int ppr;
    int control_mode;
    int control_fixperiod;
    T flux_sp;
    T flux_kp;
    T flux_ki;
    T flux_kd;
    T torque_sp;
    T torque_kp;
    T torque_ki;
    T torque_kd;
    T speed_sp;
    T speed_kp;
    T speed_ki;
    T speed_kd;
    int angle_shift;
    T vd;
    T vq;
    T fw_kp;
    T fw_ki;
    int fixed_angle;
    T max_voltage;
    T max_current;

    FOCConfig()
        : ppr(2),
          control_mode(MOD_TORQUE_WITHOUT_SPEED),
          control_fixperiod(1),
          flux_sp(0),
          flux_kp(1),
          flux_ki(0),
          flux_kd(0),
          torque_sp(0),
          torque_kp(1),
          torque_ki(0),
          torque_kd(0),
          speed_sp(0),
          speed_kp(0),
          speed_ki(0),
          speed_kd(0),
          angle_shift(0),
          vd(0),
          vq(0),
          fw_kp(0),
          fw_ki(0),
          fixed_angle(0),
          max_voltage(24),
          max_current(2.7) {}
};

template <typename T = float>
struct FOCStatus {
    T id;
    T iq;
    T flux_integral;
    T flux_error;
    T flux_output;
    T torque_integral;
    T torque_error;
    T torque_output;
    T speed;
    T speed_integral;
    T speed_error;
    T speed_output;
    int angle;
    T va_cmd;
    T vb_cmd;
    T vc_cmd;
    T ialpha;
    T ibeta;
    T ihomopolar;

    FOCStatus()
        : id(0),
          iq(0),
          flux_integral(0),
          flux_error(0),
          flux_output(0),
          torque_integral(0),
          torque_error(0),
          torque_output(0),
          speed(0),
          speed_integral(0),
          speed_error(0),
          speed_output(0),
          angle(0),
          va_cmd(0),
          vb_cmd(0),
          vc_cmd(0),
          ialpha(0),
          ibeta(0),
          ihomopolar(0) {}
};

template <typename T = float>
struct FOCContext {
    PIControllerState<T> flux_pi;
    PIControllerState<T> torque_pi;
    PIControllerState<T> speed_pi;
    PIControllerState<T> field_weakening_pi;
    T vd_weakened;
    int gen_delay;
    int gen_angle;
    int mode_prev;

    FOCContext() : vd_weakened(0), gen_delay(0), gen_angle(0), mode_prev(MOD_STOPPED) {}
};

namespace details {

template <typename T>
inline T clip(T value, T min_value, T max_value) {
#ifdef __SYNTHESIS__
#pragma HLS INLINE
#endif
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

template <int VALUE_CPR>
inline int normalize_angle(int angle) {
#ifdef __SYNTHESIS__
#pragma HLS INLINE
#endif
    while (angle < 0) angle += VALUE_CPR;
    while (angle >= VALUE_CPR) angle -= VALUE_CPR;
    return angle;
}

template <int VALUE_CPR, typename T>
inline T electrical_angle_rad(int mechanical_angle, int ppr) {
#ifdef __SYNTHESIS__
#pragma HLS INLINE
#endif
    const int pole_pairs = (ppr <= 0) ? 1 : ppr;
    const int cpr_div_ppr = (VALUE_CPR / pole_pairs) > 0 ? (VALUE_CPR / pole_pairs) : VALUE_CPR;
    int theta_e = normalize_angle<VALUE_CPR>(mechanical_angle) % cpr_div_ppr;
    return static_cast<T>(6.28318530717958647692) * static_cast<T>(theta_e) / static_cast<T>(cpr_div_ppr);
}

template <typename T>
inline void control_foc(T& vd_ctrl,
                        T& vq_ctrl,
                        T& cos_out,
                        T& sin_out,
                        int mode,
                        T cos_in,
                        T sin_in,
                        T cos_gen_angle,
                        T sin_gen_angle,
                        T flux_out,
                        T torque_out,
                        T args_vd,
                        T args_vq) {
#ifdef __SYNTHESIS__
#pragma HLS INLINE
#endif
    switch (mode) {
        case MOD_STOPPED:
            vd_ctrl = 0;
            vq_ctrl = 0;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_SPEED_WITH_TORQUE:
        case MOD_TORQUE_WITHOUT_SPEED:
        case MOD_FLUX:
            vd_ctrl = flux_out;
            vq_ctrl = torque_out;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_TORQUE_FLUX:
            vd_ctrl = args_vd;
            vq_ctrl = args_vq;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_TORQUE:
            vd_ctrl = flux_out;
            vq_ctrl = args_vq;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED:
            vd_ctrl = args_vd;
            vq_ctrl = args_vq;
            cos_out = cos_gen_angle;
            sin_out = sin_gen_angle;
            break;
        case MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE:
            vd_ctrl = args_vd;
            vq_ctrl = args_vq;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_FLUX:
            vd_ctrl = args_vd;
            vq_ctrl = torque_out;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        default:
            vd_ctrl = 0;
            vq_ctrl = 0;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
    }
}

template <int VALUE_CPR, typename T>
void foc_core_golden(T ia,
                     T ib,
                     T ic,
                     short rpm,
                     short angle,
                     T& va_cmd,
                     T& vb_cmd,
                     T& vc_cmd,
                     const FOCConfig<T>& cfg,
                     FOCStatus<T>& status,
                     FOCContext<T>& ctx) {
    const int mode = cfg.control_mode;
    const bool mode_change = (mode != ctx.mode_prev) || (mode == MOD_STOPPED);
    ctx.mode_prev = mode;

    if (ctx.gen_delay >= cfg.control_fixperiod) {
        ctx.gen_delay = 0;
        ctx.gen_angle = (ctx.gen_angle >= (VALUE_CPR - 1)) ? 0 : ctx.gen_angle + 1;
    } else {
        ++ctx.gen_delay;
    }

    int theta = static_cast<int>(angle) - cfg.angle_shift;
    if (mode == MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED) {
        theta = ctx.gen_angle;
    } else if (mode == MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE) {
        theta = cfg.fixed_angle;
    }
    theta = normalize_angle<VALUE_CPR>(theta);
    T theta_rad = electrical_angle_rad<VALUE_CPR, T>(theta, cfg.ppr);
    T cos_theta = std::cos(theta_rad);
    T sin_theta = std::sin(theta_rad);

    T ialpha = 0;
    T ibeta = 0;
    T ihomopolar = 0;
    clarke_direct_golden<T>(ialpha, ibeta, ihomopolar, ia, ib, ic);

    T id = 0;
    T iq = 0;
    park_direct_golden<T>(id, iq, ialpha, ibeta, cos_theta, sin_theta);

    T speed_pid_out = 0;
    pi_control_golden<T>(speed_pid_out, ctx.speed_pi, static_cast<T>(rpm), cfg.speed_sp, cfg.speed_kp, cfg.speed_ki,
                         mode_change);

    T flux_sp = (mode == MOD_FLUX) ? (cfg.flux_sp - ctx.vd_weakened) : cfg.flux_sp;
    T flux_pid_out = 0;
    pi_control_golden<T>(flux_pid_out, ctx.flux_pi, id, flux_sp, cfg.flux_kp, cfg.flux_ki, mode_change);
    T flux_pid_out_io = clip<T>(flux_pid_out, -cfg.max_voltage, cfg.max_voltage);

    T torque_sp = (mode == MOD_TORQUE_WITHOUT_SPEED) ? cfg.torque_sp : speed_pid_out;
    T torque_pid_out = 0;
    pi_control_golden<T>(torque_pid_out, ctx.torque_pi, iq, torque_sp, cfg.torque_kp, cfg.torque_ki, mode_change);
    T torque_pid_out_io = clip<T>(torque_pid_out, -cfg.max_voltage, cfg.max_voltage);

    T modulation = std::sqrt(flux_pid_out_io * flux_pid_out_io + torque_pid_out_io * torque_pid_out_io);
    T modulation_sp = cfg.max_voltage / static_cast<T>(2.0);
    pi_control_golden<T>(ctx.vd_weakened, ctx.field_weakening_pi, modulation, modulation_sp, cfg.fw_kp, cfg.fw_ki,
                         mode_change);
    ctx.vd_weakened = clip<T>(ctx.vd_weakened, -cfg.max_current, cfg.max_current);

    T vd_ctrl = 0;
    T vq_ctrl = 0;
    T cos_theta_ctrl = cos_theta;
    T sin_theta_ctrl = sin_theta;
    control_foc<T>(vd_ctrl, vq_ctrl, cos_theta_ctrl, sin_theta_ctrl, mode, cos_theta, sin_theta, cos_theta, sin_theta,
                   flux_pid_out_io, torque_pid_out_io, cfg.vd, cfg.vq);
    vd_ctrl = clip<T>(vd_ctrl, -cfg.max_voltage, cfg.max_voltage);
    vq_ctrl = clip<T>(vq_ctrl, -cfg.max_voltage, cfg.max_voltage);

    T valpha = 0;
    T vbeta = 0;
    park_inverse_golden<T>(valpha, vbeta, vd_ctrl, vq_ctrl, cos_theta_ctrl, sin_theta_ctrl);

    T va_raw = 0;
    T vb_raw = 0;
    T vc_raw = 0;
    clarke_inverse_golden<T>(va_raw, vb_raw, vc_raw, valpha, vbeta);

    va_cmd = clip<T>(va_raw, -cfg.max_voltage, cfg.max_voltage);
    vb_cmd = clip<T>(vb_raw, -cfg.max_voltage, cfg.max_voltage);
    vc_cmd = clip<T>(vc_raw, -cfg.max_voltage, cfg.max_voltage);

    status.id = id;
    status.iq = iq;
    status.flux_integral = ctx.flux_pi.integral_error;
    status.flux_error = ctx.flux_pi.previous_error;
    status.flux_output = flux_pid_out;
    status.torque_integral = ctx.torque_pi.integral_error;
    status.torque_error = ctx.torque_pi.previous_error;
    status.torque_output = torque_pid_out;
    status.speed = static_cast<T>(rpm);
    status.speed_integral = ctx.speed_pi.integral_error;
    status.speed_error = ctx.speed_pi.previous_error;
    status.speed_output = speed_pid_out;
    status.angle = theta;
    status.va_cmd = va_cmd;
    status.vb_cmd = vb_cmd;
    status.vc_cmd = vc_cmd;
    status.ialpha = ialpha;
    status.ibeta = ibeta;
    status.ihomopolar = ihomopolar;
}

} // namespace details
} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
