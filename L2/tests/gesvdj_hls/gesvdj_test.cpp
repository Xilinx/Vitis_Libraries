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
#include <math.h>
#include <vector>
#include <iostream>
#include "matrixUtility.hpp"
#endif

#define NCU 4
#define M 512
#define N M

typedef float DT;

void gesvdj_test(DT* dataA, DT* sigma, DT* dataU, DT* dataV, int matrixSize) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem0 port = dataA depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem1 port = sigma depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem2 port = dataU depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem3 port = dataV depth = 4*4

// clang-format on
#pragma HLS INTERFACE s_axilite port = dataA bundle = control
#pragma HLS INTERFACE s_axilite port = sigma bundle = control
#pragma HLS INTERFACE s_axilite port = dataU bundle = control
#pragma HLS INTERFACE s_axilite port = dataV bundle = control
#pragma HLS INTERFACE s_axilite port = matrixSize bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Calling for svd core function
    int info;
    xf::solver::gesvdj<DT, N, NCU>(matrixSize, dataA, matrixSize, sigma, dataU, matrixSize, dataV, matrixSize, info);
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

        DT* sigma_svd;
        DT* dataU_svd;
        DT* dataV_svd;
        DT* dataA_svd;
        int in_size = kRows * kRows;
        int out_size = kRows * kRows;
        int out_size_sigma = kRows * kRows;
        int out_size_U = kRows * kRows;
        int out_size_V = kRows * kRows;
        dataA_svd = new DT[in_size];
        sigma_svd = new DT[out_size_sigma];
        dataU_svd = new DT[out_size_U];
        dataV_svd = new DT[out_size_V];

        // Symmetric matrix N x N
        int seed = 12;
        symMatGen<DT>(kRows, seed, dataA_svd);

        DT* goldenData = new DT[in_size];
        for (int i = 0; i < in_size; ++i) {
            goldenData[i] = dataA_svd[i];
        }

        // Call gesvdj top function
        gesvdj_test(dataA_svd, sigma_svd, dataU_svd, dataV_svd, kRows);

        // Calculate A_out = U*sigma*VT and compare with original A matrix
        DT* dataVT_svd;
        DT* dataA_out;
        dataA_out = new DT[in_size];
        dataVT_svd = new DT[out_size_V];
        transposeMat<DT>(kRows, dataV_svd, dataVT_svd);
        MulMat(kRows, kRows, kRows, kRows, dataU_svd, sigma_svd, dataVT_svd, dataA_out);

        // Calculate err between dataA_svd and dataA_out
        DT errA = 0;
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kRows; j++) {
                errA += (goldenData[i * kRows + j] - dataA_out[i * kRows + j]) *
                        (goldenData[i * kRows + j] - dataA_out[i * kRows + j]);
            }
        }
        errA = std::sqrt(errA);
        DT threshold;
        if (sizeof(DT) == sizeof(double)) {
            threshold = 0.0001;
        } else if (sizeof(DT) == sizeof(float)) {
            threshold = 0.001;
        }
        if (errA > threshold) {
            std::cout << "Input matrix : " << kRows << "x" << kRows << " FAIL\n";
            allValid = false;
        }

        // Delete created buffers
        delete[] dataVT_svd;
        delete[] dataA_out;
        delete[] dataA_svd;
        delete[] dataU_svd;
        delete[] dataV_svd;
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
