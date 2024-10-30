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
* @brief demuxer_pi Dispatch synchronously data to the PIs
* @param s_axis    Formatted Input data
* @param angle_gen    Angle generated from generator (used in open loop)
* @param to_Iq_PI    output torque to PI
* @param to_Id_PI    output flux to PI
* @param to_muxer    output angle to PI
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void demuxer_pi(hls::stream<T_FOC>& s_axis,
                hls::stream<WIDTH_DATA>& angle_gen,
                hls::stream<WIDTH_DATA>& to_Iq_PI,
                hls::stream<WIDTH_DATA>& to_Id_PI,
                hls::stream<WIDTH_DATA>& to_RPM_PI,
                hls::stream<WIDTH_DATA>& to_muxer,
                hls::stream<T_STREAM_LOG>& logger_stream_in,
                hls::stream<T_STREAM_LOG>& logger_stream_out,
                volatile int& control_mode,
                volatile int& Id_,
                volatile int& Iq_,
                volatile int& theta_) {
    //#pragma HLS INTERFACE mode=axis port=s_axis
    //#pragma HLS INTERFACE mode=axis port=logger_stream_in
    //#pragma HLS INTERFACE mode=axis port=logger_stream_out
    //#pragma HLS INTERFACE mode=axis port=to_Iq_PI
    //#pragma HLS INTERFACE mode=axis port=to_Id_PI
    //#pragma HLS INTERFACE mode=axis port=to_RPM_PI
    //#pragma HLS INTERFACE mode=axis port=to_muxer
    //#pragma HLS INTERFACE mode=axis port=angle_gen
    //
    //#pragma HLS INTERFACE mode=ap_none port=control_mode
    //#pragma HLS INTERFACE mode=ap_none port=Id_
    //#pragma HLS INTERFACE mode=ap_none port=Iq_
    //#pragma HLS INTERFACE mode=ap_none port=theta_
    //
    //#pragma HLS INTERFACE mode=ap_ctrl_none port=return

    T_FOC packet;
    T_STREAM_LOG log_packet;

    WIDTH_DATA Id = 0, Iq = 0, RPM = 0, Theta = 0, Theta_gen = 0;

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    log_packet = logger_stream_in.read();
    packet = s_axis.read();
    Theta_gen = angle_gen.read();

    Id.range(BIT_WIDTH_DATA - 1, 0) = packet.range(BIT_WIDTH_STEP_STREAM - 1, 0);
    Iq.range(BIT_WIDTH_DATA - 1, 0) = packet.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    RPM.range(BIT_WIDTH_DATA - 1, 0) = packet.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = packet.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    switch (control_mode) {
        case FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE:
            Id.range(BIT_WIDTH_DATA - 1, 0) = Id_;
            Iq.range(BIT_WIDTH_DATA - 1, 0) = Iq_;
            Theta.range(BIT_WIDTH_DATA - 1, 0) = theta_;
            break;
        case FOC_Mode::MOD_MANUAL_TORQUE_FLUX_AUTO_ANGLE:
            Id.range(BIT_WIDTH_DATA - 1, 0) = Id_;
            Iq.range(BIT_WIDTH_DATA - 1, 0) = Iq_;
            Theta.range(BIT_WIDTH_DATA - 1, 0) = Theta_gen;
            break;
        default:
            break;
    }

    //	log_packet.range(MAX_IQ_LOGGER, MIN_IQ_LOGGER) = Id;
    //	log_packet.range(MAX_ID_LOGGER, MIN_ID_LOGGER) = Iq;
    log_packet.range(MAX_THETA_LOGGER, MIN_THETA_LOGGER) = Theta;

    to_RPM_PI.write(RPM);
    to_Iq_PI.write(Iq);
    to_Id_PI.write(Id);
    to_muxer.write(Theta);
    logger_stream_out.write(log_packet);

    return;
}

} // namespace motorcontrol
} // namespace xf