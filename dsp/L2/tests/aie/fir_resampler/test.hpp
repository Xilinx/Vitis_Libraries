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

/*
This file holds the declaraion of the test harness graph class for the
fir_resampler graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_resampler_graph
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
    // The taps array holds the coefficient values to be passed as input to UUT
    COEFF_TYPE taps[FIR_LEN];

   public:
#ifndef P_SSR
    static constexpr int P_SSR = 1; // until SSR is supported
#endif
#ifdef USING_UUT
    static constexpr int DUAL_INPUT_SAMPLES = (PORT_API == 1) && (DUAL_IP == 1) ? 1 : 0;
#else
    static constexpr int DUAL_INPUT_SAMPLES = 0;
#endif
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;

#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    template <unsigned int windowVSize = INPUT_SAMPLES>
    using uut_g = dsplib::fir::resampler::UUT_GRAPH<DATA_TYPE,
                                                    COEFF_TYPE,
                                                    FIR_LEN,
                                                    INTERPOLATE_FACTOR,
                                                    DECIMATE_FACTOR,
                                                    SHIFT,
                                                    ROUND_MODE,
                                                    windowVSize,
                                                    CASC_LEN,
                                                    USE_COEFF_RELOAD,
                                                    NUM_OUTPUTS,
                                                    DUAL_IP,
                                                    PORT_API>;
    // Constructor
    test_graph() {
        printConfig();

        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, FIR_LEN, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
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

#ifdef USING_UUT
        static constexpr int kMaxTaps = uut_g<INPUT_SAMPLES>::getMaxTapsPerKernel<PORT_API, DATA_TYPE>();
        printf("For this config the Maximum Taps Per Kernel is %d\n", kMaxTaps);
        static constexpr int kMinLen = uut_g<INPUT_SAMPLES>::getMinCascLen<FIR_LEN, PORT_API, DATA_TYPE, COEFF_TYPE,
                                                                           INTERPOLATE_FACTOR, DECIMATE_FACTOR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kRawOptTaps =
            uut_g<INPUT_SAMPLES>::getOptTapsPerKernel<DATA_TYPE, COEFF_TYPE, DUAL_IP + 1>();
        static constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
        printf("For this config the Optimal Taps (streaming) Per Kernel is %d\n", kOptTaps);
        static constexpr int kOptLen =
            uut_g<INPUT_SAMPLES>::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, NUM_OUTPUTS,
                                                INTERPOLATE_FACTOR, DECIMATE_FACTOR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif
#endif

// FIR sub-graph

#if (USE_COEFF_RELOAD != 0) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
#endif

#if (USE_COEFF_RELOAD == 0)
        // Static coefficients
        uut_g<INPUT_SAMPLES> firGraph(m_taps_v);
#elif (USE_COEFF_RELOAD == 1)
        // RTP coefficients
        uut_g<INPUT_SAMPLES> firGraph;
#else
        //  default. Pass coeffs through graph constructor.
        uut_g<INPUT_SAMPLES> firGraph(m_taps_v);
#endif
// Connect two identical FIRs to ensure we don't get any errors regarding interoperability.
// don't mix up num_outputs to different number of inputs
#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g<INPUT_SAMPLES * INTERPOLATE_FACTOR / DECIMATE_FACTOR> firGraph2;
#else // Multi-kernel, static coefficients
        uut_g<INPUT_SAMPLES * INTERPOLATE_FACTOR / DECIMATE_FACTOR> firGraph2(m_taps_v);
#endif
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        connect<>(in[0].out[0], firGraph.in[0]);
#if (DUAL_IP == 1)
        connect<>(in[1].out[0],
                  firGraph.in2[0]); //'dual' streams carry independent data which when combined are one channel
#endif
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
        // Chained connections mutually explusive with multiple outputs.
        connect<>(firGraph.out[0], firGraph2.in[0]);
        connect<>(firGraph2.out[0], out[0].in[0]);
#else
        connect<>(firGraph.out[0], out[0].in[0]);
#if (NUM_OUTPUTS == 2)
        connect<>(firGraph.out2[0], out[1].in[0]);
#endif
#endif
#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
#endif

#ifdef USING_UUT
#if (USE_CUSTOM_CONSTRAINT == 1)
        // place location constraints
        int LOC_XBASE = 20;
        int LOC_YBASE = 0;
        for (int i = 1; i < CASC_LEN; i++) {
            location<kernel>(*firGraph.getKernels(i)) = tile(LOC_XBASE + i, LOC_YBASE);
        }

        // overwrite the internally calculated fifo depth with some other value.
        for (int i = 1; i < CASC_LEN; i++) {
            connect<stream, stream>* net = firGraph.getInNet(i);
            fifo_depth(*net) = 384 + 16 * i;
#if (DUAL_IP == 1)
            connect<stream, stream>* net2 = firGraph.getIn2Net(i);
            fifo_depth(*net2) = 384 + 16 * i;
#endif
        }
#endif

        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        const int inputBufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
        const int outputBufferSize = (PORT_API == 1 ? 0
                                                    : (((INPUT_SAMPLES * INTERPOLATE_FACTOR) / DECIMATE_FACTOR)) *
                                                          sizeof(DATA_TYPE)); // FIR_LEN margin does not need to be
                                                                              // considered unless there's a downstream
                                                                              // FIR with margin.

        if (inputBufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()[0].in[0]);
        }
        if (outputBufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[0]);
#if (NUM_OUTPUTS == 2)
            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[1]);
#endif
        }

        // use default ping-pong buffer, unless requested buffer exceeds memory module size
        static_assert(inputBufferSize < MEMORY_MODULE_SIZE,
                      "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                      "Module size of 32kB");
        static_assert(outputBufferSize < MEMORY_MODULE_SIZE,
                      "ERROR: Output Window size (based on requrested window size and rate change) exceeds Memory "
                      "Module size of 32kB");
#endif

#ifdef USING_UUT
        // Report out for AIE Synthesizer QoR harvest
        if (&firGraph.getKernels()[0] != NULL) {
            printf("KERNEL_ARCHS: [");
            int arch = firGraph.getKernelArchs();
            printf("%d", arch);
            printf("]\n");
            printf("polyphaseLaneAlias = %d\n", firGraph.getPolyphaseLaneAlias());
        }
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
