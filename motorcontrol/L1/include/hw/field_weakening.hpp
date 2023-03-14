/*
 * Copyright 2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _FIELD_WEAKENING_HPP_
#define _FIELD_WEAKENING_HPP_

#include <hls_math.h>
#include "common.hpp"

//--------------------------------------------------------------------------
// DECOUPLING
// Flux_decoup 	 = Flux + Coupling_factor_q
// Torque_decoup = Torque - Coupling_factor_d
// Where:
//		Coupling_factor_q = RPM * (Ls * Iq)
//		Coupling_factor_d = RPM * ((Ls * Id) + Psi_m)
//--------------------------------------------------------------------------
#ifdef __SYNTHESIS__
template <class T_NUM>
void Decoupling(T_NUM& Vd_decoupled, T_NUM& Vq_decoupled, T_NUM Id, T_NUM Iq, T_NUM Vd, T_NUM Vq, T_NUM RPM) {
    static T_NUM Ls = 0.000845; // Value for BLWR111D-24V-10000	Line-To-Line Inductance 1,69mH divided by 2
    T_NUM Psi_m = 0.0080144861;
    T_NUM Flux_decoup, Torque_decoup;

    Flux_decoup = Vd + RPM * (Ls * Iq);
    Torque_decoup = Vq - RPM * ((Ls * Id) + Psi_m);

    Vd_decoupled = (T_NUM)Flux_decoup;
    Vq_decoupled = (T_NUM)Torque_decoup;
}
#endif

/**
 * @brief Decoupling Flux from the Vd and Torque from the Vq
 * @tparam T_IN	    	Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID        Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_MID2		Type of the data in the middle of calculation. ex. ap_fixed<32, 16> is enough for Q16.16
 * @tparam T_OUT		Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @param Vd_decoupled  Direct component of the voltage decoupled
 * @param Vq_decoupled  Quadrature component of the voltage decoupled
 * @param Id    		Direct component of the current
 * @param Iq    		Quadrature component of the current
 * @param Vd    		Direct component of the voltage to decouple
 * @param Vq     		Quadrature component of the voltage to decouple
 * @param RPM     		Speed, in RPM
 */
template <class T_IN, class T_MID, class T_MID2, class T_OUT, int MAX_AD_SCL>
void Decoupling_T(T_OUT& Vd_decoupled, T_OUT& Vq_decoupled, T_MID Id, T_MID Iq, T_IN Vd, T_IN Vq, T_IN RPM) {
#pragma HLS INLINE off
    // static double Ls = 0.00022; // Value for BLWR111D-24V-10000	Line-To-Line Inductance 1,69mH divided by 2
    const T_MID2 Pi = 3.1415926535;
    const T_MID AD_scl = MAX_AD_SCL;
    const T_MID Imax = COMM_MOTOR_PARA_IMAX;
    const T_MID Umax = COMM_MOTOR_PARA_UMAX;
    const T_MID ppr = COMM_MACRO_PPR;
    const T_MID2 Ls = COMM_MOTOR_PARA_LD;
    const T_MID2 Psi_m = COMM_MOTOR_PARA_FAI_M;
    T_MID2 Ls_scl = Ls * Imax / Umax;
    T_MID2 Psi_m_scl = Psi_m / Umax * AD_scl;
    T_MID2 Flux_decoup, Torque_decoup;
    T_MID2 w = 2 * Pi * RPM / 60;

    Flux_decoup = Vd + ppr * w * (Ls_scl * Iq);
    Torque_decoup = Vq - ppr * w * ((Ls_scl * Id) + Psi_m_scl);

    Vd_decoupled = (T_OUT)Flux_decoup;
    Vd_decoupled = 0;
    Vq_decoupled = (T_OUT)Torque_decoup;
}

template <class T_IN, class T_MID, class T_MID2, class T_OUT, int MAX_AD_SCL>
void Decoupling_T_ap_fixed(
    T_OUT& Vd_decoupled, T_OUT& Vq_decoupled, T_MID Id, T_MID Iq, T_IN Vd, T_IN Vq, T_IN RPM, T_OUT RPM_to_speed) {
#pragma HLS INLINE
    // static double Ls = 0.00022; // Value for BLWR111D-24V-10000	Line-To-Line Inductance 1,69mH divided by 2
    // const T_MID2 Pi = 3.1415926535;
    // const T_MID ppr = COMM_MACRO_PPR;
    const T_MID2 Ls = COMM_MOTOR_PARA_LD;
    const T_MID2 Psi_m = COMM_MOTOR_PARA_FAI_M;
    T_MID2 Flux_decoup, Torque_decoup;
    // T_MID2 w = 2 * Pi * RPM / 60;

    Flux_decoup = Vd + RPM_to_speed * RPM * (Ls * Iq);
    Torque_decoup = Vq - RPM_to_speed * RPM * ((Ls * Id) + Psi_m);

    Vd_decoupled = Flux_decoup;
    Vd_decoupled = 0;
    Vq_decoupled = Torque_decoup;
}

//--------------------------------------------------------------------------
// Field Weakening
//
// Modulation index:
//		M_index = sqrt(Vd^2 + Vq^2)
//
// Modulation Threshold:
//		M_threshold = SVM_inv_index / (2*Vdc - V_reserve)
//
// M_error = M_threshold - M_index
//
// PI regualtor:
// 		FW_PI_out = Kp * M_error + Ki * (M_error)
//
//--------------------------------------------------------------------------

#ifdef __SYNTHESIS__

static float Sqrt2(float x) {
#pragma HLS INLINE

    float xhalf, res, x2, x3, x4;
#pragma HLS BIND_OP variable = xhalf op = fmul impl = dsp
#pragma HLS BIND_OP variable = x2 op = fmul impl = dsp
#pragma HLS BIND_OP variable = x3 op = fmul impl = dsp
#pragma HLS BIND_OP variable = x4 op = sub impl = dsp
#pragma HLS BIND_OP variable = res op = frecip impl = dsp

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

template <class T_NUM>
void Field_Weakening(
    T_NUM& Vd_weakened, T_NUM Vd_decoupled, T_NUM Vq_decoupled, T_NUM V_sensed, T_NUM SVM_index, T_NUM Kp, T_NUM Ki) {
    const T_NUM V_reserve = 0;
    T_NUM MAX_CURRENT = 0.78; // 0.88 from datasheet - 10%
    T_NUM M_index, M_threshold, FW_PI_out;

    M_index = sqrt(Vd_decoupled * Vd_decoupled + Vq_decoupled * Vq_decoupled); // Modulation Index
    M_threshold = (SVM_index / ((V_sensed << 1) - V_reserve));                 // Modulation Threshold

    static T_NUM ierror; // Variable for PI regulator result

    T_NUM error = M_threshold - M_index;
    ierror += error;
    FW_PI_out = Kp * error + Ki * ierror;
    if (FW_PI_out > MAX_CURRENT) {
        FW_PI_out = MAX_CURRENT;
    }
    if (FW_PI_out < -MAX_CURRENT) {
        FW_PI_out = -MAX_CURRENT;
    }
    Vd_weakened = (T_NUM)FW_PI_out;
}
#endif

/**
 * @brief Field Weakening
 * @tparam T_IN	    	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID        Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_MID2		Type of the data in the middle of calculation. ex. ap_fixed<32, 16> is enough for Q16.16
 * @tparam T_OUT		    Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @param Vd_weakened    	Voltage Direct component, the Weakening value needed to reach higher velocity
 * @param Vd_decoupled      Direct component of the voltage decoupled
 * @param Vq_decoupled      Quadrature component of the voltage decoupled
 * @param V_sensed          Voltage sensed on the DC bus
 * @param SVM_index		    Maximum positive Phase Voltage (A or B or C) actual activated
 * @param Kp                Proportional coefficient for the PI Regulator of the Field Weakening
 * @param Ki                Integral coefficient for the PI Regulator of the Field Weakening
 */
template <class T_IN, class T_MID, class T_MID2>
void Field_Weakening_T(
    T_MID& M_index, T_MID& M_threshold, T_IN Vd_decoupled, T_IN Vq_decoupled, T_IN V_sensed, T_MID SVM_index) {
#pragma HLS INLINE off
    const T_IN V_reserve = 0;
    // const T_MID MAX_CURRENT = MAX_AD_SCL>>1; // 0.88 from datasheet - 10%
    // T_MID M_index, M_threshold, FW_PI_out;
    T_MID FW_PI_out;

    // M_index = hls::sqrt(Vd_decoupled * Vd_decoupled + Vq_decoupled * Vq_decoupled); // Modulation Index

    ap_fixed<48, 32> squareSum = Vd_decoupled * Vd_decoupled + Vq_decoupled * Vq_decoupled;
    float temp;
#pragma HLS BIND_OP variable = temp op = fsqrt impl = dsp
    temp = std::sqrt(squareSum.to_float());
    M_index = temp;

    //     ap_fixed< 32, 16> tmp_mul1;
    // #pragma HLS BIND_OP variable=tmp_mul1 op=mul impl=dsp
    //     tmp_mul1 = Vd_decoupled * Vd_decoupled;
    //     ap_fixed< 32, 16> tmp_mul2;
    // #pragma HLS BIND_OP variable=tmp_mul2 op=mul impl=dsp
    //     tmp_mul2 = Vq_decoupled * Vq_decoupled;

    //     ap_fixed< 32, 16> tmp_add1;
    // #pragma HLS BIND_OP variable=tmp_add1 op=add impl=dsp
    //     tmp_add1 = tmp_mul1 + tmp_mul2;

    //     M_index = sqrt_fixed_pipelineOff<32, 16> (tmp_add1);
    //     //M_index = hls::sqrt(tmp_mul1); // Modulation Index
    // #pragma HLS BIND_OP variable=M_threshold op=mul impl=dsp
    // #pragma HLS BIND_OP variable=M_threshold op=add impl=dsp

    M_threshold = (SVM_index / ((V_sensed << 1) - V_reserve)); // Modulation Threshold

    // pid has move outside
}

#endif // _FIELD_WEAKENING_HPP_