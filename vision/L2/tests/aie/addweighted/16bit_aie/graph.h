/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include <common/xf_aie_const.hpp>
#include "kernels.h"

static constexpr int TILE_ELEMENTS = 4096;
static constexpr int TILE_WINDOW_SIZE = TILE_ELEMENTS * sizeof(int16_t) + xf::cv::aie::METADATA_SIZE;
static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(int16_t));
using namespace adf;

/*
 * ADF dataflow graph to absdiff xfopencv basic arithmatic module
 */

class addweightedGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio in1;
    input_plio in2;
    port<input> alpha;
    port<input> beta;
    port<input> gamma;
    output_plio out;

    addweightedGraph() {
        /////////////////////////////////////////////////////1st core//////////////////////////////////////////////////
        // create kernels

        k1 = kernel::create(addweighted);

        in1 = input_plio::create("DataIn1", adf::plio_64_bits, "data/input1.txt");
        in2 = input_plio::create("DataIn2", adf::plio_64_bits, "data/input2.txt");
        out = output_plio::create("DataOut1", adf::plio_64_bits, "data/output.txt");

        // For 16-bit window size is 4096=2048*2, for 32-bit window size is 8192=2048*4
        // create nets to connect kernels and IO ports
        // create nets to connect kernels and IO ports
        connect<parameter>(alpha, async(k1.in[2]));
        connect<parameter>(beta, async(k1.in[3]));
        connect<parameter>(gamma, async(k1.in[4]));

        adf::connect<>(in1.out[0], k1.in[0]);
        adf::connect<>(in2.out[0], k1.in[1]);
        adf::connect<>(k1.out[0], out.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};
        // specify kernel sources
        source(k1) = "xf_addweighted.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};

#endif
