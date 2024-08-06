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

#ifndef ADF_GRAPH_DENORMALIZE_H
#define ADF_GRAPH_DENORMALIZE_H

#include "config.h"
#include "kernels.h"
#include <adf.h>
#include <common/xf_aie_const.hpp>

using namespace adf;

/*
 * ADF graph to compute weighted moving average of
 * the last 8 samples in a stream of numbers
 */

class DenormalizeGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_gmio in1;
    output_gmio out1;
    port<input> coeff;

    DenormalizeGraph() {
        k = kernel::create(Denormalize);

        in1 = input_gmio::create("DataIn0", 256, 1000);
        out1 = output_gmio::create("DataOut0", 256, 1000);

        // create nets to connect kernels and IO ports
        connect<>(in1.out[0], k.in[0]);
        connect<>(k.out[0], out1.in[0]);
        connect<parameter>(coeff, async(k.in[1]));

        adf::dimensions(k.in[0]) = {TILE_WINDOW_SIZE_IN};
        adf::dimensions(k.out[0]) = {TILE_WINDOW_SIZE_OUT};

        // specify kernel sources
        source(k) = "xf_denormalize.cc";

        runtime<ratio>(k) = 1.0;
    }
};

#endif
