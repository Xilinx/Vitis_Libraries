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
#ifndef _MODEL_FOC_HPP_
#define _MODEL_FOC_HPP_
#include "model_base.hpp"
#include "model_pid.hpp"
#include "park.hpp"

template <class t_float>
class FOC_Simple_1 : public Model_base {
   public:
    //
    int m_ppr;
    t_float m_Umax;
    // inputs
    t_float Ia; //	Input Votages
    t_float Ib; //	Input Votages
    t_float Ic; //	Input Votages
    t_float w;
    t_float theta_e;

    t_float* input_Ia_pull; //	Input Votages to be pull
    t_float* input_Ib_pull; //	Input Votages to be pull
    t_float* input_Ic_pull; //	Input Votages to be pull
    t_float* input_w_pull;
    t_float* input_theta_e_pull;
    // output to be push
    t_float out_va;          //	output Votages
    t_float out_vb;          //	output Votages
    t_float out_vc;          //	output Votages
    t_float* output_va_push; //	output Votages to be push
    t_float* output_vb_push; //	output Votages to be push
    t_float* output_vc_push; //	output Votages to be push
    Model_pid<double> pid_id;
    Model_pid<double> pid_Te;
    Model_pid<double> pid_w;
    // middle variable (wire signals)
    t_float wire_Id_pid_in;
    t_float wire_Iq_pid_in;
    t_float wire_w_pid_in;
    t_float wire_Id_pid_out;
    t_float wire_Iq_pid_out;
    t_float wire_w_pid_out;
    // Model_pid<double> pid_rpm;
    FOC_Simple_1() {
        id_type = MODEL_FOC;
        num_models = 3;
        list_model[0] = (Model_base*)&pid_id;
        list_model[1] = (Model_base*)&pid_Te;
        list_model[2] = (Model_base*)&pid_w;
        pid_id.setObjName("pid_id");
        pid_Te.setObjName("pid_Te");
        pid_w.setObjName("pid_w");
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
        m_ppr = COMM_MACRO_PPR;
        m_Umax = COMM_MOTOR_PARA_UMAX;
        pid_w.setPara(700, 1.01, 0, 0);
        pid_id.setPara(0, 1, 0, 0);
        pid_Te.setPara(0, 1, 0, 0);
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
        Model_base::init_ParaType();
        for (int i = 0; i < num_para; i++) list_paraType[i] = T_DOUBLE;
    };
    void updating(double dt) {
        t_float Ialpha = (2.0 * Ia - (Ib + Ic)) / 3.0;
        t_float Ibeta = (Ib - Ic) / sqrt(3.0);
        // t_float theta_e = (theta_r * N);
        // t_float theta_e = fmod(theta_e, 2.0 * 3.1415926535);
        t_float cos_theta_in = cos(theta_e);
        t_float sin_theta_in = sin(theta_e);
        t_float Ia_cos = Ialpha * cos_theta_in;
        t_float Ib_sin = Ibeta * sin_theta_in;
        t_float Ib_cos = Ibeta * cos_theta_in;
        t_float Ia_sin = Ialpha * sin_theta_in;
        wire_Id_pid_in = Ia_cos + Ib_sin;
        wire_Iq_pid_in = Ib_cos - Ia_sin;
        wire_w_pid_in = w;

        // t_float v0 = (va + vb + vc)/3.0;
        this->t_cur += dt;

        // pid_w.setPara(7, 1, 0.25, 0);
        pid_w.pullInput();
        pid_w.updating(dt);
        pid_w.pushOutput();

        // pid_id.setPara(0, 1, 0.01, 0);
        pid_id.pullInput();
        pid_id.updating(dt);
        pid_id.pushOutput();

        // pid_Te.setPara(wire_w_pid_out, 1, 0.01, 0);
        pid_Te.m_sp = wire_w_pid_out;
        pid_Te.pullInput();
        pid_Te.updating(dt);
        pid_Te.pushOutput();

        clamp<double>(wire_Id_pid_out, m_Umax);
        clamp<double>(wire_Iq_pid_out, m_Umax);
        double out_ipark_alpha;
        double out_ipark_beta;
        Park_Inverse_T_numeral<double>(out_ipark_alpha, out_ipark_beta, wire_Id_pid_out, wire_Iq_pid_out, theta_e);

        /****************CONTROL Inverse clark*******************/
        double tmp_s3vb = out_ipark_beta * sqrt(3.0);
        out_va = out_ipark_alpha;
        out_vb = (tmp_s3vb - out_ipark_alpha) / 2.0;
        out_vc = -(tmp_s3vb + out_ipark_alpha) / 2.0;
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