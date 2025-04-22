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
#ifndef _MODEL_MOTOR_HPP_
#define _MODEL_MOTOR_HPP_

#include "model_base.hpp"

template <class t_float, class t_int>
class Model_motor : public Model_base {
   public:
    // including:
    // constant parameters: PPR, N, Ld(H), Lq(H), Rs(Ohm), fai_m, J, TL_th, TL, dt(s), Umax
    // current states : w, theta_r, Id, Iq
    // next states : to be calculated based on current states, inputs and constant parameters
    // instanteneous variables : Valpha, Vbeta, Theta_e, , cos_theta_in, sin_theta_in, Va_cos, Vb_sin, Vb_cos, Va_sin, ,
    // vd, vq, v0, theta_e, d(Id)/dt, d(Iq)/dt, Te, dw/dt, BEMF_d, BEMF_q
    // inputs : va, vb, vc, t
    // nstanteneous variables will be updated after updating inputs
    // constant parameters:
    // t_int 	    PPR	    ;//	Number of pole pairs
    enum TYPE_PARA {
        MT_N = 0,
        MT_Ld,
        MT_Lq,
        MT_Rs,
        MT_fai_m,
        MT_J,
        MT_TL_th,
        MT_TL,
        MT_dt_sim,
        MT_Umax,
        MT_va,
        MT_vb,
        MT_vc,
        MT_Valpha,
        MT_Vbeta,
        MT_cos_tht,
        MT_sin_tht,
        MT_Va_cos,
        MT_Vb_sin,
        MT_Vb_cos,
        MT_Va_sin,
        MT_vd,
        MT_vq,
        MT_v0,
        MT_w,
        MT_theta_r,
        MT_Id,
        MT_Iq,
        MT_theta_e,
        MT_dId_dt,
        MT_dIq_dt,
        MT_Te,
        MT_dw_dt,
        MT_BEMF_d,
        MT_BEMF_q,
        MT_w_next,
        MT_tht_r_n,
        MT_Id_next,
        MT_Iq_next,
        MT_out_Ia,
        MT_out_Ib,
        MT_out_Ic,
        MT_TOTAL_NUM
    };
    t_int N;       //	Number of pole pairs
    t_float Ld;    //	stator d-axis inductance
    t_float Lq;    //	stator q-axis inductance
    t_float Rs;    //	resistance
    t_float fai_m; //	permanent magnet flux linkage
    t_float J;     //	Rotor Ineria
    t_float TL_th; //	static Load Torque threshold
    t_float TL;    //	Load Torque, now in fact it is treated as a instanteneous variable
    // double 	    dt_sim	;//	local time inteval for simulation
    t_float Umax; //	Umax
    // inputs
    t_float va;             //	Input Votages
    t_float vb;             //	Input Votages
    t_float vc;             //	Input Votages
    t_float* input_va_pull; //	Input Votages
    t_float* input_vb_pull; //	Input Votages
    t_float* input_vc_pull; //	Input Votages
    // output to be push
    t_float* output_theta_r_push;
    t_float* output_theta_e_push;
    t_float* output_w_push;
    t_float* output_Id_push;
    t_float* output_Te_push;
    t_float* output_va_push; //	Input Votages
    t_float* output_vb_push; //	Input Votages
    t_float* output_vc_push; //	Input Votages
    t_float out_va;
    t_float out_vb;
    t_float out_vc;
    // instanteneous variables
    t_float Valpha;       //	(2*Ia - (Ib + Ic))/3
    t_float Vbeta;        //	((Ib - Ic) /sqrt(3.0);
    t_float cos_theta_in; //
    t_float sin_theta_in; //
    t_float Va_cos;       //	Ialpha * cos_theta
    t_float Vb_sin;       //	Ibeta * sin_theta
    t_float Vb_cos;       //	Ibeta * cos_theta
    t_float Va_sin;       //	Ialpha * sin_theta

    t_float vd; //	vd=park(clark(va,vb,bc),thete)
    t_float vq; //	vq=park(clark(va,vb,bc),thete)
    t_float v0; //	"v0= 1/3*(va+vb+vc)"
    // old states : w, theta_r, Id, Iq
    t_float w;       // t0: fxied ; tn: next(tn-1)
    t_float theta_r; // t0: fxied ; tn: next(tn-1)
    t_float Id;      // t0: fxied ; tn: next(tn-1)
    t_float Iq;      // t0: fxied ; tn: next(tn-1)

    t_float theta_e; //	"theta_e =theta_r * PPR"
    t_float dId_dt;  //	"did/dt = (vd-Rs*id+N*w*iq*Lq)/Ld"
    t_float dIq_dt;  //	"diq/dt = (vq-Rs*iq-N*w*(id*Ld+fai_m))/Lq"
    t_float Te;      //	Te = 3/2*N*iq*(id*(Ld-Lq)+fai_m)
    t_float dw_dt;   //	dw/dt = (Te-TL)/J
    t_float BEMF_d;  //	BEMF_d = N*w*iq*Lq
    t_float BEMF_q;  //	BEMF_q = -N*w*(id*Ld+fai_m)

    // next states for w, theta_r, Id, Iq
    t_float w_next;       //	" = dw/dt*dt + w"
    t_float theta_r_next; //	" = w*dt + theta_r"
    t_float Id_next;      //	" = did/dt*dt + id"
    t_float Iq_next;      //	" = diq/dt*dt + iq"

    double time_cur; // current time point

    Model_motor() {
        id_type = MODEL_MOTOR;
        num_models = 0;
        Model_base::num_para = TYPE_PARA::MT_TOTAL_NUM;
        str_names[MT_N] = "MT_N      ";
        str_names[MT_Ld] = "MT_Ld     ";
        str_names[MT_Lq] = "MT_Lq     ";
        str_names[MT_Rs] = "MT_Rs     ";
        str_names[MT_fai_m] = "MT_fai_m  ";
        str_names[MT_J] = "MT_J      ";
        str_names[MT_TL_th] = "MT_TL_th  ";
        str_names[MT_TL] = "MT_TL     ";
        str_names[MT_dt_sim] = "MT_dt_sim ";
        str_names[MT_Umax] = "MT_Umax   ";
        str_names[MT_va] = "MT_va     ";
        str_names[MT_vb] = "MT_vb     ";
        str_names[MT_vc] = "MT_vc     ";
        str_names[MT_Valpha] = "MT_Valpha ";
        str_names[MT_Vbeta] = "MT_Vbeta  ";
        str_names[MT_cos_tht] = "MT_cos_tht";
        str_names[MT_sin_tht] = "MT_sin_tht";
        str_names[MT_Va_cos] = "MT_Va_cos ";
        str_names[MT_Vb_sin] = "MT_Vb_sin ";
        str_names[MT_Vb_cos] = "MT_Vb_cos ";
        str_names[MT_Va_sin] = "MT_Va_sin ";
        str_names[MT_vd] = "MT_vd     ";
        str_names[MT_vq] = "MT_vq     ";
        str_names[MT_v0] = "MT_v0     ";
        str_names[MT_w] = "MT_w      ";
        str_names[MT_theta_r] = "MT_theta_r";
        str_names[MT_Id] = "MT_Id     ";
        str_names[MT_Iq] = "MT_Iq     ";
        str_names[MT_theta_e] = "MT_theta_e";
        str_names[MT_dId_dt] = "MT_dId_dt ";
        str_names[MT_dIq_dt] = "MT_dIq_dt ";
        str_names[MT_Te] = "MT_Te     ";
        str_names[MT_dw_dt] = "MT_dw_dt  ";
        str_names[MT_BEMF_d] = "MT_BEMF_d ";
        str_names[MT_BEMF_q] = "MT_BEMF_q ";
        str_names[MT_w_next] = "MT_w_next ";
        str_names[MT_tht_r_n] = "MT_tht_r_n";
        str_names[MT_Id_next] = "MT_Id_next";
        str_names[MT_Iq_next] = "MT_Iq_next";
        str_names[MT_out_Ia] = "MT_out_Ia ";
        str_names[MT_out_Ib] = "MT_out_Ib ";
        str_names[MT_out_Ic] = "MT_out_Ic ";

        // N = 2;//	Number of pole pairs
        // Ld = 0.00022;//	stator d-axis inductance
        // Lq = 0.00022;//	stator q-axis inductance
        // Rs = 0.013;//	resistance
        // fai_m = 0.1;//	permanent magnet flux linkage
        // J = 1;//	Rotor Ineria
        // TL_th = 0;//	static Load Torque threshold
        // //TL = ;//	Load Torque, now in fact it is treated as a instanteneous variable
        // dt_sim = 0.0001;//	minmal time step for simulation
        // Umax = 2.5;//	Umax

        // New parameter of motor

        N = COMM_MACRO_PPR;            //	Number of pole pairs
        Ld = COMM_MOTOR_PARA_LD;       //	stator d-axis inductance
        Lq = COMM_MOTOR_PARA_LD;       //	stator q-axis inductance
        Rs = COMM_MOTOR_PARA_RS;       //	resistance
        fai_m = COMM_MOTOR_PARA_FAI_M; //	permanent magnet flux linkage
        J = COMM_MOTOR_PARA_J;         //	Rotor Ineria
        TL_th = COMM_MOTOR_PARA_TL_TH; //	static Load Torque threshold
        // TL = ;//	Load Torque, now in fact it is treated as a instanteneous variable
        dt_sim = COMM_MOTOR_PARA_DT_SIM; //	minmal time step for simulation
        Umax = COMM_MOTOR_PARA_UMAX;     //	Umax

        input_va_pull = NULL;
        input_vb_pull = NULL;
        input_vc_pull = NULL;
        output_theta_e_push = NULL;
        output_theta_r_push = NULL;
        output_w_push = NULL;
        output_Id_push = NULL;
        output_Te_push = NULL;
        output_va_push = NULL;
        output_vb_push = NULL;
        output_vc_push = NULL;

        w = 0;
        theta_r = 0;
        Id = 0;
        Iq = 0;

        w_next = 0;
        theta_r_next = 0;
        Id_next = 0;
        Iq_next = 0;

        time_cur = 0;
    }
    void setPara(t_int N_in,
                 t_float Ld_in,
                 t_float Lq_in,
                 t_float Rs_in,
                 t_float fai_m_in,
                 t_float J_in,
                 t_float TL_th_in,
                 double dt_sim_in,
                 t_float Umax_in,
                 t_float theta_r_in) {
        N = N_in;
        Ld = Ld_in;
        Lq = Lq_in;
        Rs = Rs_in;
        fai_m = fai_m_in;
        J = J_in;
        TL_th = TL_th_in;
        dt_sim = dt_sim_in;
        Umax = Umax_in;

        w = 0;
        theta_r = 0;
        Id = 0;
        Iq = 0;
        time_cur = 0;

        w_next = 0;
        theta_r_next = 0;
        Id_next = 0;
        Iq_next = 0;
    }
    void stop() {
        w = 0;
        theta_r = 0;
        Id = 0;
        Iq = 0;
        time_cur = 0;

        w_next = 0;
        theta_r_next = 0;
        Id_next = 0;
        Iq_next = 0;

        va = vb = vc = 0;
        updating(0.000001);
    }
    void pullInput() {
        assert(input_va_pull != NULL);
        va = *input_va_pull;
        assert(input_vb_pull != NULL);
        vb = *input_vb_pull;
        assert(input_vc_pull != NULL);
        vc = *input_vc_pull;
    }
    void pushOutput_abc() {
        if (output_va_push != NULL) *output_va_push = out_va;
        if (output_vb_push != NULL) *output_vb_push = out_vb;
        if (output_vc_push != NULL) *output_vc_push = out_vc;
    }
    void pushOutput() {
        if (output_theta_e_push != NULL) *output_theta_e_push = theta_e;
        if (output_theta_r_push != NULL) *output_theta_r_push = theta_e;
        if (output_w_push != NULL) *output_w_push = w;
        if (output_Id_push != NULL) *output_Id_push = Id;
        if (output_Te_push != NULL) *output_Te_push = Te;
        pushOutput_abc();
    }
    void iterating(double dt) {
        w = w_next;
        theta_r = theta_r_next;
        Id = Id_next;
        Iq = Iq_next;
        time_cur += dt;

        Valpha = (2.0 * va - (vb + vc)) / 3.0;
        Vbeta = (vb - vc) / sqrt(3.0);
        theta_e = (theta_r * N);
        theta_e = fmod(theta_e, 2.0 * 3.1415926535);
        cos_theta_in = cos(theta_e);
        sin_theta_in = sin(theta_e);
        Va_cos = Valpha * cos_theta_in;
        Vb_sin = Vbeta * sin_theta_in;
        Vb_cos = Vbeta * cos_theta_in;
        Va_sin = Valpha * sin_theta_in;

        vd = Va_cos + Vb_sin;
        vq = Vb_cos - Va_sin;
        v0 = (va + vb + vc) / 3.0;

        BEMF_d = N * w * Iq * Lq;
        BEMF_q = -N * w * (Id * Ld + fai_m);

        dId_dt = (vd - Rs * Id + BEMF_d) / Ld;
        dIq_dt = (vq - Rs * Iq + BEMF_q) / Lq;
        Te = 3.0 / 2.0 * N * Iq * (Id * (Ld - Lq) + fai_m);

        if ((w == 0) && (abs(Te) < TL_th)) {
            TL = Te;
        } else { // is moving or bigger that TL_th, just using the const value of TL_th
            TL = Te < 0 ? -TL_th : TL_th;
        }

        dw_dt = (Te - TL) / J; // TL is alwasy in negtive direction of Te
        // step-3 update states
        w_next = dw_dt * dt + w;
        theta_r_next = w * dt + theta_r;
        theta_r_next = fmod(theta_r_next, 2.0 * 3.1415926535);
        Id_next = dId_dt * dt + Id;
        Iq_next = dIq_dt * dt + Iq;
        double in_ipark_theta_e = theta_e;
        double in_ipark_id = Id;
        double in_ipark_iq = Iq;
        double out_ipark_alpha;
        double out_ipark_beta;
        Park_Inverse_T_numeral<double>(out_ipark_alpha, out_ipark_beta, in_ipark_id, in_ipark_iq, in_ipark_theta_e);

        /****************CONTROL Inverse clark*******************/
        double tmp_s3vb = out_ipark_beta * sqrt(3.0);
        out_va = out_ipark_alpha;
        out_vb = (tmp_s3vb - out_ipark_alpha) / 2.0;
        out_vc = -(tmp_s3vb + out_ipark_alpha) / 2.0;
    }
    void setInput(t_float va_in, t_float vb_in, t_float vc_in) {
        va = va_in;
        vb = vb_in;
        vc = vc_in;
    }

    void updating(double dt) {
        double peroid = dt;
        assert(peroid > 0);
        while (peroid > dt_sim) {
            iterating(dt_sim);
            peroid -= dt_sim;
        }
        iterating(peroid);
    }
    void init_pPara() {
        Model_base::init_pPara();
        list_pPara[MT_N] = &N;
        list_pPara[MT_Ld] = &Ld;
        list_pPara[MT_Lq] = &Lq;
        list_pPara[MT_Rs] = &Rs;
        list_pPara[MT_fai_m] = &fai_m;
        list_pPara[MT_J] = &J;
        list_pPara[MT_TL_th] = &TL_th;
        list_pPara[MT_TL] = &TL;
        list_pPara[MT_dt_sim] = &dt_sim;
        list_pPara[MT_Umax] = &Umax;
        list_pPara[MT_va] = &va;
        list_pPara[MT_vb] = &vb;
        list_pPara[MT_vc] = &vc;
        list_pPara[MT_Valpha] = &Valpha;
        list_pPara[MT_Vbeta] = &Vbeta;
        list_pPara[MT_cos_tht] = &cos_theta_in;
        list_pPara[MT_sin_tht] = &sin_theta_in;
        list_pPara[MT_Va_cos] = &Va_cos;
        list_pPara[MT_Vb_sin] = &Vb_sin;
        list_pPara[MT_Vb_cos] = &Vb_cos;
        list_pPara[MT_Va_sin] = &Va_sin;
        list_pPara[MT_vd] = &vd;
        list_pPara[MT_vq] = &vq;
        list_pPara[MT_v0] = &v0;
        list_pPara[MT_w] = &w;
        list_pPara[MT_theta_r] = &theta_r;
        list_pPara[MT_Id] = &Id;
        list_pPara[MT_Iq] = &Iq;
        list_pPara[MT_theta_e] = &theta_e;
        list_pPara[MT_dId_dt] = &dId_dt;
        list_pPara[MT_dIq_dt] = &dIq_dt;
        list_pPara[MT_Te] = &Te;
        list_pPara[MT_dw_dt] = &dw_dt;
        list_pPara[MT_BEMF_d] = &BEMF_d;
        list_pPara[MT_BEMF_q] = &BEMF_q;
        list_pPara[MT_w_next] = &w_next;
        list_pPara[MT_tht_r_n] = &theta_r_next;
        list_pPara[MT_Id_next] = &Id_next;
        list_pPara[MT_Iq_next] = &Iq_next;
    }
    void init_ParaType() {
        Model_base::init_ParaType();
        list_paraType[MT_N] = T_INT;
    }
    void prepareScreen() {
        sprintf(str_screen[MT_N], "%s : %9d", str_names[MT_N], N);
        sprintf(str_screen[MT_Ld], "%s : %0.7f", str_names[MT_Ld], Ld);
        sprintf(str_screen[MT_Lq], "%s : %0.9f", str_names[MT_Lq], Lq);
        sprintf(str_screen[MT_Rs], "%s : %3.4f", str_names[MT_Rs], Rs);
        sprintf(str_screen[MT_fai_m], "%s : %3.4f", str_names[MT_fai_m], fai_m);
        sprintf(str_screen[MT_J], "%s : %3.4f", str_names[MT_J], J);
        sprintf(str_screen[MT_TL_th], "%s : %3.4f", str_names[MT_TL_th], TL_th);
        sprintf(str_screen[MT_TL], "%s : %3.4f", str_names[MT_TL], TL);
        sprintf(str_screen[MT_dt_sim], "%s : %0.7f", str_names[MT_dt_sim], dt_sim);
        sprintf(str_screen[MT_Umax], "%s : %3.4f", str_names[MT_Umax], Umax);
        sprintf(str_screen[MT_va], "%s : %3.4f", str_names[MT_va], va);
        sprintf(str_screen[MT_vb], "%s : %3.4f", str_names[MT_vb], vb);
        sprintf(str_screen[MT_vc], "%s : %3.4f", str_names[MT_vc], vc);
        sprintf(str_screen[MT_Valpha], "%s : %3.4f", str_names[MT_Valpha], Valpha);
        sprintf(str_screen[MT_Vbeta], "%s : %3.4f", str_names[MT_Vbeta], Vbeta);
        sprintf(str_screen[MT_cos_tht], "%s : %3.4f", str_names[MT_cos_tht], cos_theta_in);
        sprintf(str_screen[MT_sin_tht], "%s : %3.4f", str_names[MT_sin_tht], sin_theta_in);
        sprintf(str_screen[MT_Va_cos], "%s : %3.4f", str_names[MT_Va_cos], Va_cos);
        sprintf(str_screen[MT_Vb_sin], "%s : %3.4f", str_names[MT_Vb_sin], Vb_sin);
        sprintf(str_screen[MT_Vb_cos], "%s : %3.4f", str_names[MT_Vb_cos], Vb_cos);
        sprintf(str_screen[MT_Va_sin], "%s : %3.4f", str_names[MT_Va_sin], Va_sin);
        sprintf(str_screen[MT_vd], "%s : %3.4f", str_names[MT_vd], vd);
        sprintf(str_screen[MT_vq], "%s : %3.4f", str_names[MT_vq], vq);
        sprintf(str_screen[MT_v0], "%s : %3.4f", str_names[MT_v0], v0);
        sprintf(str_screen[MT_w], "%s : %3.4f", str_names[MT_w], w);
        sprintf(str_screen[MT_theta_r], "%s : %3.4f", str_names[MT_theta_r], theta_r);
        sprintf(str_screen[MT_Id], "%s : %3.4f", str_names[MT_Id], Id);
        sprintf(str_screen[MT_Iq], "%s : %3.4f", str_names[MT_Iq], Iq);
        sprintf(str_screen[MT_theta_e], "%s : %3.4f", str_names[MT_theta_e], theta_e);
        sprintf(str_screen[MT_dId_dt], "%s : %3.4f", str_names[MT_dId_dt], dId_dt);
        sprintf(str_screen[MT_dIq_dt], "%s : %3.4f", str_names[MT_dIq_dt], dIq_dt);
        sprintf(str_screen[MT_Te], "%s : %3.4f", str_names[MT_Te], Te);
        sprintf(str_screen[MT_dw_dt], "%s : %3.4f", str_names[MT_dw_dt], dw_dt);
        sprintf(str_screen[MT_BEMF_d], "%s : %3.4f", str_names[MT_BEMF_d], BEMF_d);
        sprintf(str_screen[MT_BEMF_q], "%s : %3.4f", str_names[MT_BEMF_q], BEMF_q);
        sprintf(str_screen[MT_w_next], "%s : %3.4f", str_names[MT_w_next], w_next);
        sprintf(str_screen[MT_tht_r_n], "%s : %3.4f", str_names[MT_tht_r_n], theta_r_next);
        sprintf(str_screen[MT_Id_next], "%s : %3.4f", str_names[MT_Id_next], Id_next);
        sprintf(str_screen[MT_Iq_next], "%s : %3.4f", str_names[MT_Iq_next], Iq_next);
        sprintf(str_screen[MT_out_Ia], "%s : %3.4f", str_names[MT_out_Ia], out_va);
        sprintf(str_screen[MT_out_Ib], "%s : %3.4f", str_names[MT_out_Ib], out_vb);
        sprintf(str_screen[MT_out_Ic], "%s : %3.4f", str_names[MT_out_Ic], out_vc);

        for (int i = 0; i < num_para; i++) strcpy(str_values[i], str_screen[i] + strlen(str_names[i]) + strlen(" : "));
    }
    void printParameters(FILE* fp, int lin) {
        bool isTitle = lin == -1 ? true : false;
        isTitle ? fprintf(fp, "N\t") : fprintf(fp, "%3d\t", N);
        isTitle ? fprintf(fp, "Ld\t") : fprintf(fp, "%0.9f\t", Ld);
        isTitle ? fprintf(fp, "Lq\t") : fprintf(fp, "%0.9f\t", Lq);
        isTitle ? fprintf(fp, "Rs\t") : fprintf(fp, "%2.3f\t", Rs);
        isTitle ? fprintf(fp, "fai_m\t") : fprintf(fp, "%2.3f\t", fai_m);
        isTitle ? fprintf(fp, "J\t") : fprintf(fp, "%2.3f\t", J);
        isTitle ? fprintf(fp, "TL_th\t") : fprintf(fp, "%2.3f\t", TL_th);
        isTitle ? fprintf(fp, "TL\t") : fprintf(fp, "%2.3f\t", TL);
        isTitle ? fprintf(fp, "dt_sim\t") : fprintf(fp, "%0.9f\t", dt_sim);
        isTitle ? fprintf(fp, "Umax\t") : fprintf(fp, "%3.3f\t", Umax);

        isTitle ? fprintf(fp, "va\t") : fprintf(fp, "%3.3f\t", va);
        isTitle ? fprintf(fp, "vb\t") : fprintf(fp, "%3.3f\t", vb);
        isTitle ? fprintf(fp, "vc\t") : fprintf(fp, "%3.3f\t", vc);

        isTitle ? fprintf(fp, "Valpha\t") : fprintf(fp, "%3.6f\t", Valpha);
        isTitle ? fprintf(fp, "Vbeta\t") : fprintf(fp, "%3.6f\t", Vbeta);

        isTitle ? fprintf(fp, "cos_theta_in\t") : fprintf(fp, "%3.6f\t", cos_theta_in);
        isTitle ? fprintf(fp, "sin_theta_in\t") : fprintf(fp, "%3.6f\t", sin_theta_in);
        isTitle ? fprintf(fp, "Va_cos\t") : fprintf(fp, "%3.6f\t", Va_cos);
        isTitle ? fprintf(fp, "Vb_sin\t") : fprintf(fp, "%3.6f\t", Vb_sin);
        isTitle ? fprintf(fp, "Vb_cos\t") : fprintf(fp, "%3.6f\t", Vb_cos);
        isTitle ? fprintf(fp, "Va_sin\t") : fprintf(fp, "%3.6f\t", Va_sin);
        isTitle ? fprintf(fp, "vd\t") : fprintf(fp, "%3.6f\t", vd);
        isTitle ? fprintf(fp, "vq\t") : fprintf(fp, "%3.6f\t", vq);
        isTitle ? fprintf(fp, "v0\t") : fprintf(fp, "%3.6f\t", v0);

        isTitle ? fprintf(fp, "w\t") : fprintf(fp, "%3.6f\t", w);
        isTitle ? fprintf(fp, "theta_r\t") : fprintf(fp, "%3.6f\t", theta_r);
        isTitle ? fprintf(fp, "Id\t") : fprintf(fp, "%3.6f\t", Id);
        isTitle ? fprintf(fp, "Iq\t") : fprintf(fp, "%3.6f\t", Iq);
        isTitle ? fprintf(fp, "theta_e\t") : fprintf(fp, "%3.6f\t", theta_e);
        isTitle ? fprintf(fp, "dId_dt\t") : fprintf(fp, "%3.6f\t", dId_dt);
        isTitle ? fprintf(fp, "dIq_dt\t") : fprintf(fp, "%3.6f\t", dIq_dt);
        isTitle ? fprintf(fp, "Te\t") : fprintf(fp, "%3.6f\t", Te);
        isTitle ? fprintf(fp, "dw_dt\t") : fprintf(fp, "%3.6f\t", dw_dt);
        isTitle ? fprintf(fp, "BEMF_d\t") : fprintf(fp, "%3.6f\t", BEMF_d);
        isTitle ? fprintf(fp, "BEMF_q\t") : fprintf(fp, "%3.6f\t", BEMF_q);

        isTitle ? fprintf(fp, "w_next \t") : fprintf(fp, "%3.6f\t", w_next);
        isTitle ? fprintf(fp, "theta_r_next\t") : fprintf(fp, "%3.6f\t", theta_r_next);
        isTitle ? fprintf(fp, "Id_next\t") : fprintf(fp, "%3.6f\t", Id_next);
        isTitle ? fprintf(fp, "Iq_next\t") : fprintf(fp, "%3.6f\t", Iq_next);
        isTitle ? fprintf(fp, "out_va\t") : fprintf(fp, "%3.6f\t", out_va);
        isTitle ? fprintf(fp, "out_vb\t") : fprintf(fp, "%3.6f\t", out_vb);
        isTitle ? fprintf(fp, "out_vc\t") : fprintf(fp, "%3.6f\t", out_vc);
    }
    void printInputIaIbIcRpmTheta(FILE* fp, int lin) {
        bool isTitle = lin == -1 ? true : false;

        isTitle ? fprintf(fp, "va\t") : fprintf(fp, "%3.3f\t", va);
        isTitle ? fprintf(fp, "vb\t") : fprintf(fp, "%3.3f\t", vb);
        isTitle ? fprintf(fp, "vc\t") : fprintf(fp, "%3.3f\t", vc);
        isTitle ? fprintf(fp, "theta_r\t") : fprintf(fp, "%3.6f\t", theta_r);
        isTitle ? fprintf(fp, "w\n") : fprintf(fp, "%3.6f\n", w);
    }
    void printOutputGolden(FILE* fp, int lin) {
        bool isTitle = lin == -1 ? true : false;

        isTitle ? fprintf(fp, "out_va\t") : fprintf(fp, "%3.6f\t", out_va);
        isTitle ? fprintf(fp, "out_vb\t") : fprintf(fp, "%3.6f\t", out_vb);
        isTitle ? fprintf(fp, "out_vc\n") : fprintf(fp, "%3.6f\n", out_vc);
    }
};

#endif