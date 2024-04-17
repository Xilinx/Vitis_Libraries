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
#include "ip_qei.hpp"

unsigned int qei_inverse_new(hls::stream<ap_uint<1> >& A,
                             hls::stream<ap_uint<1> >& B,
                             hls::stream<ap_uint<1> >& I,
                             Dirction_QEI dir,
                             Encoding_Mode mode,
                             float& angle_start,
                             float angle_run,
                             int rpm,
                             int cpr,
                             int freq_clk) {
    double freq_AB = (double)rpm / 60.0 * cpr;
    double cycle_AB = 1.0 / freq_AB;
    int cycle_AB_freq = freq_clk * cycle_AB;
    int cycle_AB_freq_div4 = cycle_AB_freq >> 2;
    float degree_pre_cycle_AB = 360.0 / (float)cpr;
    int cycle_AB_run = (float)angle_run / 360.0 * cpr; // / degree_pre_cycle_AB;
    int start_off = cpr * (angle_start / 360.0);
    int num_write = 0;

    for (int cAB = 0; cAB < cycle_AB_run; cAB++) {
        for (int phase90 = 0; phase90 < 4; phase90++) {
            bool v_I = (start_off == cpr - 1);
            bool v_A = 0;
            bool v_B = 0;
            if ((dir == clockwise_n && mode == A_Leading_B) || (dir == clockwise_p && mode == B_Leading_A)) {
                // v_A:00110011
                // v_B:01100110
                if (phase90 == 1) {
                    v_A = 0;
                    v_B = 1;
                } else if (phase90 == 2) {
                    v_A = 1;
                    v_B = 1;
                } else if (phase90 == 3) {
                    v_A = 1;
                    v_B = 0;
                }
            } else {
                // v_A:01100110
                // v_B:00110011
                if (phase90 == 1) {
                    v_A = 1;
                    v_B = 0;
                } else if (phase90 == 2) {
                    v_A = 1;
                    v_B = 1;
                } else if (phase90 == 3) {
                    v_A = 0;
                    v_B = 1;
                }
            } // dir==1
            for (int c = 0; c < cycle_AB_freq_div4; c++) {
                A.write(v_A);
                B.write(v_B);
                I.write(v_I);
                num_write++;
            }

        } // phase90 * 4
        if (dir) {
            if (start_off == cpr - 1)
                start_off = 0;
            else
                start_off++;
        } else {
            if (start_off == 0)
                start_off = cpr - 1;
            else
                start_off--;
        }
    } // cycle_AB_run

    printf("SIM_QEI: CLK: %dM  CPR: %d  dir: %d  rmp: %5d  angle_start: %3.1f\t  run(%4d): %3.1f ", freq_clk / 1000000,
           cpr, dir, rpm * (dir == Dirction_QEI::clockwise_n ? -1 : 1), angle_start, cycle_AB_run, angle_run);

    angle_start = (float)start_off * 360.0 / (float)cpr;
    float time_used = (float)num_write / (float)freq_clk;

    // printf("\t end:%3.1f \tfreq_AB=%2.1fk cycle_AB_freq=%d  cycle_AB_run=%d, num_write=%d\n", angle_end,
    //       freq_AB / 1000.0, cycle_AB_freq, cycle_AB_run, num_write);
    printf("\tfreq_AB: %2.1fk\t num_write: %7d   time_used: %1.6f(sec)   rpm_est:%0.0f\n", freq_AB / 1000.0, num_write,
           time_used, angle_run / 360.0 / time_used * 60);
    return num_write;
}
Dirction_QEI getDir(int rpm) {
    return rpm > 0 ? clockwise_p : clockwise_n;
}
void Setting_3(hls::stream<ap_uint<1> >& strm_in_qei_A,
               hls::stream<ap_uint<1> >& strm_in_qei_B,
               hls::stream<ap_uint<1> >& strm_in_qei_I,
               AxiParameters_QEI& qei_args,
               int* golden_rpms) {
    // test_PPR_ = 500; // pulses per revolution
    int test_CPR_ = 1000; //(test_PPR_ * Encoding_Type::X4);
    int test_CLK_FQ_ = 100 * 1000 * 1000;
    qei_args.qei_args_cpr = test_CPR_;       // test_CPR_; //testing range-checking
    qei_args.qei_args_cnt_trip = TESTNUMBER; // test_NUM;  //testing range-checking
    Encoding_Mode mode = Encoding_Mode::B_Leading_A;
    qei_args.qei_args_ctrl = mode; // B leading A

    unsigned int total = 0;
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(i % 2);
        strm_in_qei_B.write(1 - i % 2);
        strm_in_qei_I.write(i % 2);
        total++;
    }
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(1);
        strm_in_qei_B.write(0);
        strm_in_qei_I.write(0);
        total++;
    }
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(0);
        strm_in_qei_B.write(1);
        strm_in_qei_I.write(0);
        total++;
    }
    float start = 355;
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[0]), mode, start, 60,
                             abs(golden_rpms[0]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[1]), mode, start, 10,
                             abs(golden_rpms[1]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[2]), mode, start, 20,
                             abs(golden_rpms[2]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[3]), mode, start, 20,
                             abs(golden_rpms[3]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[4]), mode, start, 2,
                             abs(golden_rpms[4]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[5]), mode, start, 3,
                             abs(golden_rpms[5]), test_CPR_, test_CLK_FQ_);
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, getDir(golden_rpms[6]), mode, start, 5,
                             abs(golden_rpms[6]), test_CPR_, test_CLK_FQ_);
    assert(TESTNUMBER > total);
    printf("QEI_GEN total writing =%d < %d\n", total, TESTNUMBER);
}

void Setting_4(hls::stream<ap_uint<1> >& strm_in_qei_A,
               hls::stream<ap_uint<1> >& strm_in_qei_B,
               hls::stream<ap_uint<1> >& strm_in_qei_I,
               AxiParameters_QEI& qei_args) {
    // test_PPR_ = 500; // pulses per revolution
    int test_CPR_ = 1000; //(test_PPR_ * Encoding_Type::X4);
    int test_CLK_FQ_ = 100 * 1000 * 1000;
    qei_args.qei_args_cpr = test_CPR_;                   // test_CPR_; //testing range-checking
    qei_args.qei_args_cnt_trip = TESTNUMBER;             // test_NUM;  //testing range-checking
    qei_args.qei_args_ctrl = Encoding_Mode::A_Leading_B; // A leading B

    unsigned int total = 0;
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(i % 2);
        strm_in_qei_B.write(1 - i % 2);
        strm_in_qei_I.write(i % 2);
        total++;
    }
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(1);
        strm_in_qei_B.write(0);
        strm_in_qei_I.write(0);
        total++;
    }
    for (int i = 0; i < 1000; i++) {
        strm_in_qei_A.write(0);
        strm_in_qei_B.write(1);
        strm_in_qei_I.write(0);
        total++;
    }
    float start = 0;
    total += qei_inverse_new(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, clockwise_p, A_Leading_B, start, 360, 3000,
                             test_CPR_, test_CLK_FQ_);
    assert(TESTNUMBER > total);
    printf("QEI_GEN total writing =%d < %d\n", total, TESTNUMBER);
}

int testBench() {
    hls::stream<ap_uint<1> > strm_in_qei_A("strm_in_qei_A");
    hls::stream<ap_uint<1> > strm_in_qei_B("strm_in_qei_B");
    hls::stream<ap_uint<1> > strm_in_qei_I("strm_in_qei_I");
    hls::stream<ap_uint<32> > strm_out_qei_RPM_THETA_m("qei_RPM_THETA_m");
    hls::stream<ap_uint<1> > strm_out_qei_dir("qei_dir");
    hls::stream<ap_uint<2> > strm_out_qei_err("qei_err");

    // int qei_args[QEI_ARGS_SIZE];
    AxiParameters_QEI qei_args;

    printf(
        "SIM_QEI: *****************************  Generating Input "
        "********************************************************\n");
    int golden_rpms[10] = {3000, -1000, -2000, -5000, 100, 300, 500};
    Setting_3(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, qei_args, golden_rpms);

    hls_qei(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, strm_out_qei_RPM_THETA_m, strm_out_qei_dir, strm_out_qei_err,
            (volatile int&)qei_args.qei_args_cpr, (volatile int&)qei_args.qei_args_ctrl,
            (volatile int&)qei_args.qei_stts_RPM_THETA_m, (volatile int&)qei_args.qei_stts_dir,
            (volatile int&)qei_args.qei_stts_err); //,
    //(volatile long&)qei_args.qei_args_cnt_trip);

    ap_uint<32> qei_stts_RPM_THETA_m = (ap_uint<32>)qei_args.qei_stts_RPM_THETA_m;

    int theta = qei_stts_RPM_THETA_m.range(31, 16);
    int rpm = qei_stts_RPM_THETA_m.range(15, 0);
    int cpr = qei_args.qei_args_cpr;
    long cnt_trip = qei_args.qei_args_cnt_trip;
    double cnt_trip_sec = (double)cnt_trip / 100000000.0;

    FILE* fp = fopen("qei.log", "w");
    assert(fp);
    int cnt = 0;
    fprintf(fp, "No.\tspeed\tangle\tdir\terr\t\n");

    int err = 0;
    short speed = 0;
    int angle = 0;
    int dir = Dirction_QEI::clockwise_n; // Dirction_QEI::clockwise_p

    int err_old = err;
    int speed_old = speed;
    int angle_old = angle;
    int dir_old = dir; // default Clockwise
    printf(
        "SIM_QEI: *****************************  Output analysis  "
        "********************************************************\n");
    int k = 0;
    int id = 0;
    int errors = 0;
    while (!strm_out_qei_RPM_THETA_m.empty() || !strm_out_qei_dir.empty() || !strm_out_qei_err.empty()) {
        fprintf(fp, "%d\t", cnt++);
        if (!strm_out_qei_RPM_THETA_m.empty()) {
            ap_uint<32> tmp = strm_out_qei_RPM_THETA_m.read();
            speed = tmp.range(15, 0);
            angle = tmp.range(31, 16);
            fprintf(fp, "%d\t", speed);
            fprintf(fp, "%d\t", angle);
        } else {
            fprintf(fp, "%d\t", -2);
            fprintf(fp, "%d\t", -2);
        }
        if (!strm_out_qei_dir.empty()) {
            dir = strm_out_qei_dir.read();
            fprintf(fp, "%d\t", dir);
        } else
            fprintf(fp, "%d\t", -1);
        if (!strm_out_qei_err.empty()) {
            err = strm_out_qei_err.read();
            fprintf(fp, "%d\t", err);
        } else
            fprintf(fp, "%d\t", -3);

        fprintf(fp, "\n");

        if (k != 0)
            if ((abs(err_old - err) > 0) || (abs(speed_old - speed) > 0) || (abs(dir_old - dir) > 0)) {
                printf(
                    "SIM_QEI: ** Changed out:%5d  dir: %1d  rpm: %5d  angle_detected: %3.1f\t counter:%5d\t err =%2d\n",
                    k, dir, speed, (float)angle / (float)cpr * 360, angle, err);
                if (abs(speed - golden_rpms[id]) > 100) errors++;
                id++;
            }
        err_old = err;
        speed_old = speed;
        angle_old = angle;
        dir_old = dir; // default Clockwise
        k++;
    }
    fclose(fp);

    printf(
        "SIM_QEI: *****************************AXI Parameter "
        "Value********************************************************\n");
    printf(
        "SIM_QEI: ** NAME              Type    Hex Value        Physic Value                                           "
        "   \n");
    printf(
        "SIM_QEI: "
        "--------------------------------------------------------------------------------------------------------\n");
    printf("SIM_QEI: ** args_cpr          Write   0x%8x\t %6d \n", cpr, cpr);
    printf("SIM_QEI: ** args_ctrl         Write   0x%8x\t ", qei_args.qei_args_ctrl);
    if (qei_args.qei_args_ctrl == Encoding_Mode::A_Leading_B)
        printf("A_Leading_B\n");
    else
        printf("B_Leading_A\n");
    printf("SIM_QEI: ** stts_RPM_THETA_m  Read    0x%8x\n", qei_args.qei_stts_RPM_THETA_m);
    printf("SIM_QEI:      |-RPM  (15, 0)  Read    0x%8x\t %6d\n", rpm, rpm);
    printf("SIM_QEI:      |-THETA(31, 16) Read    0x%8x\t %6d / cpr * 2 * PI = %2.4f(Rad)\n", theta, theta,
           (float)theta * 2 * 3.14159 / (float)cpr);
    printf("SIM_QEI: ** stts_dir          Read    0x%8x\t ", qei_args.qei_stts_dir);
    if (qei_args.qei_stts_dir == 1)
        printf("Clockwise\n");
    else
        printf("Counter-Clockwise\n");
    printf("SIM_QEI: ** stts_err          Read    0x%8x\t ", qei_args.qei_stts_err);
    if ((qei_args.qei_stts_err & 3) == 3)
        printf("Error: no edge detected at least for about %8.6f sec\n", (float)QEI_MAX_NO_EDGE_CYCLE / 100000000.0);
    else
        printf("\n");
    printf("SIM_QEI: ** args_cnt_trip     Write   0x%8lx\t  %6ld cycles = %11.6f sec\n", cnt_trip, cnt_trip,
           cnt_trip_sec);
    printf(
        "SIM_QEI: "
        "*********************************************************************************************************\n");
    printf("SIM_QEI: All waveform data can be found in file %s\n\n", "qei.log");

    if (errors)
        return 1;
    else
        return 0; // to be checked use motor modul
}
int main() {
    return testBench();
}
