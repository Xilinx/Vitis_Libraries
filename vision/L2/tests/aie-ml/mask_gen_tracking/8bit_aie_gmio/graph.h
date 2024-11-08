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

class maskGenTrackingGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_gmio in1;
    input_gmio in2;
    output_gmio out1;
    port<input> depth_min;
    port<input> depth_max;
    port<input> thres_f_new;
    port<input> thres_b_new;
    port<input> pred_seg_thresh;

    maskGenTrackingGraph() {
        k = kernel::create(maskGenTrack_api);
        in1 = input_gmio::create("gmIn0", 256, 1000);
        in2 = input_gmio::create("gmIn1", 256, 1000);
        out1 = output_gmio::create("gmOut0", 256, 1000);

        // create nets to connect kernels and IO ports
        connect<>(in1.out[0], k.in[0]);
        connect<>(in2.out[0], k.in[1]);
        connect<>(k.out[0], out1.in[0]);
        connect<parameter>(depth_min, async(k.in[2]));
        connect<parameter>(depth_max, async(k.in[3]));
        connect<parameter>(thres_f_new, async(k.in[4]));
        connect<parameter>(thres_b_new, async(k.in[5]));
        connect<parameter>(pred_seg_thresh, async(k.in[6]));

        adf::dimensions(k.in[0]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k.in[1]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k.out[0]) = {ELEM_WITH_METADATA_OUT};

        // specify kernel sources
        source(k) = "xf_mask_gen_track.cc";

        runtime<ratio>(k) = 0.5;
    }
};

class maskGenGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_gmio in1;
    output_gmio out1;
    port<input> depth_min;
    port<input> depth_max;
    port<input> thres_f_new;
    port<input> thres_b_new;

    maskGenGraph() {
        k = kernel::create(maskGen_api);
        in1 = input_gmio::create("gmIn2", 256, 1000);
        out1 = output_gmio::create("gmOut1", 256, 1000);

        // create nets to connect kernels and IO ports
        connect<>(in1.out[0], k.in[0]);
        connect<>(k.out[0], out1.in[0]);
        connect<parameter>(depth_min, async(k.in[1]));
        connect<parameter>(depth_max, async(k.in[2]));
        connect<parameter>(thres_f_new, async(k.in[3]));
        connect<parameter>(thres_b_new, async(k.in[4]));

        adf::dimensions(k.in[0]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k.out[0]) = {ELEM_WITH_METADATA_IN};

        // specify kernel sources
        source(k) = "xf_mask_gen.cc";

        runtime<ratio>(k) = 0.5;
    }
};

#endif
