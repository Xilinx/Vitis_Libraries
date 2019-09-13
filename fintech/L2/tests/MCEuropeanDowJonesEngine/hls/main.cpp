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
#include "mcengine_top.hpp"

#define NUM_ASSETS 30
#define DIA_DIVISOR 0.14748071991788

int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }
    unsigned int i = 0;
    ap_uint<32> seeds[2] = {1, 10001};

    // input parameters (per asset)
    TEST_DT underlying[NUM_ASSETS] = {
        163.69, // MMM
        117.38, // AXP
        182.06, // AAPL
        347.82, // BA
        122.38, // CAT
        116.65, // CVX
        54.00,  // CSCO
        50.64,  // KO
        135.32, // DIS
        49.53,  // DOW
        72.77,  // XOM
        187.86, // GS
        194.16, // HD
        131.49, // IBM
        44.27,  // INTC
        134.14, // JNJ
        109.09, // JPM
        199.87, // MCD
        81.94,  // MRK
        124.86, // MSFT
        82.31,  // NKE
        42.67,  // PFE
        106.31, // PG
        148.56, // TRV
        130.23, // UTX
        244.16, // UNH
        57.34,  // VZ
        163.21, // V
        103.97, // WMT
        50.81   // WBA
    };

    // common
    bool optionType = 1;
    TEST_DT strike = 0.0;
    TEST_DT riskFreeRate = 0.03; // BG_TODO: change to per asset?
    TEST_DT volatility = 0.20;   // BG_TODO: change to per asset?
    TEST_DT dividendYield = 0.0; // BG_TODO: change to per asset?
    TEST_DT timeLength = 1.0;
    TEST_DT requiredTolerance = 0.02;
    unsigned int requiredSamples = 1024;
    unsigned int maxSamples = 0;
    unsigned int timeSteps = 1;

    // outputs
    TEST_DT outputs[1] = {};
    TEST_DT optionValue[NUM_ASSETS] = {};
    TEST_DT optionValueSum = 0;
    TEST_DT optionValueDIA = 0;

    std::cout << "COMMON" << std::endl
              << "  strike:           " << strike << std::endl
              << "  maturity:         " << timeLength << std::endl
              << "  tolerance:        " << requiredTolerance << std::endl
              << "  required samples: " << requiredSamples << std::endl
              << "  maximum samples:  " << maxSamples << std::endl
              << "  timesteps:        " << timeSteps << std::endl
              << std::endl;
    int num;
    if (run_csim)
        num = NUM_ASSETS;
    else
        num = 1;
    for (int i = 0; i < num; i++) {
        MCEuropeanPriBypassEngine_top(underlying[i], volatility, dividendYield, riskFreeRate, timeLength, strike,
                                      optionType, seeds, outputs, requiredTolerance, requiredSamples, timeSteps);

        optionValue[i] = outputs[0];
        optionValueSum += optionValue[i];

        std::cout << "ASSET[" << i << "]:" << std::endl
                  << "  underlying:     " << underlying[i] << std::endl
                  << "  risk-free rate: " << riskFreeRate << std::endl
                  << "  volatility:     " << volatility << std::endl
                  << "  dividend yield: " << dividendYield << std::endl
                  << "  --              " << std::endl
                  << "  option value:   " << optionValue[i] << std::endl
                  << std::endl;
    }

    optionValueDIA = (optionValueSum / DIA_DIVISOR / 100);

    std::cout << "DIA:" << std::endl;
    std::cout << "  option value: " << optionValueDIA << std::endl << std::endl;

    std::cout << "strike \tcall \tput" << std::endl;
    std::cout << "------ \t---- \t---" << std::endl;
    for (strike = 250.0; strike <= 275.0; strike += 5.0) {
        TEST_DT payoff_put, payoff_call;

        payoff_put = MAX((strike - optionValueDIA), 0);
        payoff_call = MAX((optionValueDIA - strike), 0);

        std::cout << strike << "\t" << payoff_call << "\t" << payoff_put << std::endl;
    }

    return 0;
}
