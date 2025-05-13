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

/* This file holds the definition of the test harness for the Single Rate
   Asymmetrical FIR reference model graph.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"
#include "device_defs.h"

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

class test_graph : public graph {
   private:
    COEFF_TYPE taps[FIR_LEN];

   public:
    // When just a duplicate (windows dual ip), use only one plio per ssr port on input.
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR*(NUM_OUTPUTS)> out;

#if (USE_COEFF_RELOAD == 1)
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    using uut_g = dsplib::fir::sr_asym::UUT_GRAPH<DATA_TYPE,
                                                  COEFF_TYPE,
                                                  FIR_LEN,
                                                  SHIFT,
                                                  ROUND_MODE,
                                                  INPUT_WINDOW_VSIZE,
                                                  CASC_LEN,
                                                  USE_COEFF_RELOAD,
                                                  NUM_OUTPUTS,
                                                  DUAL_IP,
                                                  PORT_API,
                                                  P_SSR, // Note P_SSR forced to 1 for REF
                                                  SAT_MODE>;

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printConfig();

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
            taps_gen.gen(COEFF_STIM_TYPE, taps);
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

#ifdef USING_UUT
        static constexpr int kMinLen = uut_g::getMinCascLen<FIR_LEN, PORT_API, DATA_TYPE, COEFF_TYPE, P_SSR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kOptLen = uut_g::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, NUM_OUTPUTS, P_SSR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif
#endif

#if (USE_COEFF_RELOAD != 0) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
#endif

#if (USE_COEFF_RELOAD == 0)
        // Static coefficients
        uut_g firGraph(m_taps_v);
#elif (USE_COEFF_RELOAD == 1)
        // RTP coefficients
        uut_g firGraph;
#elif (USE_COEFF_RELOAD == 2)
#if (USE_COMPILE_TIME_COEFFS == 1)
        // Initialize with Constructor created coefficients
        uut_g firGraph(m_taps_v);
#else // Multi-kernel, static coefficients
        // Initialize with Coefficient array passed during runtime, i.e. initialize compilation.
        uut_g firGraph;
#endif
#else
        //  default. Pass coeffs through graph constructor.
        uut_g firGraph(m_taps_v);
#endif
// Connect two identical FIRs to ensure we don't get any errors regarding interoperability.
// don't mix up num_outputs to different number of inputs
#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g firGraph2;
#else // Multi-kernel, static coefficients
        uut_g firGraph2(m_taps_v);
#endif
#endif
        // Make plio connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

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

        const int MAX_PING_PONG_SIZE = __DATA_MEM_BYTES__ / 2;
        const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_WINDOW_VSIZE / P_SSR) * sizeof(DATA_TYPE));
        if (bufferSize > MAX_PING_PONG_SIZE || (SINGLE_BUF == 1 && PORT_API == 0)) {
            for (int ssr = 0; ssr < P_SSR; ssr++) {
                for (int ker = 0; ker < P_SSR * CASC_LEN; ker++) {
                    single_buffer(firGraph.getKernels()[CASC_LEN * P_SSR * ssr + ker].in[0]);
                    printf("INFO: Single Buffer Constraint applied to input buffer-0 of kernel %d.\n",
                           CASC_LEN * P_SSR * ssr + ker);

                    if (DUAL_IP == 1) {
                        single_buffer(firGraph.getKernels()[CASC_LEN * P_SSR * ssr + ker].in[1]);
                        printf("INFO: Single Buffer Constraint applied to input buffer-1 of kernel %d.\n",
                               CASC_LEN * P_SSR * ssr + ker);
                    }
                }

                single_buffer(firGraph.getKernels()[CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1)].out[0]);
                printf("INFO: Single Buffer Constraint applied to output buffer-0 of kernel %d.\n",
                       CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1));

                if (NUM_OUTPUTS == 2) {
                    single_buffer(firGraph.getKernels()[CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1)].out[1]);
                    printf("INFO: Single Buffer Constraint applied to output buffer-1 of kernel %d.\n",
                           CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1));
                }
            }
        }
#endif

#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
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
