/*
 * Copyright 2022 Xilinx, Inc.
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

#include "test_dds.hpp"
#include "test_fir.hpp"
#include "test_fft.hpp"
#include "test_matmul.hpp"
#include "test_widget_api_cast.hpp"
#include "test_widg_r2c.hpp"
#include "test_fft_window.hpp"

using namespace adf;
#define NUM_IP_FIR 1
#define NUM_IP_DDS 1
#define NUM_IP_FFT 4
#define NUM_IP_MM 2
#define NUM_OP_MM 1
#define NUM_IP_WIDG_API 2
#define NUM_OP_WIDG_API 1
#define NUM_IP_WIDG_R2C 1
#define NUM_OP_WIDG_R2C 1
#define NUM_IP_FFTW 1

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
    static constexpr unsigned int NUM_IP_ALL =
        NUM_IP_FIR + NUM_IP_DDS + NUM_IP_FFT + NUM_IP_MM + NUM_IP_WIDG_API + NUM_IP_WIDG_R2C + NUM_IP_FFTW;
    static constexpr unsigned int NUM_OP_ALL =
        NUM_IP_FIR + NUM_IP_DDS + NUM_IP_FFT + NUM_OP_MM + NUM_OP_WIDG_API + NUM_OP_WIDG_R2C + NUM_IP_FFTW;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    dds_example::test_dds uut_dds;
    fir_example::test_fir uut_fir;
    fft_example::test_fft uut_fft;
    mm_example::test_mm uut_mm;
    widg1_example::test_widg_api uut_widg_api;
    widgr2c_example::test_widg_r2c uut_widg_r2c;
    fft_win_example::test_fftw uut_fftw;

    test_example() {
        // create input file connections - first template argument indicates first index of plio port for this library
        // element
        createPLIOFileConnections<0, NUM_IP_DDS, NUM_IP_ALL>(in, "input", "dds", "in");
        createPLIOFileConnections<1, NUM_IP_FIR, NUM_IP_ALL>(in, "input", "fir", "in");
        createPLIOFileConnections<2, NUM_IP_FFT, NUM_IP_ALL>(in, "input", "fft", "in");
        createPLIOFileConnections<6, NUM_IP_MM, NUM_IP_ALL>(in, "input", "mm", "in");
        createPLIOFileConnections<8, NUM_IP_WIDG_API, NUM_IP_ALL>(in, "input", "w_api", "in");
        createPLIOFileConnections<10, NUM_IP_WIDG_R2C, NUM_IP_ALL>(in, "input", "w_r2c", "in");
        createPLIOFileConnections<11, NUM_IP_FFTW, NUM_IP_ALL>(in, "input", "fftw", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_IP_DDS, NUM_OP_ALL>(out, "output", "dds", "out");
        createPLIOFileConnections<1, NUM_IP_FIR, NUM_OP_ALL>(out, "output", "fir", "out");
        createPLIOFileConnections<2, NUM_IP_FFT, NUM_OP_ALL>(out, "output", "fft", "out");
        createPLIOFileConnections<6, NUM_OP_MM, NUM_OP_ALL>(out, "output", "mm", "out");
        createPLIOFileConnections<7, NUM_OP_WIDG_API, NUM_OP_ALL>(out, "output", "w_api", "out");
        createPLIOFileConnections<8, NUM_OP_WIDG_R2C, NUM_OP_ALL>(out, "output", "w_r2c", "out");
        createPLIOFileConnections<9, NUM_IP_FFTW, NUM_OP_ALL>(out, "output", "fftw", "out");

        // wire up dds testbench
        connect<>(in[0].out[0], uut_dds.in);
        connect<>(uut_dds.out, out[0].in[0]);

        // wire up fir testbench
        connect<>(in[1].out[0], uut_fir.in);
        connect<>(uut_fir.out, out[1].in[0]);

        // wire up fft testbench
        for (int i = 0; i < 4; i++) {
            connect<>(uut_fft.out[i], out[2 + i].in[0]);
            connect<>(in[2 + i].out[0], uut_fft.in[i]);
        }

        // wire up matmul testbench
        connect<>(in[6].out[0], uut_mm.inA);
        connect<>(in[7].out[0], uut_mm.inB);
        connect<>(uut_mm.out, out[6].in[0]);

        // wire up widget_api_cast testbench
        connect<>(in[8].out[0], uut_widg_api.in[0]);
        connect<>(in[9].out[0], uut_widg_api.in[1]);
        connect<>(uut_widg_api.out[0], out[7].in[0]);

        // wire up widget real 2 complex testbench
        connect<>(in[10].out[0], uut_widg_r2c.in);
        connect<>(uut_widg_r2c.out, out[8].in[0]);

        // wire up fft window
        connect<>(uut_fftw.out, out[9].in[0]);
        connect<>(in[11].out[0], uut_fftw.in);
    };
};
};