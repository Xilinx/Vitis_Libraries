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
#include "kernel_0_top.hpp"
#include "xf_fintech/rng.hpp"

void kernel_0_top(TEST_DT underlying,
                  TEST_DT volatility,
                  TEST_DT riskFreeRate,
                  TEST_DT dividendYield,
                  TEST_DT timeLength,
                  TEST_DT strike,
                  bool optionType,
                  ap_uint<32> seed[UN],
                  ap_uint<UN * sizeof(TEST_DT) * 8> pOut[depthP],
                  ap_uint<sizeof(TEST_DT) * 8> mOut[depthM],
                  unsigned int calibSamples,
                  unsigned int timeSteps) {
    xf::fintech::MCAmericanEnginePreSamples<TEST_DT, UN>(underlying, volatility, riskFreeRate, dividendYield,
                                                         timeLength, strike, optionType, seed, pOut, mOut, calibSamples,
                                                         timeSteps);

    const int order = 4;
#ifndef __SYNTHESIS__
    hls::stream<TEST_DT> matrixStrm[1];
    hls::stream<TEST_DT> priceStrm[UN];
    read_ddr<TEST_DT, 1, sizeof(TEST_DT) * 8>(timeSteps * (3 * (order - 1)), mOut, matrixStrm);
    // xf::fintech::details::read_ddr<TEST_DT, UN,
    // sizeof(TEST_DT)*8>(timeSteps*1024, pOut, priceStrm);

    const static int NM = 3 * (order - 1);
    for (int i = 0; i < timeSteps; ++i) {
        TEST_DT A[order][order];
        TEST_DT U[order][order];
        TEST_DT V[order][order];
        TEST_DT XtX[NM];
        for (int j = 0; j < NM; ++j) {
            XtX[j] = matrixStrm[0].read();
        }
        // Construct A
        for (int m = 0; m < order - 1; m++) {
            for (int n = 0; n < order - 1; n++) {
                A[m][n] = XtX[m + n];
            }
        }
        for (int m = 0; m < order; m++) {
            A[m][order - 1] = XtX[2 * (order - 1) - 1 + m];
            A[order - 1][m] = XtX[2 * (order - 1) - 1 + m];
        }
        std::cout << "A matrix" << std::endl;
        for (int m = 0; m < order; ++m) {
            for (int n = 0; n < order; ++n) {
                std::cout << A[m][n] << ", ";
            }
            std::cout << std::endl;
        }
    }
#endif
}
