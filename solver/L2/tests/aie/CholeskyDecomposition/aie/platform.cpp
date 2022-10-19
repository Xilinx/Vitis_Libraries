/*
 * Copyright 2022 Xilinx, Inc.
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

#include "aie/cholesky_decomp_graph.hpp"

xf::solver::GivensRotationQRD mygraph(
    "in_vec_l", "data/vec_l.txt", "in_vec_h", "data/vec_h.txt", "out_vec_h", "data/out_vec.txt");

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <iostream>
#include <fstream>
using namespace std;

bool comp(float res, float gld) {
    if (abs(gld) > 1.0) {
        if ((abs(res - gld) / abs(gld)) > 0.001) {
            std::cout << "res = " << res << ", gld = " << gld << std::endl;
            return false;
        }
    } else {
        if (abs(res - gld) > 0.001) {
            std::cout << "res = " << res << ", gld = " << gld << std::endl;
            return false;
        }
    }
    return true;
}

int main(void) {
    unsigned int row_num = 16;

    adf::return_code ret;
    mygraph.init();
    mygraph.update(mygraph.row_num, row_num);
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

    // compare output with golden
    ifstream f_gld, f_res;
    f_gld.open("data/gld.txt", ios::in);
    f_res.open("data/out_vec.txt", ios::in);

    int checked = 0;
    for (int j = 0; j < row_num; j++) {
        for (int i = 0; i < row_num; i++) {
            float gld, res;
            f_gld >> gld;
#if defined(__AIESIM__)
            // remove time stamp in output record
            string dum;
            getline(f_res, dum);
#endif
            f_res >> res;
#if defined(__AIESIM__)
            getline(f_res, dum);
#endif
            if (!comp(res, gld)) {
                checked++;
            }
        }
    }
    return checked;
}

#endif
