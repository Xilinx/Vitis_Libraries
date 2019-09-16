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
#include "m76_engine_top.h"

int main() {
    int ret;
    TEST_DT call;
    struct jump_diffusion_params<TEST_DT> p;
    p.S = 100;
    p.K = 100;
    p.r = 0.05;
    p.sigma = 0.2;
    p.T = 1;
    p.lambda = 1;
    p.kappa = 0.083287;
    p.delta = 0.4;
    TEST_DT expected_result = 18.7336;
    TEST_DT tolerance = 0.0001;

    call = m76_engine_top(&p);

    std::cout << "Expected result = " << expected_result << std::endl;
    std::cout << "Actual result   = " << call << std::endl;
    TEST_DT diff = expected_result - call;
    if (diff < 0) {
        diff = -diff;
    }
    std::cout << "Difference      = " << diff << std::endl;
    if (diff <= tolerance) {
        std::cout << "PASS: difference within allowed tolerence" << std::endl;
        ret = 0;
    } else {
        std::cout << "FAIL: difference outside allowed tolerence" << std::endl;
        ret = 1;
    }

    return ret;
}
