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
    // create 4 input ports and 1 output port
    input_gmio inprt1;
    input_gmio inprt2;
    input_gmio inprt3;
    input_gmio inprt4;
    port<input> iou_thresh;
    port<input> max_det;
    port<input> total_valid_boxes;

    output_gmio outprt;

    myGraph() {
        k1 = kernel::create(nms_aa);

        // Create and assign gmio port connections
        inprt1 = input_gmio::create("gmio1", 256, 1000);
        inprt2 = input_gmio::create("gmio2", 256, 1000);
        inprt3 = input_gmio::create("gmio3", 256, 1000);
        inprt4 = input_gmio::create("gmio4", 256, 1000);
        outprt = output_gmio::create("gmio5", 256, 1000);

        connect<>(inprt1.out[0], k1.in[0]);
        connect<>(inprt2.out[0], k1.in[1]);
        connect<>(inprt3.out[0], k1.in[2]);
        connect<>(inprt4.out[0], k1.in[3]);

        // Set he RTPs
        connect<parameter>(iou_thresh, async(k1.in[4]));
        connect<parameter>(max_det, async(k1.in[5]));
        connect<parameter>(total_valid_boxes, async(k1.in[6]));

        connect<>(k1.out[0], outprt.in[0]);

        // Total Elems for each args
        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[2]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[3]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA_OUT};

        source(k1) = "xf_nms.cc";

        // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};

#endif
