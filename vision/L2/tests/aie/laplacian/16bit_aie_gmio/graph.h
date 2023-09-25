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

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>

#include "config.h"
#include "kernels_16b.h"

using namespace adf;

class laplacianGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_gmio inprt;
    output_gmio outprt;

    port<input> kernelCoefficients;

    laplacianGraph() {
        k1 = kernel::create(laplacian);

        inprt = input_gmio::create("gmioIn1", 256, 1000);
        outprt = output_gmio::create("gmioOut1", 256, 1000);

        connect<>(inprt.out[0], k1.in[0]);
        adf::connect<parameter>(kernelCoefficients, async(k1.in[1]));
        connect<>(k1.out[0], outprt.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        source(k1) = "xf_laplacian.cc";

        // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};

#endif
