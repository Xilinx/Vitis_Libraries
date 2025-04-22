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

/// Data Type for data exchange of Voltage from FOC to SVPWM and Current from ADC to FOC
typedef ap_fixed<PWM_AP_FIXED_PARA_W2, PWM_AP_FIXED_PARA_I2> t_glb_foc2pwm;

/// Data Type for the data exchange of the speed and theta from QEI or others.
typedef ap_uint<32> t_glb_speed_theta;

int main(void) {
    hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> > input_data_stream, output_data_stream;
    hls::stream<ap_int<BIT_WIDTH_DATA> > input_data_w, output_data_w;
    hls::stream<int32_t> input_data_w_i, output_data_w_i;
    hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> > log_in, log_out;
    hls::stream<t_glb_foc2pwm> current_a, current_b, current_c;
    hls::stream<t_glb_speed_theta> qei_pack;

    ap_uint<BIT_WIDTH_STREAM_FOC> input_values_stream, output_values_stream;
    ap_uint<BIT_WIDTH_LOG_STREAM_FOC> input_log_values, output_log_values;
    ap_int<BIT_WIDTH_DATA> input_values_w_u, output_values_w_u;
    int32_t filt_a = 0, filt_b = 0, shift = 0;

    t_glb_foc2pwm curr_val = 32;

    current_a.write(curr_val);
    current_b.write(curr_val);
    current_c.write(curr_val);
    qei_pack.write(0);

    ps_iir_filter_inst(current_a, current_b, current_c, qei_pack, output_data_stream, log_out, filt_a, filt_b, shift);

    int cnt_error = 1;
    while (!output_data_stream.empty()) {
        std::cout << "VAL: " << output_data_stream.read() << std::endl;
        cnt_error = 0;
    }
    log_out.read();

    return cnt_error;
}
