/*
Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
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
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <stdint.h>

// clang-format off
/// all FOC Mode
enum FOC_Mode : int {
    // usr modes
    MOD_STOPPED = 0,
    MOD_SPEED_WITH_TORQUE,
    MOD_TORQUE_WITHOUT_SPEED,
    MOD_FLUX,
    // expert modes
    MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED,
    MOD_MANUAL_TORQUE_FLUX,
    MOD_MANUAL_TORQUE,
    MOD_MANUAL_FLUX,
    MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE,
	MOD_MANUAL_TORQUE_FLUX_AUTO_ANGLE,
    MOD_SENSORLESS,
	MOD_MANUAL_PWM,
    MOD_TOTAL_NUM
};

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

#define BIT_WIDTH_ACCUM 48
#define BIT_WIDTH_DATA 32
#define BIT_WIDTH_FRACTIONAL 16
#define BIT_WIDTH_STREAM_FOC 128
#define BIT_WIDTH_LOG_STREAM_FOC 512
#define BIT_WIDTH_STEP_STREAM 32
#define BIT_WIDTH_ADC 24

#define PWM_DATA_TYPE_ 16
#define PWM_DATA_TYPE_3_PHASE 96

#define MIN_IA_LOGGER 0
#define MAX_IA_LOGGER 31
#define MIN_IB_LOGGER 32
#define MAX_IB_LOGGER 63
#define MIN_THETA_LOGGER 64
#define MAX_THETA_LOGGER 95
#define MIN_RPM_LOGGER 96
#define MAX_RPM_LOGGER 127
#define MIN_IALPHA_LOGGER 128
#define MAX_IALPHA_LOGGER 159
#define MIN_IBETA_LOGGER 160
#define MAX_IBETA_LOGGER 191
#define MIN_IQ_LOGGER 192
#define MAX_IQ_LOGGER 223
#define MIN_ID_LOGGER 224
#define MAX_ID_LOGGER 255
#define MIN_VD_LOGGER 256
#define MAX_VD_LOGGER 287
#define MIN_VQ_LOGGER 288
#define MAX_VQ_LOGGER 319
#define MIN_VALPHA_LOGGER 320
#define MAX_VALPHA_LOGGER 351
#define MIN_VBETA_LOGGER 352
#define MAX_VBETA_LOGGER 383
#define MIN_VA_LOGGER 384
#define MAX_VA_LOGGER 415
#define MIN_VB_LOGGER 416
#define MAX_VB_LOGGER 447
#define MIN_VC_LOGGER 448
#define MAX_VC_LOGGER 479
#define MIN_ANGLE_EST_LOGGER 480
#define MAX_ANGLE_EST_LOGGER 511

#endif
