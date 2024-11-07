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
#ifndef _DSPLIB_TL_HPP_
#define _DSPLIB_TL_HPP_

/*
This file holds the declaration of the top level PLIO graph class for the
fft_ifft_decompose .
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)
#define CASC_LEN 1

#include "fft_ifft_dit_1ch_ref_graph.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace top_level {

class tl_graph : public graph {
   private:
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kPortsPerTile = API_IO == 0 ? 1 : kStreamsPerTile;
    static constexpr int ptSize = POINT_SIZE;
    static constexpr int DYN_PT_SIZE = 0;
    static constexpr int refParPow =
        ptSize <= 4096 ? 0 : ptSize == 8192 ? 1 : ptSize == 16384 ? 2 : ptSize == 32768 ? 3 : ptSize == 65536 ? 4 : -1;
    static constexpr int windowSize = ptSize;
    std::array<output_plio, (1 << refParPow)> back_o;
    std::array<input_plio, (1 << refParPow)> front_i;
    // Constructor
    tl_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Point size           = %d \n", POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", SHIFT);
        printf("Cascade Length       = %d \n", CASC_LEN);
        printf("API_IO               = %d \n", API_IO);
        printf("Round mode           = %d \n", ROUND_MODE);
        printf("Saturation mode      = %d \n", SAT_MODE);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);
        printf("========================\n");

        xf::dsp::aie::fft::dit_1ch::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN,
                                              DYN_PT_SIZE, windowSize, API_IO, refParPow, USE_WIDGETS, ROUND_MODE,
                                              SAT_MODE, TWIDDLE_MODE>
            fftGraph;
        for (int i = 0; i < (1 << refParPow); i++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            std::string filenameIn = QUOTE(INPUT_FILE);

            // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
            // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
            // FFT)
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));

            // Make connections
            front_i[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_32_bits, filenameIn);
            connect<>(front_i[i].out[0], fftGraph.in[i]);

            back_o[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_32_bits, filenameOut);
            connect<>(fftGraph.out[i], back_o[i].in[0]);
        }
    };
};
}
}
}
};

#endif // _DSPLIB_TL_HPP_