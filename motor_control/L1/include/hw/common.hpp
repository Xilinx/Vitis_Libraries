/*
Copyright (C) 2022-2022, Xilinx, Inc.
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
#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <math.h>

/*
* The following are the global defination for static MACROs, types and parameters
* which are tightly coupled in a motor control system.
* There are also some local definations of MACRO, types and parameter which
* are already aligned with the global definations in testing demonstrations
*/

// Important: Change the sine and cosine tables in the file foc.h accordingly when changing this CPR(COMM_MACRO_CPR).
/// Number of encoder steps per one full revolution.
#define COMM_MACRO_CPR (1000)

// Number of pole pairs per phase of the motor; full sinus periods per revolution.
// change this value to 1 could directly simulation the result under current Parameters (Motor, PID's kp,ki,kd...)
#define COMM_MACRO_PPR (2)

// Number of sin-cos table for 2*pi.
#define COMM_MACRO_TLB_LENTH (1000)
//
#define COMM_CLOCK_FREQ (100000000)

#define COMM_ADC_WIDTH (16) // to be cleaned

// Motor parameters
#define COMM_MOTOR_PARA_LD (0.000845)

#define COMM_MOTOR_PARA_RS (4.63)

#define COMM_MOTOR_PARA_FAI_M (0.008015)

#define COMM_MOTOR_PARA_J (0.000033 * 28.349 * 0.001 * 9.8 * 0.0254)

#define COMM_MOTOR_PARA_TL_TH (0)

#define COMM_MOTOR_PARA_DT_SIM (0.0000005)

#define COMM_MOTOR_PARA_UMAX (24) // to be uniformed with MAX_VAL_PWM

#define COMM_MOTOR_PARA_IMAX (2.7) // stall current limit which is ~2.7A per the motor datasheet

// Static Design parameters
/// Max value of the PWM voltage, 6~10kV.
#define MAX_VAL_PWM (24)
/// W parameter, word length in bits for ap_fixed version, more details to ug1399.
#define PWM_AP_FIXED_PARA_W2 (24)
/// I parameter, number of bits used to represent the integer value for ap_fixed version.
#define PWM_AP_FIXED_PARA_I2 (8)
#define COMM_W (PWM_AP_FIXED_PARA_W2)
#define COMM_I (PWM_AP_FIXED_PARA_I2)
/// Data Type for data exchange of Voltage from FOC to SVPWM and Current from ADC to FOC
typedef ap_fixed<PWM_AP_FIXED_PARA_W2, PWM_AP_FIXED_PARA_I2> t_glb_foc2pwm;

/// Data Type for the data exchange of the speed and theta from QEI or others.
typedef ap_uint<32> t_glb_speed_theta;

/// Data Type for the data buffer in calculation.
typedef ap_fixed<32, 16> t_glb_q15q16;

/// Data Type for the smo discrete sample time
typedef ap_fixed<32, 1> t_glb_smo_time;

template <class T>
struct RangeDef {
    T min;
    T max;
    T dft;
};

template <class T>
bool CheckRange(T& v, RangeDef<T>& rg) {
#pragma HLS INLINE
    if (v < rg.min || v > rg.max) {
        v = rg.dft;
        return false;
    }
    return true;
}

#endif
