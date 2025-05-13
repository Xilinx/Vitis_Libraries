/*
 * Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * Except as contained in this notice, the name of Advanced Micro Devices
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization
 * from Advanced Micro Devices, Inc.
 * */
#include "common_vars.hpp"

enum VMMode : unsigned int { PS_DC_REF = 0, PL_DC_REF = 1 };

template <typename T_FOC,
          typename T_STREAM_LOG,
          typename WIDTH_DATA,
          typename WIDTH_ACCUM,
          typename PWM_DATA_TYPE,
          typename PWM_3_PHASE>
void voltage_modulation(hls::stream<T_FOC>& s_axis,
                        hls::stream<WIDTH_DATA>& voltage_in,
                        hls::stream<PWM_3_PHASE>& output_s,
                        hls::stream<T_STREAM_LOG>& logger_stream_in,
                        hls::stream<T_STREAM_LOG>& logger_stream_out,
                        volatile unsigned int& mode,
                        volatile int& max_sym_interval,
                        volatile int& double_interval,
                        volatile int& scaling_interval_pwm,
                        volatile int& phase_a,
                        volatile int& phase_b,
                        volatile int& phase_c) {
    // #pragma HLS PIPELINE II=1

    T_STREAM_LOG log_packet;
    T_FOC in_data, res;
    WIDTH_DATA Van, Vbn, Vcn; // Normalized SVPWM data
    WIDTH_DATA Theta;
    WIDTH_ACCUM Va_pwm, Vb_pwm, Vc_pwm;
    WIDTH_DATA Va, Vb, Vc;
    PWM_DATA_TYPE pwm_mod_a, pwm_mod_b, pwm_mod_c;
    PWM_3_PHASE pwm_packet;
    volatile int voltage_mod_in;
    volatile int double_voltage_mod_in;

    const WIDTH_DATA VOLTAGE = 1365; // 1 /48 -> 1/24 not taking into account sign
    const WIDTH_DATA MIN_LIM = 0;
    // const WIDTH_DATA MAX_LIM = 65535;
    // const ap_uint<BIT_WIDTH_FRACTIONAL> HALF = 32768;
    const WIDTH_DATA MAX_LIM = scaling_interval_pwm;
    const ap_uint<BIT_WIDTH_FRACTIONAL> HALF = (scaling_interval_pwm >> 1);

    //	const WIDTH_DATA MIN_INTERVAL_VOLTAGE = -max_sym_interval;
    const WIDTH_DATA MAX_INTERVAL_VOLTAGE = max_sym_interval;
    const WIDTH_DATA DOUBLE_MAX_INTERVAL_VOLTAGE = (double_interval == 0) ? 1 : double_interval;

    if (!voltage_in.empty()) {
        voltage_mod_in = voltage_in.read().to_int();
        double_voltage_mod_in = voltage_mod_in << 1;
    } else {
        voltage_mod_in = 1572864;
        double_voltage_mod_in = voltage_mod_in << 1;
    }
    // Decode Input stream
    log_packet = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream

    Vcn.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(BIT_WIDTH_STEP_STREAM - 1, 0);
    Vbn.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    Van.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    if (mode == VMMode::PS_DC_REF) {
        Va = Van + MAX_INTERVAL_VOLTAGE;
        Vb = Vbn + MAX_INTERVAL_VOLTAGE;
        Vc = Vcn + MAX_INTERVAL_VOLTAGE;
        Va_pwm = Va * MAX_LIM;
        Vb_pwm = Vb * MAX_LIM;
        Vc_pwm = Vc * MAX_LIM;
        pwm_mod_a = Va_pwm / DOUBLE_MAX_INTERVAL_VOLTAGE;
        pwm_mod_b = Vb_pwm / DOUBLE_MAX_INTERVAL_VOLTAGE;
        pwm_mod_c = Vc_pwm / DOUBLE_MAX_INTERVAL_VOLTAGE;
    } else if (mode == VMMode::PL_DC_REF) {
        Va = Van + voltage_mod_in;
        Vb = Vbn + voltage_mod_in;
        Vc = Vcn + voltage_mod_in;
        Va_pwm = Va * MAX_LIM;
        Vb_pwm = Vb * MAX_LIM;
        Vc_pwm = Vc * MAX_LIM;
        pwm_mod_a = Va_pwm / double_voltage_mod_in;
        pwm_mod_b = Vb_pwm / double_voltage_mod_in;
        pwm_mod_c = Vc_pwm / double_voltage_mod_in;
    } else {
        pwm_mod_a = HALF; // ZERO -> DUTY CYCLE 50%
        pwm_mod_b = HALF; // ZERO -> DUTY CYCLE 50%
        pwm_mod_c = HALF; // ZERO -> DUTY CYCLE 50%
    }

    log_packet.range(MAX_VA_LOGGER, MIN_VA_LOGGER) = pwm_mod_a;
    log_packet.range(MAX_VB_LOGGER, MIN_VB_LOGGER) = pwm_mod_b;
    log_packet.range(MAX_VC_LOGGER, MIN_VC_LOGGER) = pwm_mod_c;

    phase_a = pwm_mod_a;
    phase_b = pwm_mod_b;
    phase_c = pwm_mod_c;

    // Va_out.write(pwm_mod_a);
    // Vb_out.write(pwm_mod_b);
    // Vc_out.write(pwm_mod_c);
    pwm_packet.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = pwm_mod_c;
    pwm_packet.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = pwm_mod_b;
    pwm_packet.range(BIT_WIDTH_STEP_STREAM - 1, 0) = pwm_mod_a;
    output_s.write(pwm_packet);

    logger_stream_out.write(log_packet);
}
