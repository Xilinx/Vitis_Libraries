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
#include "hcf_engine_top.hpp"

struct test_data_entry {
    TEST_DT s0;
    TEST_DT v0;
    TEST_DT K0;
    TEST_DT rho;
    TEST_DT vvol;
    TEST_DT vbar;
    TEST_DT T;
    TEST_DT r;
    TEST_DT kappa;
    TEST_DT exp_result;
};
/*
struct test_data_entry[] =
{
    {},
};
*/
int main() {
    struct hcfEngineInputDataType<TEST_DT> input_data;
    TEST_DT output_data;
    TEST_DT exp_result = 22.21177;
    TEST_DT tolerence = 0.00001;
    TEST_DT diff;

    // single test vector
    input_data.s0 = 100;
    input_data.v0 = 0.1;
    input_data.K = 100;
    input_data.rho = -0.7571;
    input_data.vvol = 0.2928;
    input_data.vbar = 0.0707;
    input_data.T = 3;
    input_data.r = 0.03;
    input_data.kappa = 5;
    input_data.dw = 0.5;
    input_data.w_max = 400;

    hcfEngine_top(&input_data, &output_data);
    std::cout << "Expected result = " << exp_result << std::endl;
    std::cout << "Actual result   = " << output_data << std::endl;
    diff = exp_result > output_data;
    if (diff < 0) {
        diff = -diff;
    }
    std::cout << "Difference      = " << diff << std::endl;
    if (diff <= tolerence) {
        std::cout << "PASS: difference within allowed tolerence" << std::endl;
    } else {
        std::cout << "FAIL: difference outside allowed tolerence" << std::endl;
    }

    return 0;
}
