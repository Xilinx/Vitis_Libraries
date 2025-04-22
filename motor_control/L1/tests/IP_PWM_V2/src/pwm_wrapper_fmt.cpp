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

// Define a static array of pwm_engine objects for handling multiple PWM channels.
static pwm_engine<pwm_size_data_t> spwm[PWMCH];

/*
void Pwm_multichannels(
        pwm_size_data_t period,
        pwm_size_data_t reserved,
        pwm_size_data_t min_pfm_pulse_set,
        uint32_t        mode,
    pwm_in<pwm_size_data_t> pwm_p[PWMCH],
    pwm_size_data_t pwm_out[PWMCH],
    bool pwm_o[PWMCH],
    hls::stream<pwm_stream_in>& pwm_stream_data,
    bool &synchstart,
    bool &synchcenter,
    bool &synch_in,
        bool power_enable_in,
        bool &power_enable_out
){
    // Pragmas to configure the FPGA AXI interfaces for the function arguments.
    #pragma HLS INTERFACE s_axilite port=return                bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=period                bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=reserved              bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=min_pfm_pulse_set     bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=mode                  bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=pwm_p                 bundle=MCMD
    #pragma HLS disaggregate variable=pwm_p
    #pragma HLS INTERFACE s_axilite port=pwm_out               bundle=MCMD
    #pragma HLS INTERFACE s_axilite port=power_enable_in       bundle=MCMD

    #pragma HLS INTERFACE mode=axis port=pwm_stream_data
    #pragma HLS INTERFACE mode=ap_none port=pwm_o
    #pragma HLS INTERFACE mode=ap_none port=synchstart
    #pragma HLS INTERFACE mode=ap_none port=synchcenter
    #pragma HLS INTERFACE mode=ap_none port=synch_in
    #pragma HLS INTERFACE mode=ap_none port=power_enable_out

    // Pragmas to partition arrays for full pipelining and parallel processing.
    #pragma HLS ARRAY_PARTITION variable=pwm_p      complete dim=1
    #pragma HLS ARRAY_PARTITION variable=spwm       complete dim=1
    #pragma HLS ARRAY_PARTITION variable=pwm_o      complete dim=1
    #pragma HLS ARRAY_PARTITION variable=pwm_out    complete dim=1


    static pwm_stream_in tmp_in;        // Temporary storage for input stream data.
    static pwm_size_data_t tmp[PWMCH];  // Temporary array for calculated duty cycles.
    #pragma HLS ARRAY_PARTITION variable=tmp complete dim=1


    uint8_t modesel;
    pwm_size_data_t periodsel;
    pwm_size_data_t min_pfm_pulse_sel;



        // Enable the power stage
    power_enable_out = power_enable_in;

    // Read from the input streams if available.
        if(!pwm_stream_data.empty()) tmp_in= pwm_stream_data.read();

    // First loop: Set up each PWM channel according to the input parameters.
    loop1:
    for(auto i=0; i<PWMCH; i++) {
        // Unroll this loop for parallel processing
        #pragma HLS UNROLL

        // Set prescaler period and the input synchronization signal

        // Set the mode, prescaler, period and the input synchronization signal
        // common for all channels

        spwm[i].control = mode;
        spwm[i].pwm_primary.shadow_reg = period;
        spwm[i].pwm_primary.min_pfm_pulse = min_pfm_pulse_set;
        spwm[i].pwm_primary.synch_in = synch_in;

        // Select if the duty cycle comes from the stream input or the static configuration.
        tmp[i] = (mode & STREAM) ? (pwm_size_data_t) tmp_in.range(((i+1)*pwm_wordlength)-1, i*pwm_wordlength)  :
pwm_p[i].dutycycle;

                // Set the duty cycle for each PWM channel.
        spwm[i].pwm_primary.dutycycle = tmp[i];
    }

    // Second loop: Generate PWM signals and capture output states.
    loop2: {
        for(auto i=0; i<PWMCH; i++) {
            // Unroll this loop for parallel processing.
            #pragma HLS UNROLL

            // Generate PWM signal, capture PWM counter, send the PWM output
            spwm[i].pwm();
                        pwm_p[i].flip_counter = spwm[i].pwm_primary.flip_counter;
            pwm_out[i] = spwm[i].pwm_primary.counter;
            pwm_o[i] = spwm[i].pwm_primary.flip;
        }


        // Send synchronization signals from the first PWM channel.
        // the start of the period in synchstart, the center of the period in synchcenter
        synchstart = spwm[0].pwm_primary.synchstart;
        synchcenter = spwm[0].pwm_primary.synchcenter;
    }

}*/

void Pwm_multichannels_7s(pwm_size_data_t period,
                          pwm_size_data_t reserved,
                          pwm_size_data_t min_pfm_pulse_set,
                          uint32_t mode,
                          pwm_in<pwm_size_data_t> pwm_p[PWMCH],
                          pwm_size_data_t pwm_out[PWMCH],
                          bool pwm_o[PWMCH],
                          hls::stream<pwm_stream_in>& pwm_stream_data,
                          bool& synchstart,
                          bool& synchcenter,
                          bool& synch_in,
                          bool power_enable_in,
                          bool& power_enable_out) {
// Pragmas to configure the FPGA AXI interfaces for the function arguments.
#pragma HLS INTERFACE s_axilite port = return bundle = MCMD
#pragma HLS INTERFACE s_axilite port = period bundle = MCMD offset = 0x100
#pragma HLS INTERFACE s_axilite port = reserved bundle = MCMD offset = 0x108
#pragma HLS INTERFACE s_axilite port = min_pfm_pulse_set bundle = MCMD offset = 0x110
#pragma HLS INTERFACE s_axilite port = mode bundle = MCMD offset = 0x118
#pragma HLS INTERFACE s_axilite port = pwm_p bundle = MCMD

#pragma HLS disaggregate variable = pwm_p
/*#pragma HLS INTERFACE s_axilite port=pwm_p->dutycycle offset=0x120
#pragma HLS INTERFACE s_axilite port=pwm_p->synch_reg_set offset=0x138
#pragma HLS INTERFACE s_axilite port=pwm_p->flip_counter offset=0x150*/
/*
#pragma HLS INTERFACE s_axilite port=pwm_p->dutycycle_0 offset=0x120
#pragma HLS INTERFACE s_axilite port=pwm_p->dutycycle_1 offset=0x128
#pragma HLS INTERFACE s_axilite port=pwm_p->dutycycle_2 offset=0x130

#pragma HLS INTERFACE s_axilite port=pwm_p->synch_reg_set_0 offset=0x138
#pragma HLS INTERFACE s_axilite port=pwm_p->synch_reg_set_1 offset=0x140
#pragma HLS INTERFACE s_axilite port=pwm_p->synch_reg_set_2 offset=0x148

#pragma HLS INTERFACE s_axilite port=pwm_p->flip_counter_0 offset=0x150
#pragma HLS INTERFACE s_axilite port=pwm_p->flip_counter_1 offset=0x158
#pragma HLS INTERFACE s_axilite port=pwm_p->flip_counter_2 offset=0x160
*/

#pragma HLS INTERFACE s_axilite port = pwm_out bundle = MCMD offset = 0x190
#pragma HLS INTERFACE s_axilite port = power_enable_in bundle = MCMD offset = 0x198

#pragma HLS INTERFACE mode = axis port = pwm_stream_data
#pragma HLS INTERFACE mode = ap_none port = pwm_o
#pragma HLS INTERFACE mode = ap_none port = synchstart
#pragma HLS INTERFACE mode = ap_none port = synchcenter
#pragma HLS INTERFACE mode = ap_none port = synch_in
#pragma HLS INTERFACE mode = ap_none port = power_enable_out

// Pragmas to partition arrays for full pipelining and parallel processing.
#pragma HLS ARRAY_PARTITION variable = pwm_p complete dim = 1
#pragma HLS ARRAY_PARTITION variable = spwm complete dim = 1
#pragma HLS ARRAY_PARTITION variable = pwm_o complete dim = 1
#pragma HLS ARRAY_PARTITION variable = pwm_out complete dim = 1

    static pwm_stream_in tmp_in;       // Temporary storage for input stream data.
    static pwm_size_data_t tmp[PWMCH]; // Temporary array for calculated duty cycles.
#pragma HLS ARRAY_PARTITION variable = tmp complete dim = 1

    uint8_t modesel;
    pwm_size_data_t periodsel;
    pwm_size_data_t min_pfm_pulse_sel;

    // Enable the power stage
    power_enable_out = power_enable_in;

    // Read from the input streams if available.
    if (!pwm_stream_data.empty()) tmp_in = pwm_stream_data.read();

// First loop: Set up each PWM channel according to the input parameters.
loop1:
    for (auto i = 0; i < PWMCH; i++) {
// Unroll this loop for parallel processing
#pragma HLS UNROLL

        // Set prescaler period and the input synchronization signal

        // Set the mode, prescaler, period and the input synchronization signal
        // common for all channels

        spwm[i].control = mode;
        spwm[i].pwm_primary.shadow_reg = period;
        spwm[i].pwm_primary.min_pfm_pulse = min_pfm_pulse_set;
        spwm[i].pwm_primary.synch_in = synch_in;

        // Select if the duty cycle comes from the stream input or the static configuration.
        tmp[i] = (mode & STREAM) ? (pwm_size_data_t)tmp_in.range(((i + 1) * pwm_wordlength) - 1, i * pwm_wordlength)
                                 : pwm_p[i].dutycycle;

        // Set the duty cycle for each PWM channel.
        spwm[i].pwm_primary.dutycycle = tmp[i];
    }

// Second loop: Generate PWM signals and capture output states.
loop2 : {
    for (auto i = 0; i < PWMCH; i++) {
// Unroll this loop for parallel processing.
#pragma HLS UNROLL

        // Generate PWM signal, capture PWM counter, send the PWM output
        spwm[i].pwm();
        pwm_p[i].flip_counter = spwm[i].pwm_primary.flip_counter;
        pwm_out[i] = spwm[i].pwm_primary.counter;
        pwm_o[i] = spwm[i].pwm_primary.flip;
    }

    // Send synchronization signals from the first PWM channel.
    // the start of the period in synchstart, the center of the period in synchcenter
    synchstart = spwm[0].pwm_primary.synchstart;
    synchcenter = spwm[0].pwm_primary.synchcenter;
}
}
