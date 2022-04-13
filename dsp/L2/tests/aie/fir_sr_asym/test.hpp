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
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/* This file holds the definition of the test harness for the Single Rate
   Asymmetrical FIR reference model graph.
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
#define UUT_GRAPH fir_sr_asym_graph
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
                plioType::create("PLIO_" + filenameInternal, adf::plio_32_bits, filenameInternal);
        }
    }
}

class test_graph : public graph {
   private:
    COEFF_TYPE taps[FIR_LEN];

    kernel casc_in_widget;
    kernel casc_out_widget;

   public:
    // When just a duplicate (windows dual ip), use only one plio per ssr port on input.
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR*(NUM_OUTPUTS)> out;

#if (USE_COEFF_RELOAD == 1)
    port<input> coeff; // todo - ssr
#endif
    template <bool COEFF_RELOAD>
    using uut_g = dsplib::fir::sr_asym::UUT_GRAPH<DATA_TYPE,
                                                  COEFF_TYPE,
                                                  FIR_LEN,
                                                  SHIFT,
                                                  ROUND_MODE,
                                                  INPUT_WINDOW_VSIZE,
                                                  CASC_LEN,
                                                  COEFF_RELOAD,
                                                  NUM_OUTPUTS,
                                                  DUAL_IP,
                                                  PORT_API,
                                                  P_SSR>; // Note P_SSR forced to 1 for REF

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples    = %d \n", INPUT_SAMPLES);
        printf("Input window [B] = %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Input margin     = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
        printf("Output samples   = %d \n", OUTPUT_SAMPLES);
        printf("FIR Length       = %d \n", FIR_LEN);
        printf("Shift            = %d \n", SHIFT);
        printf("ROUND_MODE       = %d \n", ROUND_MODE);
        printf("CASC_LEN         = %d \n", CASC_LEN);
        printf("USE_COEFF_RELOAD = %d \n", USE_COEFF_RELOAD);
        printf("NUM_OUTPUTS      = %d \n", NUM_OUTPUTS);
        printf("PORT_API         = %d \n", PORT_API);
        printf("P_SSR            = %d \n", P_SSR);
        printf("Data type        = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Coeff type       = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
        namespace dsplib = xf::dsp::aie;
        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, FIR_LEN, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(0);
        int error_tap =
            rand() %
            FIR_LEN; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = FIR_LEN - 1; // Always overwrite the last coeff only.
#endif                           // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(STIM_TYPE, taps);
            for (int i = 0; i < FIR_LEN; i++) {
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

        static constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kSrAsym, PORT_API, DATA_TYPE>();
        printf("For this config the Maximum Taps Per Kernel is %d\n", kMaxTaps);
        static constexpr int kMinLen = fir::fnGetMinCascLenSrAsym<FIR_LEN, PORT_API, DATA_TYPE>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<DATA_TYPE, COEFF_TYPE, DUAL_IP + 1>();
        static constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
        printf("For this config the Optimal Taps (streaming) Per Kernel is %d\n", kOptTaps);
        static constexpr int kOptLen =
            fir::fnGetOptCascLenSrAsym<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, NUM_OUTPUTS>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif

#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
        uut_g<USE_COEFF_RELOAD_TRUE> firGraph;
#else // Multi-kernel, static coefficients
        uut_g<USE_COEFF_RELOAD_FALSE> firGraph(m_taps_v);
#endif
// Connect two identical FIRs to ensure we don't get any errors regarding interoperability.
// don't mix up num_outputs to different number of inputs
#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g<USE_COEFF_RELOAD_TRUE> firGraph2;
#else // Multi-kernel, static coefficients
        uut_g<USE_COEFF_RELOAD_FALSE> firGraph2(m_taps_v);
#endif
#endif
        // Make plio connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE));
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE));

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
            connect<>(in[plioBaseIdx].out[0], firGraph.in[i]);
#if (DUAL_IP == 1)
            // if not using interleaved streams, just use a duplicate of in1
            connect<>(in[plioBaseIdx + DUAL_INPUT_SAMPLES].out[0], firGraph.in2[i]);
#endif

// Ensure we can connect to another element without error
#if (USE_CHAIN == 1)
            connect<>(firGraph.out[i], firGraph2.in[i]);
            connect<>(firGraph2.out[i], out[plioBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)
            connect<>(firGraph.out2[i], firGraph2.in2[i]);
            connect<>(firGraph2.out2[i], out[plioBaseIdx + 1].in[0]);
#endif
#else
            connect<>(firGraph.out[i], out[plioBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2)
            // Always feed to seperate plio
            connect<>(firGraph.out2[i], out[plioBaseIdx + 1].in[0]);
#endif
#endif
        }

#ifdef USING_UUT

#if (USE_CUSTOM_CONSTRAINT == 1)
        // place location constraints
        int LOC_XBASE = 20;
        int LOC_YBASE = 0;
        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 1; i < CASC_LEN; i++) {
                    location<kernel>(*firGraph.getKernels(outPath, inPhase, i)) =
                        tile(LOC_XBASE + inPhase * CASC_LEN + i, LOC_YBASE + 2 * outPath);
                }
            }
        }

        // overwrite the internally calculated fifo depth with some other value.

        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 0; i < CASC_LEN; i++) {
                    connect<stream, stream>* net = firGraph.getInNet(outPath, inPhase, i);
                    fifo_depth(*net) = 256 + 16 * outPath + 16 * inPhase + 16 * i;
#if (DUAL_IP == 1)
                    connect<stream, stream>* net2 = firGraph.getIn2Net(outPath, inPhase, i);
                    fifo_depth(*net2) = 512 + 8 + 16 * outPath + 16 * inPhase + 16 * i;
#endif
                }
            }
        }
#endif

        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_WINDOW_VSIZE) * sizeof(DATA_TYPE));
        if (bufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()->in[0]);
        }
        // use default ping-pong buffer, unless requested buffer exceeds memory module size
        static_assert(bufferSize < MEMORY_MODULE_SIZE,
                      "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                      "Module size of 32kB");
#endif

#if (USE_COEFF_RELOAD == 1)
        connect<>(coeff, firGraph.coeff[0]);
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
