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

#include "focusing_sa.hpp"

// INPUTS
////focusing
adf::PLIO* apo_ref_0 = new adf::PLIO("apo_ref_0", adf::plio_32_bits, "data/apo_ref_0.txt");
adf::PLIO* img_points_0 = new adf::PLIO("img_points_0", adf::plio_32_bits, "data/xdc_def_0.txt");
adf::PLIO* apo_ref_1 = new adf::PLIO("apo_ref_1", adf::plio_32_bits, "data/apo_ref_1.txt");
adf::PLIO* img_points_1 = new adf::PLIO("img_points_1", adf::plio_32_bits, "data/xdc_def_1.txt");
// OUTPUTS
adf::PLIO* focusing_output = new adf::PLIO("focusing_output", adf::plio_32_bits, "data/focusing_output.txt");

us::L2::focusing_sa<> b;

adf::simulation::platform<4, 1> platform(
    // INPUTS
    // Focusing
    apo_ref_0,
    img_points_0,
    apo_ref_1,
    img_points_1,

    // OUTPUTS
    focusing_output);

// INPUTS
// focusing
adf::connect<> input_apo_0(platform.src[0], b.apo_ref_0);
adf::connect<> input_xdc_0(platform.src[1], b.img_points_0);
adf::connect<> input_apo_1(platform.src[2], b.apo_ref_1);
adf::connect<> input_xdc_1(platform.src[3], b.img_points_1);

// OUTPUTS
adf::connect<> output_vector(b.focusing_output, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
