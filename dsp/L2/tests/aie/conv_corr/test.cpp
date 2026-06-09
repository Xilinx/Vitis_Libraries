/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
    This file holds the body of the test harness for the conv_corr graph class.
*/

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph conv_corr_TestHarness;

int main(void) {
    conv_corr_TestHarness.init();

#if (USE_RTP_VECTOR_LENGTHS == 1) // RTP

    // Fix rand() seed so UUT and reference generate identical RTP length sequences.
    // Without this, any divergence in rand() state between the two executables causes
    // getRtpFLen/getRtpGLen to produce different lengths, making the diff comparison invalid.
    srand(SEED_RTP);

    // First half: iter 0 uses minimum lengths; subsequent iters use varied non-power-of-2 lengths
    for (unsigned int iter = 0; iter < (NITER / 2); iter++) {
        unsigned int rtpFLen, rtpGLen;
        if (iter == 0) {
            rtpFLen = getMinLen<DATA_F>(); // min valid F: 2 vector loads
            rtpGLen = getMinLen<DATA_G>(); // min valid G: 2 vector loads
            if (rtpFLen < rtpGLen) {
                rtpFLen = rtpGLen; // mixed types: set F to meet F >= G
            }
        } else {
            constexpr unsigned int minFLen = getMinLen<DATA_F>();
            rtpFLen = getRtpFLen<DATA_F, REF_F_LEN>();
            rtpGLen = getRtpGLen<DATA_G, G_LEN>();
            if (rtpFLen < rtpGLen) {
                // set F to the smallest valid multiple of minFLen that satisfies F >= G.
                // Avoids collapsing to compile-time maxima on every mixed-type iteration.
                rtpFLen = ((rtpGLen + minFLen - 1u) / minFLen) * minFLen; // smallest valid F >= G
                if (rtpFLen > REF_F_LEN) {
                    rtpFLen = REF_F_LEN;
                    rtpGLen = G_LEN;
                } // G exceeds max F: use compile-time max
            }
        }

        conv_corr_TestHarness.conv_corrGraph.update_rtp(conv_corr_TestHarness, conv_corr_TestHarness.rtpVecLen, rtpFLen,
                                                        rtpGLen);
        conv_corr_TestHarness.run(1);
        conv_corr_TestHarness.wait();
    }

    conv_corr_TestHarness.conv_corrGraph.update_rtp(conv_corr_TestHarness, conv_corr_TestHarness.rtpVecLen, REF_F_LEN,
                                                    G_LEN);
    conv_corr_TestHarness.run(NITER / 2);
    conv_corr_TestHarness.wait();

#else // NON RTP
#ifdef USING_UUT
    conv_corr_TestHarness.run(NITER);
#else
#if (API_IO == 1)
    conv_corr_TestHarness.run(1);
#else
    conv_corr_TestHarness.run(NITER);
#endif
#endif
#endif // END of NON RTP

    conv_corr_TestHarness.end();

    return 0;
}
