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
#include "mcengine_top.hpp"
void MCAsian_Arithmetic_AV_Price_Engine_top(unsigned int timeSteps,
                                            TEST_DT timeLength,
                                            TEST_DT strike,
                                            TEST_DT volatility,
                                            TEST_DT underlying,
                                            TEST_DT riskFreeRate,
                                            TEST_DT dividendYield,
                                            unsigned int requiredSamples,
                                            unsigned int maxSamples,
                                            TEST_DT requiredTolerance,
                                            bool optionType,
                                            ap_uint<32> seed[2],
                                            TEST_DT outputs[1]) {
    // Preprocess of kernel part
    xf::fintech::MCAsianArithmeticAPEngine<TEST_DT, 2>(underlying, volatility, dividendYield, riskFreeRate, timeLength,
                                                       strike, optionType, seed, outputs, requiredTolerance,
                                                       requiredSamples, timeSteps, maxSamples);
}
