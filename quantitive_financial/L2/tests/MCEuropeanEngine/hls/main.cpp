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

#define LENGTH(a) (sizeof(a) / sizeof(a[0]))
int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for function verify\n";
    }

    bool optionTypes[] = {false, true};
    TEST_DT strikes[] = {75.0, 100.0, 125.0};
    TEST_DT underlyings[] = {100};
    TEST_DT riskFreeRates[] = {0.01, 0.05, 0.15};
    TEST_DT volatilitys[] = {0.11, 0.50, 1.20};
    TEST_DT dividendYields[] = {0.00, 0.05};

    TEST_DT timeLength = 1;
    TEST_DT requiredTolerance = 0.02;

    unsigned int requiredSamples = 40000;
    TEST_DT relative_err = 0.01;
    unsigned int maxSamples = 0;
    unsigned int timeSteps = 1;

    TEST_DT goldens[] = {25.7561,   32.9468, 53.2671, 28.6606,    34.839,  54.3405, 35.447,      39.5876, 56.9929,
                         20.9081,   29.1095, 49.3851, 23.7934,    30.8919, 50.4131, 30.5703,     35.3988, 52.9576,
                         4.87984,   20.1444, 45.4236, 7.15178,    21.7926, 46.5208, 14.334,      26.1325, 49.2591,
                         2.59445,   17.2806, 41.9043, 4.17224,    18.7785, 42.9474, 10.0343,     22.7598, 45.5556,
                         0.122447,  12.135,  39.3347, 0.295976,   13.4076, 40.4184, 1.72776,     16.9103, 43.1452,
                         0.0331968, 10.1321, 36.1351, 0.0918666,  11.2518, 37.1594, 0.733906,    14.3665, 39.7416,
                         0.009834,  7.20058, 27.5209, 0.00276481, 6.18119, 25.6827, 6.92225e-05, 4.14074, 21.546,
                         0.0389132, 8.24025, 28.5159, 0.0126175,  7.11112, 26.6323, 0.000456755, 4.82895, 22.3877,
                         3.88483,   19.1494, 44.4286, 2.27473,    16.9155, 41.6437, 0.404771,    12.2033, 35.3299,
                         6.47649,   21.1627, 45.7864, 4.17224,    18.7785, 42.9474, 0.982153,    13.7077, 36.5034,
                         23.8787,   35.8912, 63.0909, 19.1997,    32.3113, 59.322,  9.13635,     24.4988, 50.7337,
                         28.6665,   38.7653, 64.7684, 23.8726,    35.0325, 60.9401, 13.1995,     26.8321, 52.2071};

    TEST_DT outputs[1];
    ap_uint<32> seeds[2];
    seeds[0] = 1;
    seeds[1] = 10001;

    int idx = 0;
    int opt_len, st_len, unly_len, r_len, d_len, vol_len;
    if (run_csim) {
        opt_len = LENGTH(optionTypes);
        st_len = LENGTH(strikes);
        unly_len = LENGTH(underlyings);
        r_len = LENGTH(riskFreeRates);
        d_len = LENGTH(dividendYields);
        vol_len = LENGTH(volatilitys);
    } else {
        opt_len = 1;
        st_len = 1;
        unly_len = 1;
        r_len = 1;
        d_len = 1;
        vol_len = 1;
    }
    for (int i = 0; i < opt_len; ++i) {
        for (int j = 0; j < st_len; ++j) {
            for (int k = 0; k < 1; ++k) {
                for (int l = 0; l < unly_len; ++l) {
                    for (int m = 0; m < d_len; ++m) {
                        for (int n = 0; n < r_len; ++n) {
                            for (int p = 0; p < vol_len; ++p) {
                                bool optionType = optionTypes[i];
                                TEST_DT strike = strikes[j];
                                TEST_DT underlying = underlyings[l];
                                TEST_DT dividendYield = dividendYields[m];
                                TEST_DT riskFreeRate = riskFreeRates[n];
                                TEST_DT volatility = volatilitys[p];

                                MCEuropeanEngine_top(underlying, volatility, dividendYield,
                                                     riskFreeRate, // model parameter
                                                     timeLength, strike,
                                                     optionType, // option parameter
                                                     seeds, outputs, requiredTolerance, requiredSamples, timeSteps);

                                TEST_DT diff = std::fabs(outputs[0] - goldens[idx]) / underlying;
                                // comapre with golden result
                                if (diff > relative_err) {
                                    std::cout << "Output is wrong!" << std::endl;
                                    if (optionType)
                                        std::cout << "Put option:\n";
                                    else
                                        std::cout << "Call option:\n";
                                    std::cout << "   strike:              " << strike << "\n"
                                              << "   underlying:          " << underlying << "\n"
                                              << "   risk-free rate:      " << riskFreeRate << "\n"
                                              << "   volatility:          " << volatility << "\n"
                                              << "   dividend yield:      " << dividendYield << "\n"
                                              << "   maturity:            " << timeLength << "\n"
                                              << "   tolerance:           " << requiredTolerance << "\n"
                                              << "   requaried samples:   " << requiredSamples << "\n"
                                              << "   maximum samples:     " << maxSamples << "\n"
                                              << "   timesteps:           " << timeSteps << "\n"
                                              << "   golden:              " << goldens[idx] << "\n";
                                    std::cout << "Acutal value: " << outputs[0] << ", Expected value: " << goldens[idx]
                                              << std::endl;
                                    std::cout << "error: " << diff << ", tolerance: " << relative_err << std::endl;
                                    return -1;
                                }
                                idx++;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
