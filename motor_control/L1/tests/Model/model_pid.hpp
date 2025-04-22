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
#ifndef _MODEL_PID_HPP_
#define _MODEL_PID_HPP_
#include "model_base.hpp"

template <class t_float>
void pid_updating_core_f(t_float in_Measured,
                         // pre-set parameters
                         t_float Sp,
                         t_float Kp,
                         t_float Ki,
                         t_float Kd,
                         // bool mode_change,
                         // states
                         t_float& Error_prev,
                         t_float& I_err_prev,
                         // instantaneous variables
                         t_float& Err,
                         t_float& dErr,
                         t_float& Res_out,
                         t_float& m_P,
                         t_float& m_I,
                         t_float& m_D,
                         t_float& m_sum) {
    t_float m_bias = 0;

    Err = Sp - in_Measured;
    I_err_prev += Err;
    dErr = Err - Error_prev;
    m_P = Kp * Err;
    m_I = Ki * I_err_prev;
    m_D = Kd * dErr;
    m_sum = m_P + m_I + m_D;
    Res_out = in_Measured * 0 + m_sum + m_bias;
    Error_prev = Err;
}

template <class t_float>
void pid_updating_core_f(t_float in_Measured,
                         // pre-set parameters
                         t_float Sp,
                         t_float Kp,
                         t_float Ki,
                         t_float Kd,
                         // bool mode_change,
                         // states
                         t_float& Error_prev,
                         t_float& I_err_prev,
                         // instantaneous variables
                         t_float& Err,
                         t_float& dErr,
                         t_float& Res_out) {
    t_float m_bias = 0;
    t_float m_P;
    t_float m_I;
    t_float m_D;
    t_float m_sum;

    pid_updating_core_f(in_Measured,
                        // pre-set parameters
                        Sp, Kp, Ki, Kd,
                        // states
                        Error_prev, I_err_prev,
                        // instantaneous variables
                        Err, dErr, Res_out, m_P, m_I, m_D, m_sum);
}

template <class t_float>
class Model_pid : public Model_base {
   public:
    enum TYPE_PARA {
        PID_DIN = 0,
        PID_SP,
        PID_KP,
        PID_KI,
        PID_KD,
        PID_ERR_PRE,
        PID_ACC,
        PID_ERR,
        PID_DIFF,
        PID_P,
        PID_I,
        PID_D,
        PID_SUM,
        PID_BIAS,
        PID_OUT
    };
    // input record
    t_float* input_m_din;
    t_float m_din;
    // pre-set parameters
    t_float m_sp;
    t_float m_kp;
    t_float m_ki;
    t_float m_kd;
    t_float m_bias;
    // states
    t_float m_err_pre;
    t_float m_acc;
    // instantaneous variables
    t_float m_err;
    t_float m_diff;
    t_float m_P;
    t_float m_I;
    t_float m_D;
    t_float m_sum;
    t_float m_out;
    t_float* output_m_out;

    Model_pid() {
        id_type = MODEL_PID;
        num_models = 0;
        Model_base::num_para = TYPE_PARA::PID_OUT + 1;
        str_names[PID_DIN] = "PID_DIN    ";
        str_names[PID_SP] = "PID_SP     ";
        str_names[PID_KP] = "PID_KP     ";
        str_names[PID_KI] = "PID_KI     ";
        str_names[PID_KD] = "PID_KD     ";
        str_names[PID_ERR_PRE] = "PID_ERR_PRE";
        str_names[PID_ACC] = "PID_ACC    ";
        str_names[PID_ERR] = "PID_ERR    ";
        str_names[PID_DIFF] = "PID_DIFF   ";
        str_names[PID_P] = "PID_P      ";
        str_names[PID_I] = "PID_I      ";
        str_names[PID_D] = "PID_D      ";
        str_names[PID_SUM] = "PID_SUM    ";
        str_names[PID_BIAS] = "PID_BIAS   ";
        str_names[PID_OUT] = "PID_OUT    ";
        input_m_din = NULL;
        output_m_out = NULL;
        m_err_pre = 0;
        m_acc = 0;
        m_sp = 0;
        m_kp = 0.5;
        m_ki = 0;
        m_kd = 0;
        m_bias = 0;
    }
    void setPara(t_float sp_in, t_float kp_in, t_float ki_in, t_float kd_in) {
        m_sp = sp_in;
        m_kp = kp_in;
        m_kd = kd_in;
        m_ki = ki_in;
    }
    void stop() {
        m_err_pre = 0.0;
        m_acc = 0.0;
        m_err = 0.0;
        m_diff = 0.0;
        m_out = 0.0;
        m_P = 0.0;
        m_I = 0.0;
        m_D = 0.0;
        m_sum = 0.0;
    }
    void pullInput() {
        assert(input_m_din != NULL);
        m_din = *input_m_din;
    }
    void pushOutput() {
        if (output_m_out != NULL)
            ;
        *output_m_out = m_out;
    }
    void setInput(t_float din) { m_din = din; }
    void getOutput(t_float& out) { out = m_out; }

    void updating(double dt) {
        t_cur += dt;
        pid_updating_core_f(m_din,
                            // pre-set parameters
                            m_sp, m_kp, m_ki, m_kd,
                            // states
                            m_err_pre, m_acc,
                            // instantaneous variables
                            m_err, m_diff, m_out, m_P, m_I, m_D, m_sum);
    }
    void init_ParaType() {
        if (std::is_same<double, t_float>::value)
            for (int i = 0; i < num_para; i++) list_paraType[i] = T_DOUBLE;
        else if (std::is_same<float, t_float>::value)
            for (int i = 0; i < num_para; i++) list_paraType[i] = T_FLOAT;
    };
    void init_pPara() {
        Model_base::init_pPara();
        list_pPara[PID_DIN] = &m_din;
        list_pPara[PID_SP] = &m_sp;
        list_pPara[PID_KP] = &m_kp;
        list_pPara[PID_KI] = &m_ki;
        list_pPara[PID_KD] = &m_kd;
        list_pPara[PID_ERR_PRE] = &m_err_pre;
        list_pPara[PID_ERR] = &m_err;
        list_pPara[PID_ACC] = &m_acc;
        list_pPara[PID_DIFF] = &m_diff;
        list_pPara[PID_P] = &m_P;
        list_pPara[PID_I] = &m_I;
        list_pPara[PID_D] = &m_D;
        list_pPara[PID_SUM] = &m_sum;
        list_pPara[PID_BIAS] = &m_bias;
        list_pPara[PID_OUT] = &m_out;
    }

    void prepareScreen() {
        sprintf(str_screen[PID_DIN], "%s : %3.4f", str_names[PID_DIN], m_din);
        sprintf(str_screen[PID_SP], "%s : %3.4f", str_names[PID_SP], m_sp);
        sprintf(str_screen[PID_KP], "%s : %3.4f", str_names[PID_KP], m_kp);
        sprintf(str_screen[PID_KI], "%s : %3.4f", str_names[PID_KI], m_ki);
        sprintf(str_screen[PID_KD], "%s : %3.4f", str_names[PID_KD], m_kd);
        sprintf(str_screen[PID_ERR_PRE], "%s : %3.4f", str_names[PID_ERR_PRE], m_err_pre);
        sprintf(str_screen[PID_ERR], "%s : %3.4f", str_names[PID_ERR], m_err);
        sprintf(str_screen[PID_ACC], "%s : %4.3f", str_names[PID_ACC], m_acc);
        sprintf(str_screen[PID_DIFF], "%s : %3.4f", str_names[PID_DIFF], m_diff);
        sprintf(str_screen[PID_P], "%s : %3.4f", str_names[PID_P], m_P);
        sprintf(str_screen[PID_I], "%s : %3.4f", str_names[PID_I], m_I);
        sprintf(str_screen[PID_D], "%s : %3.4f", str_names[PID_D], m_D);
        sprintf(str_screen[PID_SUM], "%s : %3.4f", str_names[PID_SUM], m_sum);
        sprintf(str_screen[PID_BIAS], "%s : %3.4f", str_names[PID_BIAS], m_bias);
        sprintf(str_screen[PID_OUT], "%s : %3.4f", str_names[PID_OUT], m_out);
        for (int i = 0; i < num_para; i++) strcpy(str_values[i], str_screen[i] + strlen(str_names[i]) + strlen(" : "));
    }

    void printParameters(FILE* fp, int line) {
        bool isTitle = line == -1 ? true : false;
        isTitle ? fprintf(fp, "m_din\t") : fprintf(fp, "%3.6f\t", m_din);
        isTitle ? fprintf(fp, "m_sp\t") : fprintf(fp, "%3.6f\t", m_sp);
        isTitle ? fprintf(fp, "m_kp\t") : fprintf(fp, "%3.6f\t", m_kp);
        isTitle ? fprintf(fp, "m_ki\t") : fprintf(fp, "%3.6f\t", m_ki);
        isTitle ? fprintf(fp, "m_kd\t") : fprintf(fp, "%3.6f\t", m_kd);
        isTitle ? fprintf(fp, "m_err\t") : fprintf(fp, "%3.6f\t", m_err);
        isTitle ? fprintf(fp, "m_acc\t") : fprintf(fp, "%3.6f\t", m_acc);
        isTitle ? fprintf(fp, "m_diff\t") : fprintf(fp, "%3.6f\t", m_diff);
        isTitle ? fprintf(fp, "m_P\t") : fprintf(fp, "%3.6f\t", m_P);
        isTitle ? fprintf(fp, "m_I\t") : fprintf(fp, "%3.6f\t", m_I);
        isTitle ? fprintf(fp, "m_D\t") : fprintf(fp, "%3.6f\t", m_D);
        isTitle ? fprintf(fp, "m_sum\t") : fprintf(fp, "%3.6f\t", m_sum);
        isTitle ? fprintf(fp, "m_out\t") : fprintf(fp, "%3.6f\t", m_out);
    }
};
#endif