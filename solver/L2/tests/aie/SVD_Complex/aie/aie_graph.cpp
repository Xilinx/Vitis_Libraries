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

#include "aie_graph.h"
using namespace adf;

TopGraph mygraph("data/A0.txt", "data/A1.txt", "data/Res0.txt", "data/Res1.txt");

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <iostream>
#include <fstream>
using namespace std;

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

int golden_check(std::string output_file, std::string golden_file, int num) {
    int checked = 0;
    ifstream f_gld, f_out;
    f_gld.open(golden_file, ios::in);
    f_out.open(output_file, ios::in);

    float out;
    float gld;
    char c = 'T';
    string str_gld, str_out;

    for (int j = 0; j < num; j++) {
        getline(f_gld, str_gld, '\n');
        gld = atof(str_gld.c_str());

        while (getline(f_out, str_out, '\n')) {
            if (str_out.front() != c) {
                break;
            }
        }
        out = atof(str_out.c_str());
        if (0 != compare(out, gld)) {
            checked++;
            std::cout << "Golden check mis-matched, error_num=" << checked << ", element id=" << j << ", out=" << out
                      << ", gld=" << gld << std::endl;
        }
    }
    return checked;
}

int main(int argc, char** argv) {
    adf::return_code ret;
    mygraph.init();

    ret = mygraph.run(1);
    if (ret != adf::ok) {
        printf("Run Failed\n");
        return ret;
    }

    ret = mygraph.end();
    if (ret != adf::ok) {
        printf("End Failed\n");
        return ret;
    }

    // compare and check
    int num = col_num * row_num * 2;
    std::cout << "checking Res0 againt Golden" << std::endl;
    int err_n0 = golden_check("data/Res0.txt", "data/Gld0.txt", num);
    std::cout << "checking Res1 againt Golden" << std::endl;
    int err_n1 = golden_check("data/Res1.txt", "data/Gld1.txt", num);

    return err_n0 + err_n1;
}
#endif
