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

#include "imagePoints.hpp"

adf::PLIO* start_positions = new adf::PLIO("start_positions", adf::plio_32_bits, "data/start_positions.txt");
adf::PLIO* directions = new adf::PLIO("directions", adf::plio_32_bits, "data/directions.txt");
adf::PLIO* samples_arange = new adf::PLIO("samples_arange", adf::plio_32_bits, "data/samples_arange.txt");

adf::PLIO* image_points = new adf::PLIO("image_points", adf::plio_32_bits, "data/image_points.txt");

us::L2::imagePoints<> d;

adf::simulation::platform<3, 1> platform(start_positions, directions, samples_arange, image_points);

adf::connect<> src_start_positions(platform.src[0], d.start_positions);
adf::connect<> src_directions(platform.src[1], d.directions);
adf::connect<> src_samples_arange(platform.src[2], d.samples_arange);

adf::connect<> out_image_points(d.image_points, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    d.init();

    d.run(1);

    d.end();

    return 0;
}
#endif
