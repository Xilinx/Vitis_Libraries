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
#include "sin_cos_table.hpp"

namespace xf {
namespace motorcontrol {

/**
* @brief Park_Inverse Transform to map 2 phase fixed frame into 2 phase rotating frame
* @param s_axis    Formatted Input data
* @param m_axis    Formatted output data
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void Park_Inverse(hls::stream<T_FOC>& s_axis,
                  hls::stream<T_FOC>& m_axis,
                  hls::stream<T_STREAM_LOG>& logger_stream_in,
                  hls::stream<T_STREAM_LOG>& logger_stream_out) {
#pragma HLS INTERFACE mode = axis port = s_axis
#pragma HLS INTERFACE mode = axis port = m_axis
#pragma HLS INTERFACE mode = axis port = logger_stream_in
#pragma HLS INTERFACE mode = axis port = logger_stream_out

#pragma HLS INTERFACE mode = ap_ctrl_none port = return
    T_FOC in_data, packet;
    T_STREAM_LOG log_packet;
    WIDTH_DATA Vd, Vq, Theta;
    WIDTH_ACCUM Vd_cos, Vq_sin, Vq_cos, Vd_sin, Valpha_, Vbeta_;
    WIDTH_DATA Valpha, Vbeta;
    WIDTH_DATA cos_theta, sin_theta;
    ap_uint<32> Theta_Park;
    const WIDTH_DATA MIN_LIM = -16777215;
    const WIDTH_DATA MAX_LIM = 1677215;
    const WIDTH_DATA POLAR_PAIR = 2; // 2
    const WIDTH_DATA CPR = 1000;

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    // Decode Input stream
    log_packet = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream
    Vd.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    Vq.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    Theta_Park = Theta << 1;
    if (Theta_Park >= CPR) {
        Theta_Park = Theta_Park - CPR;
    }
    cos_theta = cos_table[Theta_Park];
    sin_theta = sin_table[Theta_Park];
    Vd_cos = Vd * cos_theta;
    Vq_sin = Vq * sin_theta;
    Vq_cos = Vq * cos_theta;
    Vd_sin = Vd * sin_theta;
    Valpha_ = Vd_cos - Vq_sin;
    Vbeta_ = Vq_cos + Vd_sin;
    Valpha = Valpha_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Vbeta = Vbeta_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Valpha = (Valpha > MAX_LIM) ? MAX_LIM : Valpha; // Clip max
    Valpha = (Valpha < MIN_LIM) ? MIN_LIM : Valpha; // Clip min
    Vbeta = (Vbeta > MAX_LIM) ? MAX_LIM : Vbeta;    // Clip max
    Vbeta = (Vbeta < MIN_LIM) ? MIN_LIM : Vbeta;    // Clip min

    log_packet.range(MAX_VALPHA_LOGGER, MIN_VALPHA_LOGGER) = Valpha;
    log_packet.range(MAX_VBETA_LOGGER, MIN_VBETA_LOGGER) = Vbeta;

    // Write output stream
    packet.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);
    // valpha -> valpha / vbeta->vbeta
    packet.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = Valpha.range(BIT_WIDTH_DATA - 1, 0);
    packet.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Vbeta.range(BIT_WIDTH_DATA - 1, 0);
    // valpha -> vbeta / vbeta->valpha
    //	packet.range((BIT_WIDTH_STEP_STREAM*3)-1, BIT_WIDTH_STEP_STREAM*2) = Vbeta.range(BIT_WIDTH_DATA-1, 0);
    //	packet.range((BIT_WIDTH_STEP_STREAM*2)-1, BIT_WIDTH_STEP_STREAM) = Valpha.range(BIT_WIDTH_DATA-1, 0);
    m_axis.write(packet); // Write result to the output stream
    logger_stream_out.write(log_packet);
}

} // namespace motorcontrol
} // namespace xf