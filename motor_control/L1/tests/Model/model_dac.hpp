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
#ifndef _MODEL_DAC_HPP_
#define _MODEL_DAC_HPP_
#include "model_base.hpp"
#include "math.h"

template <class t_float, class t_int>
class DAC : public Model_base {
   public:
    int scl; // accuracy of analog-to-digital converter
    t_float Umax;
    // inputs
    t_int Va; //	Input Votages
    t_int Vb; //	Input Votages
    t_int Vc; //	Input Votages

    t_int* input_Va_pull; //	Input Votages to be pull
    t_int* input_Vb_pull; //	Input Votages to be pull
    t_int* input_Vc_pull; //	Input Votages to be pull
    // output to be push
    t_float out_Va;          //	output Votages
    t_float out_Vb;          //	output Votages
    t_float out_Vc;          //	output Votages
    t_float* output_Va_push; //	output Votages to be push
    t_float* output_Vb_push; //	output Votages to be push
    t_float* output_Vc_push; //	output Votages to be push

    DAC() {
        input_Va_pull = NULL;
        input_Vb_pull = NULL;
        input_Vc_pull = NULL;

        output_Va_push = NULL;
        output_Vb_push = NULL;
        output_Vc_push = NULL;
        // initializing parameters
        scl = COMM_ADC_WIDTH - 1;
        Umax = COMM_MOTOR_PARA_UMAX;
    }
    void pullInput() {
        assert(input_Va_pull != NULL);
        Va = *input_Va_pull;
        assert(input_Vb_pull != NULL);
        Vb = *input_Vb_pull;
        assert(input_Vc_pull != NULL);
        Vc = *input_Vc_pull;
    };
    void updating(double dt) {
        out_Va = (Va * Umax) / pow(2, scl);
        out_Vb = (Vb * Umax) / pow(2, scl);
        out_Vc = (Vc * Umax) / pow(2, scl);
        // out_Va = 0.0;
        // out_Vb = 2.0;
        // out_Vc = -2.0;
    };
    void pushOutput() {
        if (output_Va_push != NULL) *output_Va_push = out_Va;
        if (output_Vb_push != NULL) *output_Vb_push = out_Vb;
        if (output_Vc_push != NULL) *output_Vc_push = out_Vc;
    };
};

#endif