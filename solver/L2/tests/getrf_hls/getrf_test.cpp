/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_solver_L2.hpp"

#define NRC 16
#define NCU 1

#ifndef __SYNTHESIS__
#include "matrixUtility.hpp"
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>
#endif

void top_getrf(double* A) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = A latency = 64 num_read_outstanding = \
    16 num_write_outstanding = 16 max_read_burst_length = 64 max_write_burst_length = 64 depth = 16*16

// clang-format on
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int info;
    xf::solver::getrf<double, NRC, NRC, NCU>(NRC, NRC, A, NRC, info);
};

#ifndef __SYNTHESIS__

int main() {
    double A[NRC * NRC];

    double** L = new double*[NRC];
    double** U = new double*[NRC];
    for (int i = 0; i < NRC; i++) {
        L[i] = new double[NRC];
        U[i] = new double[NRC];
    }

    unsigned int seedL = 2;
    unsigned int seedU = 3;
    triLowerMatGen<double>(NRC, seedL, L);
    triLowerMatGen<double>(NRC, seedU, U);

    for (int i = 0; i < NRC; i++) {
        L[i][i] = 1.0;
    }

    for (int i = 0; i < NRC; i++) {
        for (int j = 0; j < NRC; j++) {
            A[i * NRC + j] = 0.0;
            for (int k = 0; k < NRC; k++) {
                A[i * NRC + j] += L[i][k] * U[j][k];
            }
        }
    }

    // top
    top_getrf(A);

    int info = 0;
    for (int i = 0; i < NRC; i++) {
        for (int j = 0; j < NRC; j++) {
            if (i > j) {
                if (A[i * NRC + j] != L[i][j]) {
                    info = 1;
                }
            } else {
                if (A[i * NRC + j] != U[j][i]) {
                    info = 1;
                }
            };
        }
    }

    for (int i = 0; i < NRC; i++) {
        delete[] L[i];
        delete[] U[i];
    }
    delete[] L;
    delete[] U;

    return info;
};

#endif
