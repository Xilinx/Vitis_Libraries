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
#include "math.h"
#include "xf_fintech/rng.hpp"
#include "mcengine_top.hpp"

struct TestSuite {
    int fixings;
};

void Analytical_GP_Engine(unsigned int timeSteps,
                          TEST_DT timeLength,
                          TEST_DT volatility,
                          TEST_DT riskFreeRate,
                          TEST_DT dividendYield,
                          TEST_DT underlying,
                          TEST_DT strike,
                          bool optionType,
                          TEST_DT& priceRef) {
    // Control variate price ref
    TEST_DT fixings = timeSteps + 1;
    TEST_DT timeSum = (timeSteps + 1) * timeLength * 0.5;
    TEST_DT temp = timeSum * (timeSteps - 1) / 3.0;
    TEST_DT tempFC = 2 * temp + timeSum;
    TEST_DT sqrtFC = std::sqrt(tempFC);
    TEST_DT tempvf = volatility / fixings;

    TEST_DT variance = tempvf * tempvf * tempFC;
    TEST_DT nu = riskFreeRate - dividendYield - 0.5 * volatility * volatility;
    TEST_DT muG = std::log(underlying) + nu * timeLength * 0.5;
    TEST_DT forwardPrice = std::exp(muG + variance * 0.5);
    TEST_DT stDev = std::sqrt(variance);
    TEST_DT d1 = std::log(forwardPrice / strike) / stDev + 0.5 * stDev;
    TEST_DT d2 = d1 - stDev;
    TEST_DT cum_d1 = xf::fintech::internal::CumulativeNormal<TEST_DT>(d1);
    TEST_DT cum_d2 = xf::fintech::internal::CumulativeNormal<TEST_DT>(d2);
    TEST_DT alpha, beta;
    if (optionType) {
        alpha = -1 + cum_d1;
        beta = 1 - cum_d2;
    } else {
        alpha = cum_d1;
        beta = -cum_d2;
    }
    TEST_DT tmpExp = riskFreeRate * timeLength;
    TEST_DT discount = std::exp(-tmpExp);
    priceRef = discount * (forwardPrice * alpha + strike * beta);
};

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }

    bool optionType = 0;
    TEST_DT strike = 100;             // 87;          // 40;
    TEST_DT underlying = 100;         // 90;      // 36;
    TEST_DT riskFreeRate = 0.06;      // 0.025; // 0.06;
    TEST_DT volatility = 0.2;         // 0.13;    // 0.20;
    TEST_DT dividendYield = 0.03;     // 0.06; // 0.0;
    TEST_DT timeLength = 1.0;         // 11.0 / 12.0;
    TEST_DT requiredTolerance = 0.02; // 0.02;
    unsigned int requiredSamples = 0;
    unsigned int maxSamples = 0;
    unsigned int timeSteps = 18;

    //    TEST_DT golden = 3.28924; // for unroll = 2
    TEST_DT outputs[1];
    int UnrollNm = 4;
    ap_uint<32> seed[UnrollNm];
    for (int i = 0; i < UnrollNm; ++i) {
        seed[i] = 5000 + i * 1000;
    }

    std::cout << "Call option:\n"
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
    TestSuite tests[] = {{2}, {4}, {8}, {26}, {100}, {250}, {1000}};

    bool flag = true;
    int runNm;
    if (run_csim) {
        runNm = 7;
    } else {
        runNm = 1;
    }
    for (int i = 0; i < runNm; ++i) {
        timeSteps = tests[i].fixings - 1;

        MCAsian_Geometric_AV_Price_Engine_top(timeSteps, timeLength, strike, volatility, underlying, riskFreeRate,
                                              dividendYield, requiredSamples, maxSamples, requiredTolerance, optionType,
                                              seed, outputs);

        TEST_DT golden; // analytical results from Quantlib asianoption testsuit
                        // http://www.coggit.com/tools/arithmetic_asian_option_prices.html
        Analytical_GP_Engine(timeSteps, timeLength, volatility, riskFreeRate, dividendYield, underlying, strike,
                             optionType, golden);

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
