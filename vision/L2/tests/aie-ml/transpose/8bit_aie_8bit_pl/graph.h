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

class transposeGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_plio in1;
    output_plio out1;

    port<input> outputStride;

    shared_buffer<DATA_TYPE> mtx_in1;
    transposeGraph() {
        k = kernel::create(transpose_api);
        in1 = input_plio::create("DataIn0", adf::plio_128_bits, "data/input.txt");

        out1 = output_plio::create("DataOut0", adf::plio_128_bits, "data/output.txt");

        mtx_in1 = shared_buffer<DATA_TYPE>::create({ELEM_WITH_METADATA}, 1, 2);
        num_buffers(mtx_in1) = 2;
        // plio ports to mem-tile1
        connect<stream>(in1.out[0], mtx_in1.in[0]);
        write_access(mtx_in1.in[0]) = buffer_descriptor(ELEM_WITH_METADATA / 4, 0, {1}, {});

        // create nets to connect kernels and IO ports
        connect<>(mtx_in1.out[0], k.in[0]);
        adf::dimensions(k.in[0]) = {METADATA_SIZE};
        read_access(mtx_in1.out[0]) = buffer_descriptor((METADATA_SIZE) / 4, 0, {1}, {});

        connect<>(mtx_in1.out[1], k.in[1]);
        adf::dimensions(k.in[1]) = {ELEM_WITH_METADATA - METADATA_SIZE};
        read_access(mtx_in1.out[1]) = buffer_descriptor((ELEM_WITH_METADATA - METADATA_SIZE) / 4, METADATA_SIZE / 4,
                                                        {1, TILE_WIDTH * CHANNELS / 4, 1}, {1, TILE_HEIGHT});

        connect<>(k.out[0], out1.in[0]);

        connect<parameter>(outputStride, async(k.in[2]));

        adf::dimensions(k.out[0]) = {ELEM_WITH_METADATA};
        // specify kernel sources
        source(k) = "xf_transpose.cc";

        runtime<ratio>(k) = 1.0;
    }
};

#endif
