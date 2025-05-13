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

#include <adf.h>
#include "fft_ifft_dit_1ch_graph.hpp"

using namespace adf;

namespace fft_example {

#define LOC_XBASE 0
#define LOC_YBASE 0
#define DATA_TYPE_FFT cint16
#define TWIDDLE_TYPE cint16
#define POINT_SIZE 128
#define FFT_NIFFT 1
#define SHIFT 3
#define CASC_LEN 1
#define DYN_PT_SIZE 0
#define WINDOW_VSIZE 128
#define API_IO 1
#define PARALLEL_POWER 1

class test_fft : public graph {
   public:
    static constexpr int kParFactor = 2 << PARALLEL_POWER;
    xf::dsp::aie::port_array<input, kParFactor> in;
    xf::dsp::aie::port_array<output, kParFactor> out;
    xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<DATA_TYPE_FFT,
                                                       TWIDDLE_TYPE,
                                                       POINT_SIZE,
                                                       FFT_NIFFT,
                                                       SHIFT,
                                                       CASC_LEN,
                                                       DYN_PT_SIZE,
                                                       WINDOW_VSIZE,
                                                       API_IO,
                                                       PARALLEL_POWER>
        fftGraph;
    test_fft() {
        // make connections
        for (int i = 0; i < kParFactor; i++) {
            connect<>(in[i], fftGraph.in[i]);
            connect<>(fftGraph.out[i], out[i]);
        }

        // add location constraints
        for (int lane = 0; lane < 2; lane++) {
            location<kernel>(fftGraph.m_r2Comb[lane]) = tile(LOC_XBASE + lane * 2, LOC_YBASE + CASC_LEN + 1);
        }
    };
}; // end of class
};