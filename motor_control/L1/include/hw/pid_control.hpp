/*
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
#ifndef _PID_CONTROL_HPP_
#define _PID_CONTROL_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// PID_Control
// pd = err - Error_prev;
// pi = err + I_err_prev;
// p = 1/256 * (kp*err + ki/256 * pi + kd/256 * pd)
//--------------------------------------------------------------------------
/**
 * brief PI control in the form of an inline HLS function
 * tparam T_IO	        Datatype of the input/output ap_fixed. ex. ap_fixed<32,16> is enough for Q16.16
 * tparam T_ACC	    Datatype of the ap_fixed integral error. Range extension of T_IO is required when precise
 * control Area/resource.
 * tparam T_ERR	    Datatype of the ap_fixed error. Range extension of T_IO is required when precise control
 * Area/resource.
 * tparam T_PID	    Datatype of the ap_fixed coefficient.
 * param Res_out	    Result Output
 * param I_err_prev	Reference to a variable storing the value of an integral error
 * param Error_prev	    Reference to a variable storing the value of an error
 * param in_Measured	Measured process value, input. Precision and range are relative to T_IO
 * param Sp		    Setpoint. Precision and range are relative to T_IO
 * param Kp		    Proportional coefficient. Precision and range are relative to T_PID
 * param Ki		    Integral coefficient. Precision and range are relative to T_PID
 * param Kd		    differential coefficient. Precision and range are relative to T_PID
 * param mode_change	is mode_change ? 1 : 0
*/
template <class T_IO, class T_ACC, class T_ERR, class T_PID>
void PID_Control_ap_fixed(T_IO& Res_out,
                          T_ACC& I_err_prev,
                          T_ERR& Error_prev,
                          T_IO in_Measured,
                          T_IO Sp,
                          T_PID Kp,
                          T_PID Ki,
                          T_PID Kd,
                          bool mode_change) {
#pragma HLS INLINE
    T_ERR err;
    err = Sp - in_Measured;

    T_ACC acc;
    // acc = mode_change==true ? (T_ACC)0 : I_err_prev + err;
    if (mode_change == true)
        acc = 0;
    else
        acc = I_err_prev + err;

    T_ERR diff;
    diff = err - Error_prev;

    T_ERR P;
    P = Kp * err;

    T_ERR I;
    I = Ki * acc;

    T_ERR D;
    D = Kd * diff;

    T_ACC sum;
    sum = (P + I + D);
    Res_out = (T_IO)sum;
    Error_prev = err;
    I_err_prev = acc; // mode_change==true? (T_ACC)err : acc;
}

#endif
