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
#include <iostream>
#include "matrixUtility.hpp"
#endif

#define NCU 1
#define M 10
#define N M

typedef double DT;

void syevj_test(DT* dataA, DT* sigma, DT* dataU, DT* dataV, int matrixSize) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem0 port = dataA depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem1 port = sigma depth = 4
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

    // Calling for evj core function
    int info;
    xf::solver::syevj<DT, N, NCU>(matrixSize, dataA, matrixSize, sigma, dataU, matrixSize, info);
    for (int i = 0; i < matrixSize; ++i) {
        for (int j = 0; j < matrixSize; ++j) {
            dataV[i * matrixSize + j] = dataU[i * matrixSize + j];
        }
    }
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
    std::vector<int> NR = {4, 2, 3, 5, 7};

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
            // return -1;
            continue;
        }
        if (kRows <= 0 || kRows > M || kRows > N) {
            // return -2;
            continue;
        }

        DT* sigma_evj;
        DT* dataU_evj;
        DT* dataV_evj;
        DT* dataA_evj;
        int in_size = kRows * kRows;
        int out_size = kRows * kRows;
        int out_size_sigma = kRows * kRows;
        int out_size_U = kRows * kRows;
        int out_size_V = kRows * kRows;
        dataA_evj = new DT[in_size];
        sigma_evj = new DT[out_size_sigma];
        dataU_evj = new DT[out_size_U];
        dataV_evj = new DT[out_size_V];

        // Symmetric matrix N x N
        int seed = 12;
        symMatGen<DT>(kRows, seed, dataA_evj);

        DT* goldenData = new DT[in_size];
        for (int i = 0; i < in_size; ++i) {
            goldenData[i] = dataA_evj[i];
        }

        // Call geevjj top function
        syevj_test(dataA_evj, sigma_evj, dataU_evj, dataV_evj, kRows);

        // Calculate A_out = U*sigma*VT and compare with original A matrix
        DT* dataVT_evj;
        DT* dataA_out;
        DT* sigma_out;
        dataA_out = new DT[in_size];
        sigma_out = new DT[in_size];
        dataVT_evj = new DT[out_size_V];
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kRows; j++) {
                if (i == j) {
                    sigma_out[i * kRows + j] = sigma_evj[i];
                } else {
                    sigma_out[i * kRows + j] = 0;
                }
            }
        }
        transposeMat<DT>(kRows, dataV_evj, dataVT_evj);
        MulMat(kRows, kRows, kRows, kRows, dataU_evj, sigma_out, dataVT_evj, dataA_out);

        // Calculate err between dataA_evj and dataA_out
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
        delete[] dataVT_evj;
        delete[] dataA_out;
        delete[] dataA_evj;
        delete[] dataU_evj;
        delete[] dataV_evj;
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
