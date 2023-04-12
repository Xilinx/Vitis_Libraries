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