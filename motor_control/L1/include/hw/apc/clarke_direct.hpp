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
* @brief Clarke_Direct Transform to map 3 phase rotating frame into 2 phase
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
void Clarke_Direct(hls::stream<T_FOC>& s_axis,
                   hls::stream<T_FOC>& m_axis,
                   hls::stream<T_STREAM_LOG>& logger_stream_in,
                   hls::stream<T_STREAM_LOG>& logger_stream_out) {
    //#pragma HLS INTERFACE mode=axis port=s_axis
    //#pragma HLS INTERFACE mode=axis port=m_axis
    //#pragma HLS INTERFACE mode=axis port=logger_stream_in
    //#pragma HLS INTERFACE mode=axis port=logger_stream_out
    //
    //#pragma HLS INTERFACE mode=ap_ctrl_none port=return

    T_FOC in_data, res;
    WIDTH_DATA Ia, Ib, Theta, RPM;
    WIDTH_DATA Ialpha, Ibeta, Ibd, Iad;
    WIDTH_ACCUM Ibd_, Iad_, Ibeta_;
    T_STREAM_LOG log;

    const WIDTH_DATA SQRT3A = 37837;
    const WIDTH_DATA TWO_D_SQRT3 = 75674;

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    // Decode Input stream
    in_data = s_axis.read();
    log = logger_stream_in.read();
    Ia.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(BIT_WIDTH_STEP_STREAM - 1, 0);
    Ib.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    RPM.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    // Process data
    Ialpha.range(BIT_WIDTH_DATA - 1, 0) = Ia.range(BIT_WIDTH_DATA - 1, 0);
    Ibd_ = Ib * TWO_D_SQRT3;
    Iad_ = Ia * SQRT3A;
    Ibd = Ibd_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Iad = Iad_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Ibeta = Iad + Ibd;

    log.range(MAX_IALPHA_LOGGER, MIN_IALPHA_LOGGER) = Ialpha.range(BIT_WIDTH_DATA - 1, 0);
    log.range(MAX_IBETA_LOGGER, MIN_IBETA_LOGGER) = Ibeta.range(BIT_WIDTH_DATA - 1, 0);

    // Write output stream
    res.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = RPM.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Ibeta.range(BIT_WIDTH_DATA - 1, 0);
    res.range(BIT_WIDTH_STEP_STREAM - 1, 0) = Ialpha.range(BIT_WIDTH_DATA - 1, 0);

    m_axis.write(res);
    logger_stream_out.write(log); // Write result to the output stream
}

} // namespace motorcontrol
} // namespace xf