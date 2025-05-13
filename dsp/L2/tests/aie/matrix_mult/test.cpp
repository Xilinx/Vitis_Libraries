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
// This file holds the body for the test harness of the matrix mult graph class

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph matMult;

// #ifdef USING_PL_MOVER
//     //need explicit port name, so we can connect in hw.
//     PLIO* in1 = new PLIO("DataIn1", adf::plio_32_bits, QUOTE(INPUT_FILE_A));
//     PLIO* in2 = new PLIO("DataIn2", adf::plio_32_bits, QUOTE(INPUT_FILE_B));
//     PLIO* out1 = new PLIO("DataOut1", adf::plio_32_bits, QUOTE(OUTPUT_FILE));

//     simulation::platform<2, 1> platform(in1, in2, out1);
//     connect<> net0A(platform.src[0], matMult.inA[0]);
//     connect<> net0B(platform.src[1], matMult.inB[0]);
//     connect<> outNet(matMult.out, platform.sink[0]);
// #endif

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    printf("\n");
    // printf("%s %s\n", INPUT_FILES_A );
    printf("========================\n");
    printf("UUT: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Input samples   = %d * %d \n", P_INPUT_SAMPLES_A, P_INPUT_SAMPLES_B);
    printf("Output samples  = %d \n", P_OUTPUT_SAMPLES);
    printf("DIM_A        = %d \n", P_DIM_A);
    printf("DIM_AB       = %d \n", P_DIM_AB);
    printf("DIM_B        = %d \n", P_DIM_B);
    printf("Shift           = %d \n", P_SHIFT);
    printf("ROUND_MODE      = %d \n", P_ROUND_MODE);
    printf("Data type A       = ");
    printf(QUOTE(T_DATA_A));
    printf("\n");
    printf("Data type B       = ");
    printf(QUOTE(T_DATA_B));
    printf("\n");

    matMult.init();
    matMult.run(NITER);
    matMult.end();

    return 0;
}
#endif
