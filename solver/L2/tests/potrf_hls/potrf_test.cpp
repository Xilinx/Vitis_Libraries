/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENASE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANATIES OR CONADITIONAS OF ANAY KINAD, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "xf_solver_L2.hpp"

#define NCU 2
#define N 512
#define M N

#ifndef __SYNTHESIS__
#include "matrixUtility.hpp"
#include <iostream>
#endif

#define DT double

void top_potrf(DT* A, int matrixSize) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem0 port = A depth = 4*4

// clang-format on
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = matrixSize bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    int info;
    xf::solver::potrf<DT, N, NCU>(matrixSize, A, matrixSize, info);
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
    std::vector<int> NR = {4, 0, 1, 2, 3, 5, 7, 36, 56, 512, 513};

    // Then add sizes 1x1, 2x2, ...
    for (int i = 1; i <= std::min(M, N); ++i) {
        NR.push_back(i);
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

        // Skip some big matricies to shorten test duration
        if (kRows > 30) {
            continue;
        }
        std::cout << "Matrix Size : " << kRows << "x" << kRows << "...\n";

        // Handle invalid settings and input sizes
        if (M <= 0 || N <= 0) {
            continue;
        }
        if (kRows <= 0 || kRows > M || kRows > N) {
            continue;
        }

        int seed = 12;
        DT* dataA = new DT[kRows * kRows];
        DT** goldenData = new DT*[kRows];
        DT** dataB = new DT*[kRows];
        DT** dataC = new DT*[kRows];
        for (int i = 0; i < kRows; ++i) {
            goldenData[i] = new DT[kRows];
            dataB[i] = new DT[kRows];
            dataC[i] = new DT[kRows];
        }

        triLowerMatGen<DT>(kRows, seed, goldenData);
        transposeMat<DT>(kRows, goldenData, dataB);
        MulMat<DT>(kRows, kRows, kRows, goldenData, dataB, dataC);

        int k = 0;
        for (int i = 0; i < kRows; ++i) {
            for (int j = 0; j < kRows; ++j) {
                dataA[i * kRows + j] = dataC[i][j];
            }
        }

        top_potrf(dataA, kRows);

        DT errA = 0;
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j <= i; j++) {
                errA += (dataA[i * kRows + j] - goldenData[i][j]) * (dataA[i * kRows + j] - goldenData[i][j]);
            }
        }
        errA = std::sqrt(errA);

        DT threshold;
        if (sizeof(DT) == sizeof(double)) {
            threshold = 0.0001;
        } else if (sizeof(DT) == sizeof(float)) {
            threshold = 0.01;
        }
        if (errA > threshold) {
            std::cout << "Input matrix : " << kRows << "x" << kRows << " FAIL\n";
            allValid = false;
        }

        delete[] dataA;
        for (int i = 0; i < kRows; ++i) {
            delete[] goldenData[i];
            delete[] dataB[i];
            delete[] dataC[i];
        }
        delete[] goldenData;
        delete[] dataB;
        delete[] dataC;
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
