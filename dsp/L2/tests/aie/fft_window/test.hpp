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

// This file holds the definition of the test harness for the fft window graph.

#include <adf.h>
#include <vector>
#include <array>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#include "device_defs.h"
#include "fft_window_fns.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fft_window_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

template <unsigned int ssr, unsigned int dual, typename plioType>
void createPLIOFileConnections(std::array<plioType, ssr*(dual + 1)>& plioPorts, std::string filename) {
    for (unsigned int ssrIdx = 0; ssrIdx < ssr; ++ssrIdx) {
        for (unsigned int dualIdx = 0; dualIdx < (dual + 1); ++dualIdx) {
            std::string filenameInternal = filename;

#if (NUM_OUTPUTS == 2 && PORT_API == 0)
            if (dual == 1 && dualIdx == 1) {
                filenameInternal.insert(filenameInternal.length() - 4, ("_clone"));
            } else {
#ifdef USING_UUT
                // Insert SSR index and dual stream index into filename before extension (.txt)
                filenameInternal.insert(filenameInternal.length() - 4,
                                        ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
#endif
            }
#elif defined(USING_UUT)
            // Insert SSR index and dual stream index into filename before extension (.txt)
            filenameInternal.insert(filenameInternal.length() - 4,
                                    ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
#endif
            plioPorts[ssrIdx * (dual + 1) + dualIdx] =
                plioType::create("PLIO_" + filenameInternal, adf::plio_64_bits, filenameInternal);
        }
    }
}

class test_graph : public graph {
   private:
    std::array<COEFF_TYPE, POINT_SIZE*(DYN_PT_SIZE + 1)> weights;

   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kAPIFactor =
        API_IO == 0 ? 1 : kStreamsPerTile; // two ports per kernel for stream(AIE1), 1 port per kernel for windows.
    std::array<input_plio, UUT_SSR * kAPIFactor> in;
    std::array<output_plio, UUT_SSR * kAPIFactor> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== FFT Window test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("COEFF type        = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
        printf("POINT_SIZE        = %d \n", POINT_SIZE);
        printf("WINDOW_VSIZE      = %d \n", WINDOW_VSIZE);
        printf("SHIFT             = %d \n", SHIFT);
        printf("API_IO            = %d \n", API_IO);
        printf("SSR               = %d \n", UUT_SSR);
        printf("DYN_PT_SIZE       = %d \n", DYN_PT_SIZE);
        printf("ROUND_MODE        = %d \n", ROUND_MODE);
        printf("SAT_MODE          = %d \n", SAT_MODE);

        if
            constexpr(DYN_PT_SIZE == 0) {
                switch (WINDOW_CHOICE) {
                    case 0:
                        dsplib::fft::windowfn::getHammingWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[0], POINT_SIZE);
                        break;
                    case 1:
                        dsplib::fft::windowfn::getHannWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[0], POINT_SIZE);
                        break;
                    case 2:
                        dsplib::fft::windowfn::getBlackmanWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[0], POINT_SIZE);
                        break;
                    case 3:
                        dsplib::fft::windowfn::getKaiserWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[0], POINT_SIZE);
                        break;
                    default:
                        printf("ERROR: unknown window choice. Defaulting to Hamming.\n");
                        dsplib::fft::windowfn::getHammingWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[0], POINT_SIZE);
                        break;
                        break;
                }
            }
        else {
            int tableBase = 0;
            for (int ptSize = POINT_SIZE; ptSize >= 16 * UUT_SSR; ptSize = ptSize / 2) {
                switch (WINDOW_CHOICE) {
                    case 0:
                        dsplib::fft::windowfn::getHammingWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[tableBase], ptSize);
                        break;
                    case 1:
                        dsplib::fft::windowfn::getHannWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[tableBase], ptSize);
                        break;
                    case 2:
                        dsplib::fft::windowfn::getBlackmanWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[tableBase], ptSize);
                        break;
                    case 3:
                        dsplib::fft::windowfn::getKaiserWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[tableBase], ptSize);
                        break;
                    default:
                        printf("ERROR: unknown window choice. Defaulting to Hamming.\n");
                        dsplib::fft::windowfn::getHammingWindow<COEFF_TYPE>((COEFF_TYPE*)&weights[tableBase], ptSize);
                        break;
                        break;
                }
                tableBase += ptSize;
            }
        }

        // FFTWindow sub-graph
        dsplib::fft::windowfn::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, POINT_SIZE, WINDOW_VSIZE, SHIFT, API_IO, UUT_SSR,
                                         DYN_PT_SIZE, ROUND_MODE, SAT_MODE>
            fftWindowGraph(weights);

        // Make connections
        for (int i = 0; i < UUT_SSR * kAPIFactor; i++) {
            std::string filenameIn = QUOTE(INPUT_FILE);
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_64_bits, filenameIn);
            connect<>(in[i].out[0], fftWindowGraph.in[i]);

            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(fftWindowGraph.out[i], out[i].in[0]);
// valconnect<>(fftWindowGraph.out[i], fftGraph.in[i]);
// valconnect<>(fftGraph.out[i], out[i].in[0]);
#ifdef USING_UUT
#if (SINGLE_BUF == 1 && API_IO == 0)
            single_buffer(fftWindowGraph.getKernels()[i].in[0]);
            single_buffer(fftWindowGraph.getKernels()[i].out[0]);
            printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n", i);
#endif
#endif
        }

        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
