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
This file holds the body of the test harness for dds_mixer reference model graph
*/

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph ddsMix;

// void testRTP(unsigned int numIter, unsigned int RTPval) {
//         for(int k=0; k<P_SSR; k++){
//             ddsMix.update(ddsMix.PhaseRTP[k], ddsMix.PhaseRTP_vec[0]);
//         }
//         ddsMix.run(ddsMix.numIterRTP_vec[0]);
//     sampleDelayTestHarness.wait();
// }

int main(void) {
    printf("\n");
    printf("========================\n");
    printf("TEST.CPP STARTED\n");
    printf("UUT: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Input stimulus file");
    printf(QUOTE(INPUT_FILE));
    printf("\n");
    printf("Input stimulus file 2");
    printf(QUOTE(INPUT_FILE2));
    printf("\n");
    printf("Input samples   = %d \n", INPUT_SAMPLES);
    printf("Output samples  = %d \n", OUTPUT_SAMPLES);
    printf("Data type       = ");
    printf(QUOTE(DATA_TYPE));
    printf("\n");
    printf("TP_USE_PHASE_INC_RELOAD = %d\n", USE_PHASE_INC_RELOAD);
    printf("TEST.CPP PRINT FINISHED \n");
    printf("\n");

    ddsMix.init();

#if ((USE_PHASE_RELOAD == 1 && PHASE_RELOAD_API == USE_PHASE_RELOAD_API_RTP) || USE_PHASE_INC_RELOAD == 1)
    for (int i = 0; i < NITER; i++) {
#if (USE_PHASE_RELOAD == 1 && PHASE_RELOAD_API == USE_PHASE_RELOAD_API_RTP)
        if (i % 2 == 0) {
            for (int k = 0; k < P_SSR; k++) {
                ddsMix.update(ddsMix.PhaseRTP[k], ddsMix.PhaseRTP_vec[i]);
                // printf("Phase is = %d\n", ddsMix.PhaseRTP_vec[i]);
            }
        }
#endif
#if (USE_PHASE_INC_RELOAD == 1)
        if (i % 3 == 0) {
            for (int k = 0; k < P_SSR; k++) {
                ddsMix.update(ddsMix.PhaseIncRTP[k], ddsMix.PhaseIncRTP_vec[i]);
                // printf("Phase is = %d\n", ddsMix.PhaseRTP_vec[i]);
            }
        }
#endif
        ddsMix.run(1);
        ddsMix.wait();
    }
    ddsMix.end();
#else // no RTPS
    ddsMix.run(NITER);
    ddsMix.end();
#endif

    printf("TEST.CPP IS FINISHED \n");
    printf("========================\n");
    printf("========================\n");

    return 0;
}
