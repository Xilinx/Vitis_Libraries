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
#include "kernel_2_top.hpp"

void kernel_2_top(TEST_DT underlying,
                  TEST_DT volatility,
                  TEST_DT dividendYield,
                  TEST_DT riskFreeRate,
                  TEST_DT timeLength,
                  TEST_DT strike,
                  bool optionType,
                  ap_uint<32> seed[UN_K2],
                  ap_uint<8 * sizeof(TEST_DT) * COEFNM> coefIn[COEF_DEPTH],
                  TEST_DT outputs[1],
                  TEST_DT requiredTolerance,
                  unsigned int requiredSamples,
                  unsigned int timeSteps) {
    xf::fintech::MCAmericanEnginePricing(underlying, volatility, dividendYield, riskFreeRate, timeLength, strike,
                                         optionType, seed, coefIn, outputs, requiredTolerance, requiredSamples,
                                         timeSteps);
}
