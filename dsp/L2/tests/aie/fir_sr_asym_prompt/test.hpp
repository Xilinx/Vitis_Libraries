/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#include "fir_sr_asym_native_generated_graph/fir_sr_asym_generated_graph.h"
#include "ref_coeff.h"

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
   public:
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;

#ifdef USING_UUT
    // Use Generated Graph class
    using uut = fir_sr_asym_native_generated_graph;
#else
    // Use Graph class directly for reference model
    using uut = dsplib::fir::sr_asym::UUT_GRAPH<DATA_TYPE,
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
                                                P_SSR,
                                                SAT_MODE>;
#endif

#if (USE_COEFF_RELOAD == 1)
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    // kRefTaps (ref_coeff.h) is a literal C++ array generated from the same
    // requested coefficients the UUT's generated graph already embeds --
    // not read from a file at run time.
    std::vector<COEFF_TYPE> m_taps_v = kRefTaps;

    uut firGraph;

// Reload configs (UUT or ref) construct firGraph with no taps -- coefficients
// arrive later via the RTP reload port, matching fir_sr_asym.py's
// generate_graph() (its no-arg constructor also passes filter() with no
// taps when TP_USE_COEFF_RELOAD == 1). Static configs (UUT or ref) always
// pass m_taps_v -- the UUT's generated graph has an explicit
// std::vector<TT_COEFF>-taking constructor precisely so both graphs
// build from the identical vector, not two independently-sourced ones.
#if (USE_COEFF_RELOAD == 1)
    test_graph(){
#else
    test_graph() : firGraph(m_taps_v) {
#endif
        printConfig();

#if (USE_COEFF_RELOAD != 0) // Reloadable coefficients
    static_assert(NITER % 2 == 0,
                  "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
#endif

    // Match fir_helper.mk and ssr_split_zip.pl, which generate 128-bit PLIO lines.
    static constexpr int plioBitWidth = 128;
    createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES, input_plio, plioBitWidth>(in, QUOTE(INPUT_FILE), "in");
    createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1), output_plio, plioBitWidth>(out, QUOTE(OUTPUT_FILE), "out");

    for (unsigned int i = 0; i < P_SSR; ++i) {
        unsigned int plioBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
        connect<>(in[plioBaseIdx].out[0], firGraph.in[i]);
#if (DUAL_IP == 1)
        // if not using interleaved streams, just use a duplicate of in1
        connect<>(in[plioBaseIdx + DUAL_INPUT_SAMPLES].out[0], firGraph.in2[i]);
#endif

        connect<>(firGraph.out[i], out[plioBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2)
        // Always feed to seperate plio
        connect<>(firGraph.out2[i], out[plioBaseIdx + 1].in[0]);
#endif
    }

#ifdef USING_UUT

#if (USE_CUSTOM_CONSTRAINT == 1)
    // Place location constraints
    int LOC_XBASE = 20;
    int LOC_YBASE = 0;
    for (int outPath = 0; outPath < P_SSR; outPath++) {
        for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
            for (int i = 1; i < CASC_LEN; i++) {
                location<kernel>(*firGraph.filter.getKernels(outPath, inPhase, i)) =
                    tile(LOC_XBASE + inPhase * CASC_LEN + i, LOC_YBASE + 2 * outPath);
            }
        }
    }

    // Overwrite the internally calculated fifo depth with some other value.
    for (int outPath = 0; outPath < P_SSR; outPath++) {
        for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
            for (int i = 0; i < CASC_LEN; i++) {
                connect<stream, stream>* net = firGraph.filter.getInNet(outPath, inPhase, i);
                fifo_depth(*net) = 256 + 16 * outPath + 16 * inPhase + 16 * i;
#if (DUAL_IP == 1)
                connect<stream, stream>* net2 = firGraph.filter.getIn2Net(outPath, inPhase, i);
                fifo_depth(*net2) = 512 + 8 + 16 * outPath + 16 * inPhase + 16 * i;
#endif
            }
        }
    }
#endif
    // Use single_buffer constraint
    const int MAX_PING_PONG_SIZE = __DATA_MEM_BYTES__ / 2;
    const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_WINDOW_VSIZE / P_SSR) * sizeof(DATA_TYPE));
    if (bufferSize > MAX_PING_PONG_SIZE || (SINGLE_BUF == 1 && PORT_API == 0)) {
        for (int ssr = 0; ssr < P_SSR; ssr++) {
            for (int ker = 0; ker < P_SSR * CASC_LEN; ker++) {
                single_buffer(firGraph.filter.getKernels()[CASC_LEN * P_SSR * ssr + ker].in[0]);
                printf("INFO: Single Buffer Constraint applied to input buffer-0 of kernel %d.\n",
                       CASC_LEN * P_SSR * ssr + ker);

                if (DUAL_IP == 1) {
                    single_buffer(firGraph.filter.getKernels()[CASC_LEN * P_SSR * ssr + ker].in[1]);
                    printf("INFO: Single Buffer Constraint applied to input buffer-1 of kernel %d.\n",
                           CASC_LEN * P_SSR * ssr + ker);
                }
            }

            single_buffer(firGraph.filter.getKernels()[CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1)].out[0]);
            printf("INFO: Single Buffer Constraint applied to output buffer-0 of kernel %d.\n",
                   CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1));

            if (NUM_OUTPUTS == 2) {
                single_buffer(firGraph.filter.getKernels()[CASC_LEN * P_SSR * ssr + (CASC_LEN * P_SSR - 1)].out[1]);
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
    if (firGraph.filter.getKernels() != NULL) {
        printf("KERNEL_ARCHS: [");
        int arch = firGraph.filter.getKernelArchs();
        printf("%d", arch);
        printf("]\n");
    }
#endif
    printf("========================\n");
}
};
}
}
}
}
;

#endif // _DSPLIB_TEST_HPP_
