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

#include "test_func_approx.hpp"
#include "test_euclidean_distance.hpp"

using namespace adf;

#define NUM_IP_FA 1
#define NUM_IP_ED 2
#define NUM_OP_FA 1
#define NUM_OP_ED 1

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

namespace all_example {

class test_example : public graph {
   public:
    static constexpr unsigned int NUM_IP_ALL = NUM_IP_FA + NUM_IP_ED;
    static constexpr unsigned int NUM_OP_ALL = NUM_OP_FA + NUM_OP_ED;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    euclidean_distance_example::test_euclidean_distance uut_ed;
    func_approx_example::test_func_approx uut_fa;

    test_example() {
        // create input file connections - first template argument indicates first index of plio port for this library
        createPLIOFileConnections<0, NUM_IP_FA, NUM_IP_ALL>(in, "input", "fa", "in");
        createPLIOFileConnections<1, NUM_IP_ED, NUM_IP_ALL>(in, "input", "ed", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_OP_FA, NUM_OP_ALL>(out, "output", "fa", "out");
        createPLIOFileConnections<1, NUM_OP_ED, NUM_OP_ALL>(out, "output", "ed", "out");

        // wire up fa
        connect<>(in[0].out[0], uut_fa.in);
        connect<>(uut_fa.out, out[0].in[0]);

        // wire up ed
        connect<>(in[1].out[0], uut_ed.inP);
        connect<>(in[2].out[0], uut_ed.inQ);
        connect<>(uut_ed.out, out[1].in[0]);
    }
};
};