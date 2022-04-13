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

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"
#include "fir_common_traits.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_decimate_sym_graph
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
    static constexpr unsigned int kNumTaps = (FIR_LEN + 1) / 2;
    COEFF_TYPE taps[kNumTaps];

   public:
#ifndef P_SSR
    static constexpr int P_SSR = 1; // until SSR is supported
#endif
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

    COEFF_TYPE m_taps[2][kNumTaps];
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
        printf("Round_mode        = %d \n", ROUND_MODE);
        printf("Dual inputs       = %d \n", DUAL_IP);
        printf("Coeff reload      = %d \n", USE_COEFF_RELOAD);
        printf("Number of outputs = %d \n", NUM_OUTPUTS);
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Coeff type        = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
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
            taps_gen.gen(STIM_TYPE, taps);
            for (int i = 0; i < kNumTaps; i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }
        // Copy taps from C++ array into std::vector
        for (int i = 0; i < kNumTaps; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

        static constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kDecSym, PORT_API, DATA_TYPE>();
        printf("For this config the Maximum Taps Per Kernel is %d\n", kMaxTaps);
        static constexpr int kMinLen = fir::fnGetMinCascLenDecSym<FIR_LEN, PORT_API, DATA_TYPE, DECIMATE_FACTOR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<DATA_TYPE, COEFF_TYPE, DUAL_IP + 1>();
        static constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
        printf("For this config the Optimal Taps (streaming) Per Kernel is %d\n", kOptTaps);
        static constexpr int kOptLen =
            fir::fnGetOptCascLenDecSym<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, NUM_OUTPUTS, DECIMATE_FACTOR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
        dsplib::fir::decimate_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                             INPUT_SAMPLES, CASC_LEN, DUAL_IP, USE_COEFF_RELOAD_TRUE, NUM_OUTPUTS,
                                             PORT_API>
            firGraph;
#else // Static coefficients
        dsplib::fir::decimate_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                             INPUT_SAMPLES, CASC_LEN, DUAL_IP, USE_COEFF_RELOAD_FALSE, NUM_OUTPUTS,
                                             PORT_API>
            firGraph(m_taps_v);
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(
            in, QUOTE(INPUT_FILE)); // fir_sr_sym does not support dual input
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE));

        // Size of window in Bytes.
        connect<>(in[0].out[0], firGraph.in);
#if (DUAL_IP == 1)
        if (PORT_API == 0) {                       // Window
            connect<>(in[0].out[0], firGraph.in2); // clone/broadcast input
        } else {
            connect<>(in[1].out[0],
                      firGraph.in2); //'dual' streams carry independent data which when combined are one channel
        }
#endif
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
        // Chained connections mutually explusive with multiple outputs.
        dsplib::fir::decimate_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                             INPUT_SAMPLES / DECIMATE_FACTOR, CASC_LEN>
            firGraph2(m_taps_v);
        connect<>(firGraph.out, firGraph2.in);
        connect<>(firGraph2.out, out[0].in[0]);
#else
        connect<>(firGraph.out, out[0].in[0]);
#if (NUM_OUTPUTS == 2)
        connect<>(firGraph.out2, out[1].in[0]);
#endif
#endif
#if (USE_COEFF_RELOAD == 1)
        connect<>(coeff, firGraph.coeff);
#endif

#ifdef USING_UUT
        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        if (PORT_API == 0) {
            const int bufferSize = ((FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
            if (bufferSize > MAX_PING_PONG_SIZE) {
                single_buffer(firGraph.getKernels()->in[0]);
            } else {
                // use default ping-pong buffer, unless requested buffer exceeds memory module size
                static_assert(bufferSize < MEMORY_MODULE_SIZE,
                              "ERROR: Input Window size (based on requrested window size and FIR length margin) "
                              "exceeds Memory Module size of 32kB");
            }
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
