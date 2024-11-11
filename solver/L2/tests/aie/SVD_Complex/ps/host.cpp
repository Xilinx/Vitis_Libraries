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
#include "HarnessHelper.hpp"

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
    const int column_num = 16;
    const int row_num = 16;
    const int loop = 1;
    int in_num = column_num * (row_num + column_num);
    int out_num = column_num * (row_num + column_num);
    int in_sz = in_num * sizeof(float);
    int out_sz = out_num * sizeof(float);
    float* in_data[in_ch];
    float* out_data[out_ch];
    float* gld_data[out_ch];
    std::cout << "test123\n";

    for (int i = 0; i < in_ch; i++) {
        in_data[i] = (float*)malloc(in_sz);
    }
    for (int i = 0; i < out_ch; i++) {
        out_data[i] = (float*)malloc(out_sz);
        gld_data[i] = (float*)malloc(out_sz);
    }

    std::cout << "load from: " << argv[2] << std::endl;
    load_tv(argv[2], (float*)in_data[0], in_num);
    std::cout << "load from: " << argv[3] << std::endl;
    load_tv(argv[3], (float*)in_data[1], in_num);
    std::cout << "load from: " << argv[4] << std::endl;
    load_tv(argv[4], (float*)gld_data[0], out_num);
    std::cout << "load from: " << argv[5] << std::endl;
    load_tv(argv[5], (float*)gld_data[1], out_num);

    float* data;
    data = (float*)malloc(2 * in_num * sizeof(float));
    int j = 0;
    for (int i = 0; i < in_num; i += 4) {
        for (int k = 0; k < 4; k++) {
            data[j++] = in_data[0][i + k];
        }
        for (int k = 0; k < 4; k++) {
            data[j++] = in_data[1][i + k];
        }
    }

    HarnessHelper<256, float> harnessHelper(0, argv[1], {"mygraph"});
    harnessHelper.runAIEGraph(0, loop);
    harnessHelper.runPL(data, 2 * in_num, 2 * out_num, loop);
    auto result = harnessHelper.waitForRes(10000);

    j = 0;
    for (int i = 0; i < out_num; i += 4) {
        for (int k = 0; k < 4; k++) {
            out_data[0][i + k] = result[j++];
        }
        for (int k = 0; k < 4; k++) {
            out_data[1][i + k] = result[j++];
        }
    }
    int checked = 0;

    for (int i = 0; i < out_ch; i++) {
        for (int j = 0; j < out_num; j++) {
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
