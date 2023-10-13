/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "graph.hpp"

class imagePoints : public adf::graph {
   public:
    // input and output port
    adf::input_plio start_positions;
    adf::input_plio samples_arange;
    adf::input_plio directions;
    adf::output_plio image_points;

    us::L2::imagePoints_graph<> img;

    imagePoints() {
        // input & output
        start_positions = adf::input_plio::create("start_positions", adf::plio_32_bits, "data/start_positions.txt");
        samples_arange = adf::input_plio::create("samples_arange", adf::plio_32_bits, "data/samples_arange.txt");
        directions = adf::input_plio::create("directions", adf::plio_32_bits, "data/directions.txt");
        image_points = adf::output_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");

        adf::connect<>(start_positions.out[0], img.start_positions);
        adf::connect<>(samples_arange.out[0], img.samples_arange);
        adf::connect<>(directions.out[0], img.directions);
        adf::connect<>(img.image_points, image_points.in[0]);
    }
};

imagePoints d;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    d.init();

    d.run(1);

    d.end();

    return 0;
}
#endif
