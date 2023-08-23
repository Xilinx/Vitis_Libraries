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

#include "apodization_sa.hpp"

class apodization_sa : public adf::graph {
   public:
    // input and output port
    adf::input_plio image_points;
    adf::input_plio apodization_reference;
    adf::input_plio apo_distance_k;
    adf::input_plio F_number;
    adf::output_plio apodization_output;

    us::L2::apodization_sa_graph<> g;

    apodization_sa() {
        image_points = adf::input_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");
        apodization_reference =
            adf::input_plio::create("apodization_reference_tx", adf::plio_32_bits, "data/apodization_reference.txt");
        apo_distance_k = adf::input_plio::create("apo_distance_k_tx", adf::plio_32_bits, "data/apo_distance_k.txt");
        F_number = adf::input_plio::create("F_number", adf::plio_32_bits, "data/apo_distance_k.txt");
        apodization_output = adf::output_plio::create("apodization", adf::plio_32_bits, "data/apodization.txt");

        adf::connect<>(image_points.out[0], g.image_points);
        adf::connect<>(apodization_reference.out[0], g.apodization_reference);
        adf::connect<>(apo_distance_k.out[0], g.apo_distance_k);
        adf::connect<>(F_number.out[0], g.F_number);
        adf::connect<>(g.apodization_output, apodization_output.in[0]);
    }
};

apodization_sa g;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    g.init();

    g.run(1);

    g.end();

    return 0;
}
#endif
