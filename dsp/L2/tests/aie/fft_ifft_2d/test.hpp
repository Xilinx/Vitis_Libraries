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
#define UUT_GRAPH fft_ifft_2d_graph
#endif

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
        printf("Point size D1        = %d \n", POINT_SIZE_D1);
        printf("Point size D2        = %d \n", POINT_SIZE_D2);
        printf("FFT/nIFFT            = %d \n", FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", SHIFT);
        printf("Cascade Length       = %d \n", CASC_LEN);
        printf("API_IO               = %d \n", API_IO);
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

        xf::dsp::aie::fft::two_d::UUT_GRAPH<DATA_TYPE_D1, DATA_TYPE_D2, TWIDDLE_TYPE, POINT_SIZE_D1, POINT_SIZE_D2,
                                            FFT_NIFFT, SHIFT, CASC_LEN, WINDOW_VSIZE_D1, WINDOW_VSIZE_D2, API_IO,
                                            ROUND_MODE, SAT_MODE, TWIDDLE_MODE>
            fftAutomoGraph;

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        std::string filenameIn = QUOTE(INPUT_FILE);

        // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
        // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
        // FFT)
        filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(0) + "_0"));
        filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(0) + "_0"));

        // Make connections
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);

        connect<>(in[0].out[0], fftAutomoGraph.in[0]);
        connect<>(fftAutomoGraph.out[0], out[0].in[0]);
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
