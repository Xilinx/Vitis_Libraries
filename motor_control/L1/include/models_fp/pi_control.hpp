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
#ifndef _PI_CONTROL_GOLDEN_HPP_
#define _PI_CONTROL_GOLDEN_HPP_

namespace xf {
namespace motorcontrol {
namespace golden {

//--------------------------------------------------------------------------
// PI Control Golden
// err = Setpoint - Measured
// acc = mode_change ? 0 : I_err_prev + err
// P = Kp * err
// I = Ki * acc
// Output = P + I
//--------------------------------------------------------------------------

template <typename T = float>
struct PIControllerState {
    T integral_error;
    T previous_error;

    PIControllerState() : integral_error(0), previous_error(0) {}

    void reset() {
        integral_error = 0;
        previous_error = 0;
    }
};

/**
 * brief PI control golden reference model
 * tparam T            Floating-point type (default: float)
 * param output        Result Output
 * param state         Reference to a variable storing the controller state
 * param measured_value Measured process value, input
 * param setpoint      Setpoint
 * param kp            Proportional coefficient
 * param ki            Integral coefficient
 * param mode_change   is mode_change ? 1 : 0
 */
template <typename T = float>
void pi_control_golden(
    T& output, PIControllerState<T>& state, T measured_value, T setpoint, T kp, T ki, bool mode_change = false) {
    T err = setpoint - measured_value;
    T acc;
    if (mode_change == true)
        acc = 0;
    else
        acc = state.integral_error + err;
    T P = kp * err;
    T I = ki * acc;
    T sum = P + I;
    output = sum;
    state.previous_error = err;
    state.integral_error = acc;
}

} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
