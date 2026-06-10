/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#include "test_svd.hpp"
#include "test_qrd_hh.hpp"
#include "test_substitution.hpp"

using namespace adf;
using namespace ::xf::dsp::aie;

// The following defined values are for the number of ports in and out for each IP example.
#define NUM_IP_CHOLESKY 1
#define NUM_IP_QRD 1
#define NUM_IP_SVD 1
#define NUM_IP_QRD_HH 1
#define NUM_IP_SUBSTITUTION 2

#define NUM_OP_CHOLESKY 1
#define NUM_OP_QRD 2
#define NUM_OP_SVD 3
#define NUM_OP_QRD_HH 2
#define NUM_OP_SUBSTITUTION 1

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
    static constexpr unsigned int NUM_IP_ALL =
        NUM_IP_CHOLESKY + NUM_IP_QRD + NUM_IP_SVD + NUM_IP_QRD_HH + NUM_IP_SUBSTITUTION;
    static constexpr unsigned int NUM_OP_ALL =
        NUM_OP_CHOLESKY + NUM_OP_QRD + NUM_OP_SVD + NUM_OP_QRD_HH + NUM_OP_SUBSTITUTION;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    cholesky_example::test_cholesky uut_cholesky;
    qrd_example::test_qrd uut_qrd;
    qrd_hh_example::test_qrd_hh uut_qrd_hh;
    svd_example::test_svd uut_svd;
    substitution_example::test_substitution uut_substitution;

    test_example() {
        // create input file connections - first template argument indicates first index of plio port for this library
        // element
        createPLIOFileConnections<0, NUM_IP_CHOLESKY, NUM_IP_ALL>(in, "input", "cholesky", "in");
        createPLIOFileConnections<1, NUM_IP_QRD, NUM_IP_ALL>(in, "input", "qrd_hh", "in");
        createPLIOFileConnections<2, NUM_IP_SVD, NUM_IP_ALL>(in, "input", "svd", "in");
        createPLIOFileConnections<3, NUM_IP_QRD_HH, NUM_IP_ALL>(in, "input", "qrd", "in");
        createPLIOFileConnections<4, NUM_IP_SUBSTITUTION, NUM_IP_ALL>(in, "input", "substitution", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_OP_CHOLESKY, NUM_OP_ALL>(out, "output", "cholesky", "out");
        createPLIOFileConnections<1, NUM_OP_QRD, NUM_OP_ALL>(out, "output", "qrd", "out");
        createPLIOFileConnections<3, NUM_OP_SVD, NUM_OP_ALL>(out, "output", "svd", "out");
        createPLIOFileConnections<6, NUM_OP_QRD_HH, NUM_OP_ALL>(out, "output", "qrd_hh", "out");
        createPLIOFileConnections<8, NUM_OP_SUBSTITUTION, NUM_OP_ALL>(out, "output", "substitution", "out");

        // wire up cholesky testbench
        connect<>(in[0].out[0], uut_cholesky.in);
        connect<>(uut_cholesky.out, out[0].in[0]);

        // wire up qrd testbench
        connect<>(in[1].out[0], uut_qrd.inA);
        connect<>(uut_qrd.outQ, out[1].in[0]);
        connect<>(uut_qrd.outR, out[2].in[0]);

        // wire up svd testbench
        connect<>(in[2].out[0], uut_svd.in);
        connect<>(uut_svd.outU, out[3].in[0]);
        connect<>(uut_svd.outS, out[4].in[0]);
        connect<>(uut_svd.outV, out[5].in[0]);

        // wire up qrd_hh testbench
        connect<>(in[3].out[0], uut_qrd_hh.inA);
        connect<>(uut_qrd_hh.outQ, out[6].in[0]);
        connect<>(uut_qrd_hh.outR, out[7].in[0]);

        // wire up substitution testbench
        connect<>(in[4].out[0], uut_substitution.L_in);
        connect<>(in[5].out[0], uut_substitution.y_in);
        connect<>(uut_substitution.x_out, out[8].in[0]);
    };
};
};
