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
#include "kernel_1_top.hpp"

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }

    unsigned int timeSteps = 100;
    // note the number the seed used here should be equal to the unroll number UN
    ap_uint<32> seed0[2] = {1234, 3456};
    ap_uint<32> seed2[4] = {1234, 3456, 5678, 7890};
    TEST_DT requiredTolerance = 0.02;
    TEST_DT underlying = 36;
    TEST_DT riskFreeRate = 0.06;
    TEST_DT volatility = 0.20;
    TEST_DT dividendYield = 0.0;
    TEST_DT strike = 40;
    bool optionType = 1;
    TEST_DT timeLength = 1;

    unsigned int requiredSamples = 24576;
    unsigned int calibSamples = 4096;
    if (!run_csim) {
        timeSteps = 10;
        calibSamples = UN * 1024;
        requiredSamples = UN * 1024;
    }

    ap_uint<UN * sizeof(double) * 8> pOut[102400 * iteration];
    ap_uint<sizeof(double) * 8> mOut[900 * iteration];
    ap_uint<sizeof(double) * 8 * 4> coefOut[1024];
    TEST_DT outputs[1];

    xf::fintech::MCAmericanEnginePreSamples<TEST_DT, 2>(underlying, volatility, riskFreeRate, dividendYield, timeLength,
                                                        strike, optionType, seed0, pOut, mOut, calibSamples, timeSteps);

    // xf::fintech::MCAmericanEngineCalibrate<double, 2, 2>(timeLength, riskFreeRate, strike, optionType, pOut, mOut,
    //                                                     coefOut, calibSamples, timeSteps);

    kernel_1_top(timeLength, riskFreeRate, strike, optionType, pOut, mOut, coefOut, calibSamples, timeSteps);

    xf::fintech::MCAmericanEnginePricing(underlying, volatility, dividendYield, riskFreeRate, timeLength, strike,
                                         optionType, seed2, coefOut, outputs, requiredTolerance, requiredSamples,
                                         timeSteps);

    std::cout << "output =" << outputs[0] << std::endl;

    // verify the output price with golden
    // reference value:
    //  - for 222 (UN_PATH=2, UN_STEP=2, UN_PRICING=2), 3.97471
    // notice that when the employed seed changes, the result also varies.
    double golden_output = 3.97;
    double diff = std::fabs(outputs[0] - golden_output);
    if (run_csim) {
        if (diff > 0.2) {
            std::cout << "FAILURE!!! incorrect ouput value calculated!" << std::endl;
            return -1;
        }
    } else {
        golden_output = 4.51693;
        diff = std::fabs(outputs[0] - golden_output);
        if (diff > 0.2) {
            std::cout << "FAILURE!!! incorrect ouput value calculated!" << std::endl;
            return -1;
        }
    }
    return 0;
}
