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
#include <vector>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph funcApproxTestHarness;

int main(void) {
    funcApproxTestHarness.init();
#if (USE_LUT_RELOAD == 1)
    // Use arrays directly for update_rtp function
    funcApproxTestHarness.funcApproxGraph.update_rtp(funcApproxTestHarness, funcApproxTestHarness.m_luts_ab,
                                                     funcApproxTestHarness.rtpLut);
    funcApproxTestHarness.run(NITER / 2);
    funcApproxTestHarness.wait();
    funcApproxTestHarness.funcApproxGraph.update_rtp(funcApproxTestHarness, funcApproxTestHarness.m_luts_cd,
                                                     funcApproxTestHarness.rtpLut);
    funcApproxTestHarness.run(NITER / 2);
#else
    funcApproxTestHarness.run(NITER);
#endif
    funcApproxTestHarness.end();
    return 0;
}
