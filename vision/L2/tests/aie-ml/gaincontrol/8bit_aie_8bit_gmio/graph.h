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

#include "config.h"
#include "kernels.h"

using namespace adf;

class gaincontrolGraph : public adf::graph {
   private:
    kernel k1;

   public:
    // port<input> in1;
    port<input> rgain;
    port<input> bgain;
    port<input> ggain;
    // port<output> out;

    input_gmio in1;
    output_gmio out;

    gaincontrolGraph() {
        // create kernels
        k1 = kernel::create(gaincontrol<CODE>);

        // create nets to connect kernels and IO ports
        in1 = input_gmio::create("DataIn0", 256, 1000);
        out = output_gmio::create("DataOut0", 256, 1000);
        connect<>(in1.out[0], k1.in[0]);
        connect<parameter>(rgain, async(k1.in[1]));
        connect<parameter>(bgain, async(k1.in[2]));
        connect<parameter>(ggain, async(k1.in[3]));
        connect<>(k1.out[0], out.in[0]);

        adf::dimensions(k1.in[0]) = {TILE_WINDOW_SIZE};
        adf::dimensions(k1.out[0]) = {TILE_WINDOW_SIZE};
        // specify kernel sources
        source(k1) = "xf_gaincontrol.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};
#endif
