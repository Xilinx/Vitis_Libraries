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

#include "focusing.hpp"

class focusing : public adf::graph {
   public:
    // input and output port
    adf::input_plio apo_ref_0;
    adf::input_plio xdc_def_0;
    adf::input_plio apo_ref_1;
    adf::input_plio xdc_def_1;
    adf::output_plio focusing_output;

    // focusing graph
    us::L2::focusing_graph<> foc;

    focusing() {
        // INPUTS
        apo_ref_0 = adf::input_plio::create("apo_ref_0", adf::plio_32_bits, "data/apo_ref_0.txt");
        xdc_def_0 = adf::input_plio::create("xdc_def_0", adf::plio_32_bits, "data/xdc_def_0.txt");
        apo_ref_1 = adf::input_plio::create("apo_ref_1", adf::plio_32_bits, "data/apo_ref_1.txt");
        xdc_def_1 = adf::input_plio::create("xdc_def_1", adf::plio_32_bits, "data/xdc_def_1.txt");

        // OUTPUTS
        focusing_output = adf::output_plio::create("focusing_output", adf::plio_32_bits, "data/focusing_output.txt");

        // connections
        adf::connect<>(apo_ref_0.out[0], foc.apo_ref_0);
        adf::connect<>(xdc_def_0.out[0], foc.xdc_def_0);
        adf::connect<>(apo_ref_1.out[0], foc.apo_ref_1);
        adf::connect<>(xdc_def_1.out[0], foc.xdc_def_1);
        adf::connect<>(foc.focusing_output, focusing_output.in[0]);
    }
};

focusing b;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
