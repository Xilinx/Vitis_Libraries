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
#ifndef Vectorized_Sample_Delay_Stream_806dda42_GRAPH_H_
#define Vectorized_Sample_Delay_Stream_806dda42_GRAPH_H_

#include <adf.h>
#include "sample_delay_graph.hpp"

class Vectorized_Sample_Delay_Stream_806dda42 : public adf::graph {
   public:
    adf::port<input> in;
    adf::port<input> numSampleDelay;
    adf::port<output> out;
    xf::dsp::aie::sample_delay::sample_delay_graph<int8, 256, 1, 256> sample_delay_graph;

    Vectorized_Sample_Delay_Stream_806dda42() : sample_delay_graph() {
        adf::connect<>(in, sample_delay_graph.in);
        adf::connect<>(sample_delay_graph.out, out);
        adf::connect<>(numSampleDelay, sample_delay_graph.numSampleDelay); // RTP
    }
};

#endif // Vectorized_Sample_Delay_Stream_806dda42_GRAPH_H_
