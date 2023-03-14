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
#ifndef _SMO_HPP_
#define _SMO_HPP_
#include <hls_stream.h>
#include <ap_int.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ap_fixed.h>

#include "utils.hpp"
#include "pid_control.hpp"
#include <math.h>

namespace xf {
namespace motorcontrol {
namespace details {
template <class T_IO, class T_SINCOS, class T_Mode>
void Control_foc_ap_fixed(T_IO& Vd,
                          T_IO& Vq,
                          T_SINCOS& cos_out,
                          T_SINCOS& sin_out,
                          T_Mode FOC_mode,
                          T_SINCOS cos_gen_angle,
                          T_SINCOS sin_gen_angle,
                          T_SINCOS cos_in,
                          T_SINCOS sin_in,
                          T_IO Flux_out,
                          T_IO Torque_out,
                          T_IO args_vd,
                          T_IO args_vq);
}
/**
 * @brief  Args for SMO top
 * @tparam T_I	        Type of the Current input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_V   	    Type of the Voltage input data.
 * @tparam T_RPM	    Type of the RPM input data.
 * @tparam T_ANGLE		Type of the Angle.
 * @tparam T_MID	    Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 */
template <class T_V, class T_I, class T_MID, class T_ANGLE, class T_RPM>
class SMO_Observer {
   public:
    T_I Ialpha_est;
    T_I Ibeta_est;
    T_V bemf_alpha;
    T_V bemf_beta;
    T_ANGLE Theta_est;
    T_RPM rpm_est;
    SMO_Observer() { Ialpha_est = Ibeta_est = 0; }
    void EstimatingCurrent(T_V Valpha, T_V Vbeta, T_I Ialpha, T_I Ibeta) {}

    void updatingBEMF() {}

    T_ANGLE updatingTheta() { return Theta_est; }

    T_RPM updatingRPM() { return rpm_est; }
};
template <class T_V, class T_I, class T_MID, class T_ANGLE, class T_RPM>
void smo_control_T(SMO_Observer<T_V, T_I, T_MID, T_ANGLE, T_RPM>& smoer,
                   T_V Valpha,
                   T_V Vbeta,
                   T_I Ialpha,
                   T_I Ibeta,
                   T_ANGLE& angle_o,
                   T_RPM& rpm_o

                   ) {
    smoer.EstimatingCurrent();
    smoer.updatingBEMF();
    angle_o = smoer.updatingTheta();
    rpm_o = smoer.updatingRPM();
};

template <int VALUE_CPR, class T_IO, int MAX_IO>
void smo_ap_fixed(
    // Input
    T_IO Ia,
    T_IO Ib,
    T_IO Ic,
    T_IO Va,
    T_IO Vb,
    T_IO Vc,
    // Output
    short& RPM,
    short& Angle

    ) {
    const int W_pid = 32;
    const int W_sin = 16;

    typedef ap_fixed<40, 18, AP_RND, AP_WRAP> t_mid2; //
    typedef ap_fixed<W_sin, 1, AP_TRN, AP_WRAP> t_sincos;
    typedef ap_fixed<W_pid, 16, AP_TRN, AP_WRAP> t_pid;   // just a coefficients
    typedef ap_fixed<32, 1, AP_TRN, AP_SAT> t_motor_para; //
    typedef ap_fixed<32, 5, AP_TRN, AP_SAT> t_angle;      //

    static t_mid2 ialpha_est = 0;
    static t_mid2 ibeta_est = 0;
    static t_angle angle_e_est = 0;
    static t_glb_q15q16 w_e_est = 0; //

    const short N = COMM_MACRO_PPR;                     //	Number of pole pairs
    const t_motor_para Ld = COMM_MOTOR_PARA_LD;         //	stator d-axis inductance
    const t_motor_para Lq = COMM_MOTOR_PARA_LD;         //	stator q-axis inductance
    const t_glb_q15q16 Rs = COMM_MOTOR_PARA_RS;         //	resistance
    const t_motor_para fai_m = COMM_MOTOR_PARA_FAI_M;   //	permanent magnet flux linkage
    const t_motor_para dt_sim = COMM_MOTOR_PARA_DT_SIM; //	minmal time step for simulation
    const t_angle PI = 3.1415926535;                    //
    // const int table_length = 65536;
    const int table_length = VALUE_CPR / N;

    // if (angle_e_est > 2 * PI) angle_e_est = angle_e_est - 2 * PI;
    int Theta = (int)(angle_e_est / 2 / PI * table_length);    // Apply angle correction
    Theta = (Theta < 0) ? (int)(Theta + table_length) : Theta; // Correct negative angle
    Theta = (Theta >= table_length) ? (int)(Theta - table_length) : Theta;

    int Theta2 = Theta + 1;
    if (Theta2 == table_length) Theta2 = 0;

    // clang-format off
    T_IO Ialpha, Ibeta, Ihomopolar; // no promotion
    Clarke_Direct_3p_ap_fixed(
        Ialpha, 
        Ibeta, 
        Ihomopolar, 
        Ia, 
        Ib, 
        Ic);

    T_IO Ualpha, Ubeta, Uhomopolar; // no promotion
    Clarke_Direct_3p_ap_fixed(
        Ualpha, 
        Ubeta, 
        Uhomopolar, 
        Va, 
        Vb, 
        Vc);
    // clang-format on

    t_glb_q15q16 k = 100;             ////
    t_glb_q15q16 h = 2 * MAX_VAL_PWM; // enough bandwidth for field weakening
    t_mid2 valpha = k * (ialpha_est - Ialpha);
    t_mid2 vbeta = k * (ibeta_est - Ibeta);
    valpha = Clip_AP<t_mid2>(valpha, (0 - h), h);
    vbeta = Clip_AP<t_mid2>(vbeta, (0 - h), h);

    // t_sincos cos_theta; cos_theta(15, 0) = (t_sincos)cos((double)(angle_e_est));//cos_table[Theta];//
    // t_sincos sin_theta; sin_theta(15, 0) = (t_sincos)sin((double)(angle_e_est));//sin_table[Theta];//
    // t_sincos cos_theta; cos_theta(15, 0) = cos_table[Theta];//(t_sincos)cos((double)(angle_e_est));//
    // t_sincos sin_theta; sin_theta(15, 0) = sin_table[Theta];//(t_sincos)sin((double)(angle_e_est));//
    short k_cos = cos_table[Theta2] - cos_table[Theta];
    short k_sin = sin_table[Theta2] - sin_table[Theta];
    t_angle delta_theta = angle_e_est / 2 / PI * table_length - Theta;
    t_sincos cos_theta;
    cos_theta(15, 0) = cos_table[Theta] + (short)(k_cos * delta_theta);
    t_sincos sin_theta;
    sin_theta(15, 0) = sin_table[Theta] + (short)(k_sin * delta_theta);

    t_mid2 Ea_cos = valpha * cos_theta;
    t_mid2 Eb_sin = vbeta * sin_theta;

    static t_mid2 Speed_GiE_prev = 0;
    static t_mid2 Speed_Err_prev = 0;
    t_mid2 Speed_pid_dout;
    t_mid2 kp = 30000;
    t_mid2 ki = 0;
    // clang-format off
    PID_Control_ap_fixed<t_mid2, t_mid2, t_mid2, t_mid2>(
        Speed_pid_dout, 
        Speed_GiE_prev, 
        Speed_Err_prev,
        Eb_sin,     // data in
        0 - Ea_cos, // sp
        kp, 
        ki, 
        0, 
        false);
    // clang-format on

    t_mid2 dialpha_est_div_dt = (-Rs * ialpha_est - w_e_est * (Ld - Lq) * ibeta_est + Ualpha - valpha) / Ld;
    t_mid2 dibeta_est_div_dt = (w_e_est * (Ld - Lq) * ialpha_est - Rs * ibeta_est + Ubeta - vbeta) / Ld;

    ialpha_est = ialpha_est + dt_sim * dialpha_est_div_dt;
    ibeta_est = ibeta_est + dt_sim * dibeta_est_div_dt;
    angle_e_est = angle_e_est + dt_sim * w_e_est;
    if (angle_e_est > 2 * PI) angle_e_est = angle_e_est - 2 * PI;
    w_e_est = Speed_pid_dout;

    RPM = (short)(Speed_pid_dout / 2 / PI * 60 / N);
    Angle = (short)(angle_e_est / 2 / PI * VALUE_CPR / N);
}

template <int VALUE_CPR, class T_IO, int MAX_IO>
void smo_in_foc_ap_fixed(
    // Input
    T_IO Ialpha,
    T_IO Ibeta,
    T_IO Va,
    T_IO Vb,
    T_IO Vc,
    // Angle translation parameters
    T_IO speed_to_RPM,
    // Output
    short& RPM,
    short& Angle_e

    ) {
    typedef ap_fixed<40, 18, AP_RND, AP_WRAP> t_mid2;
    typedef ap_fixed<16, 1, AP_TRN, AP_WRAP> t_sincos;
    typedef ap_fixed<32, 1, AP_TRN, AP_SAT> t_motor_para;
    typedef ap_fixed<32, 5, AP_TRN, AP_SAT> t_angle;
    typedef ap_fixed<20, 18, AP_RND, AP_WRAP> t_pid;

    const int W_speed = 32;
    const int I_speed = 16;
    const int W_speedE = W_speed + 1;
    const int I_speedE = I_speed + 1;
    const int W_speedI = W_speed + 10;
    const int I_speedI = I_speed + 10;
    typedef ap_fixed<W_speed, I_speed, AP_TRN, AP_WRAP> t_speed;
    typedef ap_fixed<W_speedE, I_speedE, AP_TRN, AP_SAT> t_speedE;
    typedef ap_fixed<W_speedI, I_speedI, AP_TRN, AP_SAT> t_speedI;

    static t_mid2 ialpha_est = 0;
    static t_mid2 ibeta_est = 0;
    static t_angle angle_e_est = 0;
    static t_glb_q15q16 w_e_est = 0;

    // const short N = COMM_MACRO_PPR;                     //	Number of pole pairs
    const t_motor_para Ld = COMM_MOTOR_PARA_LD;         //	stator d-axis inductance
    const t_motor_para Lq = COMM_MOTOR_PARA_LD;         //	stator q-axis inductance
    const t_glb_q15q16 Rs = COMM_MOTOR_PARA_RS;         //	resistance
    const t_motor_para fai_m = COMM_MOTOR_PARA_FAI_M;   //	permanent magnet flux linkage
    const t_motor_para dt_sim = COMM_MOTOR_PARA_DT_SIM; //	minmal time step for simulation
    const t_angle PI = 3.1415926535;                    //
    const t_angle PI_2 = 2 * PI;
    const short table_length = COMM_MACRO_TLB_LENTH;

    // t_glb_q15q16 const_RPM = 60 / (double)PI_2 / N;
    const t_glb_q15q16 const_angle = table_length / (double)PI_2;

    static short Theta_pre = 0;
    if (Va == 0 && Vb == 0 && Vc == 0) {
        ialpha_est = 0;
        ibeta_est = 0;
        Theta_pre = 0;
        angle_e_est = 0;
        w_e_est = 0;
    }

    short Theta = Theta_pre; // (short)(angle_e_est/2/PI*table_length);              // Apply angle correction
    Theta = (Theta < 0) ? (short)(Theta + table_length) : Theta; // Correct negative angle
    Theta = (Theta >= table_length) ? (short)(Theta - table_length) : Theta;

    short Theta2 = Theta + 1;
    if (Theta2 == table_length) Theta2 = 0;

    // T_IO Ialpha, Ibeta, Ihomopolar;// no promotion
    // Clarke_Direct_3p_ap_fixed(
    //     Ialpha,
    //     Ibeta,
    //     Ihomopolar,
    //     Ia,
    //     Ib,
    //     Ic);

    // clang-format off
    //--------------------------------------------------------------------------
    T_IO Ualpha, Ubeta, Uhomopolar; // no promotion
    Clarke_Direct_3p_ap_fixed(
        Ualpha, 
        Ubeta, 
        Uhomopolar, 
        Va, 
        Vb, 
        Vc);
    // clang-format on

    RANGETRACER("SMO.CLARK.Ualpha", Ualpha);
    RANGETRACER("SMO.CLARK.Ubeta", Ubeta);

    const T_IO k = 100;             ////
    const T_IO h = 2 * MAX_VAL_PWM; // enough bandwidth for field weakening
    T_IO valpha = k * (ialpha_est - Ialpha);
    T_IO vbeta = k * (ibeta_est - Ibeta);
    valpha = Clip_AP<T_IO>(valpha, (0 - h), h);
    vbeta = Clip_AP<T_IO>(vbeta, (0 - h), h);

    short k_cos = cos_table[Theta2] - cos_table[Theta];
    short k_sin = sin_table[Theta2] - sin_table[Theta];
    t_angle delta_theta = angle_e_est * const_angle - Theta;
    t_sincos cos_theta;
    cos_theta(15, 0) = cos_table[Theta] + (short)(k_cos * delta_theta);
    t_sincos sin_theta;
    sin_theta(15, 0) = sin_table[Theta] + (short)(k_sin * delta_theta);

    t_speed Ea_cos = valpha * cos_theta;
    t_speed Eb_sin = vbeta * sin_theta;

    static t_speedI Speed_GiE_prev = 0;
    static t_speedE Speed_Err_prev = 0;
    t_speed Speed_pid_dout;
    t_pid kp = 40000;
    t_pid ki = 0;
    // clang-format off
    PID_Control_ap_fixed<t_speed, t_speedI, t_speedE, t_pid>(
        Speed_pid_dout, 
        Speed_GiE_prev, 
        Speed_Err_prev,
        Eb_sin,       // data in
        (0 - Ea_cos), // sp
        kp, 
        ki, 
        0, 
        false);
    // clang-format on

    const t_glb_q15q16 Rs_Ld = Rs / Ld;
    const t_glb_q15q16 Ld_Lq = (Ld - Lq) / Ld;
    const t_glb_q15q16 Ld_1 = 1.0 / (double)Ld;
    t_mid2 dialpha_est_div_dt, dibeta_est_div_dt;
    dialpha_est_div_dt = -Rs_Ld * ialpha_est - w_e_est * Ld_Lq * ibeta_est + (Ualpha - valpha) * Ld_1;
    dibeta_est_div_dt = w_e_est * Ld_Lq * ialpha_est - Rs_Ld * ibeta_est + (Ubeta - vbeta) * Ld_1;

    ialpha_est = ialpha_est + dt_sim * dialpha_est_div_dt;
    ibeta_est = ibeta_est + dt_sim * dibeta_est_div_dt;
    angle_e_est = angle_e_est + dt_sim * w_e_est;
    if (angle_e_est > PI_2) angle_e_est = angle_e_est - PI_2; //
    w_e_est = Speed_pid_dout;

    RPM = (short)(Speed_pid_dout * speed_to_RPM);
    Angle_e = (short)(angle_e_est * const_angle); // Theta;

    Theta_pre = Angle_e;
}

template <int VALUE_CPR, class T_IO, int MAX_IO, int W, int I>
void foc_core_ap_fixed_sensorless(
    // Input
    T_IO Ia,            // Phase A current
    T_IO Ib,            // Phase B current
    T_IO Ic,            // Phase B current
    short RPM_sensor,   // RPM
    short Angle_sensor, // Encoder count
    // Output for GPIO
    T_IO& Va_cmd,
    T_IO& Vb_cmd,
    T_IO& Vc_cmd,
    // Angle translation parameters
    int tab_map_factor,
    short cpr_div_ppr,
    T_IO speed_to_RPM,
    T_IO RPM_to_speed,
    // Inout put for parameters
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    // the following are all representated in q15q16 format
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& trigger_args,
    // int format
    volatile int& control2_args,
    // the following are all representated in q15q16 format
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    // the following are all representated in q15q16 format
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts) {
#pragma HLS INLINE off
#pragma HLS BIND_STORAGE variable = sin_table type = RAM_2P impl = BRAM
#pragma HLS BIND_STORAGE variable = cos_table type = RAM_2P impl = BRAM
    //#pragma HLS pipeline enable_flush

    const int W_pid = 16;
    const int W_sin = 16;

    typedef short t_angle;
    typedef ap_uint<4> t_mode;
    typedef ap_fixed<32, 16, AP_RND, AP_WRAP> t_mid2;
    typedef ap_fixed<W_sin, 1, AP_TRN, AP_WRAP> t_sincos;
    typedef ap_fixed<W_pid, 8, AP_TRN, AP_WRAP> t_pid; // just a coefficients
    typedef ap_fixed<W + 8, I + 8, AP_TRN, AP_WRAP> t_mid;
    // typedef ap_fixed<48, 32> t_mid2;

    //--------------------------------------------------------------------------
    const int W_RPM = 16;
    const int I_RPM = 16;
    const int W_RPME = W_RPM + 1;
    const int I_RPME = I_RPM + 1;
    const int W_RPMI = W_RPM + 10;
    const int I_RPMI = I_RPM + 10;
    typedef ap_fixed<W_RPM, I_RPM, AP_TRN, AP_WRAP> t_RPM;
    typedef ap_fixed<W_RPME, I_RPME, AP_TRN, AP_SAT> t_RPME;
    typedef ap_fixed<W_RPMI, I_RPMI, AP_TRN, AP_SAT> t_RPMI;

    const int W_IQ = 24;
    const int I_IQ = 16;
    const int W_IQE = W_IQ + 1;
    const int I_IQE = I_IQ + 1;
    const int W_IQI = W_IQ + 10;
    const int I_IQI = I_IQ + 10;
    typedef ap_fixed<W_IQ, I_IQ, AP_TRN, AP_WRAP> t_IQ;
    typedef ap_fixed<W_IQE, I_IQE, AP_TRN, AP_SAT> t_IQE;
    typedef ap_fixed<W_IQI, I_IQI, AP_TRN, AP_SAT> t_IQI;

    const int W_ID = 24;
    const int I_ID = 16;
    const int W_IDE = W_ID + 1;
    const int I_IDE = I_ID + 1;
    const int W_IDI = W_ID + 10;
    const int I_IDI = I_ID + 10;
    typedef ap_fixed<W_ID, I_ID, AP_TRN, AP_WRAP> t_ID;
    typedef ap_fixed<W_IDE, I_IDE, AP_TRN, AP_SAT> t_IDE;
    typedef ap_fixed<W_IDI, I_IDI, AP_TRN, AP_SAT> t_IDI;

    // clang-format off
    t_glb_q15q16 apx_flux_sp_args;
    apx_flux_sp_args(31, 0) = flux_sp_args;
    t_glb_q15q16 apx_flux_kp_args;
    apx_flux_kp_args(31, 0) = flux_kp_args;
    t_glb_q15q16 apx_flux_ki_args;
    apx_flux_ki_args(31, 0) = flux_ki_args;
    t_glb_q15q16 apx_flux_kd_args;
    apx_flux_kd_args(31, 0) = flux_kd_args;
    t_glb_q15q16 apx_torque_sp_args;
    apx_torque_sp_args(31, 0) = torque_sp_args;
    t_glb_q15q16 apx_torque_kp_args;
    apx_torque_kp_args(31, 0) = torque_kp_args;
    t_glb_q15q16 apx_torque_ki_args;
    apx_torque_ki_args(31, 0) = torque_ki_args;
    t_glb_q15q16 apx_torque_kd_args;
    apx_torque_kd_args(31, 0) = torque_kd_args;
    t_glb_q15q16 apx_speed_sp_args;
    apx_speed_sp_args(31, 0) = speed_sp_args;
    t_glb_q15q16 apx_speed_kp_args;
    apx_speed_kp_args(31, 0) = speed_kp_args;
    t_glb_q15q16 apx_speed_ki_args;
    apx_speed_ki_args(31, 0) = speed_ki_args;
    t_glb_q15q16 apx_speed_kd_args;
    apx_speed_kd_args(31, 0) = speed_kd_args;
    t_glb_q15q16 apx_angle_sh_args;
    apx_angle_sh_args(31, 0) = angle_sh_args;
    t_glb_q15q16 apx_vd_args;
    apx_vd_args(31, 0) = vd_args;
    t_glb_q15q16 apx_vq_args;
    apx_vq_args(31, 0) = vq_args;
    t_glb_q15q16 apx_fw_kp_args;
    apx_fw_kp_args(31, 0) = fw_kp_args;
    t_glb_q15q16 apx_fw_ki_args;
    apx_fw_ki_args(31, 0) = fw_ki_args;
    // clang-format on

    // static T_Vabc SVM_inv_index = MAX_LIM >> 1;
    // short V_fw = 1;
    static t_RPMI Speed_GiE_prev = 0;
    static t_RPME Speed_Err_prev = 0;

    static t_ID Vd_weakened = 0; // case FW

    static t_IDI Flux_GiE_prev = 0; // Variable for previous integral value
    static t_IDE Flux_Err_prev = 0; // Variable for previous derivative value

    static t_IQI Torque_GiE_prev = 0; // Variable for previous integral value
    static t_IQE Torque_Err_prev = 0; // Variable for previous derivative value

    static t_IDI FWiE_prev = 0;
    static t_IDE FW_err_prev = 0;

    static int gen_delay = 0;     // Generator period counter
    static t_angle gen_angle = 0; // Generator angle counter
    //--------------------------------------------------------------------------
    // load args
    //--------------------------------------------------------------------------
    static t_mode Mode_Prev = MOD_STOPPED; // Previous control Register.
    t_mode FOC_mode = control_mode_args;
    /*
    if(FOC_mode == MOD_STOPPED){
        Speed_GiE_prev = 0;
        Speed_Err_prev = 0;

        Vd_weakened = 0;// case FW

        Flux_GiE_prev = 0; // Variable for previous integral value
        Flux_Err_prev = 0; // Variable for previous derivative value

        Torque_GiE_prev = 0; // Variable for previous integral value
        Torque_Err_prev = 0; // Variable for previous derivative value

        FWiE_prev = 0;
        FW_err_prev = 0;

        gen_delay = 0;   // Generator period counter
        gen_angle = 0;   // Generator angle counter

        Mode_Prev = MOD_STOPPED;
        return;
    }*/
    bool Mode_Change = (FOC_mode != Mode_Prev) || (FOC_mode == MOD_STOPPED);
    Mode_Prev = FOC_mode;

    int FixPeriod = control_fixperiod_args;

    // clang-format off
    //--------------------------------------------------------------------------
    T_IO Ialpha, Ibeta, Ihomopolar; // Transfom result
    Clarke_Direct_3p_ap_fixed(
        Ialpha, 
        Ibeta, 
        Ihomopolar, 
        Ia,
        Ib, 
        Ic);

    // SMO module---------------------------------------------------------------
    short RPM_sensorless, Angle_sensorless;
    static T_IO Va = 0, Vb = 0, Vc = 0;
    smo_in_foc_ap_fixed<VALUE_CPR, T_IO, MAX_IO>(
        // Input
        Ialpha,
        Ibeta,
        Va,
        Vb,
        Vc,
        // Angle translation parameters
        speed_to_RPM,
        // Output
        RPM_sensorless,
        Angle_sensorless
    );
    //--------------------------------------------------------------------------

    // short RPM = RPM_sensor;
    short Angle = Angle_sensor;

    //--------------------------------------------------------------------------
    // Angle
    //--------------------------------------------------------------------------
    // Generated angle period = FixPeriod * latency / freq
    // FixPeriod = angle period * freq / latency
    //static int gen_delay = 0;   // Generator period counter
    //static t_angle gen_angle = 0;   // Generator angle counter
                                  // Simple angle generator for manual mode
                                  // The motor should rotate regardless of the encoder output
    if (gen_delay >= FixPeriod) { // Period loop
        gen_delay = 0;
        if (gen_angle >= (VALUE_CPR - 1)) { // Angle loop
            gen_angle = 0;
        } else {
            ++gen_angle;
        }
    } else {
        ++gen_delay;
    }
    // t_sincos cos_gen_angle; cos_gen_angle(15, 0) = cos_table[Theta];
    // t_sincos sin_gen_angle; sin_gen_angle(15, 0) = sin_table[Theta];

    t_angle Theta = Angle - angle_sh_args;                             // Apply angle correction
    Theta = (FOC_mode == MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED) ? gen_angle : Theta;
    Theta = (Theta < 0) ? (short)(Theta + VALUE_CPR) : Theta;          // Correct negative angle
    Theta = (Theta >= VALUE_CPR) ? (short)(Theta - VALUE_CPR) : Theta; // Correct angle overload to (0, CPR)
    t_angle Q = (Theta / cpr_div_ppr);                                 // Correct angle overload to (0. cpr_div_ppr)
    Theta = Theta - Q * cpr_div_ppr;
    Theta = (tab_map_factor * Theta) >> 16;
    RANGETRACER("FOC.Theta", (float)Theta);
    
    short RPM = RPM_sensorless;
    Theta = Angle_sensorless;
    
    //--------------------------------------------------------------------------
    T_IO Id, Iq;
    t_sincos cos_theta; cos_theta(15, 0) = cos_table[Theta];
    t_sincos sin_theta; sin_theta(15, 0) = sin_table[Theta];
    #pragma HLS BIND_STORAGE variable=sin_table type=RAM_2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=cos_table type=RAM_2P impl=BRAM
    Park_Direct_ap_fixed<T_IO, t_sincos>(
        Id, 
        Iq, 
        Ialpha, 
        Ibeta, 
        cos_theta,
        sin_theta);
    RANGETRACER("FOC.PARK.Id", Id);
    RANGETRACER("FOC.PARK.Iq", Iq);

    t_RPM         Speed_pid_din  = RPM;
    t_RPM         Speed_pid_dout;
    PID_Control_ap_fixed<t_RPM, t_RPMI, t_RPME, t_pid>(
        Speed_pid_dout, 
        Speed_GiE_prev, 
        Speed_Err_prev, 
        Speed_pid_din, 
        apx_speed_sp_args,
        apx_speed_kp_args,
        apx_speed_ki_args,
        apx_speed_kd_args,
        Mode_Change);
    RANGETRACER("FOC.PID.Speed_GiE_prev", Speed_GiE_prev);
    RANGETRACER("FOC.PID.Speed_pid_dout", Speed_pid_dout);
    RANGETRACER("FOC.PID.Speed_GiE_prev", Speed_GiE_prev);
    RANGETRACER("FOC.PID.Speed_Err_prev", Speed_Err_prev);
    RANGETRACER("FOC.PID.Speed_pid_din", Speed_pid_din);
    RANGETRACER("FOC.PID.apx_speed_sp_args", apx_speed_sp_args);


    //--------------------------------------------------------------------------
        // for flux
        // for flux
    //static t_ID Vd_weakened = 0;// case FW
    // for flux
    //static t_ID Vd_weakened = 0;// case FW
    t_ID SVM_inv_index = MAX_IO/2;  // case FW

    t_ID Flux_pid_dout;                // Partial results
    t_ID Flux_sp = (FOC_mode == MOD_FLUX) 
                    ? (t_ID)(apx_flux_sp_args - Vd_weakened)
                    : (t_ID)apx_flux_sp_args;
    PID_Control_ap_fixed<t_ID, t_IDI, t_IDE, t_pid>(
        Flux_pid_dout, 
        Flux_GiE_prev, 
        Flux_Err_prev, 
        Id, 
        Flux_sp, 
        apx_flux_kp_args, 
        apx_flux_ki_args, 
        apx_flux_kd_args, 
        Mode_Change);

    T_IO Flux_pid_dout_io = (T_IO)Clip_AP<t_IQ>(Flux_pid_dout, (t_IQ)(0-MAX_IO), (t_IQ)MAX_IO);
    RANGETRACER("FOC.Flux_pid.Flux_pid_dout", Flux_pid_dout);
    RANGETRACER("FOC.Flux_pid.Flux_GiE_prev", Flux_GiE_prev);
    RANGETRACER("FOC.Flux_pid.Flux_Err_prev", Flux_Err_prev);
    RANGETRACER("FOC.Flux_pid.Id", Id);
    RANGETRACER("FOC.Flux_pid.Flux_sp", Flux_sp);
    RANGETRACER("FOC.Flux_pid.apx_flux_sp_args", apx_flux_sp_args);
    RANGETRACER("FOC.Flux_pid.Vd_weakened", Vd_weakened);
    RANGETRACER("FOC.Flux_pid.Flux_pid_dout_io", Flux_pid_dout_io);

    //Torque PI Controller--------------------------------------------------------------------------
    t_IQ Torque_pid_dout;                // Partial results
    t_IQ Torque_Sp = (FOC_mode == MOD_TORQUE_WITHOUT_SPEED)
                    ? (t_IQ)(apx_torque_sp_args)
                    : (t_IQ)Speed_pid_dout; // Only in Torque mode Speed_pid not be the setpoint
    PID_Control_ap_fixed<t_IQ, t_IQI, t_IQE, t_pid>(
        Torque_pid_dout, 
        Torque_GiE_prev, 
        Torque_Err_prev, 
        Iq,
        Torque_Sp, 
        apx_torque_kp_args, 
        apx_torque_ki_args,
        apx_torque_kd_args, 
        Mode_Change);
    T_IO Torque_pid_dout_io = (T_IO)Clip_AP<t_IQ>(Torque_pid_dout, (t_IQ)(0-MAX_IO), (t_IQ)MAX_IO);

    RANGETRACER("FOC.Torque_pid.Torque_pid_dout", Torque_pid_dout);
    RANGETRACER("FOC.Torque_pid.Torque_GiE_prev", Torque_GiE_prev);
    RANGETRACER("FOC.Torque_pid.Torque_Err_prev", Torque_Err_prev);
    RANGETRACER("FOC.Torque_pid.Iq", Iq);
    RANGETRACER("FOC.Torque_pid.Torque_Sp", Torque_Sp);
    RANGETRACER("FOC.Torque_pid.apx_torque_sp_args", apx_torque_sp_args);
    RANGETRACER("FOC.Torque_pid.Speed_pid_dout", Speed_pid_dout);
    RANGETRACER("FOC.Torque_pid.Torque_pid_dout_io", Torque_pid_dout_io);
    //--------------------------------------------------------------------------
    T_IO Flux_decoupled = 0;
    T_IO Torque_decoupled = 0;
    Decoupling_T_ap_fixed<t_IQI, t_IQI, t_IQI, T_IO, MAX_IO>(
        Flux_decoupled, 
        Torque_decoupled, 
        (t_IQI)Id, 
        (t_IQI)Iq, 
        (t_IQI)Flux_pid_dout_io, 
        (t_IQI)Torque_pid_dout_io,
        (t_IQI)RPM,
        (T_IO)RPM_to_speed);

    RANGETRACER("FOC.Decoupling.Flux_decoupled", Flux_decoupled);
    RANGETRACER("FOC.Decoupling.Torque_decoupled", Torque_decoupled);
    RANGETRACER("FOC.Decoupling.Flux_pid_dout_io", Flux_pid_dout_io);
    RANGETRACER("FOC.Decoupling.Torque_pid_dout_io", Torque_pid_dout_io);

    //--------------------------------------------------------------------------
    T_IO M_index, M_threshold;
    Field_Weakening_T<T_IO, T_IO, t_mid2>(
        M_index, 
        M_threshold, 
        Flux_decoupled, 
        Torque_decoupled, 
        1,//V,
        SVM_inv_index);//SVM_inv_index);
    RANGETRACER("FOC.Field_W.M_index", M_index);
    RANGETRACER("FOC.Field_W.M_threshold", M_threshold);
    RANGETRACER("FOC.Field_W.Flux_decoupled", Flux_decoupled);
    RANGETRACER("FOC.Field_W.Torque_decoupled", Torque_decoupled);
    const T_IO MAX_CURRENT = MAX_IO>>1; // 0.88 from datasheet - 10%

    //M_index = Clip_AP<T_IO>(M_index, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);  

    PID_Control_ap_fixed<t_ID, t_IDI, t_IDE, t_pid>(
        Vd_weakened, 
        FWiE_prev, 
        FW_err_prev, 
        (t_ID)M_index,
        (t_ID)M_threshold, 
        (t_pid)apx_fw_kp_args,
        (t_pid)apx_fw_ki_args,
        (t_pid)0, 
        Mode_Change);   

    RANGETRACER("FOC.PID.Field_W.Vd_weakened", Vd_weakened);

    Vd_weakened = Clip_AP<T_IO>(Vd_weakened, (T_IO)(0-MAX_CURRENT), (T_IO)MAX_CURRENT);                                                                

   
    RANGETRACER("FOC.PID.Field_W.FWiE_prev", FWiE_prev);
    RANGETRACER("FOC.PID.Field_W.M_index", M_index);
    RANGETRACER("FOC.PID.Field_W.M_threshold", M_threshold);
    RANGETRACER("FOC.PID.Field_W.apx_fw_kp_args", apx_fw_kp_args);
    RANGETRACER("FOC.PID.Field_W.apx_fw_ki_args", apx_fw_ki_args);
    RANGETRACER("FOC.PID.Field_W.Clip_Vd_weakened", Vd_weakened);

    //--------------------------------------------------------------------------
    // volatile int Vd, Vq;
    T_IO Vd_ctrl, Vq_ctrl;
    t_sincos cos_theta_ctrl, sin_theta_ctrl;
    details::Control_foc_ap_fixed<T_IO, t_sincos, t_mode>(
        //output
        Vd_ctrl, 
        Vq_ctrl, 
        cos_theta_ctrl, 
        sin_theta_ctrl, 
        //input
        FOC_mode,
        cos_theta,//cos_gen_angle, 
        sin_theta,//sin_gen_angle, 
        cos_theta, 
        sin_theta, 
        Flux_pid_dout_io, 
        Torque_pid_dout_io,
        apx_vd_args,
        apx_vq_args);
    Vd_ctrl = Clip_AP<T_IO>(Vd_ctrl, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);
    Vq_ctrl = Clip_AP<T_IO>(Vq_ctrl, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);
    T_IO Valpha, Vbeta; // Transfom result
    Park_Inverse_ap_fixed<T_IO, t_sincos>(
        Valpha, 
        Vbeta, 
        Vd_ctrl, 
        Vq_ctrl,
        cos_theta_ctrl, 
        sin_theta_ctrl
        );
    RANGETRACER("FOC.InversPark.Valpha", Valpha);
    RANGETRACER("FOC.InversPark.Vbeta", Vbeta);
    RANGETRACER("FOC.InversPark.Vd_ctrl", Vd_ctrl);
    RANGETRACER("FOC.InversPark.Vq_ctrl", Vq_ctrl);


    T_IO Va_iclk, Vb_iclk, Vc_iclk;
    Clarke_Inverse_2p_ap_fixed<T_IO>(
        Va_iclk, 
        Vb_iclk, 
        Vc_iclk, 
        Valpha, 
        Vbeta);
    RANGETRACER("FOC.InversClarke.Va_iclk", Va_iclk);
    RANGETRACER("FOC.InversClarke.Vb_iclk", Vb_iclk);
    RANGETRACER("FOC.InversClarke.Vc_iclk", Vc_iclk);

    // clang-format on

    Va_cmd = Clip_AP<T_IO>(Va_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);
    Vb_cmd = Clip_AP<T_IO>(Vb_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);
    Vc_cmd = Clip_AP<T_IO>(Vc_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);

    Va = Va_cmd;
    Vb = Vb_cmd;
    Vc = Vc_cmd;

    t_glb_q15q16 apx_id_stts = Id;
    t_glb_q15q16 apx_iq_stts = Iq;
    t_glb_q15q16 apx_flux_acc_stts = Flux_GiE_prev;
    t_glb_q15q16 apx_flux_err_stts = Flux_Err_prev;
    t_glb_q15q16 apx_flux_out_stts = Flux_pid_dout;
    t_glb_q15q16 apx_torque_acc_stts = Torque_GiE_prev;
    t_glb_q15q16 apx_torque_err_stts = Torque_Err_prev;
    t_glb_q15q16 apx_torque_out_stts = Torque_pid_dout;
    t_glb_q15q16 apx_speed_acc_stts = Speed_GiE_prev;
    t_glb_q15q16 apx_speed_err_stts = Speed_Err_prev;
    t_glb_q15q16 apx_speed_out_stts = Speed_pid_dout;

    t_glb_q15q16 apx_speed_Ialpha_stts = Ialpha;
    t_glb_q15q16 apx_speed_Ibeta_stts = Ibeta;
    t_glb_q15q16 apx_speed_Ihomopolar_stts = Ihomopolar;

    // debugging
    // apx_flux_acc_stts =     Vd_ctrl;
    // apx_flux_err_stts =     Vq_ctrl;
    // apx_flux_out_stts =     Ihomopolar;
    // apx_torque_acc_stts = Valpha;
    // apx_torque_err_stts = Vbeta;

    t_glb_q15q16 apx_speed_stts = RPM;
    t_glb_q15q16 apx_angle_stts = Theta;

    speed_stts = apx_speed_stts.range(31, 0);
    angle_stts = apx_angle_stts.range(31, 0);
    id_stts = apx_id_stts.range(31, 0);
    iq_stts = apx_iq_stts.range(31, 0);
    flux_acc_stts = apx_flux_acc_stts.range(31, 0);     // Flux_GiE_prev;
    flux_err_stts = apx_flux_err_stts.range(31, 0);     // Flux_Err_prev;
    flux_out_stts = apx_flux_out_stts.range(31, 0);     // Flux_pid_dout;
    torque_acc_stts = apx_torque_acc_stts.range(31, 0); // Torque_GiE_prev;
    torque_err_stts = apx_torque_err_stts.range(31, 0); // Torque_Err_prev;
    torque_out_stts = apx_torque_out_stts.range(31, 0); // Torque_pid_dout;
    speed_acc_stts = apx_speed_acc_stts.range(31, 0);   // Speed_GiE_prev;
    speed_err_stts = apx_speed_err_stts.range(31, 0);   // Speed_Err_prev;
    speed_out_stts = apx_speed_out_stts.range(31, 0);   // Speed_pid_dout;
    Ialpha_stts = apx_speed_Ialpha_stts.range(31, 0);
    Ibeta_stts = apx_speed_Ibeta_stts.range(31, 0);
    Ihomopolar_stts = apx_speed_Ihomopolar_stts.range(31, 0);
}

} // namespace motorcontrol
} // namespace xf
#endif
