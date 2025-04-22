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
#ifndef _MODEL_ADC_HPP_
#define _MODEL_ADC_HPP_
#include "model_base.hpp"
#include "math.h"

template <class t_float, class t_int>
class ADC : public Model_base {
   public:
    int scl; // accuracy of analog-to-digital converter
    t_float Umax;
    t_float Imax;
    // inputs
    t_float Ia; //	Input current
    t_float Ib; //	Input current
    t_float Ic; //	Input current
    t_float w;
    t_float theta_e;

    t_float* input_Ia_pull; //	Input current to be pull
    t_float* input_Ib_pull; //	Input current to be pull
    t_float* input_Ic_pull; //	Input current to be pull
    t_float* input_w_pull;
    t_float* input_theta_e_pull;
    // output to be push
    t_int out_Ia; //	output current
    t_int out_Ib; //	output current
    t_int out_Ic; //	output current
    t_int out_w;
    t_int out_theta_e;
    t_int* output_Ia_push; //	output current to be push
    t_int* output_Ib_push; //	output current to be push
    t_int* output_Ic_push; //	output current to be push
    t_int* output_w_push;
    t_int* output_theta_e_push;

    ADC() {
        input_Ia_pull = NULL;
        input_Ib_pull = NULL;
        input_Ic_pull = NULL;
        input_w_pull = NULL;
        input_theta_e_pull = NULL;

        output_Ia_push = NULL;
        output_Ib_push = NULL;
        output_Ic_push = NULL;
        output_w_push = NULL;
        output_theta_e_push = NULL;
        // initializing parameters
        scl = COMM_ADC_WIDTH - 1;
        Umax = COMM_MOTOR_PARA_UMAX;
        Imax = COMM_MOTOR_PARA_IMAX;
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
    void updating(double dt) {
        out_Ia = (Ia / Imax) * pow(2, scl);
        out_Ib = (Ib / Imax) * pow(2, scl);
        out_Ic = (Ic / Imax) * pow(2, scl);
        out_w = (w / 2.0 / 3.1415926) * 60;
        out_theta_e = (theta_e / 2 / 3.1415926 * 500);
        // out_Ia = 0;
        // out_Ib = 0;
        // out_Ic = 0;
        // out_w  = 0;
        // out_theta_e = 0;
    };
    void pushOutput() {
        if (output_Ia_push != NULL) *output_Ia_push = out_Ia;
        if (output_Ib_push != NULL) *output_Ib_push = out_Ib;
        if (output_Ic_push != NULL) *output_Ic_push = out_Ic;
        if (output_w_push != NULL) *output_w_push = out_w;
        if (output_theta_e_push != NULL) *output_theta_e_push = out_theta_e;
    };
};

#endif