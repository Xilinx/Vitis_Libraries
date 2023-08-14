/*
 * MIT License
 *
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Advanced Micro Devices, Inc. shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this Software without prior written authorization from
 * Advanced Micro Devices, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <cstring>
#include <cmath>
#include "aie_graph_params.h"

// The test harness APIs
#include "vck190_test_harness_mgr.hpp"
using namespace vck190_test_harness;
using namespace std;

void load_tv(std::string f_path, float* data, int num) {
    ifstream file;
    file.open(f_path, ios::in);

    string str;
    for (int j = 0; j < num; j += 4) {
        for (int i = 0; i < 4; i++) {
            getline(file, str, ' ');
            data[j + i] = atof(str.c_str());
        }
        getline(file, str);
    }
    file.close();
};

int compare(float out, float gld) {
    if (1 == isnan(out)) {
        std::cout << "out is abnorman, out=" << out << std::endl;
        return 1;
    } else {
        if (abs(gld) > 1.0) {
            if ((abs(out - gld) / abs(gld)) > 0.001) {
                return 1;
            }
        } else {
            if (abs(out - gld) > 0.001) {
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    const int in_ch = 2;
    const int out_ch = 2;
    const int column_num = test_param::col_num;
    const int row_num = test_param::row_num;
    const int rep = test_param::rep_num;
    int in_sz = (column_num * (row_num + column_num)) * sizeof(float);
    int out_sz = (column_num * (row_num + column_num)) * sizeof(float);
    char* in_data[in_ch];
    char* out_data[out_ch];
    char* gld_data[out_ch];

    for (int i = 0; i < in_ch; i++) {
        in_data[i] = (char*)malloc(in_sz);
    }
    for (int i = 0; i < out_ch; i++) {
        out_data[i] = (char*)malloc(out_sz);
        gld_data[i] = (char*)malloc(out_sz);
    }

    std::cout << "load from: " << argv[2] << std::endl;
    load_tv(argv[2], (float*)in_data[0], column_num * (row_num + column_num));
    std::cout << "load from: " << argv[3] << std::endl;
    load_tv(argv[3], (float*)in_data[1], column_num * (row_num + column_num));
    std::cout << "load from: " << argv[4] << std::endl;
    load_tv(argv[4], (float*)gld_data[0], column_num * (row_num + column_num));
    std::cout << "load from: " << argv[5] << std::endl;
    load_tv(argv[5], (float*)gld_data[1], column_num * (row_num + column_num));

    test_harness_mgr mgr(0, argv[1], {"mygraph"});
    std::vector<test_harness_args> args;
    args.push_back({channel_index(Column_12_TO_AIE), in_sz, rep, 0, (char*)in_data[0]});
    args.push_back({channel_index(Column_13_TO_AIE), in_sz, rep, 0, (char*)in_data[1]});
    args.push_back({channel_index(Column_28_FROM_AIE), out_sz, rep, 0, (char*)out_data[0]});
    args.push_back({channel_index(Column_29_FROM_AIE), out_sz, rep, 0, (char*)out_data[1]});
    mgr.runAIEGraph(0, 1);
    mgr.runTestHarness(args);
    mgr.waitForRes(10000);

    int checked = 0;
    for (int i = 0; i < out_ch; i++) {
        for (int j = 0; j < (column_num * (column_num + row_num)); j++) {
            compare(out_data[i][j], gld_data[i][j]);
        }
    }

    for (int i = 0; i < in_ch; i++) {
        free(in_data[i]);
    }
    for (int i = 0; i < out_ch; i++) {
        free(out_data[i]);
        free(gld_data[i]);
    }

    return checked;
}
