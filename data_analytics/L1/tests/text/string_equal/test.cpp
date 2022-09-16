/*
 * Copyright 2022 Xilinx, Inc.
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

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include "xf_data_analytics/text/stringCompare.hpp"

#define MAX_BASE_STR_LEN 64

std::string randStr(int length) {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result = "";

    for (int i = 0; i < length; i++) result.push_back(charset[rand() % charset.length()]);
    return result;
}

void dut(hls::stream<ap_uint<64> >& i_basestr_strm,
         hls::stream<ap_uint<64> >& i_str_strm,
         hls::stream<uint32_t>& i_len_strm,
         hls::stream<bool>& i_e_strm,
         bool not_flag,

         hls::stream<bool>& o_valid_strm,
         hls::stream<bool>& o_e_strm) {
    xf::data_analytics::text::string_equal<MAX_BASE_STR_LEN>(i_basestr_strm, i_str_strm, i_len_strm, i_e_strm, not_flag,
                                                             o_valid_strm, o_e_strm);
}

int main() {
    // set flag to run test for equal/not_equal
    const bool not_flag = false; // true - not equal; false - equal;

    // generate strings
    srand(100);
    int len_base_str = rand() % (MAX_BASE_STR_LEN + 1); // [0 - 64]
    std::string base_str = randStr(len_base_str);
    std::cout << "INFO: len_base_str = " << len_base_str << " base_str = " << base_str << std::endl;

    int nm_test_strs = 1000;
    std::vector<std::string> test_strs;
    for (int i = 0; i < nm_test_strs; ++i) {
        int r = rand() % 129; // [0 - 128]
        if (i % 100 == 5) {
            test_strs.push_back(base_str);
        } else {
            test_strs.push_back(randStr(r));
        }
    }

    // generate golden
    std::vector<bool> golden_flag;
    for (int i = 0; i < test_strs.size(); ++i) {
        bool f = false;
        if (test_strs[i].compare(base_str) == 0) {
            f = not_flag ? false : true;
        } else {
            f = not_flag ? true : false;
        }
        golden_flag.push_back(f);
    }

    hls::stream<ap_uint<64> > i_basestr_strm;
    hls::stream<ap_uint<64> > i_str_strm;
    hls::stream<uint32_t> i_len_strm;
    hls::stream<bool> i_e_strm;
    hls::stream<bool> o_valid_strm;
    hls::stream<bool> o_e_strm;

    // process base string
    if (base_str.length() > MAX_BASE_STR_LEN) {
        std::cout << "ERROR: base string length exceeds limit " << MAX_BASE_STR_LEN << " Bytes" << std::endl;
        std::cout << "ERROR: Test FAIL" << std::endl;
        return -1;
    }

    ap_uint<64> base_str_cfg[1 + 1 + MAX_BASE_STR_LEN / 8] = {0};
    base_str_cfg[0] = 1;
    base_str_cfg[1] = base_str.length();
    for (int c_idx = 0; c_idx < base_str.length(); ++c_idx) {
        base_str_cfg[2 + c_idx / 8].range(c_idx % 8 * 8 + 7, c_idx % 8 * 8) = base_str[c_idx];
    }
    for (int i = 0; i < 1 + 1 + MAX_BASE_STR_LEN / 8; ++i) {
        i_basestr_strm.write(base_str_cfg[i]);
    }

    // process input test data
    for (int i = 0; i < test_strs.size(); ++i) {
        std::string i_str = test_strs[i];
        i_len_strm.write(i_str.length());
        i_e_strm.write(false);
        ap_uint<3> cnt = 0;
        ap_uint<64> tmp = 0;
        for (int c_idx = 0; c_idx < i_str.length(); ++c_idx) {
            cnt++;
            tmp.range(c_idx % 8 * 8 + 7, c_idx % 8 * 8) = i_str[c_idx];
            if (cnt == 0) {
                i_str_strm.write(tmp);
                tmp = 0;
            }
        }
        if (cnt != 0) i_str_strm.write(tmp);
    }
    i_e_strm.write(true);

    // std::cout << "DEBUG: i_basestr_strm.size() = " << i_basestr_strm.size() << std::endl;
    // std::cout << "DEBUG: i_str_strm.size() = " << i_str_strm.size() << std::endl;
    // std::cout << "DEBUG: i_len_strm.size() = " << i_len_strm.size() << std::endl;
    // std::cout << "DEBUG: i_e_strm.size() = " << i_e_strm.size() << std::endl;
    // run dut
    dut(i_basestr_strm, i_str_strm, i_len_strm, i_e_strm, not_flag, o_valid_strm, o_e_strm);
    // std::cout << "DEBUG: i_basestr_strm.size() = " << i_basestr_strm.size() << std::endl;
    // std::cout << "DEBUG: i_str_strm.size() = " << i_str_strm.size() << std::endl;
    // std::cout << "DEBUG: i_len_strm.size() = " << i_len_strm.size() << std::endl;
    // std::cout << "DEBUG: i_e_strm.size() = " << i_e_strm.size() << std::endl;

    std::vector<bool> dut_flag;
    bool end = o_e_strm.read();
    while (!end) {
        bool flag = o_valid_strm.read();
        dut_flag.push_back(flag);
        end = o_e_strm.read();
    }

    bool ret = true;
    if (golden_flag.size() != dut_flag.size()) {
        ret = false;
    } else {
        for (int i = 0; i < golden_flag.size(); ++i) {
            if (golden_flag[i] != dut_flag[i]) {
                ret = false;
                break;
            }
        }
    }

    std::cout << "INFO: golden true = " << std::count(golden_flag.begin(), golden_flag.end(), true)
              << " dut true = " << std::count(dut_flag.begin(), dut_flag.end(), true) << std::endl;

    if (ret) {
        std::cout << "INFO: Test PASS" << std::endl;
        return 0;
    } else {
        std::cout << "ERROR: Test FAIL" << std::endl;
        return -1;
    }
}
