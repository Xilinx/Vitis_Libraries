/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
/*
This file holds the body of the test harness for the function approximation
reference model graph
*/

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph funcApproxTestHarness;

int main(void) {
    funcApproxTestHarness.init();
    funcApproxTestHarness.run(NITER);
    funcApproxTestHarness.end();

    return 0;
}
