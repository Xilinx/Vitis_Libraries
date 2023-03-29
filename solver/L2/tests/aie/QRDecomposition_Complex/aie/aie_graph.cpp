/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#include "aie_graph.h"
using namespace adf;

PLIO* in0 = new PLIO("DataIn0", adf::plio_32_bits, "data/input0.txt");
PLIO* in1 = new PLIO("DataIn1", adf::plio_32_bits, "data/input1.txt");
PLIO* out0 = new PLIO("DataOut0", adf::plio_32_bits, "data/output0.txt");
PLIO* out1 = new PLIO("DataOut1", adf::plio_32_bits, "data/output1.txt");

simulation::platform<2, 2> platform(in0, in1, out0, out1);

simpleGraph addergraph;

connect<> net0(platform.src[0], addergraph.in_real);
connect<> net1(platform.src[1], addergraph.in_imag);
connect<> net2(addergraph.out_real, platform.sink[0]);
connect<> net3(addergraph.out_imag, platform.sink[1]);

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    addergraph.init();
    addergraph.run(1);
    addergraph.end();
    return 0;
}
#endif
