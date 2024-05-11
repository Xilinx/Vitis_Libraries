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

// This file holds the definition of the test harness for the fft window graph.

#include <adf.h>
#include <vector>
#include <array>

#include "fft_window_fns.hpp"
#include "fft_window_graph.hpp"

using namespace adf;
namespace fft_win_example {
#define DATA_TYPE_FFTW cint16
#define COEFF_TYPE_FFTW int16
#define POINT_SIZE_FFTW 256
#define WINDOW_VSIZE_FFTW 256
#define SHIFT_FFTW 14
#define API_IO_FFTW 0
#define UUT_SSR_FFTW 1
#define DYN_PT_SIZE_FFTW 0

class test_fftw : public graph {
   private:
    std::array<COEFF_TYPE_FFTW, POINT_SIZE_FFTW*(DYN_PT_SIZE_FFTW + 1)> weights;

   public:
    port<input> in;
    port<output> out;
    // Constructor
    test_fftw() {
        // populate the window using built-in functions
        xf::dsp::aie::fft::windowfn::getHammingWindow<COEFF_TYPE_FFTW>((COEFF_TYPE_FFTW*)&weights[0], POINT_SIZE_FFTW);
        // pass weights to the fft_window sub-graph
        xf::dsp::aie::fft::windowfn::fft_window_graph<DATA_TYPE_FFTW, COEFF_TYPE_FFTW, POINT_SIZE_FFTW,
                                                      WINDOW_VSIZE_FFTW, SHIFT_FFTW, API_IO_FFTW, UUT_SSR_FFTW,
                                                      DYN_PT_SIZE_FFTW>
            fftWindowGraph(weights);

        // Make connections
        connect<>(in, fftWindowGraph.in[0]);
        connect<>(fftWindowGraph.out[0], out);
    };
};
};

#endif // _DSPLIB_TEST_HPP_