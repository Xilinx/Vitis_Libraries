/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
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
#include "pwm_base.h"
#include "pwm_multichannels.h"

#define PWMCH 3

int main() {
    pwm_size_data_t period = 2000;
    pwm_size_data_t reserved = 0;
    pwm_size_data_t min_pfm_pulse_set = 10;
    uint32_t mode = 0x2;
    pwm_in<pwm_size_data_t> pwm_p[PWMCH];
    pwm_size_data_t pwm_out[PWMCH];
    bool pwm_o[PWMCH];
    hls::stream<pwm_stream_in> pwm_stream_data;
    bool synchstart;
    bool synchcenter;
    bool synch_in;
    bool power_enable_in = false;
    bool power_enable_out;

    Pwm_multichannels_7s(period, reserved, min_pfm_pulse_set, mode, pwm_p, pwm_out, pwm_o, pwm_stream_data, synchstart,
                         synchcenter, synch_in, power_enable_in, power_enable_out);

    return 0;
}
