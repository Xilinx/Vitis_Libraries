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

class myGraph : public adf::graph {
   private:
    kernel k1;

   public:
    port<input> alpha;
    input_gmio in1;
    input_gmio in2;
    output_gmio out1;

    myGraph() {
        k1 = kernel::create(accumulate_weighted<uint8_t, uint16_t, 16>);

        in1 = input_gmio::create("DataIn1", 256, 1000);
        in2 = input_gmio::create("DataIn2", 256, 1000);
        out1 = output_gmio::create("DataOut1", 256, 1000);

        connect<>(in1.out[0], k1.in[0]);
        connect<>(in2.out[0], k1.in[1]);
        connect<parameter>(alpha, async(k1.in[2]));
        connect<>(k1.out[0], out1.in[0]);

        adf::dimensions(k1.in[0]) = {IN_ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {IN_ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        source(k1) = "xf_accumulate_weighted.cc";

        runtime<ratio>(k1) = 0.5;
    };
};

#endif
