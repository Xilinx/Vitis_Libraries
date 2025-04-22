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
#ifndef _FIELD_WEAKENING_HPP_
#define _FIELD_WEAKENING_HPP_

#include <hls_math.h>
#include "common.hpp"

//--------------------------------------------------------------------------
// DECOUPLING
// Flux_decoup 	 = Vd + speed * (Ls * Iq)
// Torque_decoup = Vq - speed * ((Ls * Id) + FAI_M)
//--------------------------------------------------------------------------
/**
 * brief Decoupling Flux from the Vd and Torque from the Vq
 * tparam T_IN	    	Type of the input data. ex. ap_fixed<32,16> is fit for Q16.16
 * tparam T_MID        Type of the data in the middle of calculation.
 * tparam T_MID2		Type of the data in the middle of calculation.
 * tparam T_OUT		Type of the output data.
 * param Vd_decoup     Direct component of the voltage decoupled
 * param Vq_decoup     Quadrature component of the voltage decoupled
 * param Id    		Direct component of the current
 * param Iq    		Quadrature component of the current
 * param Vd    		Direct component of the voltage to decouple
 * param Vq     		Quadrature component of the voltage to decouple
 * param RPM     		Speed, in RPM
 * param RPM_to_speed  Constant factor for conversion
 */
template <class T_IN, class T_MID, class T_MID2, class T_OUT, int MAX_AD_SCL>
void Decoupling_T_ap_fixed(
    T_OUT& Vd_decoup, T_OUT& Vq_decoup, T_MID Id, T_MID Iq, T_IN Vd, T_IN Vq, T_IN RPM, T_OUT RPM_to_speed) {
#pragma HLS INLINE

    const T_MID2 Ls = COMM_MOTOR_PARA_LD;
    const T_MID2 FAI_M = COMM_MOTOR_PARA_FAI_M;
    T_MID2 Flux_decoup, Torque_decoup;

    Flux_decoup = Vd + RPM_to_speed * RPM * (Ls * Iq);
    Torque_decoup = Vq - RPM_to_speed * RPM * ((Ls * Id) + FAI_M);

    Vd_decoup = Flux_decoup;
    Vd_decoup = 0;
    Vq_decoup = Torque_decoup;
}

//--------------------------------------------------------------------------
// Field Weakening
//
// Modulation:
//		Modulation = sqrt(Vd^2 + Vq^2)
//
// Modulation set point:
//		Modulation_sp = Max_SVM / (2*Vdc - V_reserve)
//
// Modulation_error = Modulation_sp - Modulation
//
// PID for Field Weakening:
// 		PID_out = Kp * Modulation_error + Ki * (Modulation_error)
//
//--------------------------------------------------------------------------

#ifdef __SYNTHESIS__

static float Sqrt2(float x) {
#pragma HLS INLINE

    float xhalf, res, x2, x3, x4;

    xhalf = 0.5f * x;
    int i = *(int*)&x;         // get bits for floating VALUE
    i = 0x5f3759df - (i >> 1); // gives initial guess y0
    x = *(float*)&i;           // convert bits BACK to float

    x2 = x * x;
    x3 = xhalf * x2;
    x4 = (1.5f - x3);
    x = x * x4;

    x2 = x * x;
    x3 = xhalf * x2;
    x4 = (1.5f - x3);
    x = x * x4;

    res = 1.0 / x;
    return res;
}

#endif

/**
 * @brief Field Weakening
 * @tparam T_IN	    	    Type of the input data. ex. ap_fixed<32,16> is fit for Q16.16
 * @tparam T_MID            Type of the data in the middle of calculation.
 * @tparam T_MID2		    Type of the data in the middle of calculation.
 * @param Modulation        Modulation, Euclidean distance of the Vd_decoup and Vq_decoup
 * @param Modulation_sp    	Setpoint for the Modulation
 * @param Vd_decoup         Direct component of the voltage decoupled
 * @param Vq_decoup         Quadrature component of the voltage decoupled
 * @param Volt_sensord      Voltage sensed on the DC bus
 * @param Max_SVM		    Positive Phase Voltage (A or B or C) actual activated, now is [MAX_Volt]/2
 */
template <class T_IN, class T_MID, class T_MID2>
void Field_Weakening_T(
    T_MID& Modulation, T_MID& Modulation_sp, T_IN Vd_decoup, T_IN Vq_decoup, T_IN Volt_sensord, T_MID Max_SVM) {
#pragma HLS INLINE off
    const T_IN V_reserve = 0;
    // const T_MID MAX_CURRENT = MAX_AD_SCL>>1; // 0.88 from datasheet - 10%
    // T_MID Modulation, Modulation_sp, FW_PI_out;
    T_MID FW_PI_out;
    ap_fixed<48, 32> squareSum = Vd_decoup * Vd_decoup + Vq_decoup * Vq_decoup;
    float temp;
    temp = std::sqrt(squareSum.to_float());
    Modulation = temp;
    Modulation_sp = (Max_SVM / ((Volt_sensord << 1) - V_reserve)); // Modulation Threshold

    // pid has move outside
}

#endif // _FIELD_WEAKENING_HPP_
