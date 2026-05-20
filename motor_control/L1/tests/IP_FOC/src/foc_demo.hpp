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

#ifndef _FOC_DEMO_HPP_
#define _FOC_DEMO_HPP_
//#include "sin_cos_table.h"
#include "foc.hpp"
#include <fstream>
#include <sstream>

using namespace xf::motorcontrol;

//--------------------------------------------------------------------------
// Argument
//--------------------------------------------------------------------------
enum ARGES_IDX_FOC {
    PPR = 0,
    CPR,
    SAMPLE_INTERVAL_MINUS1,
    CONTROL_MODE_ARGS,
    CONTROL_FIXPERIOD_ARGS,
    FLUX_SP_ARGS,
    FLUX_KP_ARGS,
    FLUX_KI_ARGS,
    FLUX_KD_ARGS,
    TORQUE_SP_ARGS,
    TORQUE_KP_ARGS,
    TORQUE_KI_ARGS,
    TORQUE_KD_ARGS,
    SPEED_SP_ARGS,
    SPEED_KP_ARGS,
    SPEED_KI_ARGS,
    SPEED_KD_ARGS,
    ANGLE_SH_ARGS,
    VD_ARGS,
    VQ_ARGS,
    TRIGGER_ARGS,
    CONTROL2_ARGS,
    FW_KP_ARGS,
    FW_KI_ARGS,
    //
    ID_STTS,
    FLUX_ACC_STTS,
    FLUX_ERR_STTS,
    FLUX_OUT_STTS,
    IQ_STTS,
    TORQUE_ACC_STTS,
    TORQUE_ERR_STTS,
    TORQUE_OUT_STTS,
    SPEED_STTS,
    SPEED_ACC_STTS,
    SPEED_ERR_STTS,
    SPEED_OUT_STTS,
    ANGLE_STTS,
    VA_CMD_STTS,
    VB_CMD_STTS,
    VC_CMD_STTS,
    IALPHA_STTS,
    IBETA_STTS,
    IHOMOPOLAR_STTS,
    CNT_TRIP
};

#define FOC_ARGS_SIZE (CNT_TRIP + 1)

void foc_demo(
    // Input for GPIO
    short Ia,             // Phase A current
    short Ib,             // Phase B current
    short Ic,             // Phase B current
    short RPM,            // RPM
    unsigned short Angle, // Encoder count
    // short V_fw,           // Set V for feild weakenning
    // Output for GPIO
    short& Va,
    short& Vb,
    short& Vc,
    // Inout for parameters
    int args[FOC_ARGS_SIZE]);
// clang-format off
static const char* string_FOC_Mode[MOD_TOTAL_NUM] = {
    "MOD_STOPPED                       ", 
    "MOD_SPEED_WITH_TORQUE             ",
    "MOD_TORQUE_WITHOUT_SPEED          ",
    "MOD_FLUX                          ",
    "MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED",
    "MOD_MANUAL_TORQUE_FLUX            ",
    "MOD_MANUAL_TORQUE                 ",
    "MOD_MANUAL_FLUX                   ",
    "MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE"};

static const char* string_FOC_PARA[FOC_ARGS_SIZE] = {
    "PPR       ",
    "CPR       ", 
    "SAMPLE_II ", 
    "MODE      ", 
    "FIXPERIOD ", 
    "FLUX_SP   ", 
    "FLUX_KP   ", 
    "FLUX_KI   ",
    "FLUX_KD   ", 
    "TORQUE_SP ", 
    "TORQUE_KP ", 
    "TORQUE_KI ", 
    "TORQUE_KD ", 
    "SPEED_SP  ", 
    "SPEED_KP  ", 
    "SPEED_KI  ",
    "SPEED_KD  ", 
    "ANGLE_SH  ", 
    "VD        ", 
    "VQ        ", 
    "TRIGGER   ", 
    "CONTROL2  ", 
    "FW_KP     ", 
    "FW_KI     ",
    //
    "ID        ", 
    "FLUX_ACC  ", 
    "FLUX_ERR  ", 
    "FLUX_OUT  ", 
    "IQ        ", 
    "TORQUE_ACC", 
    "TORQUE_ERR", 
    "TORQUE_OUT",
    "SPEED     ", 
    "SPEED_ACC ", 
    "SPEED_ERR ", 
    "SPEED_OUT ", 
    "ANGLE     ", 
    "VA_CMD    ", 
    "VB_CMD    ", 
    "VC_CMD    ",
    "I_ALPHA   ", 
    "I_BETA    ", 
    "I_HMPLR   ", 
    "CNT_TRIP  "};
// clang-format on

template <class T>
struct FocAxiParameters {
    int ppr_args;
    int cpr_args;
    int sample_interval_minus1_args;
    int control_mode_args;
    int control_fixperiod_args;
    T flux_sp_args;
    T flux_kp_args;
    T flux_ki_args;
    T flux_kd_args;
    T torque_sp_args;
    T torque_kp_args;
    T torque_ki_args;
    T torque_kd_args;
    T speed_sp_args;
    T speed_kp_args;
    T speed_ki_args;
    T speed_kd_args;
    T angle_sh_args;
    T vd_args;
    T vq_args;
    T trigger_args;
    int control2_args;
    T fw_kp_args;
    T fw_ki_args;
    //
    T id_stts;
    T flux_acc_stts;
    T flux_err_stts;
    T flux_out_stts;
    T iq_stts;
    T torque_acc_stts;
    T torque_err_stts;
    T torque_out_stts;
    T speed_stts;
    T speed_acc_stts;
    T speed_err_stts;
    T speed_out_stts;
    T angle_stts;
    T Va_cmd_stts;
    T Vb_cmd_stts;
    T Vc_cmd_stts;
    T Ialpha_stts;
    T Ibeta_stts;
    T Ihomopolar_stts;
    T fixed_angle_args;
    long trip_cnt;
    FocAxiParameters() { Init(); }
    void Init() {
        ppr_args = COMM_MACRO_PPR;
        cpr_args = COMM_MACRO_CPR;
        sample_interval_minus1_args = 0;
        control_mode_args = FOC_Mode::MOD_STOPPED;
        control_fixperiod_args = 0;
        fixed_angle_args = 0;
        flux_sp_args = 0;
        flux_kp_args = 0;
        flux_ki_args = 0;
        flux_kd_args = 0;
        torque_sp_args = 0;
        torque_kp_args = 0;
        torque_ki_args = 0;
        torque_kd_args = 0;
        speed_sp_args = 0;
        speed_kp_args = 0;
        speed_ki_args = 0;
        speed_kd_args = 0;
        angle_sh_args = 0;
        vd_args = 0;
        vq_args = 0;
        trigger_args = 0;
        control2_args = 0;
        fw_kp_args = 0;
        fw_ki_args = 0;
        trip_cnt = 1;
    }
    void Init_24V() {
        Init();
        control_mode_args = FOC_Mode::MOD_SPEED_WITH_TORQUE;
        flux_kp_args = 1.0;
        torque_kp_args = 0.04;
        speed_sp_args = 10000;
        speed_kp_args = 2.7;
        speed_ki_args = 1.0 / 300.0;
        vq_args = 24;
        fw_kp_args = 1.0;
        fw_ki_args = 1.0 / 300.0;
    }
    void printTitle(FILE* fp) {
        assert(fp);
        fprintf(fp, "\tppr");
        fprintf(fp, "\tcpr");
        fprintf(fp, "\tII_sample");
        fprintf(fp, "\tcontrol_mode");
        fprintf(fp, "\tfixperiod");
        fprintf(fp, "\tflux_sp");
        fprintf(fp, "\tflux_kp");
        fprintf(fp, "\tflux_ki");
        fprintf(fp, "\tflux_kd");
        fprintf(fp, "\ttorque_sp");
        fprintf(fp, "\ttorque_kp");
        fprintf(fp, "\ttorque_ki");
        fprintf(fp, "\ttorque_kd");
        fprintf(fp, "\trpm_sp");
        fprintf(fp, "\trpm_kp");
        fprintf(fp, "\trpm_ki");
        fprintf(fp, "\trpm_kd");
        fprintf(fp, "\tangle_sh");
        fprintf(fp, "\tvd");
        fprintf(fp, "\tvq");
        fprintf(fp, "\ttrigger");
        fprintf(fp, "\tcontrol2");
        fprintf(fp, "\tfw_kp");
        fprintf(fp, "\tfw_ki");
        //
        fprintf(fp, "\tid");
        fprintf(fp, "\tflux_acc");
        fprintf(fp, "\tflux_err");
        fprintf(fp, "\tflux_out");
        fprintf(fp, "\tiq");
        fprintf(fp, "\ttorque_acc");
        fprintf(fp, "\ttorque_err");
        fprintf(fp, "\ttorque_out");
        fprintf(fp, "\trpm");
        fprintf(fp, "\trpm_acc");
        fprintf(fp, "\trpm_err");
        fprintf(fp, "\trpm_out");
        fprintf(fp, "\tangle");
        fprintf(fp, "\tVa_cmd");
        fprintf(fp, "\tVb_cmd");
        fprintf(fp, "\tVc_cmd");
        fprintf(fp, "\tIalpha");
        fprintf(fp, "\tIbeta");
        fprintf(fp, "\tIhomopolar");
        fprintf(fp, "\trip_cnt");
        fprintf(fp, "\tfixed_angle_args");
    }
    void printParameters(FILE* fp) {
        assert(fp);
        fprintf(fp, "\t%d", ppr_args);
        fprintf(fp, "\t%d", cpr_args);
        fprintf(fp, "\t%d", sample_interval_minus1_args);
        fprintf(fp, "\t%d", control_mode_args);
        fprintf(fp, "\t%d", control_fixperiod_args);
        fprintf(fp, "\t%5.6f", flux_sp_args.to_float());
        fprintf(fp, "\t%5.6f", flux_kp_args.to_float());
        fprintf(fp, "\t%5.6f", flux_ki_args.to_float());
        fprintf(fp, "\t%5.6f", flux_kd_args.to_float());
        fprintf(fp, "\t%5.6f", torque_sp_args.to_float());
        fprintf(fp, "\t%5.6f", torque_kp_args.to_float());
        fprintf(fp, "\t%5.6f", torque_ki_args.to_float());
        fprintf(fp, "\t%5.6f", torque_kd_args.to_float());
        fprintf(fp, "\t%5.6f", speed_sp_args.to_float());
        fprintf(fp, "\t%5.6f", speed_kp_args.to_float());
        fprintf(fp, "\t%5.6f", speed_ki_args.to_float());
        fprintf(fp, "\t%5.6f", speed_kd_args.to_float());
        fprintf(fp, "\t%5.6f", angle_sh_args.to_float());
        fprintf(fp, "\t%5.6f", vd_args.to_float());
        fprintf(fp, "\t%5.6f", vq_args.to_float());
        fprintf(fp, "\t%5.6f", trigger_args.to_float());
        fprintf(fp, "\t%d", control2_args);
        fprintf(fp, "\t%5.6f", fw_kp_args.to_float());
        fprintf(fp, "\t%5.6f", fw_ki_args.to_float());
        //
        fprintf(fp, "\t%5.6f", id_stts.to_float());
        fprintf(fp, "\t%5.6f", flux_acc_stts.to_float());
        fprintf(fp, "\t%5.6f", flux_err_stts.to_float());
        fprintf(fp, "\t%5.6f", flux_out_stts.to_float());
        fprintf(fp, "\t%5.6f", iq_stts.to_float());
        fprintf(fp, "\t%5.6f", torque_acc_stts.to_float());
        fprintf(fp, "\t%5.6f", torque_err_stts.to_float());
        fprintf(fp, "\t%5.6f", torque_out_stts.to_float());
        fprintf(fp, "\t%5.6f", speed_stts.to_float());
        fprintf(fp, "\t%5.6f", speed_acc_stts.to_float());
        fprintf(fp, "\t%5.6f", speed_err_stts.to_float());
        fprintf(fp, "\t%5.6f", speed_out_stts.to_float());
        fprintf(fp, "\t%5.6f", angle_stts.to_float());
        fprintf(fp, "\t%5.6f", Va_cmd_stts.to_float());
        fprintf(fp, "\t%5.6f", Vb_cmd_stts.to_float());
        fprintf(fp, "\t%5.6f", Vc_cmd_stts.to_float());
        fprintf(fp, "\t%5.6f", Ialpha_stts.to_float());
        fprintf(fp, "\t%5.6f", Ibeta_stts.to_float());
        fprintf(fp, "\t%5.6f", Ihomopolar_stts.to_float());
        fprintf(fp, "\t%d", trip_cnt);
        fprintf(fp, "\t%d", fixed_angle_args);
    }
    void sprintParameters(char* strm, int idx) {
        assert(strm);
        assert(idx < FOC_ARGS_SIZE);
        assert(strm);
        assert(idx < FOC_ARGS_SIZE);
        // if(idx==CONTROL_MODE_ARGS)sprintf(strm, " %s: %d", "PPR", ppr_args );
        // if(idx==CONTROL_MODE_ARGS)sprintf(strm, " %s: %d", "CPR", cpr_args );
        // if(idx==SAMPLE_INTERVAL_MINUS1)sprintf(strm, " %s: %d", "SAMPLE_INTERVAL_MINUS1", sample_interval_minus1_args
        // );
        if (idx == CONTROL_MODE_ARGS) sprintf(strm, " %s: %d", string_FOC_PARA[CONTROL_MODE_ARGS], control_mode_args);
        if (idx == CONTROL_FIXPERIOD_ARGS)
            sprintf(strm, " %s: %d", string_FOC_PARA[CONTROL_FIXPERIOD_ARGS], control_fixperiod_args);
        if (idx == FLUX_SP_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_SP_ARGS], flux_sp_args.to_float());
        if (idx == FLUX_KP_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_KP_ARGS], flux_kp_args.to_float());
        if (idx == FLUX_KI_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_KI_ARGS], flux_ki_args.to_float());
        if (idx == FLUX_KD_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_KD_ARGS], flux_kd_args.to_float());
        if (idx == TORQUE_SP_ARGS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_SP_ARGS], torque_sp_args.to_float());
        if (idx == TORQUE_KP_ARGS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_KP_ARGS], torque_kp_args.to_float());
        if (idx == TORQUE_KI_ARGS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_KI_ARGS], torque_ki_args.to_float());
        if (idx == TORQUE_KD_ARGS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_KD_ARGS], torque_kd_args.to_float());
        if (idx == SPEED_SP_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_SP_ARGS], speed_sp_args.to_float());
        if (idx == SPEED_KP_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_KP_ARGS], speed_kp_args.to_float());
        if (idx == SPEED_KI_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_KI_ARGS], speed_ki_args.to_float());
        if (idx == SPEED_KD_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_KD_ARGS], speed_kd_args.to_float());
        if (idx == ANGLE_SH_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[ANGLE_SH_ARGS], angle_sh_args.to_float());
        if (idx == VD_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[VD_ARGS], vd_args.to_float());
        if (idx == VQ_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[VQ_ARGS], vq_args.to_float());
        // if(idx==TRIGGER_ARGS)sprintf(strm, " %s: %5.6f", "TRIGGER_ARGS", trigger_args.to_float());
        // if(idx==CONTROL2_ARGS)sprintf(strm, " %s: %d", "CONTROL2_ARGS", control2_args);
        if (idx == FW_KP_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FW_KP_ARGS], fw_kp_args.to_float());
        if (idx == FW_KI_ARGS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FW_KI_ARGS], fw_ki_args.to_float());
        if (idx == ID_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[ID_STTS], id_stts.to_float());
        if (idx == FLUX_ACC_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_ACC_STTS], flux_acc_stts.to_float());
        if (idx == FLUX_ERR_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_ERR_STTS], flux_err_stts.to_float());
        if (idx == FLUX_OUT_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[FLUX_OUT_STTS], flux_out_stts.to_float());
        if (idx == IQ_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[IQ_STTS], iq_stts.to_float());
        if (idx == TORQUE_ACC_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_ACC_STTS], torque_acc_stts.to_float());
        if (idx == TORQUE_ERR_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_ERR_STTS], torque_err_stts.to_float());
        if (idx == TORQUE_OUT_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[TORQUE_OUT_STTS], torque_out_stts.to_float());
        if (idx == SPEED_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_STTS], speed_stts.to_float());
        if (idx == SPEED_ACC_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_ACC_STTS], speed_acc_stts.to_float());
        if (idx == SPEED_ERR_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_ERR_STTS], speed_err_stts.to_float());
        if (idx == SPEED_OUT_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[SPEED_OUT_STTS], speed_out_stts.to_float());
        if (idx == ANGLE_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[ANGLE_STTS], angle_stts.to_float());
        if (idx == VA_CMD_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[VA_CMD_STTS], Va_cmd_stts.to_float());
        if (idx == VB_CMD_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[VB_CMD_STTS], Vb_cmd_stts.to_float());
        if (idx == VC_CMD_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[VC_CMD_STTS], Vc_cmd_stts.to_float());
        if (idx == IALPHA_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[IALPHA_STTS], Ialpha_stts.to_float());
        if (idx == IBETA_STTS) sprintf(strm, " %s: %5.6f", string_FOC_PARA[IBETA_STTS], Ibeta_stts.to_float());
        if (idx == IHOMOPOLAR_STTS)
            sprintf(strm, " %s: %5.6f", string_FOC_PARA[IHOMOPOLAR_STTS], Ihomopolar_stts.to_float());
        if (idx == CNT_TRIP) sprintf(strm, " %s: %d", string_FOC_PARA[CNT_TRIP], trip_cnt);
    }
    void printParameters() {
        printParameters(stdout);
        printf("\n");
    }
    void printTitle() {
        printTitle(stdout);
        printf("\n");
    }

    void printParameters(FILE* fp, int nStep) {
        assert(fp);
        fprintf(fp, "\t%d", nStep);
        printParameters(fp);
        fprintf(fp, "\n");
    }

    void printParameters(const char* fname) {
        assert(fname);
        std::fstream fout(fname, std::ios::out);
        fout << ppr_args << ' ';
        fout << cpr_args << ' ';
        fout << sample_interval_minus1_args << ' ';
        fout << control_mode_args << ' ';
        fout << control_fixperiod_args << ' ';
        fout << flux_sp_args << ' ';
        fout << flux_kp_args << ' ';
        fout << flux_ki_args << ' ';
        fout << flux_kd_args << ' ';
        fout << torque_sp_args << ' ';
        fout << torque_kp_args << ' ';
        fout << torque_ki_args << ' ';
        fout << torque_kd_args << ' ';
        fout << speed_sp_args << ' ';
        fout << speed_kp_args << ' ';
        fout << speed_ki_args << ' ';
        fout << speed_kd_args << ' ';
        fout << angle_sh_args << ' ';
        fout << vd_args << ' ';
        fout << vq_args << ' ';
        fout << trigger_args << ' ';
        fout << control2_args << ' ';
        fout << fw_kp_args << ' ';
        fout << fw_ki_args << ' ';
        fout << id_stts << ' ';
        fout << flux_acc_stts << ' ';
        fout << flux_err_stts << ' ';
        fout << flux_out_stts << ' ';
        fout << iq_stts << ' ';
        fout << torque_acc_stts << ' ';
        fout << torque_err_stts << ' ';
        fout << torque_out_stts << ' ';
        fout << speed_stts << ' ';
        fout << speed_acc_stts << ' ';
        fout << speed_err_stts << ' ';
        fout << speed_out_stts << ' ';
        fout << angle_stts << ' ';
        fout << Va_cmd_stts << ' ';
        fout << Vb_cmd_stts << ' ';
        fout << Vc_cmd_stts << ' ';
        fout << trip_cnt << ' ';
        fout << fixed_angle_args << ' ' << std::endl;
        fout.close();
    }

    void fscanParameters(const char* fname) {
        std::fstream fin(fname, std::ios::in);
        assert(fin);
        std::string line;
        std::getline(fin, line);
        // printf("%s\n", line.c_str());
        std::stringstream istr(line);
        istr >> ppr_args;
        istr >> cpr_args;
        istr >> sample_interval_minus1_args;
        istr >> control_mode_args;
        istr >> control_fixperiod_args;
        istr >> flux_sp_args;
        istr >> flux_kp_args;
        istr >> flux_ki_args;
        istr >> flux_kd_args;
        istr >> torque_sp_args;
        istr >> torque_kp_args;
        istr >> torque_ki_args;
        istr >> torque_kd_args;
        istr >> speed_sp_args;
        istr >> speed_kp_args;
        istr >> speed_ki_args;
        istr >> speed_kd_args;
        istr >> angle_sh_args;
        istr >> vd_args;
        istr >> vq_args;
        istr >> trigger_args;
        istr >> control2_args;
        istr >> fw_kp_args;
        istr >> fw_ki_args;
        istr >> id_stts;
        istr >> flux_acc_stts;
        istr >> flux_err_stts;
        istr >> flux_out_stts;
        istr >> iq_stts;
        istr >> torque_acc_stts;
        istr >> torque_err_stts;
        istr >> torque_out_stts;
        istr >> speed_stts;
        istr >> speed_acc_stts;
        istr >> speed_err_stts;
        istr >> speed_out_stts;
        istr >> angle_stts;
        istr >> Va_cmd_stts;
        istr >> Vb_cmd_stts;
        istr >> Vc_cmd_stts;
        istr >> trip_cnt;
        istr >> fixed_angle_args;
        fin.close();
    }
    void printPIDs(const char* head) {
        if (head != NULL) printf("%s", head);
        printf("SPEED SP   : %2.1f \t", speed_sp_args.to_float());
        printf("|  FLUX SP: %2.4f   \t\t\t", flux_sp_args.to_float());
        printf("|  TORQUE SP: %2.4f    \t", torque_sp_args.to_float());
        printf("|  FW SP: --\n", 0); // speed_sp_args);
        if (head != NULL) printf("%s", head);
        printf("SPEED KP   : %2.4f    \t", speed_kp_args.to_float());
        printf("|  FLUX KP: %2.4f    \t\t\t", flux_kp_args.to_float());
        printf("|  TORQUE KP: %2.4f    \t", torque_kp_args.to_float());
        printf("|  FW KP: --\n", 0); //  speed_kp_args);
        if (head != NULL) printf("%s", head);
        printf("SPEED KI   : %2.4f    \t", speed_ki_args.to_float());
        printf("|  FLUX KI: %2.4f    \t\t\t", flux_ki_args.to_float());
        printf("|  TORQUE KI: %2.4f    \t", torque_ki_args.to_float());
        printf("|  FW KI: --\n", 0); // speed_ki_args);
        if (head != NULL) printf("%s", head);
        printf("SPEED ERR  : %6.3f\t", speed_err_stts.to_float());
        printf("|  FLUX ERR: %7.3f\t\t\t", flux_err_stts.to_float());
        printf("|  TORQUE ERR: %7.4f \t", torque_err_stts.to_float());
        printf("|  FW ERR: --\n", 0); // speed_err_args);
        if (head != NULL) printf("%s", head);
        printf("SPEED ACC  : %6.3f\t", speed_acc_stts.to_float());
        printf("|  FLUX ACC: %7.3f\t\t\t", flux_acc_stts.to_float());
        printf("|  TORQUE ACC: %7.4f \t", torque_acc_stts.to_float());
        printf("|  FW ACC: --\n", 0); // speed_acc_args);
    }
};

template <class T_afx, class T>
void printFocInput(FILE* fp, T_afx iaf, T_afx ibf, T_afx icf, T i_speed_theta_m) {
    fprintf(fp, "\t %5.6f", iaf.to_float());
    fprintf(fp, "\t %5.6f", ibf.to_float());
    fprintf(fp, "\t %5.6f", icf.to_float());
    fprintf(fp, "\t%d", i_speed_theta_m);
    int tmp;
    tmp = iaf.range(iaf.length() - 1, 0);
    fprintf(fp, "\t%d", tmp);
    tmp = ibf.range(ibf.length() - 1, 0);
    fprintf(fp, "\t%d", tmp);
    tmp = icf.range(icf.length() - 1, 0);
    fprintf(fp, "\t%d\n", tmp);
}

template <class T_afx>
void printFocOutput(FILE* fp, T_afx vaf, T_afx vbf, T_afx vcf) {
    fprintf(fp, "\t%5.6f", vaf.to_float());
    fprintf(fp, "\t%5.6f", vbf.to_float());
    fprintf(fp, "\t%5.6f", vcf.to_float());
    int tmp;
    tmp = vaf.range(vaf.length() - 1, 0);
    fprintf(fp, "\t%d", tmp);
    tmp = vbf.range(vbf.length() - 1, 0);
    fprintf(fp, "\t%d", tmp);
    tmp = vcf.range(vcf.length() - 1, 0);
    fprintf(fp, "\t%d\n", tmp);
}

template <class T0_afx, class T1_afx, class T2>
int getInputFromFile(const char* fname_para,
                     FocAxiParameters<T0_afx>& AxiPara,
                     const char* fname_in,
                     hls::stream<T1_afx>& strm_a,
                     hls::stream<T1_afx>& strm_b,
                     hls::stream<T1_afx>& strm_c,
                     hls::stream<T2>& strm_speed_theta_m) { // return number of loading
    int ret = 0;
    assert(fname_para);
    AxiPara.fscanParameters(fname_para);
    std::fstream fin(fname_in, std::ios::in);
    if (!fin) {
        std::cout << "Error : getInputFromFile() " << fname_in << " file doesn't exist !" << std::endl;
        exit(1);
    }
    std::string line;
    std::getline(fin, line);

    // while(std::getline(fin, line))
    do {
        std::stringstream istr(line);
        float a, b, c;
        int a1, b1, c1;
        T1_afx a2, b2, c2;
        const int w_t1 = a2.length();
        int d;
        istr >> a;
        istr >> b;
        istr >> c;
        istr >> d;
        istr >> a1;
        istr >> b1;
        istr >> c1;
        a2(w_t1 - 1, 0) = a1;
        b2(w_t1 - 1, 0) = b1;
        c2(w_t1 - 1, 0) = c1;
        strm_a.write(a2);
        strm_b.write(b2);
        strm_c.write(c2);
        strm_speed_theta_m.write(d);
        ret++;
        float a3, b3, c3;
        a3 = a2;
        b3 = b2;
        c3 = c2;
    } while (std::getline(fin, line));
    fin.close();
    return ret;
};

static int checkError(float data_in,
                      float data_ref,
                      float threshold,
                      const char* strm_chnl,
                      int step,
                      float& err_all,
                      float& err_all_abs,
                      float& err_max,
                      float& err_min) {
    float err = (float)(data_in - data_ref);
    float err_abs = abs(err);
    err_all += err;
    err_all_abs += err_abs;
    if (err > err_max) err_max = err;
    if (err < err_min) err_min = err;

    if (err_abs < threshold) return 0;
    printf(
        " Warning over limitation(%3.3f) for channel %s at step=%d\t: data =  %5.3f\t, reference = %5.6f err = %5.6f "
        "err_all = %5.6f mean(err_abs_all) = %5.6f err_max = %5.6f err_min = %5.6f\n",
        threshold, strm_chnl, step, data_in, data_ref, err, err_all, err_all_abs / (step + 1), err_max, err_min);
    return 1;
}

template <class T_afx>
int compareGoldenFromFile(const char* filename,
                          hls::stream<T_afx>& Va_strm,
                          hls::stream<T_afx>& Vb_strm,
                          hls::stream<T_afx>& Vc_strm,
                          int nStep_sim,
                          float peak_v,
                          float threshold) {
    std::fstream fin(filename, std::ios::in);
    if (!fin) {
        std::cout << "Error : compareGoldenFromFile() " << filename << "file doesn't exist !" << std::endl;
        exit(1);
    }
    std::string line;
    T_afx va, vb, vc;
    float err_all_abs = 0;
    float err_all = 0;
    float err_min = 0;
    float err_max = 0;
    int cnt_err = 0;

    const int w_t1 = va.length();
    for (int i = 0; i < nStep_sim; i++) {
        std::getline(fin, line);
        std::stringstream istr(line);
        T_afx ref_a, ref_b, ref_c;
        istr >> ref_a;
        istr >> ref_b;
        istr >> ref_c;
        int a1, b1, c1;
        T_afx a2, b2, c2;
        istr >> a1;
        istr >> b1;
        istr >> c1;
        a2(w_t1 - 1, 0) = a1;
        b2(w_t1 - 1, 0) = b1;
        c2(w_t1 - 1, 0) = c1;
        Va_strm.read(va);
        Vb_strm.read(vb);
        Vc_strm.read(vc);

        cnt_err +=
            checkError(va.to_float(), ref_a.to_float(), threshold, "cmd_c", i, err_all, err_all_abs, err_max, err_min);
        cnt_err +=
            checkError(vb.to_float(), ref_b.to_float(), threshold, "cmd_b", i, err_all, err_all_abs, err_max, err_min);
        cnt_err +=
            checkError(vc.to_float(), ref_c.to_float(), threshold, "cmd_c", i, err_all, err_all_abs, err_max, err_min);
    }
    printf("SIM_FOC_F: ***********   Comparison with golden file: ********\n");
    printf("SIM_FOC_F:  Total step  : %6d  \n", nStep_sim);
    printf("SIM_FOC_F:  Golden File : %s\n", filename);
    printf("SIM_FOC_F:  Max Voltage : %3.3f\t%2.2f%%\n", peak_v, peak_v / peak_v * 100.0);
    printf("SIM_FOC_F:  threshold   : %3.3f\t%2.2f%%\n", threshold, threshold / peak_v * 100.0);
    printf("SIM_FOC_F:  Mean error  : %3.3f\t%2.2f%%\n", err_all_abs / (float)(nStep_sim) / 3.0,
           err_all_abs / (float)(nStep_sim) / 3.0 / peak_v * 100.0);
    printf("SIM_FOC_F:  Max error   : %3.3f\t%2.2f%%\n", err_max, err_max / peak_v * 100.0);
    printf("SIM_FOC_F:  Min error   : %3.3f\t%2.2f%%\n", err_min, err_min / peak_v * 100.0);
    printf("SIM_FOC_F:  Total error : %6d  \n", cnt_err);
    // printf("SIM_FOC_F:
    // ********************************************************************************************************************************\n");

    fin.close();
    return cnt_err;
}

template <class T>
T OmigaToRpm(T w) {
    return 60.0 * (w / (2.0 * 3.1415926));
}

template <class T>
short Theta_eTo_m(T theta_e, int cpr, int ppr) {
    return (short)((cpr / ppr) * (theta_e / (2.0 * 3.1415926)));
}

template <class T>
short Theta_mTo_Theta_CPR(T theta_m, int cpr, int ppr) {
    return (short)((cpr) * (theta_m / (2.0 * 3.1415926)));
}

#endif // _FOC_DEMO_HPP_
