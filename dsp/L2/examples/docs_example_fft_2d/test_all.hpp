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
#include "test_fft_2d.hpp"

using namespace adf;

#define NUM_IP_FFT2D 1
#define NUM_OP_FFT2D 1

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
    static constexpr unsigned int NUM_IP_ALL = NUM_IP_FFT2D;
    static constexpr unsigned int NUM_OP_ALL = NUM_OP_FFT2D;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    fft_2d_example::test_fft_2d uut_fft2d;

    test_example() {
        // create input file connections - first template argument indicates first index of plio port for this library
        // createPLIOFileConnections<0, NUM_IP_FA, NUM_IP_ALL>(in, "input", "func_approx", "in");

        // create output file connections
        // createPLIOFileConnections<0, NUM_IP_FA, NUM_OP_ALL>(out, "output", "func_approx", "out");

        // create input file connections - first template argument indicates first index of plio port for this library
        createPLIOFileConnections<0, NUM_IP_FFT2D, NUM_IP_ALL>(in, "input", "fft_2d", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_OP_FFT2D, NUM_OP_ALL>(out, "output", "fft_2d", "out");

        // wire up func_approx
        // connect<>(in[0].out[0], uut_fa.in);
        // connect<>(uut_fa.out, out[0].in[0]);
        connect<>(in[0].out[0], uut_fft2d.in);
        connect<>(uut_fft2d.out, out[0].in[0]);
    }
};
};