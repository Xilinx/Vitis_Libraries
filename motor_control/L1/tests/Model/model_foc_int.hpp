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
#ifndef _MODEL_FOC_INT_HPP_
#define _MODEL_FOC_INT_HPP_
#include "stdio.h"
#include "model_base.hpp"
#include "model_pid.hpp"
#include "model_pid_int.hpp"

using namespace xf::motorcontrol;

#define Q16_SCALE 15

/// \brief The other scales could be set
const int scale_park = Q16_SCALE;
const int scale_pid = 8;

/// args for FOC core control
template <class T_ARGS>
struct args_control {
    T_ARGS FOC_Mode;
    T_ARGS FixPeriod;
    T_ARGS Angle_shift;
    T_ARGS args_vd;
    T_ARGS args_vq;
};

/**
 * @brief Args for pid control
 * @tparam T_IN  Type of the input Args
 * @param  sp    Setpoint. ex. [-32767, 32767] for Q16.16
 * @param  kp	 Proportional coefficient. value should be in [0, (1<<KP_SCALE)]
 * @param  ki    Integral coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * @param  kd    differential coefficient. value should be in [0, (1<<(KP_SCALE*2))]
 * */
template <class T_IN>
struct args_pid { // check datapack
    T_IN sp;
    T_IN kp;
    T_IN ki;
    T_IN kd;
    args_pid() { sp = kp = ki = kd = 0; };
    args_pid(T_IN sp_in, T_IN kp_in, T_IN ki_in, T_IN kd_in) {
        sp = sp_in;
        kp = kp_in;
        ki = ki_in;
        kd = kd_in;
    };
};

/**
 * @brief Structure of return data for Status Reg or Moniter Reg
 * @tparam T_REG	    	Type of the return data to Status Reg or Moniter Reg
 * @param status_	    	Return data to Status Reg.
 * @param Ia_f	    		Corrected phase A current as input
 * @param Ib_f				Corrected phase B current as input
 * @param Ic_f				Corrected phase C current as input
 * @param Id				Id as output of Park Direct.
 * @param Iq				Iq as output of Park Direct.
 * @param Vd				Vd as input of Park Inverse
 * @param Vq				Vq as input of Park Inverse
 * @param Valpha			Valpha as input of Clarke Inverse
 * @param Vbeta				Vbeta as input of Clarke Inverse
 * @param FOC_Mode		    FOC mode now
 * @param Ihomopolar		Return homopolar Current to Moniter Reg.
 */
template <class T_REG>
struct ret_monitor {
    T_REG status_RPM;
    T_REG status_Id;
    T_REG status_Iq;
    T_REG status_Angle;
    T_REG Ia_f;
    T_REG Ib_f;
    T_REG Ic_f;
    T_REG Id;
    T_REG Iq;
    T_REG Vd;
    T_REG Vq;
    T_REG Valpha;
    T_REG Vbeta;
    T_REG FOC_Mode;
    T_REG Ihomopolar;
    T_REG pid_id_m_din;
    T_REG pid_id_m_acc;
    T_REG pid_id_m_err_pre;
    T_REG pid_id_m_out;
    T_REG pid_Te_m_din;
    T_REG pid_Te_m_acc;
    T_REG pid_Te_m_err_pre;
    T_REG pid_Te_m_out;
    T_REG pid_w_m_din;
    T_REG pid_w_m_acc;
    T_REG pid_w_m_err_pre;
    T_REG pid_w_m_out;
};

/**
 * @brief Return structure data for Moniter Reg
 * @tparam T_REG	    	Type of the return data to Status Reg or Moniter Reg
 * @param ret	        	Return pointer
 * @param Ia_f	    		Corrected phase A current as output of Filters
 * @param Ib_f				Corrected phase B current as output of Filters
 * @param Ic_f				Corrected phase C current as output of Filters
 * @param Id				Id as output of Park Direct.
 * @param Iq				Iq as output of Park Direct.
 * @param Vd				Vd as input of Park Inverse
 * @param Vq				Vq as input of Park Inverse
 * @param Valpha			Valpha as input of Clarke Inverse
 * @param Vbeta				Vbeta as input of Clarke Inverse
 * @param FOC_Mode		    FOC mode now
 */
template <class T_REG>
void save_ret(ret_monitor<T_REG>* ret,
              T_REG Ia_f,
              T_REG Ib_f,
              T_REG Ic_f,
              T_REG Id,
              T_REG Iq,
              T_REG Vd,
              T_REG Vq,
              T_REG Valpha,
              T_REG Vbeta,
              T_REG FOC_Mode) {
    ret->Ia_f = Ia_f;
    ret->Ib_f = Ib_f;
    ret->Ic_f = Ic_f;
    ret->Id = Id;
    ret->Iq = Iq;
    ret->Vd = Vd;
    ret->Vq = Vq;
    ret->Valpha = Valpha;
    ret->Vbeta = Vbeta;
    ret->FOC_Mode = FOC_Mode;
}

template <class t_int>
class FOC_Simple_2 : public Model_base {
   public:
    //
    int m_ppr;
    int m_rpm;
    int CPR;
    // inputs
    t_int Ia;
    t_int Ib;
    t_int Ic;
    t_int w;
    t_int theta_e;

    t_int* input_Ia_pull;
    t_int* input_Ib_pull;
    t_int* input_Ic_pull;
    t_int* input_w_pull;
    t_int* input_theta_e_pull;
    // output to be push
    t_int out_va;          //	output Votages
    t_int out_vb;          //	output Votages
    t_int out_vc;          //	output Votages
    t_int* output_va_push; //	output Votages to be push
    t_int* output_vb_push; //	output Votages to be push
    t_int* output_vc_push; //	output Votages to be push
    Model_pid_int<t_int> pid_id;
    Model_pid_int<t_int> pid_Te;
    Model_pid_int<t_int> pid_w;

    // middle variable (wire signals)
    t_int wire_Id_pid_in;
    t_int wire_Iq_pid_in;
    t_int wire_w_pid_in;
    t_int wire_Id_pid_out;
    t_int wire_Iq_pid_out;
    t_int wire_w_pid_out;

    // int out_ipark_alpha;
    // int out_ipark_beta;
    // int Vd;
    // int Vq;

    FOC_Simple_2() {
        num_models = 3;
        init_ParaType();
        list_model[0] = (Model_base*)&pid_id;
        list_model[1] = (Model_base*)&pid_Te;
        list_model[2] = (Model_base*)&pid_w;
        pid_id.setObjName(":pid_id");
        pid_Te.setObjName(":pid_Te");
        pid_w.setObjName(":pid_w");
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
        m_ppr = 2;
        m_rpm = 19115; // need to correct
        CPR = 500;     // sine cosine table related

        pid_w.setPara(19115, 256, 0, 0);
        pid_id.setPara(0, 256, 0, 0);
        pid_Te.setPara(0, 256, 0, 0);
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
        for (int i = 0; i < num_para; i++) list_paraType[i] = T_INT;
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
