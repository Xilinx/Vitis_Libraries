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
#ifndef Window_Function_dec53fe3_GRAPH_H_
#define Window_Function_dec53fe3_GRAPH_H_

#include <adf.h>
#include "fft_window_graph.hpp"

class Window_Function_dec53fe3 : public adf::graph {
   public:
    static constexpr unsigned int TP_SSR = 1;
    template <typename dir>
    using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

    ssr_port_array<input> in;
    ssr_port_array<output> out;

    std::array<int16, 64> weights = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    xf::dsp::aie::fft::windowfn::fft_window_graph<cint16, // TT_DATA
                                                  int16,  // TT_COEFF
                                                  64,     // TP_POINT_SIZE
                                                  64,     // TP_WINDOW_VSIZE
                                                  0,      // TP_SHIFT
                                                  0,      // TP_API
                                                  1,      // TP_SSR
                                                  0       // TP_DYN_PT_SIZE
                                                  >
        fft_window;

    Window_Function_dec53fe3() : fft_window(weights) {
        adf::kernel* fft_window_kernels = fft_window.getKernels();
        for (int i = 0; i < 1; i++) {
            adf::runtime<ratio>(fft_window_kernels[i]) = 0.9;
        }
        for (int i = 0; i < TP_SSR; i++) {
            adf::connect<> net_in(in[i], fft_window.in[i]);
            adf::connect<> net_out(fft_window.out[i], out[i]);
        }
    }
};

#endif // Window_Function_dec53fe3_GRAPH_H_
