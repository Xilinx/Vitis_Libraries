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
#include <cmath>
#include <iostream>
#include "kernel_top.hpp"

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }

    unsigned int timeSteps = 100;
    std::cout << "[INFO]Reminder: please setup correct number of seeds first" << std::endl;
    ap_uint<32> seed[UN_MAX];
    seed[0] = 1234;
    seed[1] = 3456;

    TEST_DT requiredTolerance = 0.02;
    TEST_DT underlying = 36;
    TEST_DT riskFreeRate = 0.06;
    TEST_DT volatility = 0.20;
    TEST_DT dividendYield = 0.0;
    TEST_DT strike = 40;
    bool optionType = 1;
    TEST_DT timeLength = 1;
    TEST_DT output[1];
    unsigned int calibSamples = 4096;
    unsigned int requiredSamples = 24576;
    if (!run_csim) {
        timeSteps = 10;
        calibSamples = 1024 * UN;
        requiredSamples = 1024 * UN;
    }

    ap_uint<UN * sizeof(double) * 8> pData[102400 * iteration];
    ap_uint<sizeof(double) * 8> mData[900 * iteration];

    kernel_top(underlying, volatility, riskFreeRate, dividendYield, timeLength, strike, optionType, seed, pData, mData,
               output, requiredTolerance, calibSamples, requiredSamples, timeSteps);

    std::cout << "output[0] = " << output[0] << std::endl;

    // verify the correctness of output price
    // reference value:
    //  - for 112 (UN_PATH=1, UN_STEP=1, UN_PRICING=2, seed=[1234, 3456]), output
    //  price=3.97936
    // notice that when the employed seed changes, the result also varies.
    double golden_output = 3.98;
    double diff = std::fabs(output[0] - golden_output);
    if (run_csim) {
        if (diff > 0.2) {
            std::cout << "FAILURE!!! incorrect ouput value calculated!" << std::endl;
            return -1;
        }
    } else {
        golden_output = 4.43372;
        diff = std::fabs(output[0] - golden_output);
        if (diff > 1.0e-5) {
            std::cout << "Cosim mismatch!" << std::endl;
            return -1;
        }
    }
    return 0;
}
