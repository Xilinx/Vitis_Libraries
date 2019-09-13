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

#ifndef __SYNTHESIS__
#include "matrixUtility.hpp"
#include <iostream>
#include <vector>
#endif

#include "xf_solver_L2.hpp"

#define M 512
#define N 512
#define NCU 16
#define DT double

void test(DT data[M * N], int m, int n, int lda, DT tau[N]) {
    xf::solver::geqrf<DT, M, N, NCU>(m, n, data, lda, tau);
}

#ifndef __SYNTHESIS__

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for funcion verify\n";
    }

    // Vectors for input matrix row count and column count
    // First add special cases
    std::vector<int> NR = {4, 0, 1, 0, 5, 1, 3, 36, 56, 516, 516};
    std::vector<int> NC = {4, 0, 1, 5, 0, 3, 1, 37, 1, 512, 516};

    // Then add sizes 1x1, 2x2, ...
    for (int i = 1; i <= std::min(M, N); ++i) {
        NR.push_back(i);
        NC.push_back(i);
    }

    bool allValid = true;

    int runNm;
    if (run_csim) {
        runNm = NR.size();
    } else {
        runNm = 1;
    }
    // generate matrix and test
    for (int idx = 0; idx != runNm; ++idx) {
        int kRows = NR[idx];
        int kCols = NC[idx];

        // Skip some big matricies to shorten test duration
        if (kRows > 10 && kCols > 10) {
            continue;
        }
        std::cout << "Matrix Size : " << kRows << "x" << kCols << "...\n";

        // Handle invalid settings and input sizes
        if (M <= 0 || N <= 0 || NCU <= 0) {
            continue;
        }
        if (kRows <= 0 || kRows > M) {
            continue;
        }
        if (kCols <= 0 || kCols > N) {
            // return -3;
            continue;
        }

        DT* dataA = new DT[kRows * kCols];
        matGen<DT>(kRows, kCols, 1, dataA);

        DT data1D[M * N];
        DT* goldenData = new DT[kRows * kCols];
        for (int i = 0; i < kRows; ++i) {
            for (int j = 0; j < kCols; ++j) {
                int idx = i * kCols + j;
                goldenData[idx] = dataA[idx];
                data1D[idx] = dataA[idx];
            }
        }

        DT tau[N];
        int lda = kCols;

        test(data1D, kRows, kCols, lda, tau);

        for (int i = 0; i < kRows; ++i) {
            for (int j = 0; j < kCols; ++j) {
                int idx = i * kCols + j;
                dataA[idx] = data1D[idx];
            }
        }

        // get Q and R, compare Q*R with A
        DT* Q = new DT[kRows * kRows];
        constructQ<DT>(dataA, tau, kRows, kCols, Q);

        convertToRInline<DT>(dataA, kRows, kCols);

        DT* A = new DT[kRows * kCols];
        matrixMult<DT>(Q, kRows, kRows, dataA, kRows, kCols, A);
        bool equal = compareMatrices<DT>(A, goldenData, kRows, kCols, kCols);

        if (!equal) {
            std::cout << "Input matrix " << kRows << "x" << kCols << " FAIL\n";
            allValid = false;
        }

        delete[] Q;
        delete[] A;

        delete[] dataA;
        delete[] goldenData;
    }

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
}
#endif
