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

#include <iostream>
#include <cmath>
#include <fstream>
#include <string>
#include "PseudoInverse_ref.hpp"
#include "aie_graph_params.h"
using namespace std;

static void WriteFile(CMatrix& A, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0);
    myfile1.open(filename1);
    for (int j = 0; j < A.cols(); j++) {
        for (int i = 0; i < A.rows(); i += 4) {
            myfile0 << A.m_val[i][j].real() << endl;
            myfile0 << A.m_val[i][j].imag() << endl;
            myfile0 << A.m_val[i + 1][j].real() << endl;
            myfile0 << A.m_val[i + 1][j].imag() << endl;

            myfile1 << A.m_val[i + 2][j].real() << endl;
            myfile1 << A.m_val[i + 2][j].imag() << endl;
            myfile1 << A.m_val[i + 3][j].real() << endl;
            myfile1 << A.m_val[i + 3][j].imag() << endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

int main() {
    const int M = row_num; // rows
    const int N = col_num; // columns

    CMatrix A(M, N);
    A.WriteCMatrix("A_info.txt");
    WriteFile(A, "A0.txt", "A1.txt");

    CMatrix Pinv(N, M);
    A.PseudoInverse(A, Pinv);
    Pinv.WriteCMatrix("Gld_info.txt");
    WriteFile(Pinv, "Gld0.txt", "Gld1.txt");
    return 0;
}
