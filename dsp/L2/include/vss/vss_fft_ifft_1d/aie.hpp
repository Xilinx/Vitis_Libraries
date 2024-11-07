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
decomposed fft .
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "config.h"

#define Q(x) #x
#define QUOTE(x) Q(x)
#define NITER 2
#define AIE_GRAPH vss_fft_ifft_1d_graph

#include QUOTE(AIE_GRAPH.hpp)

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
    std::array<input_plio, SSR> back_i;
    std::array<output_plio, SSR> back_o;
    std::array<input_plio, SSR> front_i;
    std::array<output_plio, SSR> front_o;

    // Constructor
    tl_graph() {
        printf("========================\n");
        printf("== Graph Class: ");
        printf(QUOTE(AIE_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Point size           = %d \n", POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", SHIFT);
        printf("API_IO               = %d \n", API_IO);
        printf("Round mode           = %d \n", ROUND_MODE);
        printf("Saturation mode      = %d \n", SAT_MODE);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("========================\n");
        adf::plio_type aiePlioWidth = AIE_PLIO_WIDTH == 64 ? adf::plio_64_bits : adf::plio_128_bits;
        // FIR sub-graph
        xf::dsp::aie::fft::vss_1d::AIE_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, API_IO, SSR,
                                             ROUND_MODE, SAT_MODE, TWIDDLE_MODE>
            fftGraph;
        for (int i = 0; i < SSR; i++) {
            std::string filenameInFront = QUOTE(FRONT_INPUT_FILE);

            // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
            // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
            // FFT)
            filenameInFront.insert(filenameInFront.length() - 4, ("_" + std::to_string(i) + "_0"));

            // Make connections
            front_i[i] = input_plio::create("PLIO_front_in_" + std::to_string(i), aiePlioWidth, filenameInFront);
            connect<>(front_i[i].out[0], fftGraph.front_i[i]);

            std::string filenameInBack = QUOTE(BACK_INPUT_FILE);
            filenameInBack.insert(filenameInBack.length() - 4, ("_" + std::to_string(i) + "_0"));
            back_i[i] = input_plio::create("PLIO_back_in_" + std::to_string(i), aiePlioWidth, filenameInBack);
            connect<>(back_i[i].out[0], fftGraph.back_i[i]);

            std::string filenameOutFront = QUOTE(FRONT_OUTPUT_FILE);
            filenameOutFront.insert(filenameOutFront.length() - 4, ("_" + std::to_string(i) + "_0"));
            front_o[i] = output_plio::create("PLIO_front_out_" + std::to_string(i), aiePlioWidth, filenameOutFront);
            connect<>(fftGraph.front_o[i], front_o[i].in[0]);

            std::string filenameOutBack = QUOTE(BACK_OUTPUT_FILE);
            filenameOutBack.insert(filenameOutBack.length() - 4, ("_" + std::to_string(i) + "_0"));
            back_o[i] = output_plio::create("PLIO_back_out_" + std::to_string(i), aiePlioWidth, filenameOutBack);
            connect<>(fftGraph.back_o[i], back_o[i].in[0]);
        }
    };
};
}
}
}
};

#endif // _DSPLIB_TL_HPP_