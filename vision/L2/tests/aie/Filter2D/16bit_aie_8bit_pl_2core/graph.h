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

#include "config.h"
#include "kernels.h"
#include <adf.h>

using namespace adf;

class myGraph : public adf::graph {
   public:
    kernel k1;
    input_plio inptr;
    output_plio outptr;
    port<input> kernelCoefficients;

    myGraph() {
        k1 = kernel::create(filter2D);
        inptr = input_plio::create(adf::plio_128_bits, "data/input.txt");
        outptr = output_plio::create(adf::plio_128_bits, "data/output.txt");

        adf::connect<window<TILE_WINDOW_SIZE> >(inptr.out[0], k1.in[0]);
        adf::connect<parameter>(kernelCoefficients, async(k1.in[1]));
        adf::connect<window<TILE_WINDOW_SIZE> >(k1.out[0], outptr.in[0]);

        source(k1) = "xf_filter2d.cc";
        // Initial mapping
        runtime<ratio>(k1) = 1;
    };
};

#endif
