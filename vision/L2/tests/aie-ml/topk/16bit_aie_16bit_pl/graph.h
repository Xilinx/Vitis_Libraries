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
#include <array>
#include <type_traits>

#include "kernels.h"

using namespace adf;

class topkGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_plio in1;
    output_plio out1;
    output_plio out2;

    port<input> num_elem;
    port<input> ktop;
    port<input> start_idx;

    topkGraph() {
        k = kernel::create(topk_api);

        in1 = input_plio::create("DataIn0", adf::plio_128_bits, "data/input_score_1024x1.txt");
        out1 = output_plio::create("DataOut0", adf::plio_128_bits, "data/output_scores.txt");
        out2 = output_plio::create("DataOut1", adf::plio_128_bits, "data/output_indices.txt");

        // plio ports to aie-tile
        connect<>(in1.out[0], k.in[0]);
        connect<>(k.out[0], out1.in[0]);
        connect<>(k.out[1], out2.in[0]);

        adf::dimensions(k.in[0]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k.out[0]) = {ELEM_WITH_METADATA_OUT};
        adf::dimensions(k.out[1]) = {ELEM_WITH_METADATA_OUT};

        connect<parameter>(num_elem, async(k.in[1]));
        connect<parameter>(ktop, async(k.in[2]));
        connect<parameter>(start_idx, async(k.in[3]));
        // specify kernel sources
        source(k) = "xf_topk.cc";

        runtime<ratio>(k) = 1.0;
    }
};

#endif
