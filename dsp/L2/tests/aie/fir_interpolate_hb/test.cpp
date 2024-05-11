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

/*
This file is the test harness for the Halfband Interpolator FIR graph class.
*/
#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph filter;

int main(void) {
    filter.init();
#if (USE_COEFF_RELOAD == 1)
    // SSR configs call asym kernels, that require asym taps
    COEFF_TYPE tapsAsym[FIR_LEN];
    COEFF_TYPE tapsCenterTap;
#if (P_PARA_INTERP_POLY > 1)
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, (FIR_LEN + 1) / 2, filter.m_taps[0]);
    tapsCenterTap = filter.m_taps[0][(FIR_LEN + 1) / 4];
    for (int i = 0; i < P_SSR; i++) {
        filter.update(filter.coeff[i], tapsAsym, (FIR_LEN + 1) / 2);
        filter.update(filter.coeffCT[i], tapsCenterTap);
    }
#else
#if (__HAS_SYM_PREADD__ == 1 || USING_UUT == 0)
    filter.update(filter.coeff[0], filter.m_taps[0], ((FIR_LEN + 1) / 4 + 1));
#else
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, (FIR_LEN + 1) / 2, filter.m_taps[0]);
    tapsCenterTap = filter.m_taps[0][(FIR_LEN + 1) / 4];
    tapsAsym[(FIR_LEN + 1) / 2] = tapsCenterTap;
    filter.update(filter.coeff[0], tapsAsym, ((FIR_LEN + 1) / 2 + 1));
#endif
#endif
    filter.run(NITER / 2);
    filter.wait();
#if (P_PARA_INTERP_POLY > 1)
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, (FIR_LEN + 1) / 2, filter.m_taps[1]);
    tapsCenterTap = filter.m_taps[1][(FIR_LEN + 1) / 4];
    for (int i = 0; i < P_SSR; i++) {
        filter.update(filter.coeff[i], tapsAsym, (FIR_LEN + 1) / 2);
        filter.update(filter.coeffCT[i], tapsCenterTap);
    }
#else
#if (__HAS_SYM_PREADD__ == 1 || USING_UUT == 0)
    filter.update(filter.coeff[0], filter.m_taps[1], ((FIR_LEN + 1) / 4 + 1));
#else
    xf::dsp::aie::convert_sym_taps_to_asym(tapsAsym, (FIR_LEN + 1) / 2, filter.m_taps[1]);
    tapsCenterTap = filter.m_taps[1][(FIR_LEN + 1) / 4];
    tapsAsym[(FIR_LEN + 1) / 2] = tapsCenterTap;
    filter.update(filter.coeff[0], tapsAsym, ((FIR_LEN + 1) / 2 + 1));
#endif
#endif
    filter.run(NITER / 2);
#else
    filter.run(NITER);
#endif
    filter.end();

    return 0;
}
