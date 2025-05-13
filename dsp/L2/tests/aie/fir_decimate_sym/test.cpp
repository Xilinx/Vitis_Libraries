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
This file holds the body of the test harness for symmetric decimator FIR filter
reference model graph
*/

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph filter;

int main(void) {
    filter.init();
#if (USE_COEFF_RELOAD == 1)
    // SSR configs call asym kernels, that require asym taps
    COEFF_TYPE tapsAsym[FIR_LEN];
#if ((P_SSR > 1) || (USING_UUT == 1 && __HAS_SYM_PREADD__ == 0))
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, FIR_LEN, filter.m_taps[0]);
    for (int i = 0; i < P_SSR; i++) {
        filter.update(filter.coeff[i], tapsAsym, FIR_LEN);
    }
#else
    filter.update(filter.coeff[0], filter.m_taps[0], (FIR_LEN + 1) / 2);
#endif
    filter.run(NITER / 2);
    filter.wait();
#if ((P_SSR > 1) || (USING_UUT == 1 && __HAS_SYM_PREADD__ == 0))
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, FIR_LEN, filter.m_taps[1]);
    for (int i = 0; i < P_SSR; i++) {
        filter.update(filter.coeff[i], tapsAsym, FIR_LEN);
    }
#else
    filter.update(filter.coeff[0], filter.m_taps[1], (FIR_LEN + 1) / 2);
#endif

    filter.run(NITER / 2);
#else
    filter.run(NITER);
#endif

    filter.end();

    return 0;
}
