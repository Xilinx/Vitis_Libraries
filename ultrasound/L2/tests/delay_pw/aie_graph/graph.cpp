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

#include "delay_pw.hpp"

// INPUTS
////delayConstant
adf::PLIO* image_points_from_PL = new adf::PLIO("image_points_from_PL", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* tx_def_ref_point = new adf::PLIO("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
adf::PLIO* t_start = new adf::PLIO("t_start", adf::plio_32_bits, "data/t_start.txt");

// OUTPUTS
adf::PLIO* delay_to_PL = new adf::PLIO("delay_to_PL", adf::plio_32_bits, "data/delay_to_PL.txt");

us::L2::delay_pw<> b;

adf::simulation::platform<3, 1> platform(
    // INPUTS
    // DELAY
    image_points_from_PL,
    tx_def_ref_point,
    t_start,
    // OUTPUTS
    delay_to_PL);

// INPUTS
//////delayConstants
adf::connect<> src_image_points(platform.src[0], b.image_points_from_PL);
adf::connect<> src_delay_ref(platform.src[1], b.tx_def_reference_point);
adf::connect<> src_t_start(platform.src[2], b.t_start);
// OUTPUTS
adf::connect<> out_delay(b.delay_to_PL, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif