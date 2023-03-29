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

/*
 * Cardano dataflow graph to absdiff xfopencv basic arithmatic module
 */

class pixelwiseMulGraph : public adf::graph {
   private:
    kernel k1;

   public:
    port<input> in1;
    port<input> in2;
    port<output> out;
    port<input> scale;

    pixelwiseMulGraph() {
        /////////////////////////////////////////////////////1st
        /// core//////////////////////////////////////////////////

        k1 = kernel::create(pixelwise_mul);

        // For 16-bit window size is 4096=2048*2, for 32-bit window size is
        // 8192=2048*4
        // create nets to connect kernels and IO ports
        // create nets to connect kernels and IO ports
        connect<>(in1, k1.in[0]);
        connect<>(in2, k1.in[1]);
        connect<>(k1.out[0], out);
        connect<parameter>(scale, async(k1.in[2]));

        // specify dimensions
        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        // specify kernel sources
        source(k1) = "xf_multiplication.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};

#endif
