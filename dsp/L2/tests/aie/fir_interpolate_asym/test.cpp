/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
// This file holds the body for the test harness of the Asymmetric
// Interpolation FIR graph class.

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph filter;

int main(void) {
    filter.init();
#if (USE_COEFF_RELOAD == 1)
    for (int i = 0; i < filter.RTP_SSR; i++) {
        filter.update(filter.coeff[i], filter.m_taps[0], FIR_LEN);
    }
    filter.run(NITER / 2);
    filter.wait();
    for (int i = 0; i < filter.RTP_SSR; i++) {
        filter.update(filter.coeff[i], filter.m_taps[1], FIR_LEN);
    }
    filter.run(NITER / 2);
#else
    filter.run(NITER);
#endif

    filter.end();
    return 0;
}
