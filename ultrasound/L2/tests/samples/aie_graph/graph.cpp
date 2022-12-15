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

#include "samples.hpp"

// INPUTS
////samples
adf::PLIO* image_points_from_PL_2 =
    new adf::PLIO("image_points_from_PL_2", adf::plio_32_bits, "data/image_points_from_PL.txt");
adf::PLIO* delay = new adf::PLIO("delay_from_PL", adf::plio_32_bits, "data/delay_from_PL.txt");
adf::PLIO* xdc_def_positions = new adf::PLIO("xdc_def_positions", adf::plio_32_bits, "data/xdc_def_positions.txt");
adf::PLIO* sampling_frequency = new adf::PLIO("sampling_frequency", adf::plio_32_bits, "data/sampling_frequency.txt");

// OUTPUTS
adf::PLIO* samples_to_PL = new adf::PLIO("samples_to_PL", adf::plio_32_bits, "data/samples_to_PL.txt");

us::L2::samples<> b;

adf::simulation::platform<4, 1> platform(
    // INPUTS
    // Samples
    image_points_from_PL_2,
    delay,
    xdc_def_positions,
    sampling_frequency,
    // OUTPUT
    samples_to_PL);

// INPUTS
// samples
adf::connect<> src_image_points_2(platform.src[0], b.image_points_from_PL_2);
adf::connect<> delay_in(platform.src[1], b.delay_from_PL);
adf::connect<> xdc_def_pos_in(platform.src[2], b.xdc_def_positions);
adf::connect<> sampl_freq(platform.src[3], b.samplingFrequency);

// OUTPUTS
adf::connect<> out_samples(b.samples_to_PL, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
