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
#include "btengine_top.hpp"
using namespace xf::fintech::internal;

int main() {
    BinomialTreeInputDataType<TEST_DT> inputData;
    TEST_DT outputResult;

    inputData.S = 110;
    inputData.K = 100;
    inputData.T = 1;
    inputData.rf = 0.05;
    inputData.V = 0.2;
    inputData.q = 0;
    inputData.N = 31; // 0-31
    int optionType = BinomialTreeAmericanPut;

    binomialTreeEngine_top(&inputData, &outputResult, optionType);
    std::cout << "American Put NPV = " << outputResult << std::endl;

    optionType = BinomialTreeAmericanCall;
    binomialTreeEngine_top(&inputData, &outputResult, optionType);
    std::cout << "American Call NPV = " << outputResult << std::endl;

    optionType = BinomialTreeEuropeanPut;
    binomialTreeEngine_top(&inputData, &outputResult, optionType);
    std::cout << "European Put NPV = " << outputResult << std::endl;

    optionType = BinomialTreeEuropeanCall;
    binomialTreeEngine_top(&inputData, &outputResult, optionType);
    std::cout << "European Call NPV = " << outputResult << std::endl;

    return 0;
}
