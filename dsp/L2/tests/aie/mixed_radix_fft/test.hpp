/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/*
This file holds the declaraion of the test harness graph class for the
mixed_radix_fft graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_static_config.h"
#include "uut_config.h"
#include "test_stim.hpp"
#include "graph_utils.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH mixed_radix_fft_graph
#endif

// location constraints for POINT_SIZE=65536
#define LOC_XBASE 1
#define LOC_YBASE 0

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   private:
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kAPIFactor =
        API_IO == 0 ? 1 : kStreamsPerTile; // two ports per kernel for stream(AIE1), 1 port per kernel for windows.
    std::array<input_plio, kAPIFactor> in;
    std::array<output_plio, kAPIFactor> out;
    std::array<input_plio, DYN_PT_SIZE> inHdr;
    std::array<output_plio, DYN_PT_SIZE> outHdr;
    //    port_conditional_array<input, DYN_PT_SIZE == 1, 1> inHdr;
    //    port_conditional_array<output, DYN_PT_SIZE == 1, 1> outHdr;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type             = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type          = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("Point size            = %d \n", POINT_SIZE);
        printf("FFT/nIFFT             = %d \n", FFT_NIFFT);
        printf("Final scaling Shift   = %d \n", SHIFT);
        printf("Round mode            = %d \n", ROUND_MODE);
        printf("Saturation mode       = %d \n", SAT_MODE);
        printf("Window size           = %d \n", WINDOW_VSIZE);
        printf("Number of kernels     = %d \n", CASC_LEN);
        printf("API_IO                = %d \n", API_IO);
        printf("DYN_TP_SIZE           = %d \n", DYN_PT_SIZE);
        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);

        printf("========================\n");

        xf::dsp::aie::fft::mixed_radix_fft::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, ROUND_MODE,
                                                      SAT_MODE, WINDOW_VSIZE, CASC_LEN, API_IO, DYN_PT_SIZE>
            mixed_radix_fftGraph;

        for (int i = 0; i < kAPIFactor; i++) {
            std::string filenameIn = QUOTE(INPUT_FILE);
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_64_bits, filenameIn);
            connect<>(in[i].out[0], mixed_radix_fftGraph.in[i]);

            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(mixed_radix_fftGraph.out[i], out[i].in[0]);
        }
#if DYN_PT_SIZE == 1
        std::string filenameHdrIn = QUOTE(INPUT_HEADER_FILE);
        // printf("filenameHdrIn");
        // filenameHdrIn.insert(filenameHdrIn.length() - 4, ("_" + std::to_string(0) + "_0"));
        inHdr[0] = input_plio::create("PLIO_in_" + std::to_string(kAPIFactor), adf::plio_32_bits, filenameHdrIn);
        // printf("\n done input_plio \n");
        connect<>(inHdr[0].out[0], mixed_radix_fftGraph.headerIn[0]);
        // printf("connected \n");

        std::string filenameHdrOut = QUOTE(OUTPUT_HEADER_FILE);
        // printf("filenameHdrOut");
        // filenameHdrOut.insert(filenameHdrOut.length() - 4, ("_" + std::to_string(0) + "_0"));
        outHdr[0] = output_plio::create("PLIO_out_" + std::to_string(kAPIFactor), adf::plio_32_bits, filenameHdrOut);
        // printf("\n done input_plio \n");
        connect<>(mixed_radix_fftGraph.headerOut[0], outHdr[0].in[0]);
// printf("connected \n");
#endif // DYN_PT_SIZE == 1
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
