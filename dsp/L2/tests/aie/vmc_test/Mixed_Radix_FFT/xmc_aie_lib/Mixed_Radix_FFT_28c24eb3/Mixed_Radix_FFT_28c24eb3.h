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
#ifndef Mixed_Radix_FFT_28c24eb3_GRAPH_H_
#define Mixed_Radix_FFT_28c24eb3_GRAPH_H_

#include <adf.h>
#include "mixed_radix_fft_graph.hpp"

class Mixed_Radix_FFT_28c24eb3 : public adf::graph {
   public:
    // ports
    // template <typename dir>

    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int m_kNumPorts = 0 == 0 ? 1 : kStreamsPerTile;        // 1 for iobuffer, 2 for streams

    std::array<adf::port<input>, m_kNumPorts> in;
    std::array<adf::port<output>, m_kNumPorts> out;

    xf::dsp::aie::fft::mixed_radix_fft::mixed_radix_fft_graph<cint16, // TT_DATA
                                                              cint16, // TT_TWIDDLE
                                                              64,     // TP_POINT_SIZE
                                                              1,      // TP_FFT_NIFFT
                                                              0,      // TP_SHIFT
                                                              0,      // TP_RND
                                                              0,      // TP_SAT
                                                              64,
                                                              1,
                                                              0>
        mixed_radix_fft_graph;

    Mixed_Radix_FFT_28c24eb3() : mixed_radix_fft_graph() {
        if (0 == 0) {
            adf::connect<> net_in(in[0], mixed_radix_fft_graph.in[0]);
            adf::connect<> net_out(mixed_radix_fft_graph.out[0], out[0]);
        } else {
            for (int i = 0; i < m_kNumPorts; i++) {
                adf::connect<> net_in(in[i], mixed_radix_fft_graph.in[i]);
                adf::connect<> net_out(mixed_radix_fft_graph.out[i], out[i]);
            }
        }
    }
};

#endif // Mixed_Radix_FFT_28c24eb3_GRAPH_H_
