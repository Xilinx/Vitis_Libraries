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
fir_decimate_asym graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"
#include "fir_common_traits.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_decimate_asym_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

namespace dsplib = xf::dsp::aie;

template <unsigned int ssr, unsigned int dual, typename plioType>
void createPLIOFileConnections(std::array<plioType, ssr*(dual + 1)>& plioPorts,
                               std::string filename,
                               std::string plioDescriptor = "in") {
    for (unsigned int ssrIdx = 0; ssrIdx < ssr; ++ssrIdx) {
        for (unsigned int dualIdx = 0; dualIdx < (dual + 1); ++dualIdx) {
            std::string filenameInternal = filename;
            // Insert SSR index and dual stream index into filename before extension (.txt)
            filenameInternal.insert(filenameInternal.length() - 4,
                                    ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
            plioPorts[ssrIdx * (dual + 1) + dualIdx] = plioType::create(
                "PLIO_" + plioDescriptor + "_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx),
                adf::plio_32_bits, filenameInternal);
        }
    }
}

class test_graph : public graph {
   private:
    COEFF_TYPE taps[FIR_LEN];

   public:
#ifdef USING_UUT
    static constexpr int DUAL_INPUT_SAMPLES = (PORT_API == 1) && (DUAL_IP == 1) ? 1 : 0;
#else
    static constexpr int DUAL_INPUT_SAMPLES = 0;
#endif
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in; // 0? dual_ip - not supported by sr_sym
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;          // NUM_OUTPUTS forces to 1 for ref

#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
    port<input> coeff;
#endif

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples     = %d \n", INPUT_SAMPLES);
        printf("Input window [B]  = %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Input margin      = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
        printf("Output samples    = %d \n", OUTPUT_SAMPLES);
        printf("FIR Length        = %d \n", FIR_LEN);
        printf("Decimation factor = %d \n", DECIMATE_FACTOR);
        printf("Shift             = %d \n", SHIFT);
        printf("ROUND_MODE        = %d \n", ROUND_MODE);
        printf("CASC_LEN          = %d \n", CASC_LEN);
        printf("USE_COEFF_RELOAD  = %d \n", USE_COEFF_RELOAD);
        printf("NUM_OUTPUTS       = %d \n", NUM_OUTPUTS);
        printf("DUAL_IP           = %d \n", DUAL_IP);
        printf("PORT_API          = %d \n", PORT_API);
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Coeff type        = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
        namespace dsplib = xf::dsp::aie;
        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, FIR_LEN, 0> taps_gen;
        srand(115552);
        int error_tap =
            rand() %
            FIR_LEN; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(STIM_TYPE, taps);
            for (int i = 0; i < (FIR_LEN); i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }

        // Copy taps from C++ array into std::vector
        for (int i = 0; i < FIR_LEN; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
        dsplib::fir::decimate_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                              INPUT_SAMPLES, CASC_LEN, true, NUM_OUTPUTS, DUAL_IP, PORT_API, P_SSR>
            firGraph;
#else // Multi-kernel, static coefficients
        dsplib::fir::decimate_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                              INPUT_SAMPLES, CASC_LEN, false, NUM_OUTPUTS, DUAL_IP, PORT_API, P_SSR>
            firGraph(m_taps_v);
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioBaseIdxIn = i * (DUAL_INPUT_SAMPLES + 1);
            unsigned int plioBaseIdxOut = i * (NUM_OUTPUTS);

            connect<>(in[plioBaseIdxIn].out[0], firGraph.in[i]);
#if (DUAL_IP == 1)
            connect<>(in[plioBaseIdxIn + 1].out[0], firGraph.in2[i]);
#endif
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
            // Chained connections mutually explusive with multiple outputs.
            dsplib::fir::decimate_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                                  INPUT_SAMPLES / DECIMATE_FACTOR, CASC_LEN>
                firGraph2(m_taps_v);
            connect<>(firGraph.out[plioBaseIdxOut], firGraph2.in[plioBaseIdxIn]);
            connect<>(firGraph2.out[plioBaseIdxOut], out[plioBaseIdxOut].in[0]);
#else
            connect<>(firGraph.out[i], out[plioBaseIdxOut].in[0]);
#if (NUM_OUTPUTS == 2)
            connect<>(firGraph.out2[i], out[plioBaseIdxOut + 1].in[0]);
#endif
#endif
#if (USE_COEFF_RELOAD == 1)
            connect<>(coeff, firGraph.coeff[0]);
#endif
        }
#ifdef USING_UUT
#if (USE_CUSTOM_CONSTRAINT == 1)
        // place location constraints
        int LOC_XBASE = 20;
        int LOC_YBASE = 0;
        for (int i = 1; i < CASC_LEN; i++) {
            location<kernel>(*firGraph.getKernels(i)) = tile(LOC_XBASE + i, LOC_YBASE);
        }

        // overwrite the internally calculated fifo depth with some other value.
        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 1; i < CASC_LEN; i++) {
                    connect<stream, stream>* net = firGraph.getInNet(outPath, inPhase, i);
                    fifo_depth(*net) = 384 + 16 * i;
#if (DUAL_IP == 1)
                    connect<stream, stream>* net2 = firGraph.getIn2Net(outPath, inPhase, i);
                    fifo_depth(*net2) = 384 + 16 * i;
#endif
                }
            }
        }
#endif

        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
        if (bufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()->in[0]);
        }
        static_assert(bufferSize < MEMORY_MODULE_SIZE,
                      "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                      "Module size of 32kB");
#endif

#ifdef USING_UUT
        // Report out for AIE Synthesizer QoR harvest
        if (&firGraph.getKernels()[0] != NULL) {
            printf("KERNEL_ARCHS: [");
            int arch = firGraph.getKernelArchs();
            printf("%d", arch);
            printf("]\n");
        }
#endif
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
