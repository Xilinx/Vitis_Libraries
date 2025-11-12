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

#include "test_cholesky.hpp"
#include "test_qrd.hpp"

using namespace adf;
using namespace ::xf::dsp::aie;

//The following defined values are for the number of ports in and out for each IP example.
#define NUM_IP_CHOLESKY 1
#define NUM_IP_QRD 1
#define NUM_OP_CHOLESKY 1
#define NUM_OP_QRD 2

template <unsigned int elem_start, unsigned int num_ports, unsigned int NUM_IP_ALL, typename plioType>
void createPLIOFileConnections(std::array<plioType, NUM_IP_ALL>& plioPorts,
                               std::string pre_filename,
                               std::string lib_elem,
                               std::string plioDescriptor = "in") {
    for (int i = 0; i < num_ports; i++) {
        std::string filename = "data/" + pre_filename + "_" + lib_elem + "_" + std::to_string(i) + ".txt";
        plioPorts[elem_start + i] =
            plioType::create("PLIO_" + plioDescriptor + "_" + std::to_string(elem_start) + "_" + std::to_string(i),
                             adf::plio_32_bits, filename);
    }
}

namespace all_example_solver {

class test_example : public graph {
   public:
    static constexpr unsigned int NUM_IP_ALL = NUM_IP_CHOLESKY + NUM_IP_QRD;
    static constexpr unsigned int NUM_OP_ALL = NUM_OP_CHOLESKY + NUM_OP_QRD;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    cholesky_example::test_cholesky uut_cholesky;
    qrd_example::test_qrd uut_qrd;

    test_example() {
        // create input file connections - first template argument indicates first index of plio port for this library
        // element
        createPLIOFileConnections<0, NUM_IP_CHOLESKY, NUM_IP_ALL>(in, "input", "cholesky", "in");
        createPLIOFileConnections<1, NUM_IP_QRD, NUM_IP_ALL>(in, "input", "qrd", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_OP_CHOLESKY, NUM_OP_ALL>(out, "output", "cholesky", "out");
        createPLIOFileConnections<1, NUM_OP_QRD, NUM_OP_ALL>(out, "output", "qrd", "out");

        // wire up dds testbench
        connect<>(in[0].out[0], uut_cholesky.in);
        connect<>(uut_cholesky.out, out[0].in[0]);

        // wire up fir testbench
        connect<>(in[1].out[0], uut_qrd.inA);
        connect<>(uut_qrd.outQ, out[1].in[0]);
        connect<>(uut_qrd.outR, out[2].in[0]);
    };
};
};
