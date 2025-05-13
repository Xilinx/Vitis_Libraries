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
#ifndef FFT_Dynamic_Point_fe3e094c_GRAPH_H_
#define FFT_Dynamic_Point_fe3e094c_GRAPH_H_

#include <adf.h>
#include "fft_ifft_dit_1ch_graph.hpp"

class FFT_Dynamic_Point_fe3e094c : public adf::graph {
   public:
    // ports
    template <typename dir>
    using ssr_port_array = std::array<adf::port<dir>, 1>;

    ssr_port_array<input> in;
    ssr_port_array<output> out;

    xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<cint16, // TT_DATA
                                                       cint16, // TT_TWIDDLE
                                                       64,     // TP_POINT_SIZE
                                                       1,      // TP_FFT_NIFFT
                                                       0,      // TP_SHIFT
                                                       1,      // TP_CASC_LEN
                                                       1,      // TP_DYN_PT_SIZE
                                                       64,     // TP_WINDOW_VSIZE
                                                       0,      // TP_API
                                                       0,      // TP_PARALLEL_POWER
                                                       0,      // TP_USE_WIDGETS
                                                       2,      // TP_RND
                                                       0,      // TP_SAT
                                                       0,      // TP_TWIDDLE_MODE
                                                       cint16  // TT_OUT_DATA
                                                       >
        fft_graph;

    FFT_Dynamic_Point_fe3e094c() : fft_graph() {
        for (int i = 0; i < 1; i++) {
            adf::connect<> net_in(in[i], fft_graph.in[i]);
            adf::connect<> net_out(fft_graph.out[i], out[i]);
        }
    }
};

#endif // FFT_Dynamic_Point_fe3e094c_GRAPH_H_
