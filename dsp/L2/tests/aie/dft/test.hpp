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

/*
This file holds the declaraion of the test harness graph class for the
dft graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "pkt_switch_graph.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifdef USING_UUT
#include "dft_native_generated_graph/dft_generated_graph.h"
#endif
#ifndef UUT_GRAPH
#define UUT_GRAPH dft_graph
#endif

// location constraints for POINT_SIZE=65536
#define LOC_XBASE 1
#define LOC_YBASE 0

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
   public:
    // NPORT used directly when packet switching is used or gets defined in uut_static_config.h
    static constexpr unsigned numInPlio = NPORT_I;
    static constexpr unsigned numOutPlio = NPORT_O;

    static constexpr int plioBitWidth = PLIO_WIDTH;
#if (USE_PKT_SWITCHING != 0)
    std::array<input_plio, numInPlio> in =
        createPLIOFileConnections<numInPlio, 0, input_plio, plioBitWidth>(PKT_INPUT_FILE, "in");
    std::array<output_plio, numOutPlio> out =
        createPLIOFileConnections<numOutPlio, 0, output_plio, plioBitWidth>(PKT_OUTPUT_FILE, "out");
#else
    std::array<input_plio, numInPlio> in =
        createPLIOFileConnections<numInPlio, 0, input_plio, plioBitWidth>(QUOTE(INPUT_FILE), "in");
    std::array<output_plio, numOutPlio> out =
        createPLIOFileConnections<numOutPlio, 0, output_plio, plioBitWidth>(QUOTE(OUTPUT_FILE), "out");
#endif

#ifdef USING_UUT
    // Use Generated Graph
    using uut = dft_native_generated_graph;
#else
    // Use Graph class directly for reference model
    using uut = xf::dsp::aie::fft::dft::UUT_GRAPH<DATA_TYPE,
                                                  TWIDDLE_TYPE,
                                                  POINT_SIZE,
                                                  FFT_NIFFT,
                                                  SHIFT,
                                                  CASC_LEN,
                                                  NUM_FRAMES,
                                                  ROUND_MODE,
                                                  SAT_MODE,
                                                  UUT_SSR>;
#endif

#if (USE_PKT_SWITCHING == 1)
// The pkt_switch_graph class in unsuitable for the DFT, due to the varied inputs and outputs per "SSR" configuration.
// Use the pkt_switch_input and pkt_switch_output classes instead.
// pkt_switch_graph<UUT_SSR, NPORT_I, NPORT_O, uut> dftGraph;
#elif (USE_PKT_SWITCHING == 2)
    uut dftGraph;
    pkt_switch_input<CASC_LEN, NPORT_I> dftGraphIn;
    pkt_switch_output<UUT_SSR, NPORT_O> dftGraphOut;
#else
    uut dftGraph;
#endif

    // Constructor
    test_graph()
#if (USE_PKT_SWITCHING == 2)
        : dftGraphOut(dftGraph.out, out)
#endif
    {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type             = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("Point size            = %d \n", POINT_SIZE);
        printf("Num frames per window = %d \n", NUM_FRAMES);
        printf("FFT/nIFFT             = %d \n", FFT_NIFFT);
        printf("Final scaling Shift   = %d \n", SHIFT);
        printf("Cascade len           = %d \n", CASC_LEN);
        printf("SSR                   = %d \n", UUT_SSR);
        printf("Number of kernels     = %d \n", CASC_LEN * UUT_SSR);
        printf("Number of inputs      = %d \n", CASC_LEN * UUT_SSR);
        printf("Number of outputs     = %d \n", UUT_SSR);
        printf("Rounding mode         = %d \n", ROUND_MODE);
        printf("Saturation mode       = %d \n", SAT_MODE);

        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);

        printf("========================\n");

        typedef typename std::conditional_t<
            std::is_same<DATA_TYPE, int16>::value, cint16,
            std::conditional_t<std::is_same<DATA_TYPE, int32>::value, cint32,
                               std::conditional_t<std::is_same<DATA_TYPE, float>::value, cfloat, DATA_TYPE> > >
            T_outDataType;

#ifdef USING_UUT
#if (USE_PKT_SWITCHING == 0)
        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            for (int casc = 0; casc < CASC_LEN; casc++) {
                // Broadcast connections from PLIO to SSR paths.
                connect<>(in[casc].out[0], dftGraph.in[casc + ssr * CASC_LEN]);
#if (SINGLE_BUF == 1)
                single_buffer(dftGraph.dft_graph.getKernels()[casc + ssr * CASC_LEN].in[0]);
                printf("INFO: Single Buffer Constraint applied to input buffer of kernel %d.\n", casc + ssr * CASC_LEN);
#endif // single buffer enabled
            }
        }
        for (int ssrOut = 0; ssrOut < UUT_SSR; ssrOut++) {
            connect<>(dftGraph.out[ssrOut], out[ssrOut].in[0]);
#if (SINGLE_BUF == 1)
            single_buffer(dftGraph.dft_graph.getKernels()[(ssrOut * CASC_LEN + CASC_LEN) - 1].out[0]);
            printf("INFO: Single Buffer Constraint applied to output buffer of kernel %d.\n",
                   ssrOut * CASC_LEN + CASC_LEN);
#endif // single buffer enabled
        }
#elif (USE_PKT_SWITCHING == 2)
        // Use pkt_switch_input class
        for (int i = 0; i < NPORT_I; i++) {
            // Connect PLIOs to pkt_switch_input.
            connect<>(in[i].out[0], dftGraphIn.pkt_in[i]);
        }
        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            for (int casc = 0; casc < CASC_LEN; casc++) {
                // Connect and broadcast connections from pkt_switch_input to dftGraph casc and SSR paths.
                connect<>(dftGraphIn.pkt_ssr_out[casc], dftGraph.in[casc + ssr * CASC_LEN]);
            }
        }
// Use pkt_switch_output class
// No need to connect here, done in pkt_switch_output constructor
#endif // USE_PKT_SWITCHING == 0
#else  // using ref
        connect<>(in[0].out[0], dftGraph.in[0]);
        connect<>(dftGraph.out[0], out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
