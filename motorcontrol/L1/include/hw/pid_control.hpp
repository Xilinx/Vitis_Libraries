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
#ifndef _PID_CONTROL_HPP_
#define _PID_CONTROL_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// PI_Control
// pi = err + GiE_prev;
// p = 1/(1<<SCALE) * (kp*err + ki/(1<<SCALE) * pi)
//--------------------------------------------------------------------------

#ifdef __SYNTHESIS__
template <class T_NUM>
void PI_Control_T_numeral(T_NUM& Res_out, T_NUM in_data, T_NUM Sp, T_NUM Kp, T_NUM Ki) {
#pragma HLS INLINE off
    static T_NUM GiE_prev = 0;
    T_NUM Err = Sp - in_data;
    T_NUM GpE = Err;
    T_NUM GiE = Err + GiE_prev;
    Res_out = 1 / 256 * (Kp * Err + Ki / 256 * GiE);

    GiE_prev = GiE;
}
#endif

/**
 * @brief PI control in the form of an inline HLS function
 * @tparam T_IN	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam KP_SCALE Number significant bits of kp. ex. KP_SCALE is 8 when kp is in [0, 255]
 * @param in_data	Measured process value, input.  ex. [-32767, 32767] for Q16.16
 * @param Sp		Setpoint. ex. [-32767, 32767] for Q16.16
 * @param Kp		Proportional coefficient. value should be in [0, (1<<KP_SCALE)]
 * @param Ki		Integral coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param GiE_prev	Reference to a variable storing the value of an integral error
 * @return			New control variable, output
*/
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PI_Control_T(T_OUT& Res_out, T_MID& GiE_prev, T_IN in_data, T_IN Sp, T_IN Kp, T_IN Ki, bool mode_change) {
#pragma HLS INLINE off
    const int width_mul = 8 * (sizeof(T_IN) + sizeof(T_MID));
    const T_MID limit_ki = (MAX_OUT << KP_SCALE) + KP_SCALE - 1;
    T_MID Err = Clip_AP<T_MID>(Sp - in_data, (T_MID)MIN_OUT, (T_MID)MAX_OUT);
    T_MID GpE = Err;
    T_MID GiE = Clip_AP<T_MID>(Err + ((mode_change > 0) ? (T_MID)0 : GiE_prev), -limit_ki, limit_ki);
    Res_out = Clip_AP<T_OUT>(((ap_int<width_mul>)Kp * GpE + (((ap_int<width_mul>)Ki * GiE) >> KP_SCALE)) >> KP_SCALE,
                             (T_MID)MIN_OUT, (T_MID)MAX_OUT);

    GiE_prev = GiE;
};

template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PI_Control_T_AP(ap_int<OUT_W>& Res_out,
                     ap_int<MID_W>& GiE_prev,
                     ap_int<IN_W> in_data,
                     ap_int<IN_W> Sp,
                     ap_int<IN_W> Kp,
                     ap_int<IN_W> Ki,
                     bool mode_change) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + KP_SCALE * 2);
    PI_Control_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, KP_SCALE>(Res_out, GiE_prev, in_data, Sp,
                                                                                         Kp, Ki, mode_change);
}

//--------------------------------------------------------------------------
// PID_Control
// pd = err - Err_prev;
// pi = err + GiE_prev;
// p = 1/256 * (kp*err + ki/256 * pi + kd/256 * pd)
//--------------------------------------------------------------------------
/**
 * @brief PID control in the form of an inline HLS function
 * @tparam T_IN	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam KP_SCALE Number significant bits of kp. ex. KP_SCALE is 8 when kp is in [0, 255]
 * @param in_data	Measured process value, input.  ex. [-32767, 32767] for Q16.16
 * @param Sp		Setpoint. ex. [-32767, 32767] for Q16.16
 * @param Kp		Proportional coefficient. value should be in [0, (1<<KP_SCALE)]
 * @param Ki		Integral coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param Kd		differential coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param GiE_prev	Reference to a variable storing the value of an integral error
 * @param Err_prev  Reference to a variable storing the value of an derivative error
 * @return			New control variable, output.
*/
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PID_Control_T(T_OUT& Res_out,
                   T_MID& GiE_prev,
                   T_MID& Err_prev,
                   T_IN in_data,
                   T_IN Sp,
                   T_IN Kp,
                   T_IN Ki,
                   T_IN Kd,
                   bool mode_change) {
#pragma HLS INLINE off

    const int width_mul = 8 * (sizeof(T_IN) + sizeof(T_MID));
    const T_MID limit_ki = (MAX_OUT << KP_SCALE) + KP_SCALE - 1;

    T_MID Err = Clip_AP<T_MID>(Sp - in_data, (T_MID)MIN_OUT, (T_MID)MAX_OUT);
    T_MID GpE = Err;
    T_MID GiE = Clip_AP<T_MID>(Err + (mode_change ? (T_MID)0 : GiE_prev), -limit_ki, limit_ki);
    T_MID GdE = Err - Err_prev;
    Res_out = Clip_AP<T_OUT>(
        ((ap_int<width_mul>)Kp * GpE + (((ap_int<width_mul>)Ki * GiE) >> KP_SCALE) + ((ap_int<width_mul>)Kd * GdE) >>
         KP_SCALE) >>
            KP_SCALE,
        MIN_OUT, MAX_OUT);

    // save to reg
    GiE_prev = GiE;
    Err_prev = Err;
}

template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PID_Control_T3_noClip(T_OUT& Res_out,
                           T_MID& GiE_prev,
                           T_MID& Err_prev,
                           T_IN in_data,
                           T_IN Sp,
                           T_IN Kp,
                           T_IN Ki,
                           T_IN Kd,
                           bool mode_change) {
    T_MID err = Sp - in_data;
    T_MID acc = GiE_prev + err;
    T_MID diff = err - Err_prev;
    T_MID P = Kp * err;
    T_MID I = Ki * acc;
    T_MID D = Kd * diff;
    T_MID sum = (P + I + D) >> KP_SCALE;
    T_MID out = in_data * 0 + sum;
    if (out > MAX_OUT)
        Res_out = MAX_OUT;
    else if (out < MIN_OUT)
        Res_out = MIN_OUT;
    else
        Res_out = out;
    Err_prev = err;
    GiE_prev = mode_change == true ? err : acc;
}

template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PID_Control_T3(T_OUT& Res_out,
                    T_MID& GiE_prev,
                    T_MID& Err_prev,
                    T_IN in_data,
                    T_IN Sp,
                    T_IN Kp,
                    T_IN Ki,
                    T_IN Kd,
                    bool mode_change) {
    T_MID err = Sp - in_data;
    T_MID acc = GiE_prev + err;
    const T_MID limit_ki = (MAX_OUT << KP_SCALE) + KP_SCALE - 1;
    if (acc > limit_ki)
        acc = limit_ki;
    else if (acc < -limit_ki)
        acc = -limit_ki;
    T_MID diff = err - Err_prev;
    T_MID P = Kp * err;
    T_MID I = Ki * acc;
    T_MID D = Kd * diff;
    T_MID sum = (P + I + D) >> KP_SCALE;
    T_MID out = in_data * 0 + sum;
    if (out > MAX_OUT)
        Res_out = MAX_OUT;
    else if (out < MIN_OUT)
        Res_out = MIN_OUT;
    else
        Res_out = out;
    Err_prev = err;
    GiE_prev = mode_change == true ? err : acc;
}

template <class T_IO, class T_ACC, class T_ERR, class T_PID>
void PID_Control_ap_fixed(T_IO& Res_out,
                          T_ACC& GiE_prev,
                          T_ERR& Err_prev,
                          T_IO in_data,
                          T_IO Sp,
                          T_PID Kp,
                          T_PID Ki,
                          T_PID Kd,
                          bool mode_change) {
#pragma HLS INLINE
    T_ERR err;
#pragma HLS BIND_OP variable = err op = add impl = dsp
    err = Sp - in_data;

    T_ACC acc;
#pragma HLS BIND_OP variable = acc op = add impl = dsp
    // acc = mode_change==true ? (T_ACC)0 : GiE_prev + err;
    if (mode_change == true)
        acc = 0;
    else
        acc = GiE_prev + err;

    T_ERR diff;
#pragma HLS BIND_OP variable = diff op = add impl = dsp
    diff = err - Err_prev;

    T_ERR P;
#pragma HLS BIND_OP variable = P op = mul impl = dsp
    P = Kp * err;

    T_ERR I;
#pragma HLS BIND_OP variable = I op = mul impl = dsp
    I = Ki * acc;

    T_ERR D;
#pragma HLS BIND_OP variable = D op = mul impl = dsp
    D = Kd * diff;

    T_ACC sum;
#pragma HLS BIND_OP variable = sum op = add impl = dsp
    sum = (P + I + D);
    Res_out = (T_IO)sum;
    Err_prev = err;
    GiE_prev = acc; // mode_change==true? (T_ACC)err : acc;
}

template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int KP_SCALE>
void PID_Control_T_AP(ap_int<OUT_W>& Res_out,
                      ap_int<MID_W>& GiE_prev,
                      ap_int<MID_W>& Err_prev,
                      ap_int<IN_W> in_data,
                      ap_int<IN_W> Sp,
                      ap_int<IN_W> Kp,
                      ap_int<IN_W> Ki,
                      ap_int<IN_W> Kd,
                      bool mode_change) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + KP_SCALE * 2);
    PID_Control_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, KP_SCALE>(
        Res_out, GiE_prev, Err_prev, in_data, Sp, Kp, Ki, Kd, mode_change);
}

#endif
