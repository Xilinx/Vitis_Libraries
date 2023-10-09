/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
    std::array<input_plio, ((CASC_LEN))> in;
    std::array<output_plio, ((1))> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Point size            = %d \n", POINT_SIZE);
        printf("FFT/nIFFT             = %d \n", FFT_NIFFT);
        printf("Final scaling Shift   = %d \n", SHIFT);
        printf("Number of kernels     = %d \n", CASC_LEN);
        printf("Rounding mode     = %d \n", ROUND_MODE);
        printf("Saturation mode     = %d \n", SAT_MODE);
        printf("Num frames per window = %d \n", NUM_FRAMES);
        printf("Data type             = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
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
                                          ROUND_MODE, SAT_MODE>
            dftGraph;

#ifdef USING_UUT
        for (int i = 0; i < CASC_LEN; i++) {
            std::string filenameIn = QUOTE(INPUT_FILE);
            // filenameIn.insert(filenameIn.length()-4, ("_"+std::to_string(i)));
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));
            // Make connections
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_32_bits, filenameIn);
            connect<>(in[i].out[0], dftGraph.in[i]);
        }
#else // using ref
        std::string filenameIn = QUOTE(INPUT_FILE);
        // filenameIn.insert(filenameIn.length()-4, ("_"+std::to_string(0)));
        // Make connections
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_32_bits, filenameIn);
        connect<>(in[0].out[0], dftGraph.in[0]);
#endif

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length()-4, ("_"+std::to_string(i)+"_0"));
        filenameOut.insert(filenameOut.length() - 4, ("_0_0"));
        // filenameOut.insert(filenameOut.length()-4, ("_"+std::to_string(0)));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_32_bits, filenameOut);
        connect<>(dftGraph.out[0], out[0].in[0]);
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
