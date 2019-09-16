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

#ifndef _XF_FINTECH_TEST_CASE_HPP_
#define _XF_FINTECH_TEST_CASE_HPP_

#include <string>
#include <vector>

using namespace std;

class HestonFDTestCase {
    double m_delta;

   public:
    HestonFDTestCase(double delta) { m_delta = delta; };
    int Run(string testCase, string testScheme, std::vector<double> csvTableEntry);

   private:
    bool CompareValues(double val1, double val2);
};

#endif // _XF_FINTECH_TEST_CASE_HPP_
