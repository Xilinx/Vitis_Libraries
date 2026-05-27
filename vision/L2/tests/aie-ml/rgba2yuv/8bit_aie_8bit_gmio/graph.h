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
#include <common/xf_aie_const.hpp>
#include "kernels.h"
#include "config.h"

using namespace adf;

class rgba2yuvGraph : public adf::graph {
   private:
    adf::kernel k1;

   public:
    input_gmio in;
    output_gmio out1;
    output_gmio out2;
    port<input> tile_width;
    port<input> tile_height;

    rgba2yuvGraph() {
        /////////////////////////////////////////////////////1st core//////////////////////////////////////////////////
        // create kernels

        k1 = adf::kernel::create(rgba2yuv_api);

        // For 16-bit window size is 4096=2048*2, for 32-bit window size is 8192=2048*4
        // create nets to connect kernels and IO ports
        // create nets to connect kernels and IO ports
        in = input_gmio::create("DataIn0", 256, 1000);
        out1 = output_gmio::create("DataOut0", 256, 1000);
        out2 = output_gmio::create("DataOut1", 256, 1000);

        connect<>(in.out[0], k1.in[0]);
        connect<>(k1.out[0], out1.in[0]);
        connect<>(k1.out[1], out2.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA_RGBA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA_Y};
        adf::dimensions(k1.out[1]) = {ELEM_WITH_METADATA_UV};

        adf::connect<adf::parameter>(tile_width, async(k1.in[1]));
        adf::connect<adf::parameter>(tile_height, async(k1.in[2]));

        // specify kernel sources
        source(k1) = "xf_rgba2yuv.cc";

        // specify kernel run times
        adf::runtime<ratio>(k1) = 0.5;
    }
};

#endif
