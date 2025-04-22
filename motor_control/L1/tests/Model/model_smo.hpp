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
#ifndef _MODEL_SMO_HPP_
#define _MODEL_SMO_HPP_
#include "model_base.hpp"
#include "common.hpp"
#include "model_pid.hpp"
//#include "math.h"

template <class t_float, class t_int>
class SMO : public Model_base {
   public:
    enum TYPE_PARA { SMO_h = 0, SMO_k };
    t_int N;       //	Number of pole pairs
    t_float Ld;    //	stator d-axis inductance
    t_float Lq;    //	stator q-axis inductance
    t_float Rs;    //	resistance
    t_float fai_m; //	permanent magnet flux linkage
    // double 	    dt_sim	;//	local time inteval for simulation
    t_float Umax; //	Umax
    // inputs
    t_float Ia; //	Input current
    t_float Ib; //	Input current
    t_float Ic; //	Input current
    t_float Va; //	Input voltage
    t_float Vb; //	Input voltage
    t_float Vc; //	Input voltage

    t_float* input_Ia_pull; //	Input current to be pull
    t_float* input_Ib_pull; //	Input current to be pull
    t_float* input_Ic_pull; //	Input current to be pull
    t_float* input_Va_pull; //	Input voltage to be pull
    t_float* input_Vb_pull; //	Input voltage to be pull
    t_float* input_Vc_pull; //	Input voltage to be pull

    // t_float*    input_w_pull;
    // t_float*    input_theta_e_pull;

    // output to be push

    Model_pid<double> pid_pll;
    t_int* output_w_push;
    t_int* output_theta_e_push;

    // current state variable
    t_float ialpha_est;
    t_float ibeta_est;
    t_float theta_e_est;
    t_float w_e_est;
    t_float w_est;

    // next state variable
    t_float ialpha_est_next;
    t_float ibeta_est_next;
    t_float theta_e_est_next;
    t_float w_e_est_next;

    // middle variable
    t_float valpha; // SMO control input
    t_float vbeta;  // SMO control input
    t_float dialpha_est_div_dt;
    t_float dibeta_est_div_dt;
    t_float E_alpha_fil;
    t_float E_beta_fil;
    t_float Ea_cos;
    t_float Eb_sin;

    t_float wire_pid_in;
    t_float wire_pid_out;
    // parameter
    t_float h;
    t_float k;

    SMO() {
        num_models = 1;
        Model_base::num_para = TYPE_PARA::SMO_k + 1;
        str_names[SMO_h] = "SMO_h      ";
        str_names[SMO_k] = "SMO_k      ";
        list_model[0] = (Model_base*)&pid_pll;
        pid_pll.setObjName("pid_pll");
        input_Ia_pull = NULL;
        input_Ib_pull = NULL;
        input_Ic_pull = NULL;
        input_Va_pull = NULL;
        input_Vb_pull = NULL;
        input_Vc_pull = NULL;

        output_w_push = NULL;
        output_theta_e_push = NULL;
        // initializing parameters
        pid_pll.input_m_din = &wire_pid_in;
        pid_pll.output_m_out = &wire_pid_out;

        // N = 2;//	Number of pole pairs
        // Ld = 0.00022;//	stator d-axis inductance
        // Lq = 0.00022;//	stator q-axis inductance
        // Rs = 0.013;//	resistance
        // fai_m = 0.1;//	permanent magnet flux linkage
        // dt_sim = 0.0001;//	minmal time step for simulation
        // Umax = 2.5;//	Umax

        N = COMM_MACRO_PPR;              //	Number of pole pairs
        Ld = COMM_MOTOR_PARA_LD;         //	stator d-axis inductance
        Lq = COMM_MOTOR_PARA_LD;         //	stator q-axis inductance
        Rs = COMM_MOTOR_PARA_RS;         //	resistance
        fai_m = COMM_MOTOR_PARA_FAI_M;   //	permanent magnet flux linkage
        dt_sim = COMM_MOTOR_PARA_DT_SIM; //	minmal time step for simulation
        Umax = COMM_MOTOR_PARA_UMAX;     //	Umax
        pid_pll.setPara(0, 40000, 0.00001, 0);

        h = COMM_MOTOR_PARA_UMAX;
        k = 100.0;

        ialpha_est = 0;
        ibeta_est = 0;
        theta_e_est = 0;
        w_e_est = 0;

        init_pPara();
        init_ParaType();
    }
    void setPara(t_float h_in, t_float k_in) {
        h = h_in;
        k = k_in;
    }
    void pullInput() {
        assert(input_Ia_pull != NULL);
        Ia = *input_Ia_pull;
        assert(input_Ib_pull != NULL);
        Ib = *input_Ib_pull;
        assert(input_Ic_pull != NULL);
        Ic = *input_Ic_pull;
        assert(input_Va_pull != NULL);
        Va = *input_Va_pull;
        assert(input_Vb_pull != NULL);
        Vb = *input_Vb_pull;
        assert(input_Vc_pull != NULL);
        Vc = *input_Vc_pull;
    };
    void init_ParaType() {
        Model_base::init_ParaType();
        for (int i = 0; i < num_para; i++) list_paraType[i] = T_DOUBLE;
    };
    void init_pPara() {
        Model_base::init_pPara();
        list_pPara[SMO_h] = &h;
        list_pPara[SMO_k] = &k;
    }
    void prepareScreen() {
        Model_base::prepareScreen();
        sprintf(str_screen[SMO_h], "%s : %3.4f", str_names[SMO_h], h);
        sprintf(str_screen[SMO_k], "%s : %3.4f", str_names[SMO_k], k);
        for (int i = 0; i < num_para; i++) {
            strcpy(str_values[i], str_screen[i] + strlen(str_names[i]) + strlen(" : "));
            str_screen[i][this->len_screen - 1] = '\0';
        }
    }
    void updating(double dt) {
        ialpha_est = ialpha_est_next;
        ibeta_est = ibeta_est_next;
        theta_e_est = theta_e_est_next;
        w_e_est = w_e_est_next;
        w_est = w_e_est / COMM_MACRO_PPR;

        t_float Ialpha = (2.0 * Ia - (Ib + Ic)) / 3.0;
        t_float Ibeta = (Ib - Ic) / sqrt(3.0);
        t_float Ualpha = (2.0 * Va - (Vb + Vc)) / 3.0;
        t_float Ubeta = (Vb - Vc) / sqrt(3.0);

        valpha = k * (ialpha_est - Ialpha);
        vbeta = k * (ibeta_est - Ibeta);
        clamp<t_float>(valpha, h);
        clamp<t_float>(vbeta, h);

        E_alpha_fil = valpha; // need low pass filter?
        E_beta_fil = vbeta;   // need low pass filter?

        Ea_cos = E_alpha_fil * cos(theta_e_est);
        Eb_sin = E_beta_fil * sin(theta_e_est);

        this->t_cur += dt;

        wire_pid_in = Eb_sin;
        pid_pll.m_sp = -Ea_cos;
        pid_pll.pullInput();
        pid_pll.updating(dt);
        pid_pll.pushOutput();

        dialpha_est_div_dt = (-Rs * ialpha_est - w_e_est * (Ld - Lq) * ibeta_est + Ualpha - valpha) / Ld;
        dibeta_est_div_dt = (w_e_est * (Ld - Lq) * ialpha_est - Rs * ibeta_est + Ubeta - vbeta) / Ld;

        ialpha_est_next = ialpha_est + dt * dialpha_est_div_dt;
        ibeta_est_next = ibeta_est + dt * dibeta_est_div_dt;
        w_e_est_next = wire_pid_out;
        theta_e_est_next = theta_e_est + dt * w_e_est;
    };
    void pushOutput() {
        if (output_w_push != NULL) *output_w_push = w_e_est;
        if (output_theta_e_push != NULL) *output_theta_e_push = theta_e_est;
    };
    void printParameters(FILE* fp, int idx) {
        pid_pll.printParameters(fp, idx);
        bool isTitle = idx == -1 ? true : false;
        isTitle ? fprintf(fp, "smo_w_est\t") : fprintf(fp, "%0.9f\t", w_est);
        // isTitle ? fprintf(fp, "smo_ialpha_est\t") : fprintf(fp, "%0.9f\t", ialpha_est);
        // isTitle ? fprintf(fp, "smo_ibeta_est\t") : fprintf(fp, "%0.9f\t", ibeta_est);
        // isTitle ? fprintf(fp, "dialpha_est_div_dt\t") : fprintf(fp, "%0.9f\t", dialpha_est_div_dt);
        // isTitle ? fprintf(fp, "dibeta_est_div_dt\t") : fprintf(fp, "%0.9f\t", dibeta_est_div_dt);
    }
};

#endif