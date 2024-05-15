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
fft_ifft_dit_1ch graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

//#ifndef UUT_GRAPH
#ifdef USING_UUT
#define UUT_GRAPH fft_ifft_dit_2d_graph
#else
#define UUT_GRAPH fft_ifft_dit_1ch_ref_graph
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
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kPortsPerTile = API_IO == 0 ? 1 : kStreamsPerTile;
    std::array<input_plio, UUT_SSR> back_i;
    std::array<output_plio, UUT_SSR> back_o;
    std::array<input_plio, UUT_SSR> front_i;
    std::array<output_plio, UUT_SSR> front_o;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples        = %d \n", INPUT_SAMPLES);
        printf("Input window (bytes) = %lu\n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Output samples       = %d \n", OUTPUT_SAMPLES);
        printf("Point size           = %d \n", POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", SHIFT);
        printf("Cascade Length       = %d \n", CASC_LEN);
        printf("Dynamic point size   = %d \n", DYN_PT_SIZE);
        printf("Window Size          = %d \n", WINDOW_VSIZE);
        printf("API_IO               = %d \n", API_IO);
        printf("Round mode           = %d \n", ROUND_MODE);
        printf("Saturation mode      = %d \n", SAT_MODE);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);
        printf("========================\n");

// FIR sub-graph
#ifdef USING_UUT
        xf::dsp::aie::fft::ifft_2d_aie_pl::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN,
                                                     DYN_PT_SIZE, WINDOW_VSIZE, API_IO, UUT_SSR, USE_WIDGETS,
                                                     ROUND_MODE, SAT_MODE, TWIDDLE_MODE>
            fftGraph;
        for (int i = 0; i < UUT_SSR; i++) {
            std::string filenameInFront = QUOTE(FRONT_INPUT_FILE);

            // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
            // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
            // FFT)
            filenameInFront.insert(filenameInFront.length() - 4, ("_" + std::to_string(i) + "_0"));

            // Make connections
            front_i[i] = input_plio::create("PLIO_front_in_" + std::to_string(i), adf::plio_128_bits, filenameInFront);
            connect<>(front_i[i].out[0], fftGraph.front_i[i]);

            std::string filenameInBack = QUOTE(BACK_INPUT_FILE);
            filenameInBack.insert(filenameInBack.length() - 4, ("_" + std::to_string(i) + "_0"));
            back_i[i] = input_plio::create("PLIO_back_in_" + std::to_string(i), adf::plio_128_bits, filenameInBack);
            connect<>(back_i[i].out[0], fftGraph.back_i[i]);

            std::string filenameOutFront = QUOTE(FRONT_OUTPUT_FILE);
            filenameOutFront.insert(filenameOutFront.length() - 4, ("_" + std::to_string(i) + "_0"));
            front_o[i] =
                output_plio::create("PLIO_front_out_" + std::to_string(i), adf::plio_128_bits, filenameOutFront);
            connect<>(fftGraph.front_o[i], front_o[i].in[0]);

            std::string filenameOutBack = QUOTE(BACK_OUTPUT_FILE);
            filenameOutBack.insert(filenameOutBack.length() - 4, ("_" + std::to_string(i) + "_0"));
            back_o[i] = output_plio::create("PLIO_back_out_" + std::to_string(i), adf::plio_128_bits, filenameOutBack);
            connect<>(fftGraph.back_o[i], back_o[i].in[0]);
        }
#else
        xf::dsp::aie::fft::dit_1ch::UUT_GRAPH<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN,
                                              DYN_PT_SIZE, WINDOW_VSIZE, API_IO, 0, USE_WIDGETS, ROUND_MODE, SAT_MODE,
                                              TWIDDLE_MODE>
            fftGraph;
        for (int i = 0; i < 1; i++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            std::string filenameIn = QUOTE(INPUT_FILE);

            // Insert SSR index into filename before extension (.txt), e.g. input_X_Y.txt
            // where X is ssr index (used even when there is only one port) and Y is for dual stream format (not used in
            // FFT)
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));

            // Make connections
            front_i[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_32_bits, filenameIn);
            connect<>(front_i[i].out[0], fftGraph.in[i]);

            back_o[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_32_bits, filenameOut);
            connect<>(fftGraph.out[i], back_o[i].in[0]);
        }
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
