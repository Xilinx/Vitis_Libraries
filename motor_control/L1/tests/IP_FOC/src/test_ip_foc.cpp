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
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <ncurses.h>

#include <iostream>
#include <utility>
#include <vector>
#include <numeric>
#include <fstream>
#include <sstream>

#include "foc_demo.hpp"
#include "test_foc_modes.hpp"

#include "ip_foc.hpp"

using namespace xf::motorcontrol;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void call_foc_strm(bool usePeriodic,
                   hls::stream<t_glb_foc2pwm>& strm_ia,
                   hls::stream<t_glb_foc2pwm>& strm_ib,
                   hls::stream<t_glb_foc2pwm>& strm_ic,
                   hls::stream<t_glb_speed_theta>& strm_speed_theta_m, // RPM & Theta_m
                   // Output
                   hls::stream<t_glb_foc2pwm>& Va_cmd,
                   hls::stream<t_glb_foc2pwm>& Vb_cmd,
                   hls::stream<t_glb_foc2pwm>& Vc_cmd,
                   FocAxiParameters<t_glb_q15q16>& AxiPara) {
    if (usePeriodic)
        // clang-format off
            hls_foc_periodic_ap_fixed(//<COMM_MACRO_CPR, t_glb_foc2pwm,  MAX_VAL_PWM, PWM_AP_FIXED_PARA_W2, PWM_AP_FIXED_PARA_I2, t_glb_speed_theta>(//foc_strm_driven_ap_fixed //hls_foc_dataflow_ap_fixed//foc_core_ap_fixed
                strm_ia, strm_ib, strm_ic,
                strm_speed_theta_m,
                Va_cmd, Vb_cmd, Vc_cmd,
                (volatile int&)AxiPara.ppr_args,
                (volatile int&)AxiPara.control_mode_args,
                (volatile int&)AxiPara.control_fixperiod_args,
                (volatile int&)AxiPara.flux_sp_args,
                (volatile int&)AxiPara.flux_kp_args,
                (volatile int&)AxiPara.flux_ki_args,
                (volatile int&)AxiPara.flux_kd_args,
                (volatile int&)AxiPara.torque_sp_args,
                (volatile int&)AxiPara.torque_kp_args,
                (volatile int&)AxiPara.torque_ki_args,
                (volatile int&)AxiPara.torque_kd_args,
                (volatile int&)AxiPara.speed_sp_args,
                (volatile int&)AxiPara.speed_kp_args,
                (volatile int&)AxiPara.speed_ki_args,
                (volatile int&)AxiPara.speed_kd_args,
                (volatile int&)AxiPara.angle_sh_args,
                (volatile int&)AxiPara.vd_args,
                (volatile int&)AxiPara.vq_args,
                (volatile int&)AxiPara.fw_kp_args,
                (volatile int&)AxiPara.fw_ki_args,
                //
                (volatile int&)AxiPara.id_stts,
                (volatile int&)AxiPara.flux_acc_stts,
                (volatile int&)AxiPara.flux_err_stts,
                (volatile int&)AxiPara.flux_out_stts,
                (volatile int&)AxiPara.iq_stts,
                (volatile int&)AxiPara.torque_acc_stts,
                (volatile int&)AxiPara.torque_err_stts,
                (volatile int&)AxiPara.torque_out_stts,
                (volatile int&)AxiPara.speed_stts,
                (volatile int&)AxiPara.speed_acc_stts,
                (volatile int&)AxiPara.speed_err_stts,
                (volatile int&)AxiPara.speed_out_stts,
                (volatile int&)AxiPara.angle_stts,
                (volatile int&)AxiPara.Va_cmd_stts, 
                (volatile int&)AxiPara.Vb_cmd_stts, 
                (volatile int&)AxiPara.Vc_cmd_stts,
                (volatile int&)AxiPara.Ialpha_stts, 
                (volatile int&)AxiPara.Ibeta_stts, 
                (volatile int&)AxiPara.Ihomopolar_stts,
                (volatile int&)AxiPara.fixed_angle_args
            );
        else
            hls_foc_oneSample_ap_fixed(
                strm_ia, strm_ib, strm_ic,
                strm_speed_theta_m,
                Va_cmd, Vb_cmd, Vc_cmd,
                (volatile int&)AxiPara.ppr_args,
                (volatile int&)AxiPara.control_mode_args,
                (volatile int&)AxiPara.control_fixperiod_args,
                (volatile int&)AxiPara.flux_sp_args,
                (volatile int&)AxiPara.flux_kp_args,
                (volatile int&)AxiPara.flux_ki_args,
                (volatile int&)AxiPara.flux_kd_args,
                (volatile int&)AxiPara.torque_sp_args,
                (volatile int&)AxiPara.torque_kp_args,
                (volatile int&)AxiPara.torque_ki_args,
                (volatile int&)AxiPara.torque_kd_args,
                (volatile int&)AxiPara.speed_sp_args,
                (volatile int&)AxiPara.speed_kp_args,
                (volatile int&)AxiPara.speed_ki_args,
                (volatile int&)AxiPara.speed_kd_args,
                (volatile int&)AxiPara.angle_sh_args,
                (volatile int&)AxiPara.vd_args,
                (volatile int&)AxiPara.vq_args,
                (volatile int&)AxiPara.fw_kp_args,
                (volatile int&)AxiPara.fw_ki_args,
                //
                (volatile int&)AxiPara.id_stts,
                (volatile int&)AxiPara.flux_acc_stts,
                (volatile int&)AxiPara.flux_err_stts,
                (volatile int&)AxiPara.flux_out_stts,
                (volatile int&)AxiPara.iq_stts,
                (volatile int&)AxiPara.torque_acc_stts,
                (volatile int&)AxiPara.torque_err_stts,
                (volatile int&)AxiPara.torque_out_stts,
                (volatile int&)AxiPara.speed_stts,
                (volatile int&)AxiPara.speed_acc_stts,
                (volatile int&)AxiPara.speed_err_stts,
                (volatile int&)AxiPara.speed_out_stts,
                (volatile int&)AxiPara.angle_stts,
                (volatile int&)AxiPara.Va_cmd_stts, 
                (volatile int&)AxiPara.Vb_cmd_stts, 
                (volatile int&)AxiPara.Vc_cmd_stts,
                (volatile int&)AxiPara.Ialpha_stts, 
                (volatile int&)AxiPara.Ibeta_stts, 
                (volatile int&)AxiPara.Ihomopolar_stts,
                (volatile int&)AxiPara.fixed_angle_args
                );
    // clang-format on
}
int call_foc_scalar(bool usePeriodic,
                    t_glb_foc2pwm iaf,
                    t_glb_foc2pwm ibf,
                    t_glb_foc2pwm icf,
                    float motor_w,
                    float motor_theta_r,
                    // Output
                    t_glb_foc2pwm& vaf,
                    t_glb_foc2pwm& vbf,
                    t_glb_foc2pwm& vcf,
                    FocAxiParameters<t_glb_q15q16>& AxiPara) {
    short rpm = OmigaToRpm(motor_w);
    short theta_m = Theta_mTo_Theta_CPR(motor_theta_r, AxiPara.cpr_args, AxiPara.ppr_args);

    t_glb_speed_theta speed_theta_m;
    speed_theta_m(31, 16) = theta_m;
    speed_theta_m(15, 0) = rpm;
    int i_speed_theta_m = speed_theta_m;
    hls::stream<t_glb_foc2pwm> strm_ia;
    hls::stream<t_glb_foc2pwm> strm_ib;
    hls::stream<t_glb_foc2pwm> strm_ic;
    hls::stream<t_glb_speed_theta> strm_speed_theta_m;
    strm_ia << iaf;
    strm_ib << ibf;
    strm_ic << icf;
    strm_speed_theta_m << speed_theta_m;
    hls::stream<t_glb_foc2pwm> Va_cmd;
    hls::stream<t_glb_foc2pwm> Vb_cmd;
    hls::stream<t_glb_foc2pwm> Vc_cmd;
    call_foc_strm(usePeriodic, strm_ia, strm_ib, strm_ic, strm_speed_theta_m, Va_cmd, Vb_cmd, Vc_cmd, AxiPara);
    vaf = Va_cmd.read();
    vbf = Vb_cmd.read();
    vcf = Vc_cmd.read();
    return i_speed_theta_m;
}

void reset_foc() {
    FocAxiParameters<t_glb_q15q16> AxiPara;
    AxiPara.trip_cnt = 1;
    AxiPara.control_mode_args = FOC_Mode::MOD_STOPPED;
    t_glb_foc2pwm va, vb, vc;
    call_foc_scalar(false, 0, 0, 0, 0, 0, va, vb, vc, AxiPara);
}

template <class T_Mdoulbe, class T_Mint>
void ModelBasedSim(bool usePeriodic,
                   Model_motor<T_Mdoulbe, T_Mint>& motor,
                   FocAxiParameters<t_glb_q15q16>& AxiPara,
                   double& time_start,
                   double dt, // timescale
                   int num_step,
                   int inteval_print, // print inteval for full table
                   FILE* fp_ModelFoc,
                   const char* fname // prefix for log files including:
                   ) {
    // Interactive simulation based on motor model, creating simulation files
    // simulation files:
    // <fname>.FocPara.log :initial parameters for simulation.
    // <fname>.FocIn.log : file-based sim input:
    // <fname>.FocOut.log : can be used for file-based sim golden
    assert(fp_ModelFoc);

    char fname_FocPara[128];
    char fname_FocIn[128];
    char fname_FocOut[128];

    sprintf(fname_FocPara, "%s.para.foc", fname);
    sprintf(fname_FocIn, "%s.in.foc", fname);
    sprintf(fname_FocOut, "%s.out.foc", fname);

    FILE* fp_FocPara = fopen(fname_FocPara, "w");
    FILE* fp_FocIn = fopen(fname_FocIn, "w");
    FILE* fp_FocOut = fopen(fname_FocOut, "w");

    assert(fp_FocPara);
    assert(fp_FocIn);
    assert(fp_FocOut);

    AxiPara.printParameters(fp_FocPara);
    fprintf(fp_FocPara, "\n");

    for (int i = 0; i < num_step; i++) {
        // updating motor
        motor.updating(dt);
        t_glb_foc2pwm iaf, ibf, icf, vaf, vbf, vcf;
        iaf = motor.out_va;
        ibf = motor.out_vb;
        icf = motor.out_vc;
        // running FOC
        int i_speed_theta_m =
            call_foc_scalar(usePeriodic, iaf, ibf, icf, motor.w, motor.theta_r, vaf, vbf, vcf, AxiPara);

        motor.va = vaf;
        motor.vb = vbf;
        motor.vc = vcf;
        // Printing files//////////////////////////////////////////////////////////////////
        //
        printFocInput(fp_FocIn, iaf, ibf, icf, i_speed_theta_m);
        //
        printFocOutput(fp_FocOut, vaf, vbf, vcf);
        //
        if (0 == i % inteval_print) {
            fprintf(fp_ModelFoc, "MM2_B_Time is \t%04.6f(s)\t", time_start);
            motor.printParameters(fp_ModelFoc, 0);
            AxiPara.printParameters(fp_ModelFoc);
            fprintf(fp_ModelFoc, "\tMM2_E\n");
        }
        time_start += dt;
    } // for loop

    fclose(fp_FocPara);
    fclose(fp_FocIn);
    fclose(fp_FocOut);

    // clang-format off
    printf("SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************\n");
    printf("SIM_FOC_M:  Timescale  : %6d (us) \t", (int)(dt * 1000000)); printf("|  motor.w     : %5.4f\t(rad/s)\t", motor.w    );   printf("|  Log of parameters : %s\n", fname_FocPara);
    printf("SIM_FOC_M:  Total step : %6d      \t", num_step);            printf("|  motor.theta :  %2.6f\t(rad)\t", motor.theta_r);  printf("|  Log of FOC inputs : %s\n", fname_FocIn);
    printf("SIM_FOC_M:  Total time : %3.6f (s)\t", num_step * dt);       printf("|  motor.Id    : %3.6f\t( A ) \t", motor.Id     );  printf("|  Log of FOC outputs: %s\n", fname_FocOut);
    printf("SIM_FOC_M:  Inteval    : %6d      \t", inteval_print);       printf("|  motor.Iq    : %3.6f\t( A )  \n", motor.Iq     );
    printf("SIM_FOC_M:  FOC MODE   : %s \n", string_FOC_Mode[AxiPara.control_mode_args]);
    printf("SIM_FOC_M:  FOC CPR    : %6d     \t", AxiPara.cpr_args);                 printf("|  FOC PPR: %6d     \n", AxiPara.ppr_args);
    printf("SIM_FOC_M:************ PID Final Status *********\n");
    AxiPara.printPIDs("SIM_FOC_M:  ");
    printf("SIM_FOC_M:************************************************************************************************************************************\n");
    // clang-format on
}

int FileBasedSim(
    bool usePeriodic, const char* fname, FocAxiParameters<t_glb_q15q16>& AxiPara, float Umax, float th_error) {
    // clang-format off
// Interactive simulation based on motor model, creating simulation files
// simulation files: 
    // <fname>.FocPara.log :initial parameters for simulation.  
    // <fname>.FocIn.log : file-based sim input: 
    // <fname>.FocOut.log : can be used for file-based sim golden
    // clang-format on

    char fname_FocPara[128];
    char fname_FocIn[128];
    char fname_FocOut[128];

    sprintf(fname_FocPara, "%s.para.foc", fname);
    sprintf(fname_FocIn, "%s.in.foc", fname);
    sprintf(fname_FocOut, "%s.out.foc", fname);

    hls::stream<t_glb_foc2pwm> strm_ia;
    hls::stream<t_glb_foc2pwm> strm_ib;
    hls::stream<t_glb_foc2pwm> strm_ic;
    hls::stream<t_glb_speed_theta> strm_speed_theta_m;
    hls::stream<t_glb_foc2pwm> Va_cmd;
    hls::stream<t_glb_foc2pwm> Vb_cmd;
    hls::stream<t_glb_foc2pwm> Vc_cmd;

    int cnt_sim_in =
        getInputFromFile(fname_FocPara, AxiPara, fname_FocIn, strm_ia, strm_ib, strm_ic, strm_speed_theta_m);
    // cnt_sim_in++; // to compensate error in getInputFromFile

    AxiPara.trip_cnt = cnt_sim_in;
    // bool usePeriodic = false;
    if (usePeriodic)
        call_foc_strm(usePeriodic, strm_ia, strm_ib, strm_ic, strm_speed_theta_m, Va_cmd, Vb_cmd, Vc_cmd, AxiPara);
    else
        for (int i = 0; i < cnt_sim_in; i++)
            call_foc_strm(usePeriodic, strm_ia, strm_ib, strm_ic, strm_speed_theta_m, Va_cmd, Vb_cmd, Vc_cmd, AxiPara);

    // clang-format off
    printf("SIM_FOC_F:********************************************************************************************************************************\n");
    printf("SIM_FOC_F:****Loading parameters and input from files ************************************************************************************\n");
    printf("SIM_FOC_F:  parameters file is %s, format: \n", fname_FocPara); 
    printf("SIM_FOC_F:  FOC inputs file is %s, format: <float va> <float vb> <float vb> <theta_m, rpm> <int va> <int vb> <int vc>\n", fname_FocIn);    
    printf("SIM_FOC_F:  FOC output file is %s, format: <float va> <float vb> <float vb> <theta_m, rpm> <int va> <int vb> <int vc>\n", fname_FocOut);

    char tmp[128];
    //AxiPara.sprintParameters(tmp, PPR); printf("SIM_FOC_F: %s\n", tmp);
    //AxiPara.sprintParameters(tmp, CPR); printf("SIM_FOC_F: %s\n", tmp);
    //AxiPara.sprintParameters(tmp, SAMPLE_INTERVAL_MINUS1); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, CONTROL_MODE_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    //AxiPara.sprintParameters(tmp, CONTROL_FIXPERIOD_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, FLUX_SP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, FLUX_KP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, FLUX_KI_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, TORQUE_SP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, TORQUE_KP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, TORQUE_KI_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, SPEED_SP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, SPEED_KP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, SPEED_KI_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, VD_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, VQ_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, FW_KP_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, FW_KI_ARGS); printf("SIM_FOC_F: %s\n", tmp);
    AxiPara.sprintParameters(tmp, CNT_TRIP); printf("SIM_FOC_F: %s\n", tmp);

    int cnt_err = compareGoldenFromFile(
        fname_FocOut,
        Va_cmd, Vb_cmd, Vc_cmd,
        cnt_sim_in,
        Umax,
        th_error
    );

    printf("SIM_FOC_F:********************************************************************************************************************************\n");
    // clang-format on
    return cnt_err;
}

#ifndef __SYNTHESIS__
RangeTracer ranger;
#endif

int main(int argc, char** argv) {
    int inteval_print = 3;
    double dt_sim = 0.00001; // 10us, 100k
    bool isPrintf = true;
    bool isPrint = false;
    char file_log[256];
    int cnt = 3000;

    cmd_findParaValue(argc, argv, "-dt", dt_sim);
    cmd_findParaValue(argc, argv, "-pii", inteval_print);

    int idx = cmd_findPara(argc, argv, "-log");
    if (idx > 0 && argc > idx + 1)
        strcpy(file_log, argv[idx + 1]);
    else
        strcpy(file_log, "sim_foc");

    cnt = TESTNUMBER;
    idx = cmd_findPara(argc, argv, "-cnt");
    if (-1 != idx && idx < argc - 1) cnt = atoi(argv[idx + 1]);

    Model_motor<double, int> motor;
    motor.setObjName("motor");
    motor.dt_sim = dt_sim; // 0.00001;//10us
    motor.va = 0;
    motor.vb = 0;
    motor.vc = 0;
    double t_cur = 0;

    FocAxiParameters<t_glb_q15q16> AxiPara;
    AxiPara.Init_24V(); // MOD_SPEED_WITH_TORQUE by default
    AxiPara.sample_interval_minus1_args = 10;

    char fname_ModelFoc[128];
    sprintf(fname_ModelFoc, "%s_ModelFoc.log", file_log);
    FILE* fp_ModelFoc = fopen(fname_ModelFoc, "w");
    fprintf(fp_ModelFoc, "MM2_B_Time is \t%04.6f(s)\t", t_cur);
    motor.printParameters(fp_ModelFoc, -1);
    AxiPara.printTitle(fp_ModelFoc);
    fprintf(fp_ModelFoc, "\tMM2_E\n");

    const char* fname_prefix_torqueWithoutSpeed = "sim_torqueWithoutSpeed";
    const char* fname_prefix_rpm10k = "sim_rpm10k";
    const char* fname_prefix_rpm16k = "sim_rpm16k";
    const char* fname_prefix_rpm16kweak = "sim_rpm16k_weak";
    const char* fname_prefix_manualTorqueFlux = "sim_manualTorqueFlux";
    const char* fname_prefix_manualTorque = "sim_manualTorque";
    const char* fname_prefix_manualFlux = "sim_manualFlux";
    const char* fname_prefix_stop = "sim_stop";
    const char* fname_prefix_manualTorqueFluxFixedSpeed1 = "sim_manualTorqueFluxFixedSpeed1";
    const char* fname_prefix_manualTorqueFluxFixedAngle = "sim_manualTorqueFluxFixedAngle";
    const char* fname_prefix_manualTorqueFluxFixedAngle1 = "sim_manualTorqueFluxFixedAngle1";
    int nStep_sim_rpm10k = cnt;
    int nStep_sim_rpm16k = cnt;
    int nStep_sim_rpm16kweak = cnt;
    int nStep_sim_torqueWithoutSpeed = cnt;
    int nStep_sim_manualTorqueFlux = cnt;
    int nStep_sim_manualTorque = cnt;
    int nStep_sim_manualTorqueFluxFixedSpeed = cnt;
    int nStep_sim_manualFlux = cnt;
    int nStep_sim_stop = cnt;
    int nStep_sim_total = nStep_sim_rpm10k + nStep_sim_rpm16k + nStep_sim_rpm16kweak + nStep_sim_torqueWithoutSpeed +
                          nStep_sim_manualTorqueFlux + nStep_sim_manualTorque + nStep_sim_manualTorqueFluxFixedSpeed +
                          nStep_sim_manualFlux + nStep_sim_stop;
    bool usePeriodic1 = false;

    AxiPara.control_mode_args = FOC_Mode::MOD_TORQUE_WITHOUT_SPEED;
    AxiPara.torque_sp_args = COMM_MOTOR_PARA_UMAX * 0.2f;
    AxiPara.torque_kp_args = 5.0f;
    AxiPara.torque_ki_args = 1.0 / 300.0;
    motor.TL_th = 0.03f;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_torqueWithoutSpeed, inteval_print, fp_ModelFoc,
                  fname_prefix_torqueWithoutSpeed);

    AxiPara.control_mode_args = FOC_Mode::MOD_SPEED_WITH_TORQUE;
    AxiPara.torque_kp_args = 0.04;
    motor.TL_th = 0;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_rpm10k, inteval_print, fp_ModelFoc,
                  fname_prefix_rpm10k);

    AxiPara.speed_sp_args = -16000;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_rpm16k, inteval_print, fp_ModelFoc,
                  fname_prefix_rpm16k);

    AxiPara.control_mode_args = FOC_Mode::MOD_FLUX;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_rpm16kweak, inteval_print, fp_ModelFoc,
                  fname_prefix_rpm16kweak);

    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX;
    AxiPara.vq_args = COMM_MOTOR_PARA_UMAX * 0.6f;
    AxiPara.vd_args = 0;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualTorqueFlux, inteval_print, fp_ModelFoc,
                  fname_prefix_manualTorqueFlux);

    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE;
    AxiPara.vq_args = COMM_MOTOR_PARA_UMAX * 0.5f;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualTorque, inteval_print, fp_ModelFoc,
                  fname_prefix_manualTorque);

    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_FLUX;
    AxiPara.vd_args = COMM_MOTOR_PARA_UMAX * 0.25f * 0.25f;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualFlux, inteval_print, fp_ModelFoc,
                  fname_prefix_manualFlux);

    AxiPara.control_mode_args = FOC_Mode::MOD_STOPPED;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_stop, inteval_print, fp_ModelFoc,
                  fname_prefix_stop);

    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED;
    AxiPara.control_fixperiod_args = 1;
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualTorqueFluxFixedSpeed, inteval_print,
                  fp_ModelFoc, fname_prefix_manualTorqueFluxFixedSpeed1);

    // To test addtional mode MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE
    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE;
    AxiPara.fixed_angle_args = 50; // 50 = w_set * (CPR/PPR) / (2*Pi) = 0.2Pi * 500 / 2*Pi

    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualTorqueFluxFixedSpeed, inteval_print,
                  fp_ModelFoc, fname_prefix_manualTorqueFluxFixedAngle);

    AxiPara.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE;
    AxiPara.fixed_angle_args = 100; // 100 = w_set * (CPR/PPR) / (2*Pi) = 0.4Pi * 500 / 2*Pi
    ModelBasedSim(usePeriodic1, motor, AxiPara, t_cur, dt_sim, nStep_sim_manualTorqueFluxFixedSpeed, inteval_print,
                  fp_ModelFoc, fname_prefix_manualTorqueFluxFixedAngle1);

    bool usePeriodic2 = true;

    FocAxiParameters<t_glb_q15q16> AxiPara_10k;
    FocAxiParameters<t_glb_q15q16> AxiPara_16k;
    FocAxiParameters<t_glb_q15q16> AxiPara_16kweak;
    FocAxiParameters<t_glb_q15q16> AxiPara_torqueWithoutSpeed;
    FocAxiParameters<t_glb_q15q16> AxiPara_manualTorqueFlux;
    FocAxiParameters<t_glb_q15q16> AxiPara_manualTorque;
    FocAxiParameters<t_glb_q15q16> AxiPara_manualFlux;
    FocAxiParameters<t_glb_q15q16> AxiPara_stop;
    FocAxiParameters<t_glb_q15q16> AxiPara_manualTorqueFluxFixedSpeed1;
    reset_foc();
    float maxVoltage = motor.Umax;
    float threshold = motor.Umax * 0.05;
    // clang-format off
    int cnt_err_torqueWithoutSpeed = FileBasedSim(usePeriodic2,
        fname_prefix_torqueWithoutSpeed, 
        AxiPara_torqueWithoutSpeed, 
        maxVoltage, 
        threshold);

    int cnt_err_10k = FileBasedSim(usePeriodic2,
        fname_prefix_rpm10k, 
        AxiPara_10k, 
        maxVoltage, 
        threshold);
    
    int cnt_err_16k = FileBasedSim(usePeriodic2,
        fname_prefix_rpm16k, 
        AxiPara_16k, 
        maxVoltage, 
        threshold);
    
    int cnt_err_16kweak = FileBasedSim(usePeriodic2,
        fname_prefix_rpm16kweak, 
        AxiPara_16kweak, 
        maxVoltage, 
        threshold);

    int cnt_err_manualTorqueFlux = FileBasedSim(usePeriodic2,
        fname_prefix_manualTorqueFlux, 
        AxiPara_manualTorqueFlux, 
        maxVoltage, 
        threshold);
    
    int cnt_err_manualTorque = FileBasedSim(usePeriodic2,
        fname_prefix_manualTorque, 
        AxiPara_manualTorque, 
        maxVoltage, 
        threshold);

    int cnt_err_manualFlux = FileBasedSim(usePeriodic2,
        fname_prefix_manualFlux, 
        AxiPara_manualFlux, 
        maxVoltage, 
        threshold);

    int cnt_err_stop = FileBasedSim(usePeriodic2,
        fname_prefix_stop, 
        AxiPara_stop, 
        maxVoltage, 
        threshold);

    int cnt_err_manualTorqueFluxFixedSpeed1 = FileBasedSim(usePeriodic2,
        fname_prefix_manualTorqueFluxFixedSpeed1, 
        AxiPara_manualTorqueFluxFixedSpeed1, 
        maxVoltage, 
        threshold);

    fclose(fp_ModelFoc);
    nStep_sim_total = 0;
    float t0 = 0.0;
    float t1 = 0.0;
    printf("SIM_FOC********************************************************** TEST SUMMARY ***********************************************************\n");
    printf("SIM_FOC_M:  ------ Summary for Model-based simulation -----------------------------------------------------------\n");
    printf("SIM_FOC_M:  Kernel sampling mode           : %s \n", usePeriodic1? "one calling multi-sample" : "one calling one sample"); 
    printf("SIM_FOC_M:  Simulation resolution          : %0.6f (ms)\n", dt_sim*1000);
    printf("SIM_FOC_M:  Simulation total time          : %3.6f (ms)\n", t_cur*1000);
    printf("SIM_FOC_M:  Inteval for printing wave data : %d\n", inteval_print);
    printf("SIM_FOC_M:  Wave data for all 9 phases test: %s \n", fname_ModelFoc);
    printf("SIM_FOC_M:  Phase-1 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_torqueWithoutSpeed);
    printf("SIM_FOC_M:  Phase-2 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_rpm10k);
    printf("SIM_FOC_M:  Phase-3 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_rpm16k);
    printf("SIM_FOC_M:  Phase-4 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_rpm16kweak);
    printf("SIM_FOC_M:  Phase-5 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualTorqueFlux);
    printf("SIM_FOC_M:  Phase-6 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualTorque);
    printf("SIM_FOC_M:  Phase-7 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualFlux);
    printf("SIM_FOC_M:  Phase-8 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_stop);
    printf("SIM_FOC_M:  Phase-9 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualTorqueFluxFixedSpeed1);
    printf("SIM_FOC_M:  Phase-10 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualTorqueFluxFixedAngle);
    printf("SIM_FOC_M:  Phase-11 generated files        : %s<.para.foc> <.in.foc> <.out.foc> \n", fname_prefix_manualTorqueFluxFixedAngle1);
    printf("SIM_FOC_F:  ------ Summary for File-based simulation based on Model-based outputs -------------------------------\n");
    printf("SIM_FOC_F:  Kernel sampling mode           : %s \n", usePeriodic2? "one calling multi-sample" : "one calling one sample"); 
    t0 = t1; 
    t1 = t1 + nStep_sim_torqueWithoutSpeed * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-1: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_torqueWithoutSpeed.control_mode_args], (int)AxiPara_torqueWithoutSpeed.speed_sp_args.to_float(), threshold, cnt_err_torqueWithoutSpeed);

    t0 = t1; 
    t1 = t1 + nStep_sim_rpm10k * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-2: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_10k.control_mode_args], (int)AxiPara_10k.speed_sp_args.to_float(), threshold, cnt_err_10k);
    t0 = t1; 
    t1 = t1 + nStep_sim_rpm16k * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-3: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_16k.control_mode_args], (int)AxiPara_16k.speed_sp_args.to_float(), threshold, cnt_err_16k);
    t0 = t1; 
    t1 = t1 + nStep_sim_rpm16kweak * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-4: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_16kweak.control_mode_args], (int)AxiPara_16kweak.speed_sp_args.to_float(), threshold, cnt_err_16kweak);
    t0 = t1; 
    t1 = t1 + nStep_sim_manualTorqueFlux * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-5: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_manualTorqueFlux.control_mode_args], (int)AxiPara_manualTorqueFlux.speed_sp_args.to_float(), threshold, cnt_err_manualTorqueFlux);
    t0 = t1; 
    t1 = t1 + nStep_sim_manualTorque * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-6: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_manualTorque.control_mode_args], (int)AxiPara_manualTorque.speed_sp_args.to_float(), threshold, cnt_err_manualTorque);
    t0 = t1; 
    t1 = t1 + nStep_sim_manualFlux * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-7: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_manualFlux.control_mode_args], (int)AxiPara_manualFlux.speed_sp_args.to_float(), threshold, cnt_err_manualFlux);
    t0 = t1; 
    t1 = t1 + nStep_sim_stop * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-8: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV): %d \n", 
    t0, t1, string_FOC_Mode[AxiPara_stop.control_mode_args], (int)AxiPara_stop.speed_sp_args.to_float(), threshold, cnt_err_stop);
    t0 = t1; 
    t1 = t1 + nStep_sim_manualTorqueFluxFixedSpeed * dt_sim * 1000;
    printf("SIM_FOC_F:  Phase-9: %0.3f(ms) ~ %0.3f(ms)\t Mode: %s RPM: %5d\t Sampling II: Depending on II after synthesis\t Over threshold(%2.2fV). \n", 
    t0, t1, string_FOC_Mode[AxiPara_manualTorqueFluxFixedSpeed1.control_mode_args], (int)AxiPara_manualTorqueFluxFixedSpeed1.speed_sp_args.to_float(), threshold);

    printf("SIM_FOC_F:********************************************************************************************************************************\n");

// clang-format on
#ifndef __SYNTHESIS__
    idx = cmd_findPara(argc, argv, "-range");
    if (idx != -1) ranger.print(stdout);
#endif
    printf("\n");
    printf("csim.exe [-dt <time scale in sec> ]\t Simulation resolution\n");
    printf("         [-pii <print interval>]   \t Wave data file%s sampling interval\n", fname_ModelFoc);
    printf("         [-log <char*>]            \t Setting log file's prefix with surfix _ModelFoc.log\n");
    // printf("         [-ii <sampling II>]       |\n");
    printf("         [-range]                  \t Enable range tracing for internal varialbes in FOC\n");
    printf("         [-cnt <Number of input>]  \t Number of sample for a mode\n");
    printf("\n");
    return (cnt_err_torqueWithoutSpeed + cnt_err_10k + cnt_err_16k + cnt_err_16kweak + cnt_err_manualTorqueFlux +
            cnt_err_manualTorque + cnt_err_manualFlux + cnt_err_stop);
}
