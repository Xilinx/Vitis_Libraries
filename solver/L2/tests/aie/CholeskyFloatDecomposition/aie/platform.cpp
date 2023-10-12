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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expouts or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sstream>
#include <cstring>
#include <fstream>
#include <iostream>

#include "aie_graph.h"

TopGraph mygraph;

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
            if ((abs(out - gld) / abs(gld)) > 0.2) {
                return 1;
            }
        } else {
            if (abs(out - gld) > 0.2) {
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

int main(void) {
    /*
    // To generate input data and golden data
    HermitianMatrix<double > A(DIM);
    HermitianMatrix<double > L(DIM);
    HermitianMatrix<double > P(DIM);
    L.gen_lower_triangular_randf(1, 4, 0);
    L.gen_file_matrixL_by_column(L, matL_file);
    std::cout << "matrix L: \n";
    //L.print();
    A.gen_hermitianMatrix(L);
    //A.gen_file_matrixA_by_row(A, matA_file);
    A.gen_file_matrixA_by_column(A, matA_file);
    std::cout << "matrix A: \n";
    //A.print();

    P.cholesky_opt_1(A);
    P.gen_file_matrixL_by_column(P, gldMatL_file);
    std::cout << "matrix cholesky(A): \n";
    //P.print();
    */

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

    // compare output with golden
    int num = (1 + DIM) * DIM / 2 + 2;
    int errNum = 0;
    errNum = golden_check(outMatL_file, gldMatL_file, num);
    if (errNum != 0) {
        std::cout << "File: " << outMatL_file << " Mis-Matched! \n";
        std::cout << "Test failed! \n";
    } else {
        std::cout << "Test passed! \n";
    }
    return errNum;
}

#endif
