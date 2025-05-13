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

#include "test_dds.hpp"
#include "test_fir.hpp"
#include "test_fir_tdm.hpp"
#include "test_fft.hpp"
#include "test_matmul.hpp"
#include "test_widget_api_cast.hpp"
#include "test_widg_r2c.hpp"
#include "test_fft_window.hpp"
#include "test_dds_lut.hpp"
#include "test_outer_tensor.hpp"
#include "test_hadamard.hpp"
#include "test_kronecker.hpp"
#include "test_conv.hpp"
#include "test_corr.hpp"
#include "test_dft.hpp"
#include "test_matvec.hpp"
#include "test_bitonic_sort.hpp"

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
#define NUM_IP_DDS_LUT 1
#define NUM_OP_DDS_LUT 1
#define NUM_IP_OT 2
#define NUM_OP_OT 1
#define NUM_IP_HAD 2
#define NUM_OP_HAD 1
#define NUM_IP_KMP 2
#define NUM_OP_KMP 1
#define NUM_IP_CONV 2
#define NUM_OP_CONV 1
#define NUM_IP_CORR 2
#define NUM_OP_CORR 1
#define NUM_IP_DFT 2
#define NUM_OP_DFT 2
#define NUM_IP_MV 4
#define NUM_OP_MV 2
#define NUM_IP_FIR_TDM 1
#define NUM_IP_BS 1
#define NUM_OP_BS 1

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
    static constexpr unsigned int NUM_IP_ALL = NUM_IP_FIR + NUM_IP_DDS + NUM_IP_FFT + NUM_IP_MM + NUM_IP_WIDG_API +
                                               NUM_IP_WIDG_R2C + NUM_IP_FFTW + NUM_IP_DDS_LUT + NUM_IP_OT + NUM_IP_HAD +
                                               NUM_IP_KMP + NUM_IP_CONV + NUM_IP_CORR + NUM_IP_DFT + NUM_IP_MV +
                                               NUM_IP_FIR_TDM + NUM_IP_BS;
    static constexpr unsigned int NUM_OP_ALL = NUM_IP_FIR + NUM_IP_DDS + NUM_IP_FFT + NUM_OP_MM + NUM_OP_WIDG_API +
                                               NUM_OP_WIDG_R2C + NUM_IP_FFTW + NUM_OP_DDS_LUT + NUM_OP_OT + NUM_OP_HAD +
                                               NUM_OP_KMP + NUM_OP_CONV + NUM_OP_CORR + NUM_OP_DFT + NUM_OP_MV +
                                               NUM_IP_FIR_TDM + NUM_OP_BS;

    std::array<input_plio, NUM_IP_ALL> in;
    std::array<output_plio, NUM_OP_ALL> out;

    dds_example::test_dds uut_dds;
    fir_example::test_fir uut_fir;
    fir_tdm_example::test_fir_tdm uut_fir_tdm;
    fft_example::test_fft uut_fft;
    mm_example::test_mm uut_mm;
    widg1_example::test_widg_api uut_widg_api;
    widgr2c_example::test_widg_r2c uut_widg_r2c;
    fft_win_example::test_fftw uut_fftw;
    dds_lut_example::test_dds_lut uut_dds_lut;
    outer_tensor_example::test_outer_tensor uut_outer_tensor;
    hadamard_example::test_hadamard uut_hadamard;
    kronecker_example::test_kronecker uut_kronecker;
    conv_example::test_conv uut_conv;
    corr_example::test_corr uut_corr;
    dft_example::test_dft uut_dft;
    mv_example::test_mv uut_mv;
    bitonic_sort_example::test_bitonic_sort uut_bitonic_sort;

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
        createPLIOFileConnections<12, NUM_IP_DDS_LUT, NUM_IP_ALL>(in, "input", "ddslut", "in");
        createPLIOFileConnections<13, NUM_IP_OT, NUM_IP_ALL>(in, "input", "outer_tensor", "in");
        createPLIOFileConnections<15, NUM_IP_HAD, NUM_IP_ALL>(in, "input", "hadamard", "in");
        createPLIOFileConnections<17, NUM_IP_KMP, NUM_IP_ALL>(in, "input", "kronecker", "in");
        createPLIOFileConnections<19, NUM_IP_CONV, NUM_IP_ALL>(in, "input", "conv", "in");
        createPLIOFileConnections<21, NUM_IP_CORR, NUM_IP_ALL>(in, "input", "corr", "in");
        createPLIOFileConnections<23, NUM_IP_DFT, NUM_IP_ALL>(in, "input", "dft", "in");
        createPLIOFileConnections<25, NUM_IP_MV, NUM_IP_ALL>(in, "input", "mv", "in");
        createPLIOFileConnections<29, NUM_IP_FIR_TDM, NUM_IP_ALL>(in, "input", "fir_tdm", "in");
        createPLIOFileConnections<30, NUM_IP_BS, NUM_IP_ALL>(in, "input", "bitonic_sort", "in");

        // create output file connections
        createPLIOFileConnections<0, NUM_IP_DDS, NUM_OP_ALL>(out, "output", "dds", "out");
        createPLIOFileConnections<1, NUM_IP_FIR, NUM_OP_ALL>(out, "output", "fir", "out");
        createPLIOFileConnections<2, NUM_IP_FFT, NUM_OP_ALL>(out, "output", "fft", "out");
        createPLIOFileConnections<6, NUM_OP_MM, NUM_OP_ALL>(out, "output", "mm", "out");
        createPLIOFileConnections<7, NUM_OP_WIDG_API, NUM_OP_ALL>(out, "output", "w_api", "out");
        createPLIOFileConnections<8, NUM_OP_WIDG_R2C, NUM_OP_ALL>(out, "output", "w_r2c", "out");
        createPLIOFileConnections<9, NUM_IP_FFTW, NUM_OP_ALL>(out, "output", "fftw", "out");
        createPLIOFileConnections<10, NUM_OP_DDS_LUT, NUM_OP_ALL>(out, "output", "ddslut", "out");
        createPLIOFileConnections<11, NUM_OP_OT, NUM_OP_ALL>(out, "output", "outer_tensor", "out");
        createPLIOFileConnections<12, NUM_OP_HAD, NUM_OP_ALL>(out, "output", "hadamard", "out");
        createPLIOFileConnections<13, NUM_OP_KMP, NUM_OP_ALL>(out, "output", "kronecker", "out");
        createPLIOFileConnections<14, NUM_OP_CONV, NUM_OP_ALL>(out, "output", "conv", "out");
        createPLIOFileConnections<15, NUM_OP_CORR, NUM_OP_ALL>(out, "output", "corr", "out");
        createPLIOFileConnections<16, NUM_OP_DFT, NUM_OP_ALL>(out, "output", "dft", "out");
        createPLIOFileConnections<18, NUM_OP_MV, NUM_OP_ALL>(out, "output", "mv", "out");
        createPLIOFileConnections<20, NUM_IP_FIR_TDM, NUM_OP_ALL>(out, "output", "fir_tdm", "out");
        createPLIOFileConnections<21, NUM_OP_BS, NUM_OP_ALL>(out, "output", "bitonic_sort", "out");

        // wire up dds testbench
        connect<>(in[0].out[0], uut_dds.in);
        connect<>(uut_dds.out, out[0].in[0]);

        // wire up fir testbench
        connect<>(in[1].out[0], uut_fir.in);
        connect<>(uut_fir.out, out[1].in[0]);

        // wire up fft testbench
        for (int i = 0; i < 4; i++) {
            connect<>(in[2 + i].out[0], uut_fft.in[i]);
            connect<>(uut_fft.out[i], out[2 + i].in[0]);
        }

        // wire up matmul testbench
        connect<>(in[6].out[0], uut_mm.inA[0]);
        connect<>(in[7].out[0], uut_mm.inB[0]);
        connect<>(uut_mm.out[0], out[6].in[0]);

        // wire up widget_api_cast testbench
        connect<>(in[8].out[0], uut_widg_api.in[0]);
        connect<>(in[9].out[0], uut_widg_api.in[1]);
        connect<>(uut_widg_api.out[0], out[7].in[0]);

        // wire up widget real 2 complex testbench
        connect<>(in[10].out[0], uut_widg_r2c.in);
        connect<>(uut_widg_r2c.out, out[8].in[0]);

        // wire up fft window
        connect<>(in[11].out[0], uut_fftw.in);
        connect<>(uut_fftw.out, out[9].in[0]);

        // wire up dds lut
        connect<>(in[12].out[0], uut_dds_lut.in);
        connect<>(uut_dds_lut.out, out[10].in[0]);

        // wire up outer tensor
        connect<>(in[13].out[0], uut_outer_tensor.inA);
        connect<>(in[14].out[0], uut_outer_tensor.inB);
        connect<>(uut_outer_tensor.out, out[11].in[0]);

        // wire up hadamard
        connect<>(in[15].out[0], uut_hadamard.inA);
        connect<>(in[16].out[0], uut_hadamard.inB);
        connect<>(uut_hadamard.out, out[12].in[0]);

        // wire up kronecker
        connect<>(in[17].out[0], uut_kronecker.inA);
        connect<>(in[18].out[0], uut_kronecker.inB);
        connect<>(uut_kronecker.out, out[13].in[0]);

        // wire up convolution
        connect<>(in[19].out[0], uut_conv.inF[0]);
        connect<>(in[20].out[0], uut_conv.inG[0]);
        connect<>(uut_conv.out[0], out[14].in[0]);

        // wire up correlation
        connect<>(in[21].out[0], uut_corr.inF[0]);
        connect<>(in[22].out[0], uut_corr.inG[0]);
        connect<>(uut_corr.out[0], out[15].in[0]);

        // wire up dft
        connect<>(in[23].out[0], uut_dft.in[0]);
        connect<>(in[24].out[0], uut_dft.in[1]);
        connect<>(uut_dft.out[0], out[16].in[0]);
        connect<>(uut_dft.out[1], out[17].in[0]);

        // wire up matrix_vector_mul
        connect<>(in[25].out[0], uut_mv.inA[0]);
        connect<>(in[26].out[0], uut_mv.inB[0]);
        connect<>(in[27].out[0], uut_mv.inA[1]);
        connect<>(in[28].out[0], uut_mv.inB[1]);
        connect<>(uut_mv.out[0], out[18].in[0]);
        connect<>(uut_mv.out[1], out[19].in[0]);

        // wire up fir tdm testbench
        connect<>(in[29].out[0], uut_fir_tdm.in);
        connect<>(uut_fir_tdm.out, out[20].in[0]);

        // wire up bitonic sort testbench
        connect<>(in[30].out[0], uut_bitonic_sort.in);
        connect<>(uut_bitonic_sort.out, out[21].in[0]);
    };
};
};