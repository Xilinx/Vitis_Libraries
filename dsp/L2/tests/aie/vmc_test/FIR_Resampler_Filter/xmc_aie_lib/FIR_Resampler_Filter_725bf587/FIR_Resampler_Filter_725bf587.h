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
#ifndef FIR_Resampler_Filter_725bf587_GRAPH_H_
#define FIR_Resampler_Filter_725bf587_GRAPH_H_

#include <adf.h>
#include "fir_resampler_graph.hpp"

using namespace adf;
class FIR_Resampler_Filter_725bf587 : public adf::graph {
   public:
    static constexpr unsigned int IN_SSR = 1;
    static constexpr unsigned int RTP_SSR = 1;
    static constexpr unsigned int OUT_SSR = 1;

    template <typename dir, unsigned int num_ports>
    using ssr_port_array = std::array<adf::port<dir>, num_ports>;

    ssr_port_array<input, IN_SSR> in;
    // No dual input
    // No coeff port
    ssr_port_array<output, OUT_SSR> out;
    // No dual output

    std::vector<int16> taps = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    xf::dsp::aie::fir::resampler::fir_resampler_graph<cint16, // TT_DATA
                                                      int16,  // TT_COEFF
                                                      30,     // TP_FIR_LEN
                                                      3,      // TP_INTERPOLATE_FACTOR
                                                      2,      // TP_DECIMATE_FACTOR
                                                      0,      // TP_SHIFT
                                                      0,      // TP_RND
                                                      256,    // TP_INPUT_WINDOW_VSIZE
                                                      1,      // TP_CASC_LEN
                                                      0,      // TP_USE_COEFF_RELOAD
                                                      1,      // TP_NUM_OUTPUTS
                                                      0,      // TP_DUAL_IP
                                                      0,      // TP_API
                                                      1,      // TP_SSR
                                                      1,      // TP_PARA_INTERP_POLY
                                                      1,      // TP_PARA_DECI_POLY
                                                      0       // TP_SAT
                                                      >
        filter;

    FIR_Resampler_Filter_725bf587() : filter(taps) {
        kernel* filter_kernels = filter.getKernels();
        for (int i = 0; i < 1; i++) {
            runtime<ratio>(filter_kernels[i]) = 0.9;
        }

        for (unsigned int i = 0; i < IN_SSR; ++i) {
            // Size of window in Bytes.
            connect<>(in[i], filter.in[i]);
            // No dual input
        }

        for (unsigned int i = 0; i < OUT_SSR; ++i) {
            connect<>(filter.out[i], out[i]);
            // No dual output
        }

        // No coeff port
    }
};

#endif // FIR_Resampler_Filter_725bf587_GRAPH_H_
