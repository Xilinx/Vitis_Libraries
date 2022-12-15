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

#include "apodization_sa.hpp"

adf::PLIO* image_points = new adf::PLIO("image_points", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* apodization_reference =
    new adf::PLIO("apodization_reference_tx", adf::plio_32_bits, "data/apodization_reference.txt");
adf::PLIO* apo_distance_k = new adf::PLIO("apo_distance_k_tx", adf::plio_32_bits, "data/apo_distance_k.txt");
adf::PLIO* F_number = new adf::PLIO("F_number", adf::plio_32_bits, "data/apo_distance_k.txt");

adf::PLIO* apodization_output = new adf::PLIO("apodization", adf::plio_32_bits, "data/apodization.txt");

us::L2::apodization_sa<> g;

adf::simulation::platform<4, 1> platform(
    image_points, apodization_reference, apo_distance_k, F_number, apodization_output);

adf::connect<> src_image_points(platform.src[0], g.image_points);
adf::connect<> src_apodization_reference(platform.src[1], g.apodization_reference_i);
adf::connect<> src_apo_distance_k(platform.src[2], g.apo_distance_k);
adf::connect<> src_F_number(platform.src[3], g.F_number);

adf::connect<> out_apodization_output(g.apodization_output, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    g.init();

    g.run(1);

    g.end();

    return 0;
}
#endif
