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
#include <iostream>
#include "mcengine_top.hpp"

struct TestSuite {
    int fixings;
    double result;
};

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }

    bool optionType = 0;
    TEST_DT strike = 87;          // 40;
    TEST_DT underlying = 90;      // 36;
    TEST_DT riskFreeRate = 0.025; // 0.06;
    TEST_DT volatility = 0.13;    // 0.20;
    TEST_DT dividendYield = 0.06; // 0.0;
    TEST_DT timeLength = 11.0 / 12.0;
    TEST_DT requiredTolerance = 0.02; // 0.02;
    unsigned int requiredSamples = 0;
    unsigned int maxSamples = 0;
    unsigned int timeSteps = 12;
    //    TEST_DT golden = 3.10669; //
    TEST_DT outputs[1];
    int UnrollNm = 2;
    ap_uint<32> seed[UnrollNm];
    for (int i = 0; i < UnrollNm; ++i) {
        seed[i] = 10 + i * 1000;
    }

    std::cout << "Call option:\n"
              << "   strike:              " << strike << "\n"
              << "   underlying:          " << underlying << "\n"
              << "   risk-free rate:      " << riskFreeRate << "\n"
              << "   volatility:          " << volatility << "\n"
              << "   dividend yield:      " << dividendYield << "\n"
              << "   maturity:            " << timeLength << "\n"
              << "   tolerance:           " << requiredTolerance << "\n"
              << "   requaried samples:   " << requiredSamples << "\n";
    //              << "   maximum samples:     " << maxSamples << "\n"
    //              << "   timesteps:           " << timeSteps << "\n"
    //              << "   golden:              " << golden << "\n";

    // Test suite
    TestSuite tests[] = {{2, 1.51917595129},   {4, 1.67940165674},   {12, 1.77595318693},  {26, 1.81430536630},
                         {100, 1.83822402464}, {250, 1.83875059026}, {1000, 1.83887181884}};

    // Call engine
    bool flag = true;
    int runNm;
    if (run_csim) {
        runNm = 7;
    } else {
        runNm = 1;
    }
    for (int i = 0; i < runNm; ++i) {
        timeSteps = tests[i].fixings - 1;

        MCAsian_Arithmetic_AV_Strike_Engine_top(timeSteps, timeLength, strike, volatility, underlying, riskFreeRate,
                                                dividendYield, requiredSamples, maxSamples, requiredTolerance,
                                                optionType, seed, outputs);
        TEST_DT golden = tests[i].result; // analytical results from Quantlib asianoption testsuit
                                          // http://www.coggit.com/tools/arithmetic_asian_option_prices.html
        std::cout << "fixings = " << tests[i].fixings << "\tcalculated value is " << outputs[0]
                  << "\ttheoretical value is " << golden << std::endl;

        // compare the result
        TEST_DT diff = std::fabs(outputs[0] - golden);
        if (diff > requiredTolerance) {
            std::cout << "Output is wrong!" << std::endl;
            std::cout << "Acutal value: " << outputs[0] << ", Expected value: " << golden << std::endl;
            flag = false;
        }
    }

    std::cout << "The results are all correct" << std::endl;
    if (flag) {
        return 0;
    } else {
        return -1;
    }
}
