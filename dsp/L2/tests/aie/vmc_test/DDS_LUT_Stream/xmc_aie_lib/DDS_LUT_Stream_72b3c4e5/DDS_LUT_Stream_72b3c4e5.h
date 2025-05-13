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
#ifndef DDS_LUT_Stream_72b3c4e5_GRAPH_H_
#define DDS_LUT_Stream_72b3c4e5_GRAPH_H_

#include <adf.h>
#include "dds_mixer_lut_graph.hpp"

class DDS_LUT_Stream_72b3c4e5 : public adf::graph {
   public:
    // ports
    std::array<adf::port<output>, 1> out;

    xf::dsp::aie::mixer::dds_mixer::dds_mixer_lut_graph<cint16, // TT_DATA
                                                        0,      // TP_MIXER_MODE
                                                        40,     // TP_SFDR
                                                        1,      // TP_API
                                                        64,     // TP_INPUT_WINDOW_VSIZE
                                                        1,      // TP_SSR
                                                        0,      // TP_RND
                                                        0,      // TP_SAT
                                                        0       // TP_USE_PHASE_RELOAD
                                                        >
        mixer_graph;
    DDS_LUT_Stream_72b3c4e5() : mixer_graph(0, 0) {
        // kernels
        // runtime_ratio
        // connections in loop
        for (unsigned ssrIdx = 0; ssrIdx < 1; ssrIdx++) {
            adf::connect<>(mixer_graph.out[ssrIdx], out[ssrIdx]);
        }
    }
};

#endif // DDS_LUT_Stream_72b3c4e5_GRAPH_H_
