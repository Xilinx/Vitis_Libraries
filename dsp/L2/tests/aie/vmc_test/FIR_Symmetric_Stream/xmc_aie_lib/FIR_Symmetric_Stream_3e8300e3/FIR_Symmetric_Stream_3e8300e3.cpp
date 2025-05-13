/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include <adf.h>
#include "FIR_Symmetric_Stream_3e8300e3.h"

class DUT : public adf::graph {
   public:
    FIR_Symmetric_Stream_3e8300e3 mygraph;
    adf::input_plio DUT_in[1];
    adf::output_plio DUT_out[1];

    DUT() {
        DUT_in[0] = adf::input_plio::create("DUT_in[0]", adf::plio_32_bits, "data/i0");
        DUT_out[0] = adf::output_plio::create("DUT_out[0]", adf::plio_32_bits, "data/o0");

        adf::connect<> ni0(DUT_in[0].out[0], mygraph.in[0]);
        adf::connect<> no0(mygraph.out[0], DUT_out[0].in[0]);
    }
};

DUT g;

#ifdef __AIESIM__
int main(void) {
    g.init();
    g.run();
    g.end();
    return 0;
}
#endif
