/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ADF_RGB2HLS_GRAPH_H
#define ADF_RGB2HLS_GRAPH_H

#include <adf.h>
#include <array>
#include <type_traits>

#include "kernels.h"
#include "config.h"

using namespace adf;

class RGB2HLSGraph : public adf::graph {
   private:
    kernel k[NO_CORES_PER_COL];

   public:
    input_plio in1;
    output_plio out1;

    shared_buffer<uint8_t> mtx_out, mtx_in;

    RGB2HLSGraph(int tile_col, int tile_row, int CORE_IDX) {
        int mt_col = tile_col;

        std::stringstream ssi;
        ssi << "DataIn" << (0 + CORE_IDX);
        in1 = input_plio::create(ssi.str().c_str(), adf::plio_128_bits, "data/input.txt");

        std::stringstream sso;
        sso << "DataOut" << (0 + CORE_IDX);
        out1 = output_plio::create(sso.str().c_str(), adf::plio_128_bits, "data/output.txt");

        mtx_in = shared_buffer<uint8_t>::create({TILE_WINDOW_SIZE_IN * NO_CORES_PER_COL}, 1, NO_CORES_PER_COL);
        num_buffers(mtx_in) = 2;
        location<buffer>(mtx_in) = {address(mt_col, 0, 0),
                                    address(mt_col, 0, (TILE_WINDOW_SIZE_IN * NO_CORES_PER_COL))};

        mtx_out = shared_buffer<uint8_t>::create({TILE_WINDOW_SIZE_OUT * NO_CORES_PER_COL}, NO_CORES_PER_COL, 1);
        num_buffers(mtx_out) = 2;
        location<buffer>(mtx_out) = {address(mt_col, 1, 0),
                                     address(mt_col, 1, (TILE_WINDOW_SIZE_OUT * NO_CORES_PER_COL))};

        connect<>(in1.out[0], mtx_in.in[0]);
        write_access(mtx_in.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_IN * NO_CORES_PER_COL) / 4, 0, {1}, {});

        for (int i = 0; i < NO_CORES_PER_COL; i++) {
            k[i] = kernel::create_object<RGB2HLSRunner>();

            connect<>(mtx_in.out[i], k[i].in[0]);
            adf::dimensions(k[i].in[0]) = {TILE_WINDOW_SIZE_IN};
            read_access(mtx_in.out[i]) =
                buffer_descriptor((TILE_WINDOW_SIZE_IN) / 4, (i * (TILE_WINDOW_SIZE_IN / 4)), {1}, {});

            connect<>(k[i].out[0], mtx_out.in[i]);
            adf::dimensions(k[i].out[0]) = {TILE_WINDOW_SIZE_OUT};
            write_access(mtx_out.in[i]) =
                buffer_descriptor((TILE_WINDOW_SIZE_OUT) / 4, (i * TILE_WINDOW_SIZE_OUT / 4), {1}, {});

            // specify kernel sources
            source(k[i]) = "xf_rgb2hls.cc";
            location<kernel>(k[i]) = tile(tile_col, i);
            runtime<ratio>(k[i]) = 1.0;
        }

        connect<stream>(mtx_out.out[0], out1.in[0]);
        read_access(mtx_out.out[0]) = buffer_descriptor((NO_CORES_PER_COL * TILE_WINDOW_SIZE_OUT) / 4, 0, {1}, {});
    }
};
#endif
