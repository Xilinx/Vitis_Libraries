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
This file holds the body of the test harness for the sample delay reference model graph
*/

#include <stdio.h>
#include "test.hpp"
#include "sample_delay.hpp"

xf::dsp::aie::testcase::test_graph sampleDelayTestHarness;

void testDelay(unsigned int numIter, unsigned int delayVal) {
    sampleDelayTestHarness.update(sampleDelayTestHarness.numSampleDelay_rtp, delayVal);
    sampleDelayTestHarness.run(numIter);
    sampleDelayTestHarness.wait();
}

int main(void) {
    static constexpr unsigned int vecSize = 256 / 8 / sizeof(DATA_TYPE);
    unsigned int delayVal = 0;
    unsigned int totalDelay = 0;
    bool allTests = false;
    if (ALL_TESTS == 0) { // used for development/debugging
        sampleDelayTestHarness.init();
        delayVal = DELAY_INIT_VALUE;
        testDelay(NITER / 2, delayVal);
        delayVal = 10;
        testDelay(NITER / 2, delayVal);
        sampleDelayTestHarness.end();
    } else { // testing
        /*
        <iter # 1-8: Delay is an integer multiple of vector size>
        iter # 1 : DELAY_INIT_VALUE (run for both zero and non-zero DELAY_INIT_VALUE)
        iter # 2 : no delay
        iter # 3 : no delay
        iter # 4 : delay
        iter # 5 : MAX_DELAY
        iter # 6 : no delay
        iter # 7 : advance
        iter # 8 : no delay
        < iter # 9-16: Delay is NOT an integer multiple of vector size >
        iter # 9 : DELAY_INIT_VALUE
        iter # 10 : no delay
        iter # 11 : delay
        iter # 12 : request delay
        iter # 13 : delay
        iter # 14 : no delay
        iter # 15 : advance
        iter # 16 : no delay
        */

        sampleDelayTestHarness.init();

        delayVal = DELAY_INIT_VALUE;
        testDelay(1, delayVal); // iter # 1
        testDelay(2, delayVal); // iter # 2,3
        delayVal = 2 * vecSize;
        testDelay(1, delayVal); // iter # 4
        delayVal = 3 * vecSize;
        testDelay(1, delayVal); // iter # 5
        testDelay(1, delayVal); // iter # 6
        delayVal = 10;
        testDelay(2, delayVal); // iter # 7, 8
        delayVal = 10;
        testDelay(1, delayVal); // iter # 9
        testDelay(2, delayVal); // iter # 10,11
        delayVal = vecSize + 2;
        testDelay(1, delayVal); // iter # 12
        delayVal = vecSize + 3;
        testDelay(1, delayVal); // iter # 13
        testDelay(1, delayVal); // iter # 14
        delayVal = vecSize;
        testDelay(2, delayVal); // iter # 15, 16

        sampleDelayTestHarness.end();
    }
    return 0;
}
