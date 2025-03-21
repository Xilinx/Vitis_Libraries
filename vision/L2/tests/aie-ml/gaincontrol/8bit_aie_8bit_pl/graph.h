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

class gaincontrolGraph : public adf::graph {
   private:
    kernel k1;

   public:
    port<input> rgain;
    port<input> bgain;
    port<input> ggain;

    input_plio in1;
    output_plio out1;

    gaincontrolGraph() {
        // create kernels
        k1 = kernel::create(gaincontrol<XF_BAYER_RG>);

        in1 = input_plio::create("DataIn0", adf::plio_128_bits, "data/input.txt");
        out1 = output_plio::create("DataOut0", adf::plio_128_bits, "data/output.txt");

        // create nets to connect kernels and IO ports
        connect<>(in1.out[0], k1.in[0]);
        connect<>(k1.out[0], out1.in[0]);
        connect<parameter>(rgain, async(k1.in[1]));
        connect<parameter>(bgain, async(k1.in[2]));
        connect<parameter>(ggain, async(k1.in[3]));

        adf::dimensions(k1.in[0]) = {TILE_WINDOW_SIZE};
        adf::dimensions(k1.out[0]) = {TILE_WINDOW_SIZE};

        // specify kernel sources
        source(k1) = "xf_gaincontrol.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};
#endif
