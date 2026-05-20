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

#include <iostream>
#include "ip_svpwm.hpp"

int cmd_findPara(int argc, char** argv, const char* para) {
    assert(para);
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], para)) return i;
    }
    return -1;
}

template <class T_apfx>
struct AxiParameters_PWM {
    int stt_pwm_cycle;
    int args_pwm_freq;
    int args_dead_cycles;
    int args_phase_shift;
    T_apfx args_dc_link_ref;
    int stt_cnt_iter;
    int stt_cnt_read_foc;
    int stt_cnt_read_dc;
    int args_dc_src_mode;
    int args_sample_ii;
    long args_cnt_trip;
    t_svpwm_cmd pwm_stt_Va_cmd;
    t_svpwm_cmd pwm_stt_Vb_cmd;
    t_svpwm_cmd pwm_stt_Vc_cmd;
    t_glb_q15q16 pwm_stt_duty_ratio_a;
    t_glb_q15q16 pwm_stt_duty_ratio_b;
    t_glb_q15q16 pwm_stt_duty_ratio_c;
};

// test wrapper
void SVPWM_wrapper(hls::stream<t_svpwm_cmd>& strm_Va_cmd,
                   hls::stream<t_svpwm_cmd>& strm_Vb_cmd,
                   hls::stream<t_svpwm_cmd>& strm_Vc_cmd,
                   hls::stream<t_svpwm_cmd>& strm_dc_link,
                   hls::stream<ap_uint<1> >& strm_pwm_h_a,
                   hls::stream<ap_uint<1> >& strm_pwm_h_b,
                   hls::stream<ap_uint<1> >& strm_pwm_h_c,
                   hls::stream<ap_uint<1> >& strm_pwm_l_a,
                   hls::stream<ap_uint<1> >& strm_pwm_l_b,
                   hls::stream<ap_uint<1> >& strm_pwm_l_c,
                   hls::stream<ap_uint<1> >& strm_pwm_sync_a,
                   hls::stream<ap_uint<1> >& strm_pwm_sync_b,
                   hls::stream<ap_uint<1> >& strm_pwm_sync_c,
                   volatile int& pwm_stt_pwm_cycle,
                   volatile int& pwm_args_pwm_freq,
                   volatile int& pwm_args_dead_cycles,
                   volatile int& pwm_args_phase_shift,
                   volatile int& pwm_args_dc_link_ref,
                   volatile int& pwm_stt_cnt_iter,
                   volatile int& pwm_stt_cnt_read_foc,
                   volatile int& pwm_stt_cnt_read_dc,
                   volatile int& pwm_args_dc_src_mode,
                   volatile int& pwm_args_sample_ii,
                   volatile int& pwm_stt_Va_cmd,
                   volatile int& pwm_stt_Vb_cmd,
                   volatile int& pwm_stt_Vc_cmd,
                   volatile int& pwm_stt_duty_ratio_a,
                   volatile int& pwm_stt_duty_ratio_b,
                   volatile int& pwm_stt_duty_ratio_c) {
    // hls::stream<pwmPassedArgs > strm_args;// fixme : refine no useful config will save resouce
    // #pragma HLS STREAM depth = 4 variable = strm_args

    hls::stream<t_svpwm_ratio> strm_duty_cycle_a;
    hls::stream<t_svpwm_ratio> strm_duty_cycle_b;
    hls::stream<t_svpwm_ratio> strm_duty_cycle_c;
#pragma HLS STREAM depth = 4 variable = strm_duty_cycle_a
#pragma HLS STREAM depth = 4 variable = strm_duty_cycle_b
#pragma HLS STREAM depth = 4 variable = strm_duty_cycle_c

#pragma HLS DATAFLOW
    hls_svpwm_duty(strm_Va_cmd, strm_Vb_cmd, strm_Vc_cmd, strm_dc_link, strm_duty_cycle_a, strm_duty_cycle_b,
                   strm_duty_cycle_c, /*strm_args,*/
                   pwm_args_dc_link_ref, pwm_stt_cnt_iter, pwm_args_dc_src_mode, pwm_args_sample_ii, pwm_stt_Va_cmd,
                   pwm_stt_Vb_cmd, pwm_stt_Vc_cmd);
    pwm_stt_cnt_read_foc = pwm_stt_cnt_read_dc = pwm_stt_cnt_iter;

    hls_pwm_gen(strm_duty_cycle_a, strm_duty_cycle_b, strm_duty_cycle_c, /*strm_args,*/
                strm_pwm_h_a, strm_pwm_h_b, strm_pwm_h_c, strm_pwm_l_a, strm_pwm_l_b, strm_pwm_l_c, strm_pwm_sync_a,
                strm_pwm_sync_b, strm_pwm_sync_c, pwm_args_pwm_freq, pwm_args_dead_cycles, pwm_args_phase_shift,
                pwm_stt_pwm_cycle, pwm_args_sample_ii, pwm_stt_duty_ratio_a, pwm_stt_duty_ratio_b,
                pwm_stt_duty_ratio_c);
}

/** \brief Values of \f$V_a\f$ to test with. */
// const t_svpwm_cmd Va[16] = {0,   max_pwm_in, min_pwm_in, 0,    -600, -540, 2000, 2200,
//                                             100, 555,     -255,    3333, -765, 333,  200,  -543};

/** \brief Values of \f$V_b\f$ to test with. */
// const t_svpwm_cmd Vb[16] = {min_pwm_in, max_pwm_in, min_pwm_in, 0,    3000, 3300, -500, -450,
//                                             7000,    1000,    -123,    -800, 9000, 789,  -444};

/** \brief Values of \f$V_c\f$ to test with. */
// const t_svpwm_cmd Vc[16] = {max_pwm_in, max_pwm_in, min_pwm_in, 0,   -300, -270, -100, -90,
//                                             1000,    -100,    -500,    800, 1000, 189,  -744};
// sector  x	1	6	5	4	3	2	1 x x x
// const t_svpwm_cmd Va[10] = {0, 633, 1195, 708, -677, -1197, -622, 528, max_pwm_in, min_pwm_in};
// const t_svpwm_cmd Vb[10] = {0, -1196, -643, 480, 1194, 573, -574, -1195,max_pwm_in, min_pwm_in};
// const t_svpwm_cmd Vc[10] = {0, 563, -552, -1189, -516, 623, 1196, 667, max_pwm_in, min_pwm_in};

// sector  x	1	6	5	4	3	2	1 x x x
// t_svpwm_cmd Va0[TESTNUMBER] = {0,	(int)7.406250,	(int)13.968750,	(int)8.250000,	(int)-7.968750,
// (int)-14.062500,	(int)-7.312500,	(int)6.187500,	(int)MAX_VAL_PWM,	(int)-MAX_VAL_PWM};
// t_svpwm_cmd Vb0[TESTNUMBER] = {0,	(int)-14.062500,	(int)-7.593750,	(int)5.625000,	(int)13.968750,
// (int)6.656250,	(int)-6.750000,	(int)-14.062500,	(int)MAX_VAL_PWM,	(int)-MAX_VAL_PWM};
// t_svpwm_cmd Vc0[TESTNUMBER] = {0,	(int)6.5625,	(int)-6.46875,	(int)-13.96875,	(int)-6.09375,
// (int)7.21875,	(int)13.96875,	(int)7.78125,	(int)MAX_VAL_PWM,	(int)-MAX_VAL_PWM};
float Va0_f[TESTNUMBER] = {0,          7.406250,  13.968750, 8.250000,    -7.968750,
                           -14.062500, -7.312500, 6.187500,  MAX_VAL_PWM, -MAX_VAL_PWM};
float Vb0_f[TESTNUMBER] = {0,        -14.062500, -7.593750,  5.625000,    13.968750,
                           6.656250, -6.750000,  -14.062500, MAX_VAL_PWM, -MAX_VAL_PWM};
float Vc0_f[TESTNUMBER] = {0,       6.5625,   -6.46875, -13.96875,   -6.09375,
                           7.21875, 13.96875, 7.78125,  MAX_VAL_PWM, -MAX_VAL_PWM};

t_svpwm_cmd Va0[TESTNUMBER]; // = {0,	7.406250,	13.968750,	8.250000,	-7.968750,
                             // -14.062500,	-7.312500,	6.187500,	MAX_VAL_PWM,	-MAX_VAL_PWM};
t_svpwm_cmd Vb0[TESTNUMBER]; // = {0,	-14.062500,	-7.593750,	5.625000,	13.968750,
                             // 6.656250,	-6.750000,	-14.062500,	MAX_VAL_PWM,	-MAX_VAL_PWM};
t_svpwm_cmd Vc0[TESTNUMBER]; // = {0,	6.5625,	-6.46875,	-13.96875,	-6.09375,	7.21875,
                             // 13.96875,	7.78125,	MAX_VAL_PWM,	-MAX_VAL_PWM};

t_svpwm_cmd Va1[TESTNUMBER] = {18, 18, 18, 18, 18, 18, 18, 18, 18, 18};
t_svpwm_cmd Vb1[TESTNUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
t_svpwm_cmd Vc1[TESTNUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

t_svpwm_cmd Va2[TESTNUMBER] = {24, 22, 18, 12, 6, 3, 0, -12, -18, -24};
t_svpwm_cmd Vb2[TESTNUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
t_svpwm_cmd Vc2[TESTNUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int main(int argc, char** argv) {
#ifndef SIM_FINITE
    printf("\nSIM_SVPWM: Warning! The macor \"SIM_FINITE\" has not been defined!\n");
    printf("SIM_SVPWM: Warning! CSIM will not be terminated!\n\n");
#endif

    /******* User Definition *********/
    int ret_err_val = 0;
    double allowed_res_ratio = 0.05;

    AxiParameters_PWM<t_glb_q15q16> axi_para;
    int clk_fq = COMM_CLOCK_FREQ;
    axi_para.args_pwm_freq = 20000;
    axi_para.args_dead_cycles = 10;
    axi_para.args_phase_shift =
        xf::motorcontrol::MODE_PWM_PHASE_SHIFT::SHIFT_ZERO; // 0 for phase shifting, 1 for no phase shift
    axi_para.args_dc_src_mode = xf::motorcontrol::MODE_PWM_DC_SRC::DC_SRC_ADC;
    // 0: mode 0, PWM voltage reference based on ADC measured DC link;
    // 1: mode 1, PWM voltage reference uses static register value

    axi_para.args_dc_link_ref = (t_glb_q15q16)MAX_VAL_PWM;

    axi_para.args_cnt_trip = TESTNUMBER;

    axi_para.pwm_stt_Va_cmd = 0;
    axi_para.pwm_stt_Vb_cmd = 0;
    axi_para.pwm_stt_Vc_cmd = 0;
    axi_para.pwm_stt_duty_ratio_a = 0;
    axi_para.pwm_stt_duty_ratio_b = 0;
    axi_para.pwm_stt_duty_ratio_c = 0;

    if (-1 != cmd_findPara(argc, argv, "-shift_120"))
        axi_para.args_phase_shift = xf::motorcontrol::MODE_PWM_PHASE_SHIFT::SHIFT_120;

    if (-1 != cmd_findPara(argc, argv, "-dc_ref"))
        axi_para.args_dc_src_mode = xf::motorcontrol::MODE_PWM_DC_SRC::DC_SRC_REF;

    int idx;
    idx = cmd_findPara(argc, argv, "-pwm_fq");
    if (-1 != idx && idx < argc - 1) axi_para.args_pwm_freq = atoi(argv[idx + 1]);

    idx = cmd_findPara(argc, argv, "-dead");
    if (-1 != idx && idx < argc - 1) axi_para.args_dead_cycles = atoi(argv[idx + 1]);

    idx = cmd_findPara(argc, argv, "-cnt");
    if (-1 != idx && idx < argc - 1) axi_para.args_cnt_trip = atoi(argv[idx + 1]);

    const t_svpwm_cmd dc_link_adc = (float)MAX_VAL_PWM * (float)0.9; //(t_glb_q15q16)90 / (t_glb_q15q16)100;
    double factor = (double)(clk_fq / axi_para.args_pwm_freq);
    axi_para.args_sample_ii = (int)factor;

    idx = cmd_findPara(argc, argv, "-ii");
    if (-1 != idx && idx < argc - 1) axi_para.args_sample_ii = atoi(argv[idx + 1]);

    /*********************************/

    hls::stream<t_svpwm_cmd> inputVa;
    hls::stream<t_svpwm_cmd> inputVb;
    hls::stream<t_svpwm_cmd> inputVc;
    hls::stream<t_svpwm_cmd> dc_link;

    hls::stream<ap_uint<1> > pwm_h[3], pwm_h_a, pwm_h_b, pwm_h_c;
    hls::stream<ap_uint<1> > pwm_l[3], pwm_l_a, pwm_l_b, pwm_l_c;
    ;
    hls::stream<ap_uint<1> > pwm_sync[3], pwm_sync_a, pwm_sync_b, pwm_sync_c;

/*preparing va0 vb0 and vc0 according to data type*/
#ifdef _SVPWM_USING_INT_
    for (int i = 0; i < TESTNUMBER; i++) {
        Va0[i] = Va0_f[i];
        Vb0[i] = Vb0_f[i];
        Vc0[i] = Vc0_f[i];
    }
#else
    for (int i = 0; i < TESTNUMBER; i++) {
        Va0[i] = (t_svpwm_cmd)Va0_f[i];
        Vb0[i] = (t_svpwm_cmd)Vb0_f[i];
        Vc0[i] = (t_svpwm_cmd)Vc0_f[i];
    }
#endif

    t_svpwm_cmd* Va = Va0;
    t_svpwm_cmd* Vb = Vb0;
    t_svpwm_cmd* Vc = Vc0;
    if (-1 != cmd_findPara(argc, argv, "-v0")) {
        Va = Va0;
        Vb = Vb0;
        Vc = Vc0;
    }
    if (-1 != cmd_findPara(argc, argv, "-v1")) {
        Va = Va1;
        Vb = Vb1;
        Vc = Vc1;
    }
    if (-1 != cmd_findPara(argc, argv, "-v2")) {
        Va = Va2;
        Vb = Vb2;
        Vc = Vc2;
    }

    for (int i = 0; i < TESTNUMBER; i++) {
        inputVa << Va[i];
        inputVb << Vb[i];
        inputVc << Vc[i];
        dc_link << dc_link_adc;
    }

    // hls_svpwm(
    SVPWM_wrapper(inputVa, inputVb, inputVc, dc_link, pwm_h_a, pwm_h_b, pwm_h_c, pwm_l_a, pwm_l_b, pwm_l_c, pwm_sync_a,
                  pwm_sync_b, pwm_sync_c, (volatile int&)axi_para.stt_pwm_cycle, (volatile int&)axi_para.args_pwm_freq,
                  (volatile int&)axi_para.args_dead_cycles, (volatile int&)axi_para.args_phase_shift,
                  (volatile int&)axi_para.args_dc_link_ref, (volatile int&)axi_para.stt_cnt_iter,
                  (volatile int&)axi_para.stt_cnt_read_foc, (volatile int&)axi_para.stt_cnt_read_dc,
                  (volatile int&)axi_para.args_dc_src_mode, (volatile int&)axi_para.args_sample_ii,
                  (volatile int&)axi_para.pwm_stt_Va_cmd, (volatile int&)axi_para.pwm_stt_Vb_cmd,
                  (volatile int&)axi_para.pwm_stt_Vc_cmd, (volatile int&)axi_para.pwm_stt_duty_ratio_a,
                  (volatile int&)axi_para.pwm_stt_duty_ratio_b, (volatile int&)axi_para.pwm_stt_duty_ratio_c);
    //(volatile long&)axi_para.args_cnt_trip);
    //        volatile t_svpwm_cmd& pwm_stt_Va_cmd,
    //    volatile t_svpwm_cmd& pwm_stt_Vb_cmd,
    //    volatile t_svpwm_cmd& pwm_stt_Vc_cmd

    char fname2[64];
    sprintf(fname2, "wave_all%d", TESTNUMBER);
    FILE* fp2 = fopen(fname2, "w");
    assert(fp2);
    char strm_table2[TESTNUMBER + 1][256];
    sprintf(strm_table2[0], "SIM_SVPWM: Item:");
    sprintf(strm_table2[0] + strlen(strm_table2[0]), "  | Vcmd_a\t High%% Low%% Dead%%  High   Low  Dead");
    sprintf(strm_table2[0] + strlen(strm_table2[0]), "  | Vcmd_b\t High%% Low%% Dead%%  High   Low  Dead");
    sprintf(strm_table2[0] + strlen(strm_table2[0]), "  | Vcmd_c\t High%% Low%% Dead%%  High   Low  Dead");
    for (int i = 0; i < TESTNUMBER; i++) {
        int h_cnt[3] = {0, 0, 0};
        int l_cnt[3] = {0, 0, 0};
        char fname[64];
        sprintf(fname, "wave_%d", i);
        FILE* fp = fopen(fname, "w");
        assert(fp);
        ap_uint<1> high[3];
        ap_uint<1> low[3];
        ap_uint<1> sync[3];
        for (int j = 0; j < clk_fq / axi_para.args_pwm_freq; j++) {
            pwm_h_a.read(high[0]);
            pwm_h_b.read(high[1]);
            pwm_h_c.read(high[2]);
            pwm_l_a.read(low[0]);
            pwm_l_b.read(low[1]);
            pwm_l_c.read(low[2]);
            pwm_sync_a.read(sync[0]);
            pwm_sync_b.read(sync[1]);
            pwm_sync_c.read(sync[2]);

            if (high[0] == 1) {
                h_cnt[0]++;
            }
            if (high[1] == 1) {
                h_cnt[1]++;
            }
            if (high[2] == 1) {
                h_cnt[2]++;
            }
            if (low[0] == 1) {
                l_cnt[0]++;
            }
            if (low[1] == 1) {
                l_cnt[1]++;
            }
            if (low[2] == 1) {
                l_cnt[2]++;
            }
            if (j == 0)
                // clang-format off
            fprintf(fp, "SVPWM_WAVE\t wave:%d\t i=%d\t high\t high0\t high1\t high2\t low\t low0\t low1\t low2\t sync\t sync0\t sync1\t sync2\t Va\t Vb\t Vc\t\n", i, j);
            fprintf(fp, "SVPWM_WAVE\t wave:%d\t i=%d\t high\t %d\t %d\t %d\t low\t %d\t %d\t %d\t sync\t %d\t %d\t %d\t%d\t %d\t %d\t \n", i, j, high[0],high[1],high[2], low[0],low[1],low[2], sync[0],sync[1],sync[2], Va[i], Vb[i], Vc[i]);
            // clang-format on
            if (j == 0 && i == 0)
                // clang-format off
            fprintf(fp2, "SVPWM_WAVE\t wave:%d\t i=%d\t high\t high0\t high1\t high2\t low\t low0\t low1\t low2\t sync\t sync0\t sync1\t sync2\t Va\t Vb\t Vc\t \n", i, j);
            fprintf(fp2, "SVPWM_WAVE\t wave:%d\t i=%d\t high\t %d\t %d\t %d\t low\t %d\t %d\t %d\t sync\t %d\t %d\t %d\t %d\t %d\t %d\t \n", i, j, high[0],high[1],high[2], low[0],low[1],low[2], sync[0],sync[1],sync[2], Va[i], Vb[i], Vc[i]);
            // clang-format on
        }
        fclose(fp);

        double high_per[3];
        double low_per[3];

        high_per[0] = (h_cnt[0] / factor) * 100;
        high_per[1] = (h_cnt[1] / factor) * 100;
        high_per[2] = (h_cnt[2] / factor) * 100;
        low_per[0] = (l_cnt[0] / factor) * 100;
        low_per[1] = (l_cnt[1] / factor) * 100;
        low_per[2] = (l_cnt[2] / factor) * 100;

        int pwm_cycle = (clk_fq / axi_para.args_pwm_freq);

        // calculate the residual ratio
        double res_diff = high_per[0] - (int)high_per[0];
        double res_ratio = res_diff / 100;
        if (allowed_res_ratio < res_ratio) {
            ret_err_val++;
        }

// clang-format off
#ifdef _SVPWM_USING_INT_  
        printf("SIM_SVPWM: voltages               \t%9d(V)\t%9d(V)\t%9d(V)\t\n", Va[i],  Vb[i],  Vc[i] );
#else
        printf("SIM_SVPWM: voltages               \t%5.3f(V)\t%5.3f(V)\t%5.3f(V)\t\n", Va[i].to_float(),  Vb[i].to_float(),  Vc[i].to_float() );
#endif
        printf("SIM_SVPWM: High percentages       \t   %5.2f%%\t   %5.2f%%\t   %5.2f%%\t\n", high_per[0] , high_per[1] , high_per[2]);
        printf("SIM_SVPWM: Low percentages        \t   %5.2f%%\t   %5.2f%%\t   %5.2f%%\t\n", low_per[0] , low_per[1] , low_per[2]);
        printf("SIM_SVPWM: Dead Cycle percentages \t   %5.2f%%\t   %5.2f%%\t   %5.2f%%\t\n", 100 - high_per[0] - low_per[0],  100 - high_per[1] - low_per[1] ,100 - high_per[2] - low_per[2]);
        printf("SIM_SVPWM: High cycles            \t  %6d\t  %6d\t  %6d\t\n", h_cnt[0] , h_cnt[1]  , h_cnt[2] );
        printf("SIM_SVPWM: Low cycles             \t  %6d\t  %6d\t  %6d\t\n", l_cnt[0] , l_cnt[1] ,  l_cnt[2] );
        printf("SIM_SVPWM: Dead cycles            \t  %6d\t  %6d\t  %6d\t\n", pwm_cycle-h_cnt[0]-l_cnt[0] , pwm_cycle-h_cnt[1]-l_cnt[1] ,  pwm_cycle-h_cnt[2]-l_cnt[2] );
        printf("SIM_SVPWM: axi_para.args_dc_link_ref : %5.3f(V)\n", axi_para.args_dc_link_ref.to_float());   
        printf("SIM_SVPWM: pwm_cycle(%d) = clk_fq(%d) / axi_para.args_pwm_freq(%d)   \n", pwm_cycle, clk_fq, axi_para.args_pwm_freq);   
        printf("SIM_SVPWM: Waveform data can be found in file %s\nSIM_SVPWM:\n", fname);

        sprintf(strm_table2[i+1], "SIM_SVPWM: %4d ", i);
#ifdef _SVPWM_USING_INT_
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  |%4d.0(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", (int)Va[i], (int)high_per[0], (int)low_per[0], (int)(100.5 - high_per[0] - low_per[0]), h_cnt[0], l_cnt[0], pwm_cycle-h_cnt[0]-l_cnt[0]);
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  |%4d.0(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", (int)Vb[i], (int)high_per[1], (int)low_per[1], (int)(100.5 - high_per[1] - low_per[1]), h_cnt[1], l_cnt[1], pwm_cycle-h_cnt[1]-l_cnt[1]);
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  |%4d.0(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", (int)Vc[i], (int)high_per[2], (int)low_per[2], (int)(100.5 - high_per[2] - low_per[2]), h_cnt[2], l_cnt[2], pwm_cycle-h_cnt[2]-l_cnt[2]);
#else
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  | %-4.1f(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", Va[i].to_float(), (int)high_per[0], (int)low_per[0], (int)(100.5 - high_per[0] - low_per[0]), h_cnt[0], l_cnt[0], pwm_cycle-h_cnt[0]-l_cnt[0]);
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  | %-4.1f(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", Vb[i].to_float(), (int)high_per[1], (int)low_per[1], (int)(100.5 - high_per[1] - low_per[1]), h_cnt[1], l_cnt[1], pwm_cycle-h_cnt[1]-l_cnt[1]);
        sprintf(strm_table2[i+1] + strlen(strm_table2[i+1]), "  | %-4.1f(V)\t %3d%%  %3d%%  %3d%%  %4d  %4d  %4d", Vc[i].to_float(), (int)high_per[2], (int)low_per[2], (int)(100.5 - high_per[2] - low_per[2]), h_cnt[2], l_cnt[2], pwm_cycle-h_cnt[2]-l_cnt[2]);
#endif
        // clang-format on
    }
    fclose(fp2);
    for (int i = 0; i < TESTNUMBER + 1; i++) {
        // clang-format off
        if(i==0) printf("SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("%s\n",strm_table2[i] );
        if(i==0) printf("SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        // clang-format on
    }

    t_svpwm_cmd V_ref = (axi_para.args_dc_src_mode == xf::motorcontrol::MODE_PWM_DC_SRC::DC_SRC_ADC)
                            ? dc_link_adc
                            : (t_svpwm_cmd)axi_para.args_dc_link_ref;
    float V_ref_f; // = (float)V_ref;
#ifdef _SVPWM_USING_INT_
    V_ref_f = (float)V_ref;
#else
    V_ref_f = V_ref.to_float();
    int dc_link_adc_int = dc_link_adc.range(dc_link_adc.length() - 1, 0);
#endif

    int tmp_link = axi_para.args_dc_link_ref.range(31, 0);
    // printf("SIM_SVPWM:
    // **************************************************************************************************************************\n");

    // clang-format off
    printf("\n\n");
    printf("SIM_SVPWM: **************************************     Global Const Parameters    ****************************************************\n");
    printf("SIM_SVPWM: ** NAME              Type    \tHex Value        Physic Value   Unit           ValueFormat      Command-line   \n");
    printf("SIM_SVPWM: ** TESTNUMBER        const   \t0x%8x\t %12d \t               \t  long\n", TESTNUMBER, TESTNUMBER);
    printf("SIM_SVPWM: ** clock_freq.       const   \t0x%8x\t %12d \tMHz            \t   int\n", clk_fq, clk_fq/1000000);
    printf("SIM_SVPWM: **************************************************************************************************************************\n");
    printf("\n\n");
    printf("SIM_SVPWM: **************************************************************************************************************************\n");  
    printf("SIM_SVPWM: --------------------------------------------    SVPWM_DUTY SECTION   -----------------------------------------------------\n");
    printf("SIM_SVPWM: ******************************************      AXI-lite Parameter    ****************************************************\n");
    printf("SIM_SVPWM: ** NAME              Type    \tHex Value        Physic Value   Unit           ValueFormat      Command-line   \n");
    printf("SIM_SVPWM: ** stt_cnt_iter      Read    \t0x%8x\t %12d \ttimes          \t   int\n", axi_para.stt_cnt_read_foc+1, axi_para.stt_cnt_iter+1);//as in kernel: stt_cnt_iter = iter++;
    printf("SIM_SVPWM: ** args_dc_link_ref  Write   \t0x%8x\t     %5.5f\t V         \t   q15q16 \n",tmp_link, axi_para.args_dc_link_ref.to_float());
    printf("SIM_SVPWM: ** args_dc_src_mode  Write   \t0x%8x\t ", axi_para.args_dc_src_mode);
    if(axi_para.args_dc_src_mode==xf::motorcontrol::MODE_PWM_DC_SRC::DC_SRC_ADC) printf("  DC_SRC_ADC\t"); else printf("  DC_SRC_REF\t"); printf("\t\t\t\t[-dc_adc/-dc_ref]\n");
    printf("SIM_SVPWM: ** args_sample_ii    Write   \t0x%8x\t %12d \t               \t   int\t\t[-ii <sampling II>]\n", axi_para.args_sample_ii, axi_para.args_sample_ii);
    printf("SIM_SVPWM: ** \033[1;31;40margs_sample_ii    IMPORTANT NOTICE for value setting: \033[0m\n");
    printf("SIM_SVPWM: **                   for CSIM  \t = clock_freq / args_pwm_freq\n");
    printf("SIM_SVPWM: **                   for COSIM \t depends on the latency of cascading cousumer.\n");
    printf("SIM_SVPWM: **                   for HW run\t '1' is better to avoid backpressure to upstream ADC \n");
    printf("SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------\n");
    printf("SIM_SVPWM: ---------------------------------------------   Static data types --------------------------------------------------------\n");
#ifdef _SVPWM_USING_INT_
    printf("SIM_SVPWM: ** Using integer type for Va, Vb, Vc and dc_link_adc \t using ap_uint<16> for ratio \n");
    printf("SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------\n");
    printf("SIM_SVPWM: -------------------------------------------- Inside kernel key values ----------------------------------------------------\n");
    printf("SIM_SVPWM: ** dc_link_adc      stream   \t0x%8x\t %12d \t V          \t   int\n", dc_link_adc, dc_link_adc);
    printf("SIM_SVPWM: ** V_ref            internal \t0x%8x\t %12d \t V          \t   int\n", (int)V_ref_f, (int)V_ref_f);
#else
    printf("SIM_SVPWM: **  Using ap_ufixed<%d, %d> type  for Va, Vb, Vc and dc_link_adc\t using ap_ufixed<16, 0> for ratio\n",  dc_link_adc.length(), PWM_AP_FIXED_PARA_I2);
    printf("SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------\n");
    printf("SIM_SVPWM: -------------------------------------------- Inside kernel key values ----------------------------------------------------\n");
    printf("SIM_SVPWM: ** dc_link_adc      stream   \t0x%8x\t     %5.5f\t V         \t  ap_ufixed<%d, %d>\n", dc_link_adc_int, dc_link_adc.to_float(), dc_link_adc.length(), PWM_AP_FIXED_PARA_I2);
    printf("SIM_SVPWM: ** V_ref            internal \t0x%8x\t     %5.0f\t V         \t  int\n", V_ref_f, V_ref_f);
#endif
    printf("SIM_SVPWM: ----------------------------------------------    SVPWM_DUTY END   ---------------------------------------------------------\n");
    printf("SIM_SVPWM: **************************************************************************************************************************\n");  
    printf("\n\n");
    printf("SIM_SVPWM: **************************************************************************************************************************\n");  
    printf("SIM_SVPWM: ---------------------------------------------    PWM_GEN SECTION   ---------------------------------------------------------\n");
    printf("SIM_SVPWM: ******************************************      AXI-lite Parameter    ******************************************************\n");
    printf("SIM_SVPWM: ** stt_pwm_cycle     Read    \t0x%8x\t %12d \tcycle of clk_fq\t   int\n", axi_para.stt_pwm_cycle, axi_para.stt_pwm_cycle);
    printf("SIM_SVPWM: ** args_pwm_freq     Write   \t0x%8x\t %12d \tHz             \t   int\t\t[-pwm_fq <pwm frequency>]\n", axi_para.args_pwm_freq, axi_para.args_pwm_freq);
    printf("SIM_SVPWM: ** args_dead_cycles  Write   \t0x%8x\t %12d \tcycle of clk_fq\t   int\t\t[-dead <dead cycles>]\n", axi_para.args_dead_cycles, axi_para.args_dead_cycles);
    printf("SIM_SVPWM: ** args_phase_shift  Write   \t0x%8x\t ", axi_para.args_phase_shift);
    if(axi_para.args_phase_shift==xf::motorcontrol::MODE_PWM_PHASE_SHIFT::SHIFT_ZERO) printf("  SHIFT_ZERO\t"); else printf("  SHIFT_120\t"); printf("\t\t\t\t[-shift_0/-shift_120]\n");
    printf("SIM_SVPWM: ** args_sample_ii    Write   \t0x%8x\t %12d \t               \t   int\t\t[-ii <sampling II>]\n", axi_para.args_sample_ii, axi_para.args_sample_ii);
    printf("SIM_SVPWM: ** \033[1;31;40margs_sample_ii    IMPORTANT NOTICE for value setting: \033[0m\n");
    printf("SIM_SVPWM: **                   for CSIM  \t = 1\n");
    printf("SIM_SVPWM: **                   for COSIM \t default value is 1.\n");
    printf("SIM_SVPWM: **                   for HW run\t '1' is better to avoid backpressure to upstream ADC \n");
    printf("SIM_SVPWM: ---------------------------------------------      PWM_GEN END   ---------------------------------------------------------\n");

    printf("SIM_SVPWM: **************************************************************************************************************************\n");  
    printf("SIM_SVPWM: All %d commands' waveform data can be found in file %s\n", TESTNUMBER, fname2);
    printf("SIM_SVPWM: csim.exe [-shift_0/-shift_120] | [-dc_adc/-dc_ref] | [-pwm_fq <pwm frequency>] | [-dead <dead cycles>] [-ii <sampling II>]\n");
    printf("SIM_SVPWM:          [-v0/-v1/-v2] #for selecting different test vector \n\n");

    // clang-format on
    return ret_err_val; // to be checked use motor modul
}
