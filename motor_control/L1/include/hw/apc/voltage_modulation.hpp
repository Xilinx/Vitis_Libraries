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
#pragma once
#include "common_vars.hpp"

namespace xf {
namespace motorcontrol {

/**
* @brief voltage_modulation     Voltage modulation to the DC tension
* @param s_axis    Formatted Input data
* @param m_axis    Formatted output data
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param Va_out    Value of A phase
* @param Vb_out    Value of B phase
* @param Vc_out    Value of C phase
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM, typename PWM_DATA_TYPE>
void voltage_modulation(hls::stream<T_FOC>& s_axis,
                        hls::stream<PWM_DATA_TYPE>& Va_out,
                        hls::stream<PWM_DATA_TYPE>& Vb_out,
                        hls::stream<PWM_DATA_TYPE>& Vc_out,
                        hls::stream<T_STREAM_LOG>& logger_stream_in,
                        hls::stream<T_STREAM_LOG>& logger_stream_out) {
//#pragma HLS INTERFACE mode=axis port=s_axis
////#pragma HLS INTERFACE mode=axis port=voltage_in
//#pragma HLS INTERFACE mode=axis port=Va_out
//#pragma HLS INTERFACE mode=axis port=Vb_out
//#pragma HLS INTERFACE mode=axis port=Vc_out
//#pragma HLS INTERFACE mode=axis port=logger_stream_in
//#pragma HLS INTERFACE mode=axis port=logger_stream_out
//
//#pragma HLS INTERFACE mode=ap_ctrl_none port=return

#pragma HLS PIPELINE II = 1

    T_STREAM_LOG log_packet;
    T_FOC in_data, res;
    WIDTH_DATA Va, Vb, Vc, Theta; // SVPWM internals
    WIDTH_DATA Van, Vbn, Vcn;     // Normalized SVPWM data
    ap_int<BIT_WIDTH_FRACTIONAL> Va_mod, Vb_mod, Vc_mod;
    WIDTH_ACCUM Va_pwm, Vb_pwm, Vc_pwm;
    PWM_DATA_TYPE pwm_mod_a, pwm_mod_b, pwm_mod_c;

    //	const int32_t ampl_ = MAX_LIM; //2**15 -> PWM SIGNED
    const WIDTH_DATA VOLTAGE = 1365; // 1 /48 -> 1/24 not taking into account sign
    const WIDTH_DATA MIN_LIM = 0;
    const WIDTH_DATA MAX_LIM = 65535;
    const ap_int<BIT_WIDTH_FRACTIONAL> HALF = 32768;

    // Decode Input stream
    log_packet = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream

    Vcn.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(BIT_WIDTH_STEP_STREAM - 1, 0);
    Vbn.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    Van.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);
    // * 1 / 48
    Va_pwm = Van * VOLTAGE;
    Vb_pwm = Vbn * VOLTAGE;
    Vc_pwm = Vcn * VOLTAGE;
    // Crop normalized value
    Va_mod.range(BIT_WIDTH_FRACTIONAL - 1, 0) = Va_pwm.range(BIT_WIDTH_DATA - 1, BIT_WIDTH_FRACTIONAL);
    Vb_mod.range(BIT_WIDTH_FRACTIONAL - 1, 0) = Vb_pwm.range(BIT_WIDTH_DATA - 1, BIT_WIDTH_FRACTIONAL);
    Vc_mod.range(BIT_WIDTH_FRACTIONAL - 1, 0) = Vc_pwm.range(BIT_WIDTH_DATA - 1, BIT_WIDTH_FRACTIONAL);
    // from S to U
    pwm_mod_a = Va_mod + HALF;
    pwm_mod_b = Vb_mod + HALF;
    pwm_mod_c = Vc_mod + HALF;

    log_packet.range(MAX_VA_LOGGER, MIN_VA_LOGGER) = pwm_mod_a;
    log_packet.range(MAX_VB_LOGGER, MIN_VB_LOGGER) = pwm_mod_b;
    log_packet.range(MAX_VC_LOGGER, MIN_VC_LOGGER) = pwm_mod_c;

    Va_out.write(pwm_mod_a);
    Vb_out.write(pwm_mod_b);
    Vc_out.write(pwm_mod_c);

    logger_stream_out.write(log_packet);
}

} // namespace motorcontrol
} // namespace xf