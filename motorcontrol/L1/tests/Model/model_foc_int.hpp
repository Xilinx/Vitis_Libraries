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
#ifndef _MODEL_FOC_INT_HPP_
#define _MODEL_FOC_INT_HPP_
#include "stdio.h"
#include "model_base.hpp"
#include "model_pid.hpp"
#include "model_pid_int.hpp"
//#include "sin_cos_table.h"

using namespace xf::motorcontrol;

#define Q16_SCALE 15

/// \brief Maximum positive value for saturated arithmetic.
#define MAX_LIM ((1 << (COMM_ADC_WIDTH - 1)) - 1) //((1<<Q16_p_SHIFT)-1) //32767

/// \brief Minimum negative value for saturated arithmetic.
#define MIN_LIM -MAX_LIM // -32767

/// \brief The number \f$\frac{1}{\sqrt{3}}\f$ in the Q16.16 format.
#define SQRT3A 0x000093CD

/// \brief The number \f$\sqrt{3}\f$ in the Q16.16 format.
#define SQRT3C 0x0000DDB4

/// \brief The other scales could be set
const int scale_park = Q16_SCALE;
const int scale_pid = 8;
const int SQRT3C_scale = 15;
const int SQRT3A_scale = 16;

/// args for FOC core control
template <class T_ARGS>
struct args_control {
    T_ARGS FOC_Mode;
    T_ARGS FixPeriod;
    T_ARGS Angle_shift;
    T_ARGS args_vd;
    T_ARGS args_vq;
};

/**
 * @brief Args for pid control
 * @tparam T_IN  Type of the input Args
 * @param  sp    Setpoint. ex. [-32767, 32767] for Q16.16
 * @param  kp	 Proportional coefficient. value should be in [0, (1<<KP_SCALE)]
 * @param  ki    Integral coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param  kd    differential coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * */
template <class T_IN>
struct args_pid { // check datapack
    T_IN sp;
    T_IN kp;
    T_IN ki;
    T_IN kd;
    args_pid() { sp = kp = ki = kd = 0; };
    args_pid(T_IN sp_in, T_IN kp_in, T_IN ki_in, T_IN kd_in) {
        sp = sp_in;
        kp = kp_in;
        ki = ki_in;
        kd = kd_in;
    };
};

/**
 * @brief Structure of return data for Status Reg or Moniter Reg
 * @tparam T_REG	    	Type of the return data to Status Reg or Moniter Reg
 * @param status_	    	Return data to Status Reg.
 * @param Ia_f	    		Corrected phase A current as input
 * @param Ib_f				Corrected phase B current as input
 * @param Ic_f				Corrected phase C current as input
 * @param Id				Id as output of Park Direct.
 * @param Iq				Iq as output of Park Direct.
 * @param Vd				Vd as input of Park Inverse
 * @param Vq				Vq as input of Park Inverse
 * @param Valpha			Valpha as input of Clarke Inverse
 * @param Vbeta				Vbeta as input of Clarke Inverse
 * @param FOC_Mode		    FOC mode now
 * @param Ihomopolar		Return homopolar Current to Moniter Reg.
 */
template <class T_REG>
struct ret_monitor {
    T_REG status_RPM;
    T_REG status_Id;
    T_REG status_Iq;
    T_REG status_Angle;
    T_REG Ia_f;
    T_REG Ib_f;
    T_REG Ic_f;
    T_REG Id;
    T_REG Iq;
    T_REG Vd;
    T_REG Vq;
    T_REG Valpha;
    T_REG Vbeta;
    T_REG FOC_Mode;
    T_REG Ihomopolar;
    T_REG pid_id_m_din;
    T_REG pid_id_m_acc;
    T_REG pid_id_m_err_pre;
    T_REG pid_id_m_out;
    T_REG pid_Te_m_din;
    T_REG pid_Te_m_acc;
    T_REG pid_Te_m_err_pre;
    T_REG pid_Te_m_out;
    T_REG pid_w_m_din;
    T_REG pid_w_m_acc;
    T_REG pid_w_m_err_pre;
    T_REG pid_w_m_out;
};

/**
 * @brief Return structure data for Moniter Reg
 * @tparam T_REG	    	Type of the return data to Status Reg or Moniter Reg
 * @param ret	        	Return pointer
 * @param Ia_f	    		Corrected phase A current as output of Filters
 * @param Ib_f				Corrected phase B current as output of Filters
 * @param Ic_f				Corrected phase C current as output of Filters
 * @param Id				Id as output of Park Direct.
 * @param Iq				Iq as output of Park Direct.
 * @param Vd				Vd as input of Park Inverse
 * @param Vq				Vq as input of Park Inverse
 * @param Valpha			Valpha as input of Clarke Inverse
 * @param Vbeta				Vbeta as input of Clarke Inverse
 * @param FOC_Mode		    FOC mode now
 */
template <class T_REG>
void save_ret(ret_monitor<T_REG>* ret,
              T_REG Ia_f,
              T_REG Ib_f,
              T_REG Ic_f,
              T_REG Id,
              T_REG Iq,
              T_REG Vd,
              T_REG Vq,
              T_REG Valpha,
              T_REG Vbeta,
              T_REG FOC_Mode) {
    ret->Ia_f = Ia_f;
    ret->Ib_f = Ib_f;
    ret->Ic_f = Ic_f;
    ret->Id = Id;
    ret->Iq = Iq;
    ret->Vd = Vd;
    ret->Vq = Vq;
    ret->Valpha = Valpha;
    ret->Vbeta = Vbeta;
    ret->FOC_Mode = FOC_Mode;
}

//--------------------------------------------------------------------------
// The motor should rotate regardless of the encoder output
//--------------------------------------------------------------------------
/**
 * @brief IIR Filter in the form of an inline HLS function
 * @tparam T_IN	    Type of the Current input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the Voltage input data.
 * @tparam T_SINCOS	Type of the Trigonometric function input data.
 * @tparam CPR      Number of encoder steps per one full revolution. ex. 1000
 */
template <class T_IN, class T_MID, class T_OUT, class T_SINCOS, int CPR>
void Control_foc(T_OUT& Vd,
                 T_OUT& Vq,
                 T_SINCOS& cos_out,
                 T_SINCOS& sin_out,
                 T_MID FixPeriod,
                 FOC_Mode FOC_Mode,
                 const T_SINCOS* cos_table,
                 const T_SINCOS* sin_table,
                 T_SINCOS cos_in,
                 T_SINCOS sin_in,
                 T_IN Flux_out,
                 T_IN Torque_out,
                 T_IN args_vd,
                 T_IN args_vq) {
#pragma HLS INLINE off
    static T_MID gen_delay = 0;   // Generator period counter
    static T_MID gen_angle = 0;   // Generator angle counter
                                  // Simple angle generator for manual mode
                                  // The motor should rotate regardless of the encoder output
    if (gen_delay >= FixPeriod) { // Period loop
        gen_delay = 0;
        if (gen_angle >= (CPR - 1)) { // Angle loop
            gen_angle = 0;
        } else {
            ++gen_angle;
        }
    } else {
        ++gen_delay;
    }

    // Control Vd and Vq depending on work mode
    switch (FOC_Mode) {
        case MOD_STOPPED: // Motor stop
            Vd = 0;       // Set zero Vd
            Vq = 0;       // Set zero Vq
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_SPEED_WITH_TORQUE: // Work mode speed loop
            Vd = Flux_out;          // Sorce Vd from Flux PI
            Vq = Torque_out;        // Sorce Vq from Torque PI
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        // case MOD_SPEED_WITHOUT_TORQUE:		// Speed loop (Torque PI bypass)
        // 	Vd = Flux_out;						// Sorce Vd from Flux PI
        // 	Vq = Speed_out;						// Sorce Vq from Speed PI
        // 	cos_out = cos_in;
        // 	sin_out = sin_in;
        // 	break;
        case MOD_TORQUE_WITHOUT_SPEED: // Disable Speed PI
            Vd = Flux_out;             // Sorce Vd from Flux PI
            Vq = Torque_out;           // Sorce Vq from Torque PI
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_FLUX:       // TODO
            Vd = Flux_out;   // TODO
            Vq = Torque_out; // TODO
            cos_out = cos_in;
            sin_out = sin_in;
            break;

        case MOD_MANUAL_TORQUE_FLUX: // Manual Vd/Vq with real angle
            Vd = args_vd;            // Sorce Vd from register
            Vq = args_vq;            // Sorce Vq from register
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_TORQUE: // Manual torque
            Vd = Flux_out;      // Sorce Vd from Flux PI
            Vq = args_vq;       // Sorce Vq from register
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        // Manual mode with angle generator
        case MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED:
            Vd = args_vd;                   // Sorce Vd from register
            Vq = args_vq;                   // Sorce Vq from register
            cos_out = cos_table[gen_angle]; // Generated angle cos
            sin_out = sin_table[gen_angle]; // Generated angle sin
            break;
        case MOD_MANUAL_FLUX: // TODO
            Vd = args_vd;     // TODO
            Vq = Torque_out;  // TODO
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        default:    // Motor OFF
            Vd = 0; // Set zero Vd
            Vq = 0; // Set zero Vq
            cos_out = cos_in;
            sin_out = sin_in;
            break;
    }
}
//--------------------------------------------------------------------------
// foc top functions with 3 phase and PID control
// Argument reg and status reg to help to control the system
//--------------------------------------------------------------------------
// clang-format off
/**
 * @brief sensor based field-orientated control (FOC) in the form of an inline HLS function
 * @tparam T_Iabc	    Type of the Current input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_Vabc	    Type of the Voltage input data.
 * @tparam T_Speed	    Type of the RPM input data.
 * @tparam T_SINCOS	    Type of the Trigonometric function data.
 * @tparam T_ANGLE		Type of the Angle.
 * @tparam T_MID	    Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam MAX_OUT	    Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	    Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam I_SQRT3C The Number \f$\sqrt{3}\f$ for Q16.16
 * @tparam SQRT3C_SCALE Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 15 for Q16.16
 * @tparam I_SQRT3A The Number \f$\frac{1}{\sqrt{3}}\f$ for Q16.16
 * @tparam SQRT3A_SCALE Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 16 for Q16.16
 * @tparam SINCOS_SCALE Number significant bits of Trigonometric function table. ex. SINCOS_SCALE is 16 for Q16.16
 * @tparam PI_SCALE     Number significant bits of kp. ex. KP_SCALE is 8 when kp is in [0, 255]
 * @tparam CPR          Number of encoder steps per one full revolution. ex. 1000
 * @param  Ia_f			Input Phase A current 
 * @param  Ib_f			Input Phase B current 
 * @param  Ic_f			Input Phase C current
 * @param  RPM_f		Input RPM
 * @param  V 			Input V
 * @param  Angle		Input Rotor Angle
 * @param  pid_Flux     Input Args for PID control of Flux, as structure args_pid
 * @param  pid_Speed    Input Args for PID control of Speed, as structure args_pid
 * @param  pid_Torque   Input Args for PID control of Torque, as structure args_pid
 * @param  args_ctrl    Input Args for system control, as structure args_control
 * @param  cos_table    Input cos_table for Trigonometric function table as ROM
 * @param  sin_table    Input sin_table for Trigonometric function table as ROM
 * @param  Va 			Output Va
 * @param  Vb 			Output Vb
 * @param  Vc 			Output Vc
 * @param  SVM_inv_idx  Output V_inv
 * @param  ret_monitor  Return data for Status Reg or Moniter Reg, as structure ret_monitor
 */
// clang-format on
template <class T_Iabc,
          class T_Vabc,
          class T_Speed,
          class T_SINCOS,
          class T_ANGLE,
          class T_MID,
          class T_MID2,
          int MAX_OUT,
          int MIN_OUT,
          int V_SQRT3C,
          int SCALE_SQRT3C,
          int I_SQRT3A,
          int SQRT3A_SCALE,
          int SINCOS_SCALE,
          int PI_SCALE,
          int CPR>
void foc_core(T_Iabc Ia_f,
              T_Iabc Ib_f,
              T_Iabc Ic_f,
              T_Speed RPM_f,
              T_Vabc V,
              T_ANGLE Angle,
              args_pid<T_Iabc> pid_Flux,
              args_pid<T_Speed> pid_Speed,
              args_pid<T_Iabc> pid_Torque,
              args_pid<T_Vabc> fw,
              args_control<T_MID> args_ctrl,

              // tables
              const T_SINCOS* cos_table,
              const T_SINCOS* sin_table,
              // output to SVPWM
              T_Vabc& Va,
              T_Vabc& Vb,
              T_Vabc& Vc,
              T_Vabc& SVM_inv_idx,
              // return to monitor
              ret_monitor<T_MID>& ret_monitor) {
#pragma HLS INLINE off
    //--------------------------------------------------------------------------
    // load args
    //--------------------------------------------------------------------------
    static T_MID Mode_Prev = MOD_STOPPED; // Previous control Register.
    bool Mode_Change;

    FOC_Mode FOC_Mode = xf::motorcontrol::FOC_Mode(args_ctrl.FOC_Mode);
    T_MID FixPeriod = (T_MID)args_ctrl.FixPeriod;
    Mode_Change = FOC_Mode != Mode_Prev;

    // for flux
    static T_MID Vd_weakened = 0;      // case FW
    T_MID SVM_inv_index = SVM_inv_idx; // case FW
    //--------------------------------------------------------------------------
    // Angle
    //--------------------------------------------------------------------------
    T_MID Angle_shift = args_ctrl.Angle_shift;             // Read angle shift parameter
    T_MID Theta = (T_MID)Angle - Angle_shift;              // Apply angle correction
    Theta = (Theta < 0) ? (T_MID)(Theta + CPR) : Theta;    // Correct negative angle
    Theta = (Theta >= CPR) ? (T_MID)(Theta - CPR) : Theta; // Correct angle overload
    ret_monitor.status_Angle = Theta;

    //--------------------------------------------------------------------------
    T_Iabc Ialpha, Ibeta, Ihomopolar; // Transfom result
    Clarke_Direct_3p_T<T_Iabc, T_MID, T_Iabc, MAX_OUT, MIN_OUT, I_SQRT3A, SQRT3A_SCALE>(Ialpha, Ibeta, Ihomopolar, Ia_f,
                                                                                        Ib_f, Ic_f);
    ret_monitor.Ihomopolar = Ihomopolar; // Pass Ihomopolar to monitor

    //--------------------------------------------------------------------------
    T_Iabc Id, Iq;
    T_SINCOS cos_theta = cos_table[(T_ANGLE)Theta];
    T_SINCOS sin_theta = sin_table[(T_ANGLE)Theta];
    Park_Direct_T<T_Iabc, T_MID, T_Iabc, MAX_OUT, MIN_OUT, T_SINCOS, SINCOS_SCALE>(Id, Iq, Ialpha, Ibeta, cos_theta,
                                                                                   sin_theta);

    ret_monitor.status_Id = ~Id;
    ret_monitor.status_Iq = ~Iq;

    //--------------------------------------------------------------------------
    static T_MID Speed_GiE_prev = 0; // Variable for previous integral value
    static T_MID Speed_Err_prev = 0; // Variable for previous derivative value
    T_Vabc Speed_pid;                // Partial results
    PID_Control_T3<T_Speed, T_MID, T_Vabc, MAX_OUT, MIN_OUT, PI_SCALE>(Speed_pid, Speed_GiE_prev, Speed_Err_prev, RPM_f,
                                                                       pid_Speed.sp, pid_Speed.kp, pid_Speed.ki,
                                                                       pid_Speed.kd, Mode_Change);

    //--------------------------------------------------------------------------
    static T_MID Flux_GiE_prev = 0; // Variable for previous integral value
    static T_MID Flux_Err_prev = 0; // Variable for previous derivative value
    T_Vabc Flux_pid;                // Partial results
    // T_MID Flux_sp = (FOC_Mode == MOD_FLUX)
    //                     ? (0 - Vd_weakened)
    //                     : 0; //(FOC_Mode == MOD_FLUX) ? -(pid_Flux.sp) - Vd_weakened : -(pid_Flux.sp);
    T_MID Flux_sp = (FOC_Mode == MOD_FLUX) ? pid_Flux.sp - Vd_weakened : pid_Flux.sp;
    PID_Control_T3<T_Iabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, PI_SCALE>(
        Flux_pid, Flux_GiE_prev, Flux_Err_prev, Id, Flux_sp, pid_Flux.kp, pid_Flux.ki, pid_Flux.kd, Mode_Change);

    //--------------------------------------------------------------------------
    static T_MID Torque_GiE_prev = 0; // Variable for previous integral value
    static T_MID Torque_Err_prev = 0; // Variable for previous derivative value
    T_MID Torque_Sp;                  // PI parameters
    T_Vabc Torque_pid;                // Partial results
    Torque_Sp = (FOC_Mode == MOD_TORQUE_WITHOUT_SPEED)
                    ? (T_MID)(pid_Torque.sp)
                    : (T_MID)Speed_pid; // Only in Torque mode Speed_pid not be the setpoint
    PID_Control_T3<T_Iabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, PI_SCALE>(Torque_pid, Torque_GiE_prev, Torque_Err_prev, Iq,
                                                                      Torque_Sp, pid_Torque.kp, pid_Torque.ki,
                                                                      pid_Torque.kd, Mode_Change);

    //--------------------------------------------------------------------------
    T_Vabc Flux_decoupled = 0, Torque_decoupled = 0;
    Decoupling_T<T_Iabc, T_MID, T_MID2, T_Vabc, MAX_OUT>(Flux_decoupled, Torque_decoupled, Id, Iq, Flux_pid, Torque_pid,
                                                         RPM_f);
    // Flux_pid = Flux_decoupled;
    // Torque_pid = Torque_decoupled;

    //--------------------------------------------------------------------------
    T_MID M_index, M_threshold;
    Field_Weakening_T<T_Vabc, T_MID, T_MID2>(M_index, M_threshold, Flux_decoupled, Torque_decoupled, V, SVM_inv_index);

    const T_MID MAX_CURRENT = MAX_OUT >> 1; // 0.88 from datasheet - 10%
    static T_MID FWiE_prev = 0;
    static T_MID FW_err_prev = 0;

    PID_Control_T3<T_Vabc, T_MID, T_MID, MAX_CURRENT, -MAX_CURRENT, PI_SCALE>(
        Vd_weakened, FWiE_prev, FW_err_prev, M_index, M_threshold, fw.kp, fw.ki, 0, Mode_Change);

    //--------------------------------------------------------------------------
    // volatile int Vd, Vq;
    T_Vabc Vd, Vq;
    T_SINCOS cos_theta_ctrl, sin_theta_ctrl;
    Control_foc<T_Vabc, T_MID, T_Vabc, T_SINCOS, CPR>(Vd, Vq, cos_theta_ctrl, sin_theta_ctrl, FixPeriod, FOC_Mode,
                                                      //   cos_table, sin_table, cos_theta, sin_theta, Flux_decoupled,
                                                      //   Torque_decoupled, args_ctrl.args_vd, args_ctrl.args_vq);
                                                      cos_table, sin_table, cos_theta, sin_theta, Flux_pid, Torque_pid,
                                                      args_ctrl.args_vd, args_ctrl.args_vq);

    T_Vabc Valpha, Vbeta; // Transfom result
    Park_Inverse_T<T_Vabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, T_SINCOS, SINCOS_SCALE>(Valpha, Vbeta, Vd, Vq,
                                                                                    cos_theta_ctrl, sin_theta_ctrl);

    Clarke_Inverse_2p_T<T_Vabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, V_SQRT3C, SCALE_SQRT3C>(Va, Vb, Vc, Valpha, Vbeta);
    save_ret<T_MID>(&ret_monitor, Ia_f, Ib_f, Ic_f, Id, Iq, Vd, Vq, Valpha, Vbeta, FOC_Mode);
    save_ret<T_MID>(&ret_monitor, Ia_f, Ib_f, Ic_f, Id, Iq, Vd, Vq, Valpha, Vbeta, FOC_Mode);
    ret_monitor.pid_id_m_din = Id;
    ret_monitor.pid_id_m_acc = Flux_GiE_prev;
    ret_monitor.pid_id_m_err_pre = Flux_Err_prev;
    ret_monitor.pid_id_m_out = Flux_pid;

    ret_monitor.pid_Te_m_din = Iq;
    ret_monitor.pid_Te_m_acc = Torque_GiE_prev;
    ret_monitor.pid_Te_m_err_pre = Torque_Err_prev;
    ret_monitor.pid_Te_m_out = Torque_pid;

    ret_monitor.pid_w_m_din = RPM_f;
    ret_monitor.pid_w_m_acc = Speed_GiE_prev;
    ret_monitor.pid_w_m_err_pre = Speed_Err_prev;
    ret_monitor.pid_w_m_out = Speed_pid;

    //--------------------------------------------------------------------------
    // Update control register state.
    //--------------------------------------------------------------------------
    Mode_Prev = FOC_Mode;
}

template <int VALUE_CPR>
void foc_data_driven(
    // Input for GPIO
    short Ia,             // Phase A current
    short Ib,             // Phase B current
    short Ic,             // Phase B current
    short RPM,            // RPM
    unsigned short Angle, // Encoder count
    // Output for GPIO
    short& Va,
    short& Vb,
    short& Vc,
    // Inout put for parameters
    volatile int args[FOC_ARGS_SIZE]) {
#pragma HLS INLINE off
    //#pragma HLS pipeline enable_flush

    typedef short T_Iabc;
    typedef short T_Vabc;
    typedef uint16_t T_Speed;
    typedef short T_SINCOS;
    typedef int T_MID;
    typedef ap_fixed<32, 16, AP_RND, AP_SAT> T_MID2;
    typedef uint16_t T_ANGLE;

    args_pid<T_Iabc> args_Flux(args[ARGES_IDX_FOC::FLUX_SP_ARGS], args[ARGES_IDX_FOC::FLUX_KP_ARGS],
                               args[ARGES_IDX_FOC::FLUX_KI_ARGS], args[ARGES_IDX_FOC::FLUX_KD_ARGS]);
    args_pid<T_Speed> args_Speed(args[ARGES_IDX_FOC::SPEED_SP_ARGS], args[ARGES_IDX_FOC::SPEED_KP_ARGS],
                                 args[ARGES_IDX_FOC::SPEED_KI_ARGS], args[ARGES_IDX_FOC::SPEED_KD_ARGS]);
    args_pid<T_Iabc> args_Torque(args[ARGES_IDX_FOC::TORQUE_SP_ARGS], args[ARGES_IDX_FOC::TORQUE_KP_ARGS],
                                 args[ARGES_IDX_FOC::TORQUE_KI_ARGS], args[ARGES_IDX_FOC::TORQUE_KD_ARGS]);
    args_pid<T_Vabc> args_FW(0, args[ARGES_IDX_FOC::FW_KP_ARGS], args[ARGES_IDX_FOC::FW_KI_ARGS], 0);

    args_control<T_MID> args_ctrl;

    args_ctrl.FOC_Mode = args[ARGES_IDX_FOC::CONTROL_MODE_ARGS];
    args_ctrl.FixPeriod = args[ARGES_IDX_FOC::CONTROL_FIXPERIOD_ARGS];
    args_ctrl.Angle_shift = (T_MID)args[ARGES_IDX_FOC::ANGLE_SH_ARGS];
    args_ctrl.args_vd = (T_MID)args[ARGES_IDX_FOC::VD_ARGS];
    args_ctrl.args_vq = (T_MID)args[ARGES_IDX_FOC::VQ_ARGS];

    ret_monitor<T_MID> ret;

    T_Speed RPM_f; // input of the RPM
    T_Iabc Ia_f;   // Corrected phase A current
    T_Iabc Ib_f;   // Corrected phase B current
    T_Iabc Ic_f;   // Corrected phase C current

    FOC_Mode FOC_Mode = xf::motorcontrol::FOC_Mode(args_ctrl.FOC_Mode);
    bool is_Mode_idle = (FOC_Mode == MOD_STOPPED);

    static T_Vabc SVM_inv_index = MAX_LIM >> 1;
    short V_fw = 1;

    foc_core<T_Iabc, T_Vabc, T_Speed, T_SINCOS, T_ANGLE, T_MID, T_MID2, MAX_LIM, MIN_LIM, SQRT3C, SQRT3C_scale, SQRT3A,
             SQRT3A_scale, scale_park, scale_pid, VALUE_CPR>(Ia, Ib, Ic, RPM, V_fw, Angle, args_Flux, args_Speed,
                                                             args_Torque, args_FW, args_ctrl, cos_table, sin_table, Va,
                                                             Vb, Vc, SVM_inv_index, ret);

    args[ARGES_IDX_FOC::ID_STTS] = ret.pid_id_m_din;
    args[ARGES_IDX_FOC::FLUX_ACC_STTS] = ret.pid_id_m_acc;
    args[ARGES_IDX_FOC::FLUX_ERR_STTS] = ret.pid_id_m_err_pre;
    args[ARGES_IDX_FOC::FLUX_OUT_STTS] = ret.pid_id_m_out;
    args[ARGES_IDX_FOC::IQ_STTS] = ret.pid_Te_m_din;
    args[ARGES_IDX_FOC::TORQUE_ACC_STTS] = ret.pid_Te_m_acc;
    args[ARGES_IDX_FOC::TORQUE_ERR_STTS] = ret.pid_Te_m_err_pre;
    args[ARGES_IDX_FOC::TORQUE_OUT_STTS] = ret.pid_Te_m_out;
    args[ARGES_IDX_FOC::SPEED_STTS] = ret.pid_w_m_din;
    args[ARGES_IDX_FOC::SPEED_ACC_STTS] = ret.pid_w_m_acc;
    args[ARGES_IDX_FOC::SPEED_ERR_STTS] = ret.pid_w_m_err_pre;
    args[ARGES_IDX_FOC::SPEED_OUT_STTS] = ret.pid_w_m_out;
    args[ARGES_IDX_FOC::ANGLE_STTS] = ret.status_Angle;
}

template <class t_int>
class FOC_Simple_2 : public Model_base {
   public:
    //
    int m_ppr;
    int m_rpm;
    int CPR;
    // inputs
    t_int Ia;
    t_int Ib;
    t_int Ic;
    t_int w;
    t_int theta_e;

    t_int* input_Ia_pull;
    t_int* input_Ib_pull;
    t_int* input_Ic_pull;
    t_int* input_w_pull;
    t_int* input_theta_e_pull;
    // output to be push
    t_int out_va;          //	output Votages
    t_int out_vb;          //	output Votages
    t_int out_vc;          //	output Votages
    t_int* output_va_push; //	output Votages to be push
    t_int* output_vb_push; //	output Votages to be push
    t_int* output_vc_push; //	output Votages to be push
    Model_pid_int<t_int> pid_id;
    Model_pid_int<t_int> pid_Te;
    Model_pid_int<t_int> pid_w;

    // middle variable (wire signals)
    t_int wire_Id_pid_in;
    t_int wire_Iq_pid_in;
    t_int wire_w_pid_in;
    t_int wire_Id_pid_out;
    t_int wire_Iq_pid_out;
    t_int wire_w_pid_out;

    // int out_ipark_alpha;
    // int out_ipark_beta;
    // int Vd;
    // int Vq;

    FOC_Simple_2() {
        num_models = 3;
        init_ParaType();
        list_model[0] = (Model_base*)&pid_id;
        list_model[1] = (Model_base*)&pid_Te;
        list_model[2] = (Model_base*)&pid_w;
        pid_id.setObjName(":pid_id");
        pid_Te.setObjName(":pid_Te");
        pid_w.setObjName(":pid_w");
        input_Ia_pull = NULL;
        input_Ib_pull = NULL;
        input_Ic_pull = NULL;
        input_w_pull = NULL;
        input_theta_e_pull = NULL;
        output_va_push = NULL;
        output_vb_push = NULL;
        output_vc_push = NULL;
        // initializing internal connections with internal models' input and output points
        pid_id.input_m_din = &wire_Id_pid_in;
        pid_Te.input_m_din = &wire_Iq_pid_in;
        pid_w.input_m_din = &wire_w_pid_in;
        pid_id.output_m_out = &wire_Id_pid_out;
        pid_Te.output_m_out = &wire_Iq_pid_out;
        pid_w.output_m_out = &wire_w_pid_out;
        // initializing parameters
        out_va = 0;
        out_vb = 0;
        out_vc = 0;
        m_ppr = 2;
        m_rpm = 19115; // need to correct
        CPR = 500;     // sine cosine table related

        pid_w.setPara(19115, 256, 0, 0);
        pid_id.setPara(0, 256, 0, 0);
        pid_Te.setPara(0, 256, 0, 0);
    }
    void pullInput() {
        assert(input_Ia_pull != NULL);
        Ia = *input_Ia_pull;
        assert(input_Ib_pull != NULL);
        Ib = *input_Ib_pull;
        assert(input_Ic_pull != NULL);
        Ic = *input_Ic_pull;
        assert(input_w_pull != NULL);
        w = *input_w_pull;
        assert(input_theta_e_pull != NULL);
        theta_e = *input_theta_e_pull;
    };
    void init_ParaType() {
        for (int i = 0; i < num_para; i++) list_paraType[i] = T_INT;
    };
    void updating(double dt) {
        t_cur += dt;
        int args[FOC_ARGS_SIZE];
        // pid_w.setPara(19115, 256, 0, 0);
        // pid_id.setPara(0, 256, 0, 0);
        // pid_Te.setPara(0, 256, 0, 0);

        args[ARGES_IDX_FOC::CONTROL_MODE_ARGS] = FOC_Mode::MOD_SPEED_WITH_TORQUE;
        args[ARGES_IDX_FOC::CONTROL_FIXPERIOD_ARGS] = 0;
        args[ARGES_IDX_FOC::FLUX_SP_ARGS] = pid_id.m_sp;
        args[ARGES_IDX_FOC::FLUX_KP_ARGS] = pid_id.m_kp;
        args[ARGES_IDX_FOC::FLUX_KI_ARGS] = pid_id.m_ki;
        args[ARGES_IDX_FOC::FLUX_KD_ARGS] = pid_id.m_kd;
        args[ARGES_IDX_FOC::TORQUE_SP_ARGS] = pid_Te.m_sp;
        args[ARGES_IDX_FOC::TORQUE_KP_ARGS] = pid_Te.m_kp;
        args[ARGES_IDX_FOC::TORQUE_KI_ARGS] = pid_Te.m_ki;
        args[ARGES_IDX_FOC::TORQUE_KD_ARGS] = pid_Te.m_kd;
        args[ARGES_IDX_FOC::SPEED_SP_ARGS] = pid_w.m_sp; // 19115; // rpm sp
        args[ARGES_IDX_FOC::SPEED_KP_ARGS] = pid_w.m_kp;
        args[ARGES_IDX_FOC::SPEED_KI_ARGS] = pid_w.m_ki;
        args[ARGES_IDX_FOC::SPEED_KD_ARGS] = pid_w.m_kd;
        args[ARGES_IDX_FOC::ANGLE_SH_ARGS] = 0; // ANGLE_SH_REG
        args[ARGES_IDX_FOC::VD_ARGS] = 0;       // vd
        args[ARGES_IDX_FOC::VQ_ARGS] = 15000;   // vq
        args[ARGES_IDX_FOC::FW_KP_ARGS] = 5;    // fw kp
        args[ARGES_IDX_FOC::FW_KI_ARGS] = 2;    // fw ki

        short rpm = w;
        short theta_m = (theta_e + 1) / 2;
        short Va, Vb, Vc;

        const short cpr = 1000;
        foc_data_driven<cpr>(
            // Input for GPIO
            Ia,      // Phase A current
            Ib,      // Phase B current
            Ic,      // Phase B current
            rpm,     // RPM
            theta_e, // Encoder count
            // Output for GPIO
            Va, Vb, Vc, args);

        pid_id.m_sp = args[ARGES_IDX_FOC::FLUX_SP_ARGS];
        pid_id.m_din = args[ARGES_IDX_FOC::ID_STTS];
        pid_id.m_acc = args[ARGES_IDX_FOC::FLUX_ACC_STTS];
        pid_id.m_err_pre = args[ARGES_IDX_FOC::FLUX_ERR_STTS];
        pid_id.m_out = args[ARGES_IDX_FOC::FLUX_OUT_STTS];

        pid_Te.m_sp = pid_w.m_out; // args[ARGES_IDX_FOC::TORQUE_SP_ARGS];
        pid_Te.m_din = args[ARGES_IDX_FOC::IQ_STTS];
        pid_Te.m_acc = args[ARGES_IDX_FOC::TORQUE_ACC_STTS];
        pid_Te.m_err_pre = args[ARGES_IDX_FOC::TORQUE_ERR_STTS];
        pid_Te.m_out = args[ARGES_IDX_FOC::TORQUE_OUT_STTS];

        pid_w.m_sp = args[ARGES_IDX_FOC::SPEED_SP_ARGS];
        pid_w.m_din = args[ARGES_IDX_FOC::SPEED_STTS];
        pid_w.m_acc = args[ARGES_IDX_FOC::SPEED_ACC_STTS];
        pid_w.m_err_pre = args[ARGES_IDX_FOC::SPEED_ERR_STTS];
        pid_w.m_out = args[ARGES_IDX_FOC::SPEED_OUT_STTS];

        out_va = Va;
        out_vb = Vb;
        out_vc = Vc;
    }
    void updating_work(double dt) {
        const int I_SQRT3A = 37837;
        const int SQRT3A_SCALE = 16;
        //--------------------------------------------------------------------------
        typedef short T_Iabc;
        typedef int T_MID;
        const int MAX_OUT = 32767;
        const int MIN_OUT = -32767;

        T_Iabc Ialpha, Ibeta, Ihomopolar; // Transfom result
        Clarke_Direct_3p_T<T_Iabc, T_MID, T_Iabc, MAX_OUT, MIN_OUT, I_SQRT3A, SQRT3A_SCALE>(Ialpha, Ibeta, Ihomopolar,
                                                                                            Ia, Ib, Ic);
        theta_e = (theta_e < 0) ? (theta_e + CPR) : theta_e;    // Correct negative angle
        theta_e = (theta_e >= CPR) ? (theta_e - CPR) : theta_e; // Correct angle overload

        typedef short T_SINCOS;
        typedef uint16_t T_ANGLE;
        const int SINCOS_SCALE = 15;
        T_ANGLE Theta = theta_e;
        //--------------------------------------------------------------------------
        T_Iabc Id, Iq;
        T_SINCOS cos_theta_in = cos_table[(T_ANGLE)Theta];
        T_SINCOS sin_theta_in = sin_table[(T_ANGLE)Theta];
        Park_Direct_T<T_Iabc, T_MID, T_Iabc, MAX_OUT, MIN_OUT, T_SINCOS, SINCOS_SCALE>(Id, Iq, Ialpha, Ibeta,
                                                                                       cos_theta_in, sin_theta_in);

        pid_w.m_din = w;
        PID_Control_T2<int, int, int, 32767, -32767, 8>(pid_w.m_out, pid_w.m_acc, pid_w.m_err_pre, pid_w.m_din,
                                                        pid_w.m_sp, pid_w.m_kp, pid_w.m_ki, pid_w.m_kd, false);
        pid_id.m_din = Id;
        PID_Control_T2<int, int, int, 32767, -32767, 8>(pid_id.m_out, pid_id.m_acc, pid_id.m_err_pre, pid_id.m_din,
                                                        pid_id.m_sp, pid_id.m_kp, pid_id.m_ki, pid_id.m_kd, false);
        pid_Te.m_din = Iq;
        pid_Te.m_sp = pid_w.m_out;
        PID_Control_T2<int, int, int, 32767, -32767, 8>(pid_Te.m_out, pid_Te.m_acc, pid_Te.m_err_pre, pid_Te.m_din,
                                                        pid_Te.m_sp, pid_Te.m_kp, pid_Te.m_ki, pid_Te.m_kd, false);

        t_int Vd = pid_id.m_out; // wire_Id_pid_out;
        t_int Vq = pid_Te.m_out; // wire_Iq_pid_out;

        typedef short T_Vabc;
        const int V_SQRT3C = 56756;
        const int SQRT3A_SCALE2 = 15;
        T_SINCOS cos_theta = cos_theta_in;
        T_SINCOS sin_theta = sin_theta_in;
        T_Vabc Valpha, Vbeta; // Transfom result
        T_Vabc Va, Vb, Vc;
        Park_Inverse_T<T_Vabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, T_SINCOS, SINCOS_SCALE>(Valpha, Vbeta, Vd, Vq,
                                                                                        cos_theta, sin_theta);

        Clarke_Inverse_2p_T<T_Vabc, T_MID, T_Vabc, MAX_OUT, MIN_OUT, V_SQRT3C, SQRT3A_SCALE2>(Va, Vb, Vc, Valpha,
                                                                                              Vbeta);

        out_va = Va;
        out_vb = Vb;
        out_vc = Vc;
    };

    void pushOutput() {
        if (output_va_push != NULL) *output_va_push = out_va;
        if (output_vb_push != NULL) *output_vb_push = out_vb;
        if (output_vc_push != NULL) *output_vc_push = out_vc;
    };
    void printParameters(FILE* fp, int idx) {
        pid_id.printParameters(fp, idx);
        pid_Te.printParameters(fp, idx);
        pid_w.printParameters(fp, idx);
    }
};

#endif
