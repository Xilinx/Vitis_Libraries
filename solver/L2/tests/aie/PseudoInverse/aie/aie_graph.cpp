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

#include "pseudoinverse_graph.hpp"
#include "aie_graph_params.h"
using namespace adf;

#define ITER_CNT 1
#define GRAPH_ITER_CNT ((SPLIT / NUM_KERNEL) * SPLIT * ITER_CNT)

xf::solver::PseudoInverseComplexFloat mygraph;

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
    ret = mygraph.run(GRAPH_ITER_CNT);
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
    int num = (P_DIM_A * P_DIM_B) / NUM_KERNEL;
    int err_n = 0;
    for (int i = 0; i < NUM_KERNEL; i++) {
        std::cout << "checking C" + std::to_string(i) + " againt Golden" << std::endl;
        std::string GoldenDir = "aiesim_data/gemm_" + std::to_string(P_DIM_A) + 'x' + std::to_string(P_DIM_AB) + 'x' +
                                std::to_string(P_DIM_B) + "_ioFiles/c" + std::to_string(i) + "_final_output.txt";
        int err_n0 = golden_check("data/c" + std::to_string(i) + ".txt", GoldenDir, num);
        err_n += err_n0;
    }

    return err_n;
}
#endif
