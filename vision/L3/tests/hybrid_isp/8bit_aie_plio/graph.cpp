/*
 * Copyright 2021 Xilinx, Inc.
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

#include "graph.h"
//#include "ccm-params.h"

// instantiate adf dataflow graph
TopPipelineGraph<1> TOP[CORES] = {{20, 0, 0}, {22, 0, 1}, {24, 0, 2}}; //, {24, 0, 3}};

#define SAT_U8(x) std::max(0, std::min(255, (static_cast<int>(x))))

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include "ccm-params.h"
int main(int argc, char** argv) {
    uint8_t rgain1 = 128;
    uint8_t bgain1 = 128;
    uint8_t ggain1 = 64;
    uint8_t black_level = 32;
    const int MaxLevel = 255; // 8b input value
    float MulValue1 = (float)((float)MaxLevel / (MaxLevel - black_level));
    uint16_t MulValue = 37470; // Q(1.15)

    int16_t coeffs[25];
    uint16_t* coeffs_awb = (uint16_t*)(coeffs + 16);
    int16_t* coeffs_ccm = coeffs;

    float min[4], max[4];
    min[0] = -0.5;
    min[1] = -0.5;
    min[2] = -0.5;
    min[3] = 0;
    max[0] = 230.5;
    max[1] = 223.5;
    max[2] = 250.5;
    max[3] = 0;
    ccmparams<0>(coeffs_ccm);

    for (int i = 0; i < CORES; i++) {
        // Empty
        TOP[i].init();
        TOP[i].update(TOP[i].blk_val, black_level);
        TOP[i].update(TOP[i].mul_val, MulValue);
        TOP[i].update(TOP[i].rgain, rgain1);
        TOP[i].update(TOP[i].bgain, bgain1);
        TOP[i].update(TOP[i].ggain, ggain1);
        TOP[i].update(TOP[i].coeffs, coeffs, 25);
        TOP[i].run(NUM_TILES);
    }

    for (int i = 0; i < CORES; i++) {
        TOP[i].wait();
    }

    for (int i = 0; i < CORES; i++) {
        TOP[i].end();
    }
    return 0;
}
#endif
