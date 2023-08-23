/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __US_SYSTEM_HPP__
#define __US_SYSTEM_HPP__
#include <stdio.h>
#include <fstream>
#include <iostream>

#include "json.hpp"
#include "us_model_base.hpp"
#include "us_example_parameter.hpp"
#include "us_device.hpp"
#include "us_op_imagepoint.hpp"
#include "us_op_focus.hpp"
#include "us_op_delay.hpp"
#include "us_op_apodization.hpp"
#include "us_op_interpolation.hpp"
#include "us_op_sample.hpp"
#include "us_op_mult.hpp"
//#include "us_models.hpp"

#define SYNCBUFF(a, b, n) \
    for (int i = 0; i < n; i++) a[i] = b[i]
#define SYNCBUFF_SEG(a, b, n, s) \
    for (int i = 0; i < n; i++) a[i + n * s] = b[i]
#define SYNCBUFF_SEG_S(a, b, n, s)  \
    for (int i = 0; i < n; i++) {   \
        a[i + n * s] = b[0].read(); \
        b[1].write(a[i + n * s]);   \
    }

template <class T, int NUM_LINE_t, int NUM_SAMPLE_t>
void Json_out(us_op_mult<T>& mult, std::string output_json_path) {
    std::fstream outJson;
    nlohmann::json json_out;
    std::string res;

    printf("MODEL_TEST_SCANLINE: Saving image in Json file %s\n", output_json_path.c_str());
    outJson.open(output_json_path, std::ios::out);
    if (!outJson.is_open()) {
        std::cout << "Can't open file" << std::endl;
    }

    float rf_data_[NUM_LINE_t * NUM_SAMPLE_t * 4] = {0};
    assert(NUM_LINE_t * NUM_SAMPLE_t * 4 >= mult.m_num_line * mult.m_num_sample * 4);
    for (int i = 0; i < mult.m_num_line * mult.m_num_sample * 4; i++) rf_data_[i] = mult.m_p_data[i];
    json_out["rf_data"] = rf_data_;
    res = json_out.dump();
    outJson << res;
    outJson.close();
}

template <class T>
int fdatacmp(const char* fn1, const char* fn2, T th_abs, float th_ratio) {
    std::fstream fin1(fn1, std::ios::in);
    std::fstream fin2(fn2, std::ios::in);
    if (!fin1) {
        std::cout << "Error : fdatacmp() " << fn1 << "file doesn't exist !" << std::endl;
        exit(1);
    }
    if (!fin2) {
        std::cout << "Error : fdatacmp() " << fn2 << "file doesn't exist !" << std::endl;
        exit(1);
    }
    const int range = 20;
    long cnt_diff[range] = {0};
    long cnt_pass = 0;
    long cnt_same = 0;
    long cnt_all_z = 0; // both values are zero
    long cnt_all = 0;
    int off = abs(std::log10(th_abs));
    T max_diff = 0;
    T max_diff_d1;
    T max_diff_d2;
    long max_cnt;
    T max_d1_p = 0;
    T max_d2_p = 0;
    T min_d1_n = 0;
    T min_d2_n = 0;

    std::string line1;
    std::string line2;
    bool valid1;
    bool valid2;
    valid1 = std::getline(fin1, line1) ? true : false;
    valid2 = std::getline(fin2, line2) ? true : false;
    int cnt1 = 0;
    int cnt2 = 0;
    cnt1 = valid1 ? cnt1 + 1 : cnt1;
    cnt2 = valid2 ? cnt2 + 1 : cnt2;
    while (valid1 && valid2) {
        std::stringstream istr1(line1);
        std::stringstream istr2(line2);
        T d1, d2;
        istr1 >> d1;
        istr2 >> d2;
        T diff = abs(d2 - d1);
        float diff_r = d1 == 0 ? 0 : (float)diff / (float)d1;
        int idx = 0;
        if (diff > th_abs) {
            idx = std::log10(diff) + off;
            cnt_diff[idx]++;
            if (diff > max_diff) {
                max_diff = max_diff;
                max_diff_d1 = d1;
                max_diff_d2 = d2;
                max_cnt = cnt_all;
            }
        }
        if (!(diff > th_abs && diff_r > th_ratio)) {
            cnt_pass++;
            if (diff == (T)0.0) {
                cnt_same++;
                if (d1 == (T)0.0) cnt_all_z++;
            }
            if (idx != 0) cnt_diff[0]++;
        }
        max_d1_p = d1 > max_d1_p ? d1 : max_d1_p;
        max_d2_p = d2 > max_d2_p ? d2 : max_d1_p;
        min_d1_n = d1 < min_d1_n ? d1 : min_d1_n;
        min_d2_n = d2 < min_d2_n ? d2 : min_d1_n;

        valid1 = std::getline(fin1, line1) ? true : false;
        valid2 = std::getline(fin2, line2) ? true : false;
        cnt1 = valid1 ? cnt1 + 1 : cnt1;
        cnt2 = valid2 ? cnt2 + 1 : cnt2;
        cnt_all++;
    } // while
    // clang-format off
    printf("***********     Comparison   ************\n");
    printf("*********** File 1                           : %s \n", fn1);
    printf("*********** File 2                           : %s \n", fn2);
    printf("*********** Error judgement                  : diff_abs > %e && diff_ratio > %e\n", th_abs, th_ratio);
    printf("*********** Total tested data number         : %ld\n",cnt_all);//, cnt1, cnt2); 
    printf("*********** Total passed data number         : %ld\t %.2f\%\n", cnt_pass,     (float)cnt_pass  /(float)cnt_all*100.0);
    printf("***********       complete same data number  : %ld\t %.2f\%\n", cnt_same,     (float)cnt_same  /(float)cnt_all*100.0);
    printf("***********       complete zero data number  : %ld\t %.2f\%\n", cnt_all_z,    (float)cnt_all_z /(float)cnt_all*100.0);

    printf("*********** Absolute Errors distributions   ************\n");
    printf("*********** Checked data range of First file : [%e, %e]\n",  min_d1_n, max_d1_p);
    printf("*********** Checked data range of Second file: [%e, %e]\n",  min_d2_n, max_d2_p);
    for(int i=1; i<range; i++){
       // if(cnt_diff[i]!=0)
            printf("Differences range from [ 10^%2d to 10^%2d )    : \t %ld\t %.2f\%\n", i-off-1, i-off,  cnt_diff[i], (float)cnt_diff[i]/(float)cnt_all*100.0);
    }
    // clang-format on
    fin1.close();
    fin2.close();
    return cnt_all - cnt_pass;
}

;
#endif