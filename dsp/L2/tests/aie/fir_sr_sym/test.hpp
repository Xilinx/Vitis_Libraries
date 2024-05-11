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
Single Rate Symmetrical FIR graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"

#include "test_utils.hpp"
#include "fir_common_traits.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   private:
    static constexpr unsigned int kNumTaps = (FIR_LEN + 1) / 2;

    COEFF_TYPE taps[kNumTaps];

   public:
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in; // 0? dual_ip - not supported by sr_sym
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;          // NUM_OUTPUTS forces to 1 for ref

    using uut_g = dsplib::fir::sr_sym::UUT_GRAPH<DATA_TYPE,
                                                 COEFF_TYPE,
                                                 FIR_LEN,
                                                 SHIFT,
                                                 ROUND_MODE,
                                                 INPUT_SAMPLES,
                                                 CASC_LEN,
                                                 DUAL_IP,
                                                 USE_COEFF_RELOAD,
                                                 NUM_OUTPUTS,
                                                 PORT_API,
                                                 P_SSR,
                                                 SAT_MODE>;

#if (USE_COEFF_RELOAD == 1)
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    COEFF_TYPE m_taps[2][kNumTaps];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printConfig();

        namespace dsplib = xf::dsp::aie;

        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, kNumTaps, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            kNumTaps; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = kNumTaps - 1; // Always overwrite the last coeff only.
#endif                            // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(COEFF_STIM_TYPE, taps);
            for (int i = 0; i < kNumTaps; i++) {
                m_taps[j][i] = taps[i];
                // if (i == error_tap && j == 1) {
                if (j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }
        // Copy taps from C++ array into std::vector
        for (int i = 0; i < kNumTaps; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

#ifdef USING_UUT
        static constexpr int kMinLen = uut_g::getMinCascLen<FIR_LEN, PORT_API, DATA_TYPE, COEFF_TYPE, P_SSR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kOptLen =
            uut_g::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, NUM_OUTPUTS, P_SSR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif
#endif

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
        uut_g firGraph;
#else // Static coefficients
        uut_g firGraph(m_taps_v);
#endif

#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g firGraph2;
#else // Multi-kernel, static coefficients
        uut_g firGraph2(m_taps_v);
#endif
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioIpBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
            unsigned int plioOpBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
            connect<>(in[plioIpBaseIdx].out[0], firGraph.in[i]);

#if (DUAL_IP == 1 && PORT_API == 0) // dual input to avoid contention
            // if not using interleaved streams, just use a duplicate of in1
            connect<>(in[0].out[0], firGraph.in2[i]); // firGraph.in2[i] later. Script performs clone. If it doesn't,
                                                      // remove DUAL_INPUT_SAMPLES here.
#endif
#if (DUAL_IP == 1 && PORT_API == 1)
            connect<>(in[plioIpBaseIdx + 1].out[0], firGraph.in2[i]); // firGraph.in2[i] once graph supports array ports
#endif

#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
            // Chained connections mutually exclusive with multiple outputs.
            connect<>(firGraph.out[i], firGraph2.in[i]);
            connect<>(firGraph2.out[i], out[0].in[0]);
#else
            connect<>(firGraph.out[i], out[plioOpBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2)
#ifdef USING_UUT
            connect<>(firGraph.out2[i], out[plioOpBaseIdx + 1].in[0]);
#endif // USING_UUT
#endif
#endif
        }

#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
#endif

#ifdef USING_UUT
        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
#if (P_SSR == 1) // Check buffer size is within limits, when buffer interface is used.
        const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
        if (bufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()->in[0]);
            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[0]);
            if (DUAL_IP == 1) {
                single_buffer(firGraph.getKernels()->in[1]);
            }
            if (NUM_OUTPUTS == 2) {
                single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[1]);
            }

        } else {
            // use default ping-pong buffer, unless requested buffer exceeds memory module size
            static_assert(bufferSize < MEMORY_MODULE_SIZE,
                          "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds "
                          "Memory Module size of 32kB");
        }
#endif
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
