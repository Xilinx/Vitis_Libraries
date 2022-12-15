/*
 * Copyright 2022 Xilinx, Inc.
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

#include "bSpline.hpp"

// INPUTS
// interpolation
adf::PLIO* P1 = new adf::PLIO("P1", adf::plio_32_bits, "data/P0.txt");
adf::PLIO* P2 = new adf::PLIO("P2", adf::plio_32_bits, "data/P0.txt");
adf::PLIO* P3 = new adf::PLIO("P3", adf::plio_32_bits, "data/P0.txt");
adf::PLIO* P4 = new adf::PLIO("P4", adf::plio_32_bits, "data/P0.txt");
adf::PLIO* P5 = new adf::PLIO("P5", adf::plio_32_bits, "data/P0.txt");
adf::PLIO* P6 = new adf::PLIO("P6", adf::plio_32_bits, "data/P0.txt");

// OUTPUTS
adf::PLIO* C = new adf::PLIO("C", adf::plio_32_bits, "data/C.txt");

us::L2::bSpline<> b;

adf::simulation::platform<6, 1> platform(
    // INPUTS
    // Interpolation
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    // OUTPUTS
    C);

// INPUTS
// interpolator
adf::connect<> in_P1(platform.src[0], b.P1);
adf::connect<> in_P2(platform.src[1], b.P2);
adf::connect<> in_P3(platform.src[2], b.P3);
adf::connect<> in_P4(platform.src[3], b.P4);
adf::connect<> in_P5(platform.src[4], b.P5);
adf::connect<> in_P6(platform.src[5], b.P6);

// OUTPUTS
adf::connect<> res_C(b.C, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
