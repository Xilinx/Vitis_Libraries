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

#ifndef __SYNTHESIS__
#include "matrixUtility.hpp"
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>
#endif

#define N 16
#define NCU 1
#define DT double

void top_getrf(DT* A, int* P, int matrixSize) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = A latency = 64 \
  num_read_outstanding = 16 num_write_outstanding = 16 \
  max_read_burst_length = 64 max_write_burst_length = 64 depth=16*16

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = P latency = 64 \
  num_read_outstanding = 16 num_write_outstanding = 16 \
  max_read_burst_length = 64 max_write_burst_length = 64 depth=16

// clang-format on
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = P bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int info;
    xf::solver::getrf<DT, N, NCU>(matrixSize, A, matrixSize, P, info);
};

#ifndef __SYNTHESIS__

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for funcion verify\n";
    }
    // Vectors for input matrix row count and column count
    // First add special cases
    std::vector<int> NR = {4, 0, 1, 2, 3, 5, 7, 36, 56, 512, 513};

    // Then add sizes 1x1, 2x2, ...
    for (int i = 1; i <= N; ++i) {
        NR.push_back(i);
    }

    bool allValid = true;

    int runNm;
    if (run_csim) {
        runNm = NR.size();
    } else {
        runNm = 1;
    }

    DT* A = new DT[N * N];
    DT* A1 = new DT[N * N];

    DT** L = new DT*[N];
    DT** U = new DT*[N];
    for (int i = 0; i < N; i++) {
        L[i] = new DT[N];
        U[i] = new DT[N];
    }
    int* P = new int[N];

    // generate matrix and test
    for (int idx = 0; idx != runNm; ++idx) {
        int kRows = NR[idx];

        // Skip some big matricies to shorten test duration
        if (kRows > 30) {
            continue;
        }

        std::cout << "Matrix Size : " << kRows << "x" << kRows << "...\n";

        // Handle invalid settings and input sizes
        if (N <= 0) {
            continue;
        }
        if (kRows <= 0 || kRows > N) {
            continue;
        }

        // L, U
        unsigned int seedL = 2;
        unsigned int seedU = 3;
        triLowerMatGen<DT>(kRows, seedL, L);
        triLowerMatGen<DT>(kRows, seedU, U);

        for (int i = 0; i < kRows; i++) {
            L[i][i] = 1.0;
        }

        // A = L * U
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kRows; j++) {
                A[i * kRows + j] = 0.0;
                for (int k = 0; k < kRows; k++) {
                    A[i * kRows + j] += L[i][k] * U[j][k];
                }
                A1[i * kRows + j] = A[i * kRows + j];
            }
        }

        // top: LU decomposition
        top_getrf(A, P, kRows);

        // compute error
        DT errsum = 0.0;
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kRows; j++) {
                double z = 0.0;
                for (int k = 0; k < kRows; k++) {
                    double x, y;
                    if (k < i)
                        x = A[i * kRows + k];
                    else if (k == i)
                        x = 1.0;
                    else
                        x = 0.0;

                    if (k <= j)
                        y = A[k * kRows + j];
                    else
                        y = 0.0;

                    z += x * y;
                }
                DT err = z - A1[P[i] * kRows + j];
                errsum += err * err;
            }
        }

        int info = 0;
        if (errsum > 1e-10) {
            std::cout << errsum << std::endl;
            info = 1;
            allValid = false;
        }

        if (info == 1) {
            std::cout << "Input matrix : " << kRows << "x" << kRows << " FAIL\n";
        }
    };

    for (int i = 0; i < N; i++) {
        delete[] L[i];
        delete[] U[i];
    }
    delete[] A;
    delete[] A1;
    delete[] L;
    delete[] U;
    delete[] P;

    std::cout << "-------------- " << std::endl;
    if ((!allValid)) {
        std::cout << "result false" << std::endl;
        std::cout << "-------------- " << std::endl;
        return -1;
    } else {
        std::cout << "result correct" << std::endl;
        std::cout << "-------------- " << std::endl;
        return 0;
    }
    std::cout << "-------------- " << std::endl;
};

#endif
