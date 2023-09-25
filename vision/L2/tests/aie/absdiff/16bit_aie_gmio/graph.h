/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include "kernels.h"
#include "config.h"

using namespace adf;

class myGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_gmio inprt1;
    input_gmio inprt2;

    output_gmio outprt;

    myGraph() {
        k1 = kernel::create(absdiff);

        inprt1 = input_gmio::create("gmioIn1", 256, 1000);
        inprt2 = input_gmio::create("gmioIn2", 256, 1000);
        outprt = output_gmio::create("gmioOut1", 256, 1000);

        adf::connect<>(inprt1.out[0], k1.in[0]);
        adf::connect<>(inprt2.out[0], k1.in[1]);
        adf::connect<>(k1.out[0], outprt.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        source(k1) = "xf_absdiff.cc";
        // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};

#endif
