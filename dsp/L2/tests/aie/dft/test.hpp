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
dft graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

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
    std::array<input_plio, (UUT_SSR * CASC_LEN)> in;
    std::array<output_plio, (UUT_SSR)> out;

    // Constructor
    test_graph() {
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

        // constexpr unsigned int kWindowSize = xf::dsp::aie::fft::dft::getWindowSize<T_outDataType,POINT_SIZE,
        // NUM_FRAMES>();

        xf::dsp::aie::fft::dft::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN, NUM_FRAMES,
                                          ROUND_MODE, SAT_MODE, UUT_SSR>
            dftGraph;

#ifdef USING_UUT
        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            for (int casc = 0; casc < CASC_LEN; casc++) {
                std::string filenameIn = QUOTE(INPUT_FILE);
                // Each rank of ssr receives same input data
                filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(ssr) + "_" + std::to_string(casc)));
                // Make connections
                in[casc + ssr * CASC_LEN] = input_plio::create("PLIO_in_" + std::to_string(casc + ssr * CASC_LEN),
                                                               adf::plio_64_bits, filenameIn);
                connect<>(in[casc + ssr * CASC_LEN].out[0], dftGraph.in[casc + ssr * CASC_LEN]);
#if (SINGLE_BUF == 1)
                single_buffer(dftGraph.getKernels()[casc + ssr * CASC_LEN].in[0]);
                printf("INFO: Single Buffer Constraint applied to input buffer of kernel %d.\n", casc + ssr * CASC_LEN);
#endif // single buffer enabled
            }
        }
        for (int ssrOut = 0; ssrOut < UUT_SSR; ssrOut++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(ssrOut) + "_0"));
            out[ssrOut] = output_plio::create("PLIO_out_" + std::to_string(ssrOut), adf::plio_64_bits, filenameOut);
            connect<>(dftGraph.out[ssrOut], out[ssrOut].in[0]);
#if (SINGLE_BUF == 1)
            single_buffer(dftGraph.getKernels()[(ssrOut * CASC_LEN + CASC_LEN) - 1].out[0]);
            printf("INFO: Single Buffer Constraint applied to output buffer of kernel %d.\n",
                   ssrOut * CASC_LEN + CASC_LEN);
#endif // single buffer enabled
        }
#else // using ref
        std::string filenameIn = QUOTE(INPUT_FILE);
        // Make connections
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_32_bits, filenameIn);
        connect<>(in[0].out[0], dftGraph.in[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length() - 4, ("_0_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(dftGraph.out[0], out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
