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
/*!
 * \file pi_control.h
 */

#ifndef PI_CONTROL_H
#define PI_CONTROL_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>

const int in_width = 16; // Bit width of the input data. ex. short or ap_int<16> is fit for Q16.16
const int mid_width =
    32; // Bit width  of the data in the middle of calculation. ex. int_32t or ap_int<32> is fit for Q16.16
const int out_width = 16; // Bit width  of the output data. ex. short or ap_int<16> is fit for Q16.16
const int max_limit =
    32767; //(1<<(out_width -1) - 1)  // Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
const int min_limit = -32767; // Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16 data width for
                              // middle data in the calculation
const int kp_scale = 8;       // number significant bits of kp. ex. KP_SCALE is 8 when kp is in [0, 255]

/**
 * \brief PI Controller as AXI4-Stream IP core
 * @param s_axis Input AXI4-Stream Feedback data as 16 bit signed integer values
 * @param m_axis Output AXI4-Stream Control
 * @param Sp Value of the setpoint
 * @param Kp Proportional coefficient
 * @param Ki Integral coefficient
 * @param mode Current operation mode of the FOC
 * @param GiE_prev Reference to a variable storing the value of an integral error.
 * \return Functions implementing an IP core do not return a value.
 */
void PI_Control_axi(
    hls::stream<int16_t>& s_axis, hls::stream<int16_t>& m_axis, int16_t Sp, int16_t Kp, int16_t Ki, int32_t mode);

/**
 * \brief PI Controller as AXI4-Stream IP core
 * @param s_axis Input AXI4-Stream Feedback data as 16 bit signed integer values
 * @param m_axis Output AXI4-Stream Control
 * @param Sp Value of the setpoint
 * @param Kp Proportional coefficient. value should be in [0, (1<<KP_SCALE)]
 * @param Ki Integral coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param Ki differential coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param mode Current operation mode of the FOC
 * @param GiE_prev Reference to a variable storing the value of an integral error.
 * @param Err_prev Reference to a variable storing the value of an derivative error.
 * \return Functions implementing an IP core do not return a value.
 */
// limit Limit of the integral part of the control variable.
void PID_Control_axi(hls::stream<int16_t>& s_axis,
                     hls::stream<int16_t>& m_axis,
                     int16_t Sp,
                     int16_t Kp,
                     int16_t Ki,
                     int16_t Kd,
                     int32_t mode,
                     int32_t limit);

#endif // PI_CONTROL_H
