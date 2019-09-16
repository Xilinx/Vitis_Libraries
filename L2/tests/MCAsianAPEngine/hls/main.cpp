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

    bool optionType = 1;
    TEST_DT strike = 87;
    TEST_DT underlying = 90;
    TEST_DT riskFreeRate = 0.025;
    TEST_DT volatility = 0.13;
    TEST_DT dividendYield = 0.06;
    TEST_DT timeLength = 11.0 / 12.0;
    TEST_DT requiredTolerance = 0.02;
    unsigned int requiredSamples = 0;
    unsigned int maxSamples = 0;
    unsigned int timeSteps;
    TEST_DT outputs[1];
    int UnrollNm = 2;
    ap_uint<32> seed[UnrollNm];
    for (int i = 0; i < UnrollNm; ++i) {
        seed[i] = 54000 + i * 1000;
    }

    std::cout << "Put option:\n"
              << "   strike:              " << strike << "\n"
              << "   underlying:          " << underlying << "\n"
              << "   risk-free rate:      " << riskFreeRate << "\n"
              << "   volatility:          " << volatility << "\n"
              << "   dividend yield:      " << dividendYield << "\n"
              << "   maturity:            " << timeLength << "\n"
              << "   tolerance:           " << requiredTolerance << "\n"
              << "   requaried samples:   " << requiredSamples << "\n"
              << "   maximum samples:     " << maxSamples << "\n";

    // Test suite
    TestSuite tests[] = {{2, 1.3942835683},   {4, 1.5852442983},   {8, 1.66970673},     {12, 1.6980019214},
                         {26, 1.7255070456},  {52, 1.7401553533},  {100, 1.7478303712}, {250, 1.7490291943},
                         {500, 1.7515113291}, {1000, 1.7537344885}};

    // Call engine
    bool flag = true;
    int runNm;
    if (run_csim) {
        runNm = 1;
    } else {
        runNm = 10;
    }
    for (int i = 0; i < runNm; ++i) {
        timeSteps = tests[i].fixings - 1;

        MCAsian_Arithmetic_AV_Price_Engine_top(timeSteps, timeLength, strike, volatility, underlying, riskFreeRate,
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
