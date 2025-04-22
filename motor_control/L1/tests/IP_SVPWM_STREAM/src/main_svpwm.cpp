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
#include <iostream>
#include "common_definitions.hpp"

int main(void) {
    hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> > input_data_stream, output_data_stream;
    hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> > log_in, log_out;

    ap_uint<BIT_WIDTH_STREAM_FOC> input_values_stream, output_values_stream;
    ap_uint<BIT_WIDTH_LOG_STREAM_FOC> input_log_values, output_log_values;
    volatile int mode = 2;
    volatile int phase_a = 0;
    volatile int phase_b = 0;
    volatile int phase_c = 0;

    input_values_stream.range(BIT_WIDTH_STEP_STREAM - 1, 0) = 32768; // Va 0.5A -> 0.5 << FRACTIONAL_BIT(16)
    input_values_stream.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) =
        45000; // Vb 0A -> 0 << FRACTIONAL_BIT(16)
    input_values_stream.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = 500; // Vc 500
    input_values_stream.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = 125; // theta 125

    input_log_values = 0;

    input_data_stream.write(input_values_stream);
    log_in.write(input_log_values);

    SVPWM_Inst(input_data_stream, output_data_stream, log_in, log_out, mode, phase_a, phase_b, phase_c);

    int cnt_error = 1;
    while (!output_data_stream.empty()) {
        output_values_stream = output_data_stream.read();
        cnt_error = 0;
    }

    std::cout << "VAL Va: " << output_values_stream.range(BIT_WIDTH_STEP_STREAM - 1, 0) << std::endl;
    std::cout << "VAL Vb: " << output_values_stream.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM)
              << std::endl;
    std::cout << "VAL Vc: " << output_values_stream.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2)
              << std::endl;

    return cnt_error;
}
