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

#include "samples.hpp"

class samples : public adf::graph {
   public:
    // input and output port
    adf::input_plio image_points_from_PL;
    adf::input_plio delay_from_PL;
    adf::input_plio xdc_def_positions;
    adf::input_plio sampling_frequency;
    adf::output_plio samples_to_PL;

    us::L2::samples_graph<> sam;

    samples() {
        // input & output port
        image_points_from_PL =
            adf::input_plio::create("image_points_from_PL", adf::plio_32_bits, "data/image_points_from_PL.txt");
        delay_from_PL = adf::input_plio::create("delay_from_PL", adf::plio_32_bits, "data/delay_from_PL.txt");
        xdc_def_positions =
            adf::input_plio::create("xdc_def_positions", adf::plio_32_bits, "data/xdc_def_positions.txt");
        sampling_frequency =
            adf::input_plio::create("sampling_frequency", adf::plio_32_bits, "data/sampling_frequency.txt");
        samples_to_PL = adf::output_plio::create("samples_to_PL", adf::plio_32_bits, "data/samples_to_PL.txt");

        // connections
        adf::connect<>(image_points_from_PL.out[0], sam.image_points_from_PL);
        adf::connect<>(delay_from_PL.out[0], sam.delay_from_PL);
        adf::connect<>(xdc_def_positions.out[0], sam.xdc_def_positions);
        adf::connect<>(sampling_frequency.out[0], sam.sampling_frequency);
        adf::connect<>(sam.samples_to_PL, samples_to_PL.in[0]);
    }
};

samples b;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
