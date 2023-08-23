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

#include "bSpline.hpp"

class bSpline : public adf::graph {
   public:
    // interpolator
    adf::input_plio P1;
    adf::input_plio P2;
    adf::input_plio P3;
    adf::input_plio P4;
    adf::input_plio P5;
    adf::input_plio P6;
    adf::output_plio C;

    us::L2::bSpline_graph<> inter;

    bSpline() {
        // INPUTS
        // interpolation
        P1 = adf::input_plio::create("P1", adf::plio_32_bits, "data/P0.txt");
        P2 = adf::input_plio::create("P2", adf::plio_32_bits, "data/P0.txt");
        P3 = adf::input_plio::create("P3", adf::plio_32_bits, "data/P0.txt");
        P4 = adf::input_plio::create("P4", adf::plio_32_bits, "data/P0.txt");
        P5 = adf::input_plio::create("P5", adf::plio_32_bits, "data/P0.txt");
        P6 = adf::input_plio::create("P6", adf::plio_32_bits, "data/P0.txt");
        C = adf::output_plio::create("C", adf::plio_32_bits, "data/C.txt");

        ///////////// CONNECTIONS /////////////////////////////////////////////////////////////////////////////////////
        adf::connect<>(P1.out[0], inter.P1);
        adf::connect<>(P2.out[0], inter.P2);
        adf::connect<>(P3.out[0], inter.P3);
        adf::connect<>(P4.out[0], inter.P4);
        adf::connect<>(P5.out[0], inter.P5);
        adf::connect<>(P6.out[0], inter.P6);
        adf::connect<>(inter.C, C.in[0]);
    }
};

bSpline b;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    b.init();

    b.run(1);

    b.end();

    return 0;
}
#endif
