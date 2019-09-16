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
#include "kernel_1_top.hpp"
#include "xf_fintech/rng.hpp"

void kernel_1_top(TEST_DT timeLength,
                  TEST_DT riskFreeRate,
                  TEST_DT strike,
                  bool optionType,
                  ap_uint<8 * sizeof(TEST_DT) * UN> priceIn[depthP],
                  ap_uint<8 * sizeof(TEST_DT)> matIn[depthM],
                  ap_uint<8 * sizeof(TEST_DT) * COEFNM> coefOut[COEF_DEPTH],
                  unsigned int calibSamples,
                  unsigned int timeSteps) {
    xf::fintech::MCAmericanEngineCalibrate(timeLength, riskFreeRate, strike, optionType, priceIn, matIn, coefOut,
                                           calibSamples, timeSteps);
}
