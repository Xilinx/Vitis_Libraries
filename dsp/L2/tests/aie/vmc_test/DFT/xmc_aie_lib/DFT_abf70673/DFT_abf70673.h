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
#ifndef DFT_abf70673_GRAPH_H_
#define DFT_abf70673_GRAPH_H_

#include <adf.h>
#include "dft_graph.hpp"

class DFT_abf70673 : public adf::graph {
   public:
    // ports
    // template <typename dir>

    std::array<adf::port<input>, 1 * 1> in;
    std::array<adf::port<output>, 1> out;

    xf::dsp::aie::fft::dft::dft_graph<cint16, // TT_DATA
                                      cint16, // TT_TWIDDLE
                                      64,     // TP_POINT_SIZE
                                      1,      // TP_FFT_NIFFT
                                      0,      // TP_SHIFT
                                      1,      // TP_CASC_LEN
                                      64,     // TP_NUM_FRAMES
                                      0,      // TP_RND
                                      0,      // TP_SAT
                                      1       // TP_SSR

                                      >
        dft_graph;

    DFT_abf70673() : dft_graph() {
        for (int ssrIdx = 0; ssrIdx < 1; ssrIdx++) {
            for (int cascIdx = 0; cascIdx < 1; cascIdx++) {
                adf::connect<> net_in(in[cascIdx + ssrIdx * 1], dft_graph.in[cascIdx + ssrIdx * 1]);
            }
            adf::connect<> net_out(dft_graph.out[ssrIdx], out[ssrIdx]);
        }
    }
};

#endif // DFT_abf70673_GRAPH_H_
