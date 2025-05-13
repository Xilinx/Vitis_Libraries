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
#ifndef FIR_Interpolation_63551773_GRAPH_H_
#define FIR_Interpolation_63551773_GRAPH_H_

#include <adf.h>
#include "fir_interpolate_asym_graph.hpp"

class FIR_Interpolation_63551773 : public adf::graph {
   public:
    static constexpr unsigned int TP_SSR = 1;
    static constexpr unsigned int TP_PARA_INTERP_POLY = 1;

    std::array<adf::port<input>, TP_SSR> in;
    // No dual input
    // No coeff port
    std::array<adf::port<output>, TP_SSR * TP_PARA_INTERP_POLY> out;
    // No dual output

    std::vector<int16> taps = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    xf::dsp::aie::fir::interpolate_asym::fir_interpolate_asym_graph<cint16, // TT_DATA
                                                                    int16,  // TT_COEFF
                                                                    32,     // TP_FIR_LEN
                                                                    2,      // TP_INTERPOLATE_FACTOR
                                                                    0,      // TP_SHIFT
                                                                    0,      // TP_RND
                                                                    256,    // TP_INPUT_WINDOW_VSIZE
                                                                    1,      // TP_CASC_LEN
                                                                    0,      // TP_USE_COEFF_RELOAD
                                                                    0,      // TP_DUAL_IP
                                                                    1,      // TP_NUM_OUTPUTS
                                                                    0,      // TP_API
                                                                    1,      // TP_SSR
                                                                    1,      // TP_PARA_INTERP_POLY
                                                                    0       // TP_SAT
                                                                    >
        filter;

    FIR_Interpolation_63551773() : filter(taps) {
        adf::kernel* filter_kernels = filter.getKernels();
        for (int i = 0; i < 1; i++) {
            adf::runtime<ratio>(filter_kernels[i]) = 0.9;
        }
        for (int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
            adf::connect<> net_in(in[ssrIdx], filter.in[ssrIdx]);
            // No dual input
        }

        for (int paraPolyIdx = 0; paraPolyIdx < TP_PARA_INTERP_POLY; paraPolyIdx++) {
            for (int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                unsigned outPortIdx = paraPolyIdx + ssrIdx * TP_PARA_INTERP_POLY;
                adf::connect<> net_out(filter.out[outPortIdx], out[outPortIdx]);
                // No dual output
                // No coeff port
            }
        }
    }
};

#endif // FIR_Interpolation_63551773_GRAPH_H_
