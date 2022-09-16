/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _DUT_UTILS_HPP_
#define _DUT_UTILS_HPP_
#if !defined(__SYNTHESIS__)

#include <ap_int.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

template <int W>
void convertStr2Int(std::string str, ap_uint<W>* out) {
    ap_uint<W> out_internel = 0;
    int len = str.length();
    int out_len = len;
    int tmp = 0;
    int i = 0;
    if ((str[1] == 'x') || (str[1] == 'X')) {
        i = 2;
    }
    for (; i < len; i++) {
        if ((str[i] >= '0') && (str[i] <= '9')) {
            tmp = str[i] - '0';
        } else if ((str[i] >= 'A') && (str[i] <= 'F')) {
            tmp = str[i] - 'A' + 10;
        } else if ((str[i] >= 'a') && (str[i] <= 'f')) {
            tmp = str[i] - 'a' + 10;
        } else {
            continue;
        }
        out_internel.range((len - 1 - i) * 4 + 3, (len - 1 - i) * 4) = tmp;
    }
    *out = out_internel;
}

template <int W>
void readDataFromFile_0(std::string fileName, std::vector<ap_uint<W> >& datas) {
    std::ifstream ifile1;
    std::string s;
    ap_uint<W> d = 0;
    int cnt = 0;

    ifile1.open(fileName);
    if (ifile1.is_open()) {
        while (std::getline(ifile1, s)) {
            convertStr2Int<W>(s, &d);
            datas.push_back(d);
            cnt++;
        }
        ifile1.close();
    } else {
        std::cout << "File is not opened \n";
    }
}

template <int W>
void readDataFromFile_1(std::string fileName, std::vector<ap_uint<W> >& datas) {
    std::ifstream ifile;
    std::string line;
    std::string m;
    ap_uint<W> element = 0;
    ifile.open(fileName);
    if (ifile.is_open()) {
        while (std::getline(ifile, line)) {
            std::istringstream iss(line);
            while (iss >> m) {
                convertStr2Int<W>(m, &element);
                datas.push_back(element);
            }
        }
        ifile.close();
    } else {
        std::cout << "File is not opened \n";
    }
}

ap_uint<256> swapBytes256(ap_uint<256> val) {
    int n = 256 / 8;

    ap_uint<256> input = val;
    ap_uint<256> result = 0;

    for (int i = 0; i < n; i++) {
        result.range(8 * i + 7, 8 * i) = input.range(8 * (n - 1 - i) + 7, 8 * (n - 1 - i));
    }
    return result;
}

#endif // (__SYNTHESIS__)
#endif
