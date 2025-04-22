/*
Copyright (C) 2024-2024, Advanced Micro Devices, Inc.
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
* @brief muxer_pi Patch synchronously data from the PIs
* @param m_axis    Formatted output data
* @param from_gen    Angle generated from generator (used in open loop)
* @param from_torque    output torque from PI
* @param from_flux    output flux from PI
* @param from_demux    output angle from demuxer
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void muxer_pi(hls::stream<WIDTH_DATA>& from_torque,
              hls::stream<WIDTH_DATA>& from_flux,
              hls::stream<WIDTH_DATA>& from_demux,
              hls::stream<WIDTH_DATA>& from_gen,
              hls::stream<T_FOC>& m_axis,
              hls::stream<T_STREAM_LOG>& logger_stream_in,
              hls::stream<T_STREAM_LOG>& logger_stream_out,
              volatile int& control_mode,
              volatile int& Vd_ps,
              volatile int& Vq_ps) {
    //#pragma HLS INTERFACE mode=axis port=from_torque
    //#pragma HLS INTERFACE mode=axis port=logger_stream_in
    //#pragma HLS INTERFACE mode=axis port=logger_stream_out
    //#pragma HLS INTERFACE mode=axis port=from_flux
    //#pragma HLS INTERFACE mode=axis port=from_demux
    //#pragma HLS INTERFACE mode=axis port=from_gen
    //#pragma HLS INTERFACE mode=axis port=m_axis
    //
    //#pragma HLS INTERFACE mode=ap_none port=control_mode
    //#pragma HLS INTERFACE mode=ap_none port=Vd_ps
    //#pragma HLS INTERFACE mode=ap_none port=Vq_ps
    //
    //#pragma HLS INTERFACE mode=ap_ctrl_none port=return

    T_FOC packet;
    T_STREAM_LOG log_packet;

    WIDTH_DATA Vd = 0, Vq = 0, Theta = 0, Theta_gen = 0;

#pragma HLS PIPELINE II = 1

    Vd.range(BIT_WIDTH_DATA - 1, 0) = from_flux.read();
    Vq.range(BIT_WIDTH_DATA - 1, 0) = from_torque.read();
    log_packet.range(BIT_WIDTH_LOG_STREAM_FOC - 1, 0) = logger_stream_in.read();
    Theta.range(BIT_WIDTH_DATA - 1, 0) = from_demux.read();
    Theta_gen.range(BIT_WIDTH_DATA - 1, 0) = from_gen.read();

    switch (control_mode) {
        case FOC_Mode::MOD_STOPPED:
            Vd = 0;
            Vq = 0;
            break;
        case FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED:
            Vd = Vd_ps;
            Vq = Vq_ps;
            Theta = Theta_gen;
            break;
        default:
            break;
    }

    log_packet.range(MAX_VQ_LOGGER, MIN_VQ_LOGGER) = Vq.range(BIT_WIDTH_DATA - 1, 0);
    log_packet.range(MAX_VD_LOGGER, MIN_VD_LOGGER) = Vd.range(BIT_WIDTH_DATA - 1, 0);

    packet.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);
    packet.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = Vq.range(BIT_WIDTH_DATA - 1, 0);
    packet.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Vd.range(BIT_WIDTH_DATA - 1, 0);

    m_axis.write(packet); // Write result to the output stream
    logger_stream_out.write(log_packet);

    return;
}

} // namespace motorcontrol
} // namespace xf