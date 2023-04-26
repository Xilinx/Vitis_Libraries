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
#ifndef _FOC_SENSORLESS_HPP_
#define _FOC_SENSORLESS_HPP_
#include "foc.hpp"
#include "smo.hpp"
// clang-format on
namespace xf {
namespace motorcontrol {
//--------------------------------------------------------------------------
// FOC-sensorless templated top functions'hls_foc_strm_ap_fixed' has following features
// 1) Based on ap_fixed data type to retain the meaning of physical quantities;
// 2) stream-style input and output consumed and fed withing a almost infinate loop.
//--------------------------------------------------------------------------
// clang-format off
/**
 * @brief sensorless field-orientated control (FOC) in the form of a demo
 * @tparam VALUE_CPR        Number of encoder steps per one full revolution. ex. 1000
 * @tparam T_IO             Data type for input currents and output commands. ex. ap_fixed<24, 8>
 * @tparam MAX_IO           Maximum absolute value for input currents and output commands. ex. 24(V) 
 * @tparam W                Width of T_IO. ex. 24 for ap_fixed<24, 8>
 * @tparam I                Integer part width of T_IO(inluding sign bit). ex. 8 for ap_fixed<24, 8>
 * @tparam T_RPM_THETA_FOC  Data type for packaged RPM and Theta scale value mode by VALUE_CPR, 32-bit aligned  
 * @param  Ia			    Input Phase A current
 * @param  Ib			    Input Phase B current
 * @param  Ic			    Input Phase C current
 * @param  Va_smo			Input Phase A voltage
 * @param  Vb_smo			Input Phase B voltage
 * @param  Vc_smo			Input Phase C voltage
 * @param  Va_cmd 			Output Va
 * @param  Vb_cmd 			Output Vb
 * @param  Vc_cmd 			Output Vc
 * @param  ppr_args         input number of pole pairs per phase of the motor; full sinus periods per revolution.
 * @param  control_mode_args            Input control mode of foc, enum FOC_Mode. Read every latency cycles of LOOP_FOC
 * @param  control_fixperiod_args       input control_fixperiod. Read every latency cycles of LOOP_FOC
 * @param  flux_sp_args     Input Args setting point for PID control of Flux
 * @param  flux_kp_args     Input Args Proportional coefficient for PID control of Flux
 * @param  flux_ki_args     Input Args Integral coefficient for PID control of Flux
 * @param  flux_kd_args     Input Args Differential coefficient for PID control of Flux
 * @param  torque_sp_args    Input Args setting point for PID control of Torque
 * @param  torque_kp_args    Input Args Proportional coefficient for PID control of Torque
 * @param  torque_ki_args    Input Args Integral coefficient for PID control of Torque
 * @param  torque_kd_args    Input Args Differential coefficient for PID control of Torque
 * @param  speed_sp_args    Input Args setting point for PID control of RPM
 * @param  speed_kp_args    Input Args Proportional coefficient for PID control of RPM
 * @param  speed_ki_args    Input Args Integral coefficient for PID control of RPM
 * @param  speed_kd_args    Input Args Differential coefficient for PID control of RPM
 * @param  angle_sh_args    Input Args for angle shift
 * @param  vd_args          Input Args for setting fixed vd
 * @param  vq_args          Input Args for setting fixed vq
 * @param  fw_kp_args       Input Args setting point for PID control of field weakening
 * @param  fw_ki_args       Input Args Integral coefficient for PID control of field weakening
 * @param  id_stts          Output status to monitor stator d-axis current
 * @param  flux_acc_stts    Output status to monitor flux accumulate value
 * @param  flux_err_stts    Output status to monitor flux latest error value
 * @param  flux_out_stts    Output status to monitor flux PID's output
 * @param  iq_stts          Output status to monitor stator q-axis current
 * @param  torque_acc_stts  Output status to monitor torque accumulate value
 * @param  torque_err_stts  Output status to monitor torque latest error value
 * @param  torque_out_stts  Output status to monitor torque PID's output
 * @param  speed_stts       Output status to monitor speed(RPM) of motor
 * @param  speed_acc_stts   Output status to monitor speed(RPM) accumulate value
 * @param  speed_err_stts   Output status to monitor speed(RPM) latest error value
 * @param  speed_out_stts   Output status to monitor speed(RPM)  PID's output
 * @param  angle_stts       Output status to monitor Theta_m of motor (scale value to [0, VALUE_CPR]) 
 * @param  Va_cmd_stts      Output status to monitor Output Va
 * @param  Vb_cmd_stts      Output status to monitor Output Vb
 * @param  Vc_cmd_stts      Output status to monitor Output Vc
 * @param  Ialpha_stts      Output status to monitor Ialpha (output of Clarke_Direct) 
 * @param  Ibeta_stts       Output status to monitor Ibeta (output of Clarke_Direct) 
 * @param  Ihomopolar_stts  Output status to monitor Ihomopolar (output of Clarke_Direct) 
 * @param  trip_cnt         Input Args to set the trip count of foc loop
 */
// clang-format on
template <int VALUE_CPR, typename T_IO, int MAX_IO, int W, int I, typename T_RPM_THETA_FOC>
void hls_foc_strm_ap_fixed_sensorless(
    // Input
    hls::stream<T_IO>& Ia,
    hls::stream<T_IO>& Ib,
    hls::stream<T_IO>& Ic,
    hls::stream<T_IO>& Va_smo,
    hls::stream<T_IO>& Vb_smo,
    hls::stream<T_IO>& Vc_smo,
    // hls::stream<T_RPM_THETA_FOC>& FOC_RPM_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<T_IO>& Va_cmd,
    hls::stream<T_IO>& Vb_cmd,
    hls::stream<T_IO>& Vc_cmd,
    // In-out for parameters
    volatile int& ppr_args,
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    //
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Va_cmd_stts,
    volatile int& Vb_cmd_stts,
    volatile int& Vc_cmd_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts,
    volatile long& trip_cnt) {
    short cpr_div_ppr = VALUE_CPR / ppr_args;
    unsigned int tab_map_factor = ((COMM_MACRO_TLB_LENTH * (unsigned int)ppr_args) << 16) / VALUE_CPR;
    const T_IO RPM_factor = 60.0 / 2 / 3.1415926535;
    T_IO speed_to_RPM = RPM_factor / (T_IO)ppr_args;
    T_IO RPM_to_speed = (T_IO)ppr_args / RPM_factor;
LOOP_FOC_STRM:
    for (long i = 0; i < trip_cnt; i++) {
//#pragma HLS pipeline II = 1
#pragma HLS PIPELINE off
        bool Filebased_flag = (control_mode_args & 0x40000000) != 0;
        int control_mode = control_mode_args & 0x0000FFFF;
        // static int FOC_RPM_THETA_m_in; // = FOC_RPM_THETA_m.read();
        static T_IO Ia_in;
        static T_IO Ib_in;
        static T_IO Ic_in;
        static T_IO Va_in;
        static T_IO Vb_in;
        static T_IO Vc_in;
        if (Filebased_flag) {
            if (!Ia.empty()) {
                Ia_in = Ia.read();
                Ib_in = Ib.read();
                Ic_in = Ic.read();

                Va_in = Va_smo.read();
                Vb_in = Vb_smo.read();
                Vc_in = Vc_smo.read();
            }
        } else {
            while (!Ia.empty()) { // hardware on board
                Ia_in = Ia.read();
                Ib_in = Ib.read();
                Ic_in = Ic.read();

                Va_in = Va_smo.read();
                Vb_in = Vb_smo.read();
                Vc_in = Vc_smo.read();
            }
        }
        // if (!FOC_RPM_THETA_m.empty()) FOC_RPM_THETA_m_in = FOC_RPM_THETA_m.read();

        // short RPM_in = (FOC_RPM_THETA_m_in & 0x0000FFFF);
        // short Angle_in = (FOC_RPM_THETA_m_in & 0xFFFF0000) >> 16;

        short RPM_in = 0;
        short Angle_in = 0;

        T_IO Va_out, Vb_out, Vc_out;
        // reserved word
        // int ppr_args = 0;
        // int cpr_args = 0;
        int trigger_args = 0;
        int control2_args = 0;

        // details::foc_core_ap_fixed<VALUE_CPR, T_IO, MAX_IO , W, I>(
        foc_core_ap_fixed_sensorless<VALUE_CPR, T_IO, MAX_IO, W, I>(
            Ia_in, Ib_in, Ic_in, Va_in, Vb_in, Vc_in, RPM_in, Angle_in, Va_out, Vb_out, Vc_out, tab_map_factor,
            cpr_div_ppr, speed_to_RPM, RPM_to_speed, control_mode, control_fixperiod_args, flux_sp_args, flux_kp_args,
            flux_ki_args, flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args,
            speed_kp_args, speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args, trigger_args, control2_args,
            fw_kp_args, fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Ialpha_stts,
            Ibeta_stts, Ihomopolar_stts);

        T_IO va = Va_out;
        T_IO vb = Vb_out;
        T_IO vc = Vc_out;

        t_glb_q15q16 apx_va = va;
        t_glb_q15q16 apx_vb = vb;
        t_glb_q15q16 apx_vc = vc;

        Va_cmd_stts = apx_va(31, 0);
        Vb_cmd_stts = apx_vb(31, 0);
        Vc_cmd_stts = apx_vc(31, 0);

        if (!Va_cmd.full()) Va_cmd.write(va);
        if (!Vb_cmd.full()) Vb_cmd.write(vb);
        if (!Vc_cmd.full()) Vc_cmd.write(vc);
    } // for(long i = 0; i < trip_cnt; i++)
}

template <int VALUE_CPR, int MAX_IO, int W, int I, typename T_RPM_THETA_FOC>
void hls_foc_strm_int_sensorless(
    // Input
    hls::stream<int>& Ia,
    hls::stream<int>& Ib,
    hls::stream<int>& Ic,
    hls::stream<int>& Va_smo,
    hls::stream<int>& Vb_smo,
    hls::stream<int>& Vc_smo,
    // hls::stream<T_RPM_THETA_FOC>& FOC_RPM_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<int>& Va_cmd,
    hls::stream<int>& Vb_cmd,
    hls::stream<int>& Vc_cmd,
    // In-out for parameters
    volatile int& ppr_args,
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    //
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Va_cmd_stts,
    volatile int& Vb_cmd_stts,
    volatile int& Vc_cmd_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts,
    volatile long& trip_cnt) {
    short cpr_div_ppr = VALUE_CPR / ppr_args;
    unsigned int tab_map_factor = ((COMM_MACRO_TLB_LENTH * (unsigned int)ppr_args) << 16) / VALUE_CPR;
    const t_glb_q15q16 RPM_factor = 60.0 / 2 / 3.1415926535;
    t_glb_q15q16 speed_to_RPM = RPM_factor / (t_glb_q15q16)ppr_args;
    t_glb_q15q16 RPM_to_speed = (t_glb_q15q16)ppr_args / RPM_factor;
LOOP_FOC_STRM:
    for (long i = 0; i < trip_cnt; i++) {
#pragma HLS pipeline II = 1
        // static int FOC_RPM_THETA_m_in;
        static int Ia_in0;
        static int Ib_in0;
        static int Ic_in0;
        static int Va_in0;
        static int Vb_in0;
        static int Vc_in0;
        if (!Ia.empty()) {
            Ia_in0 = Ia.read();
            Ib_in0 = Ib.read();
            Ic_in0 = Ic.read();

            Va_in0 = Va_smo.read();
            Vb_in0 = Vb_smo.read();
            Vc_in0 = Vc_smo.read();
        }
        t_glb_q15q16 Ia_in;
        t_glb_q15q16 Ib_in;
        t_glb_q15q16 Ic_in;

        t_glb_q15q16 Va_in;
        t_glb_q15q16 Vb_in;
        t_glb_q15q16 Vc_in;

        Ia_in(31, 0) = Ia_in0;
        Ib_in(31, 0) = Ib_in0;
        Ic_in(31, 0) = Ic_in0;

        Va_in(31, 0) = Va_in0;
        Vb_in(31, 0) = Vb_in0;
        Vc_in(31, 0) = Vc_in0;

        // if (!FOC_RPM_THETA_m.empty()) FOC_RPM_THETA_m_in = FOC_RPM_THETA_m.read();

        // short RPM_in = (FOC_RPM_THETA_m_in & 0x0000FFFF);
        // short Angle_in = (FOC_RPM_THETA_m_in & 0xFFFF0000) >> 16;
        short RPM_in = 0;
        short Angle_in = 0;

        typedef ap_fixed<W, I> T_M;
        T_M Va_out, Vb_out, Vc_out;

        // reserved word
        // int ppr_args = 0;
        // int cpr_args = 0;
        int trigger_args = 0;
        int control2_args = 0;

        foc_core_ap_fixed_sensorless<VALUE_CPR, T_M, MAX_IO, W, I>(
            (T_M)Ia_in, (T_M)Ib_in, (T_M)Ic_in, (T_M)Va_in, (T_M)Vb_in, (T_M)Vc_in, RPM_in, Angle_in, Va_out, Vb_out,
            Vc_out, tab_map_factor, cpr_div_ppr, (T_M)speed_to_RPM, (T_M)RPM_to_speed, control_mode_args,
            control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args, torque_sp_args,
            torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args, speed_kd_args,
            angle_sh_args, vd_args, vq_args, trigger_args, control2_args, fw_kp_args, fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Ialpha_stts,
            Ibeta_stts, Ihomopolar_stts);

        t_glb_q15q16 apx_va = Va_out;
        t_glb_q15q16 apx_vb = Vb_out;
        t_glb_q15q16 apx_vc = Vc_out;

        Va_cmd_stts = apx_va(31, 0);
        Vb_cmd_stts = apx_vb(31, 0);
        Vc_cmd_stts = apx_vc(31, 0);

        if (!Va_cmd.full()) Va_cmd.write(apx_va(31, 0));
        if (!Vb_cmd.full()) Vb_cmd.write(apx_vb(31, 0));
        if (!Vc_cmd.full()) Vc_cmd.write(apx_vc(31, 0));
    } // for(long i = 0; i < trip_cnt; i++)
}
// clang-format on

} // namespace motorcontrol
} // namespace xf
#endif
