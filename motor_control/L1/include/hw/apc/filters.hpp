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
* @brief ps_iir_filter Data patcher and filter stage before FOC
* @param output_stream    Formatted output data
* @param Ia    A phase current measured from ADC
* @param Ib    B phase current measured from ADC
* @param Ic    C phase current measured from ADC
* @param SPEED_THETA_m    output packet from QEI interface
* @param logger_stream    Formatted output data of the logger
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
* @param T_ADC_WIDTH    width of the variable from ADC (24 bits, if change please change the Vivado design as well)
* @param T_QEI_WIDTH    width of the stream from QEI
 */

template <typename T_FOC,
          typename T_STREAM_LOG,
          typename WIDTH_DATA,
          typename WIDTH_ACCUM,
          typename T_ADC_WIDTH,
          typename T_QEI_WIDTH>
void ps_iir_filter(

    hls::stream<T_ADC_WIDTH>& Ia,
    hls::stream<T_ADC_WIDTH>& Ib,
    hls::stream<T_ADC_WIDTH>& Ic,
    hls::stream<T_QEI_WIDTH>& SPEED_THETA_m, // RPM & Theta_m

    hls::stream<T_FOC>& output_stream,
    hls::stream<T_STREAM_LOG>& logger_stream,

    int32_t& filt_a,
    int32_t& filt_b,
    int32_t& angle_shift) {
    //#pragma HLS interface axis port=Ia
    //#pragma HLS interface axis port=Ib
    //#pragma HLS interface axis port=Ic
    //#pragma HLS interface axis port=SPEED_THETA_m
    //#pragma HLS interface axis port=output_stream
    //#pragma HLS interface axis port=logger_stream
    //#pragma HLS INTERFACE ap_none port=filt_a
    //#pragma HLS INTERFACE ap_none port=filt_b
    //#pragma HLS INTERFACE ap_none port=angle_shift
    //
    //#pragma HLS interface ap_ctrl_none port=return

    T_ADC_WIDTH Ia_s, Ib_s, Ic_s;
    volatile int Ia_, Ib_, Ic_;
    WIDTH_DATA angle_, speed, packet_rpm_angle;
    T_FOC output_var;
    T_STREAM_LOG output_log;

    Ia_s = Ia.read();
    Ib_s = Ib.read();
    Ic_s = Ic.read();

    Ia_ = (Ia_s.range(PWM_AP_FIXED_PARA_W2 - 1, 0)).to_int();
    Ib_ = (Ib_s.range(PWM_AP_FIXED_PARA_W2 - 1, 0)).to_int();
    Ic_ = (Ic_s.range(PWM_AP_FIXED_PARA_W2 - 1, 0)).to_int();

    packet_rpm_angle = SPEED_THETA_m.read();

    speed = (packet_rpm_angle.range(15, 0)).to_int();
    angle_ = (packet_rpm_angle.range(31, 16)).to_int();
    angle_ = angle_ - angle_shift;

    output_var.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = angle_.range(BIT_WIDTH_DATA - 1, 0);
    output_var.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = speed.range(BIT_WIDTH_DATA - 1, 0);
    output_var.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Ib_;
    output_var.range(BIT_WIDTH_STEP_STREAM - 1, 0) = Ia_;

    output_log.range(MAX_THETA_LOGGER, MIN_THETA_LOGGER) = angle_.range(BIT_WIDTH_DATA - 1, 0);
    output_log.range(MAX_RPM_LOGGER, MIN_RPM_LOGGER) = speed.range(BIT_WIDTH_DATA - 1, 0);
    output_log.range(MAX_IB_LOGGER, MIN_IB_LOGGER) = Ib_;
    output_log.range(MAX_IA_LOGGER, MIN_IA_LOGGER) = Ia_;

    output_stream.write(output_var);
    logger_stream.write(output_log);
}

} // namespace motorcontrol
} // namespace xf
