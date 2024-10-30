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

typedef ap_uint<1> bit;

int main() {
    hls::stream<bit> strm_in_qei_A("strm_in_qei_A");
    hls::stream<bit> strm_in_qei_B("strm_in_qei_B");
    hls::stream<bit> strm_in_qei_I("strm_in_qei_I");
    hls::stream<ap_uint<32> > strm_qei_RPM_THETA_m("strm_qei_RPM_THETA_m");
    hls::stream<bit> strm_out_qei_dir("qei_dir");
    hls::stream<ap_uint<2> > strm_out_qei_err("qei_err");
    hls::stream<ap_uint<256> > logger("logger");
    // init parameter of Encoder
    int test_CPR_ = 1000;
    int qei_cnt_trip_debug = 30000;
    // generation parameters for 1 and 0 to simulate streams incoming from A, B, I
    unsigned int num_phase_ = 768;
    unsigned int start_a = 384;
    unsigned int start_b = 0;
    bool b_leading_a = (start_b > start_a) ? true : false;
    int cont_I = -1;
    ap_uint<1> a_var_strm = (ap_uint<1>)0;
    ap_uint<1> b_var_strm = (ap_uint<1>)0;
    ap_uint<1> i_var_strm = (ap_uint<1>)0;
    const unsigned int num_cycle = 30;
    unsigned int cont = 0;

    const unsigned int BIT_DEPTH = 32;

    std::vector<int> debug_a;
    std::vector<int> debug_b;
    std::vector<int> debug_I;

    bool flag_I = false;
    bool first_time_set = true;
    bool a_bool, b_bool;

    while (cont < num_cycle) {
        // create stream of data
        strm_in_qei_A.write(a_var_strm);
        strm_in_qei_B.write(b_var_strm);

        debug_a.push_back(a_var_strm.to_int());
        debug_b.push_back(b_var_strm.to_int());
        // index count
        if (start_a == 0 || start_b == 0) {
            cont_I = (cont_I + 1) % (test_CPR_);
            if (!first_time_set) {
                flag_I = (cont_I == 0) ? true : false;
            } else {
                first_time_set = false;
            }
        }
        // when boath are 0's and we pass the CPR count, simulate to set index
        if (flag_I && b_var_strm == (ap_uint<1>)0 && a_var_strm == (ap_uint<1>)0) {
            strm_in_qei_I.write(1);
            debug_I.push_back(1);
        } else {
            strm_in_qei_I.write(0);
            debug_I.push_back(0);
        }
        // every num of simulated 1's or 0's invert input
        start_a = (start_a + 1) % num_phase_;
        start_b = (start_b + 1) % num_phase_;

        if (start_b == 0) {
            b_var_strm = 1 - b_var_strm;
            if (b_leading_a) {
                cont++;
            }
        }

        if (start_a == 0) {
            a_var_strm = 1 - a_var_strm;
            if (!b_leading_a) {
                cont++;
            }
        }
    }
    int AB_filt_size = 256;
    int I_filt_size = 8;
    int zero_ = 0;
    int mode_ = 3; // enc_count_mode::X4;
    // test var
    volatile int& qei_args_cpr = test_CPR_;
    volatile int& qei_args_ctrl = test_CPR_;
    volatile int& qei_stts_RPM_THETA_m = zero_;
    volatile int& qei_stts_dir = zero_;
    volatile int& qei_stts_err = zero_;
    volatile int& qei_args_flt_size = AB_filt_size;
    volatile int& qei_args_flt_noise = test_CPR_;
    volatile int& qei_args_cnt_trip = qei_cnt_trip_debug;
    volatile int& mode_qei = mode_; // enc_count_mode::X4;
    volatile int& qei_debug_rpm = zero_;
    volatile int& qei_args_flt_size_i = I_filt_size;

    ap_uint<32> RPM_Angle_pack;

    hls_qei(strm_in_qei_A, strm_in_qei_B, strm_in_qei_I, strm_qei_RPM_THETA_m, logger, qei_args_cpr, qei_args_ctrl,
            qei_stts_RPM_THETA_m, qei_stts_dir, qei_stts_err, qei_args_flt_size, qei_args_cnt_trip, qei_debug_rpm,
            mode_qei, qei_args_flt_size_i);

    int cnt_data = 0;
    for (unsigned int i = 0; i < qei_args_cnt_trip; ++i) {
        if (!strm_qei_RPM_THETA_m.empty()) {
            RPM_Angle_pack = strm_qei_RPM_THETA_m.read();
            cnt_data++;
            std::cout << "VELOCITY: " << (RPM_Angle_pack & 0x0000FFFF) << std::endl;
            std::cout << "ANGLE: " << ((RPM_Angle_pack & 0xFFFF0000) >> 16) << std::endl;
        }
        if (!logger.empty()) logger.read();
    }
    return cnt_data != 0 ? 0 : 1;
}
