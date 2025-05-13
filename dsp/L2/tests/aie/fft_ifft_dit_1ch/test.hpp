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
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/*
This file holds the declaraion of the test harness graph class for the
fft_ifft_dit_1ch graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fft_ifft_dit_1ch_graph
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

class test_graph : public graph {
   private:
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kPortsPerTile = API_IO == 0 ? 1 : kStreamsPerTile;
    std::array<input_plio, (kPortsPerTile << PARALLEL_POWER)> in;
    std::array<output_plio, (kPortsPerTile << PARALLEL_POWER)> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples        = %d \n", INPUT_SAMPLES);
        printf("Input window (bytes) = %lu\n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Output samples       = %d \n", OUTPUT_SAMPLES);
        printf("Point size           = %d \n", POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", SHIFT);
        printf("Cascade Length       = %d \n", CASC_LEN);
        printf("Dynamic point size   = %d \n", DYN_PT_SIZE);
        printf("Window Size          = %d \n", WINDOW_VSIZE);
        printf("API_IO               = %d \n", API_IO);
        printf("PARALLEL_POWER       = %d \n", PARALLEL_POWER);
        printf("Use widgets          = %d \n", USE_WIDGETS);
        printf("Round mode           = %d \n", ROUND_MODE);
        printf("Saturation mode      = %d \n", SAT_MODE);
        printf("Twiddle mode         = %d \n", TWIDDLE_MODE);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Data out type        = ");
        printf(QUOTE(DATA_OUT_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);

        printf("========================\n");

        // FIR sub-graph
        xf::dsp::aie::fft::dit_1ch::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN,
                                              DYN_PT_SIZE, WINDOW_VSIZE, API_IO, PARALLEL_POWER, USE_WIDGETS,
                                              ROUND_MODE, SAT_MODE, TWIDDLE_MODE, DATA_OUT_TYPE>
            fftGraph;
        for (int i = 0; i < (kPortsPerTile << PARALLEL_POWER); i++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            std::string filenameIn = QUOTE(INPUT_FILE);

            // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
            // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
            // FFT)
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));

            // Make connections
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_64_bits, filenameIn);
            connect<>(in[i].out[0], fftGraph.in[i]);

            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(fftGraph.out[i], out[i].in[0]);

#ifdef USING_UUT
#if (SINGLE_BUF == 1 && API_IO == 0)
#if (PARALLEL_POWER == 0)
            single_buffer(fftGraph.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTwinproc.getKernels()->out[0]);
            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif // PARALLEL_POWER == 0
#if (PARALLEL_POWER == 1)
            single_buffer(fftGraph.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.m_r2Comb[i].out[0]);

            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif // PARALLEL_POWER == 1
#if (PARALLEL_POWER == 2)
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.m_r2Comb[i].out[0]);

            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif // PARALLEL_POWER == 2
#if (PARALLEL_POWER == 3)
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);

            single_buffer(fftGraph.m_r2Comb[i].out[0]);

            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif // PARALLEL_POWER == 3
#if (PARALLEL_POWER == 4)
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);

            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe0.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe0.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe1.FFTsubframe0.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe1.FFTsubframe1.FFTwinproc.getKernels()->in[0]);
            single_buffer(fftGraph.m_r2Comb[i].out[0]);

            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif // PARALLEL_POWER == 3
#endif // SINGLE BUFFER CONSTRAINT
#endif // USING UUT

// apply location constraints for TP_POINT_SIZE=64k
#if 0
#ifdef USING_UUT
#if (PARALLEL_POWER == 1)
        for (int lane=0; lane<2; lane++) {
          location<kernel>(fftGraph.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2, LOC_YBASE + CASC_LEN + 1);
        }
#endif //(PARALLEL_POWER == 1)
#if (PARALLEL_POWER == 2)
        for (int lane=0; lane<4; lane++) {
          location<kernel>(fftGraph.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2, LOC_YBASE + CASC_LEN + 2);
        }
        for (int lane=0; lane<2; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN + 1);
          location<kernel>(fftGraph.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+4,  LOC_YBASE + CASC_LEN + 1);
        }
#endif //(PARALLEL_POWER == 2)
#if (PARALLEL_POWER == 3)
        for (int lane=0; lane<8; lane++) {
          location<kernel>(fftGraph.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2, LOC_YBASE + CASC_LEN + 3);
        }
        for (int lane=0; lane<4; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN + 2);
          location<kernel>(fftGraph.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+8,  LOC_YBASE + CASC_LEN + 2);
        }
        for (int lane=0; lane<2; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN + 1);
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+4,  LOC_YBASE + CASC_LEN + 1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+8,  LOC_YBASE + CASC_LEN + 1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+12, LOC_YBASE + CASC_LEN + 1);
        }
#endif //(PARALLEL_POWER == 3)
#if (PARALLEL_POWER == 4)
        for (int lane=0; lane<16; lane++) {
          location<kernel>(fftGraph.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2, LOC_YBASE + CASC_LEN + 4);
        }
        for (int lane=0; lane<8; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN + 3);
          location<kernel>(fftGraph.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+16, LOC_YBASE + CASC_LEN + 3);
        }
        for (int lane=0; lane<4; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN + 2);
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+8,  LOC_YBASE + CASC_LEN + 2);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+16, LOC_YBASE + CASC_LEN + 2);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+24, LOC_YBASE + CASC_LEN + 2);
        }
        for (int lane=0; lane<2; lane++) {
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2,    LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe0.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+4,  LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+8,  LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe0.FFTsubframe1.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+12, LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+16, LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe0.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+20, LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe0.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+24, LOC_YBASE + CASC_LEN +1);
          location<kernel>(fftGraph.FFTsubframe1.FFTsubframe1.FFTsubframe1.m_r2Comb[lane]) = tile(LOC_XBASE+lane*2+28, LOC_YBASE + CASC_LEN +1);
        }
#endif //(PARALLEL_POWER == 4)
#endif // USING_UUT
#endif
        }
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
