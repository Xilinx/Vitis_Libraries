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
#include "kernels.h"
#include "config.h"

using namespace adf;

class RGBA2GreyGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_gmio in;
    output_gmio out;

    RGBA2GreyGraph() {
        // create kernels
        k = kernel::create(rgba2gray);

        in = input_gmio::create("gmioIn", 64, 1000);
        out = output_gmio::create("gmioOut", 64, 1000);

        // create nets to connect kernels and IO ports
        connect<>(in.out[0], k.in[0]);
        connect<>(k.out[0], out.in[0]);

        adf::dimensions(k.in[0]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k.out[0]) = {ELEM_WITH_METADATA_OUT};

        // specify kernel sources
        source(k) = "xf_rgba2gray.cc";

        // specify kernel run times
        runtime<ratio>(k) = 0.5;
    }
};
#endif
