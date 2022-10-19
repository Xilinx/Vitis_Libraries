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

#include "aie/qrd_graph.hpp"

xf::solver::GivensRotationQRD mygraph("in_vec_l",
                                      "data/in_vec_l.txt",
                                      "in_vec_h",
                                      "data/in_vec_h.txt",
                                      "out_vec_l",
                                      "data/out_vec_l.txt",
                                      "out_vec_h",
                                      "data/out_vec_h.txt");

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
    unsigned int column_num = 16;

    adf::return_code ret;
    mygraph.init();
    mygraph.update(mygraph.row_num0, row_num);
    mygraph.update(mygraph.row_num1, row_num);
    mygraph.update(mygraph.row_num2, row_num);
    mygraph.update(mygraph.row_num3, row_num);
    mygraph.update(mygraph.column_num0, column_num);
    mygraph.update(mygraph.column_num1, column_num);
    mygraph.update(mygraph.column_num2, column_num);
    mygraph.update(mygraph.column_num3, column_num);
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
    ifstream f_gld_l, f_gld_h, f_res_l, f_res_h;
    f_gld_l.open("data/gld_vec_l.txt", ios::in);
    f_gld_h.open("data/gld_vec_h.txt", ios::in);
    f_res_l.open("data/out_vec_l.txt", ios::in);
    f_res_h.open("data/out_vec_h.txt", ios::in);

    int checked = 0;
    for (int j = 0; j < column_num - 1; j++) {
        for (int i = row_num - 1; i > j; i--) {
            for (int k = 0; k < column_num; k++) {
                float gld_l, gld_h, res_l, res_h;
                f_gld_l >> gld_l;
                f_gld_h >> gld_h;
#if defined(__AIESIM__)
                // remove time stamp in output record
                string dum_l, dum_h;
                getline(f_res_l, dum_l);
                getline(f_res_h, dum_h);
#endif
                f_res_l >> res_l;
                f_res_h >> res_h;
#if defined(__AIESIM__)
                getline(f_res_l, dum_l);
                getline(f_res_h, dum_h);
#endif
                if (!comp(res_l, gld_l)) {
                    checked++;
                }

                if (!comp(res_h, gld_h)) {
                    checked++;
                }
            }
        }
    }

    return checked;
}
#endif
