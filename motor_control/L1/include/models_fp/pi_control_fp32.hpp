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
#ifndef _PI_CONTROL_FP32_HPP_
#define _PI_CONTROL_FP32_HPP_

namespace xf {
namespace motorcontrol {
namespace hls {

//--------------------------------------------------------------------------
// PI Control FP32
// err = Setpoint - Measured
// acc = mode_change ? 0 : I_err_prev + err
// P = Kp * err
// I = Ki * acc
// Output = P + I
//--------------------------------------------------------------------------

struct PIControllerState_fp32 {
    float integral_error;
    float previous_error;

    PIControllerState_fp32() : integral_error(0), previous_error(0) {}

    void reset() {
        integral_error = 0;
        previous_error = 0;
    }
};

/**
 * brief PI control in the form of an inline HLS function
 * param output          Result Output
 * param setpoint        Setpoint
 * param measured_value  Measured process value, input
 * param kp              Proportional coefficient
 * param ki              Integral coefficient
 * param mode_change     is mode_change ? 1 : 0
 * param state           Reference to a variable storing the controller state
 */
inline void PI_Control_fp32(float& output,
                            float setpoint,
                            float measured_value,
                            float kp,
                            float ki,
                            bool mode_change,
                            PIControllerState_fp32& state) {
#pragma HLS INLINE
#pragma HLS PIPELINE II = 1
    float err = setpoint - measured_value;
    float acc;
    if (mode_change == true)
        acc = 0.0f;
    else
        acc = state.integral_error + err;
    float P;
    P = kp * err;
    float I;
    I = ki * acc;
    float sum = P + I;
    output = sum;
    state.previous_error = err;
    state.integral_error = acc;
}

} // namespace hls
} // namespace motorcontrol
} // namespace xf

#endif
