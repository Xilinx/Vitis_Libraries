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
#include <sstream>
#include <type_traits>

using namespace adf;

/*
 * ADF graph to compute weighted moving average of
 * the last 8 samples in a stream of numbers
 */

template <int CORES>
class demosaicGraph : public adf::graph {
   private:
    std::array<kernel, CORES> k;

   public:
    std::array<input_plio, CORES> in1;
    std::array<output_plio, CORES> outr;
    std::array<output_plio, CORES> outg;
    std::array<output_plio, CORES> outb;

    demosaicGraph(int start_col, int start_row) { create_core<0>(start_col, start_row); }

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX >= CORES)>::type* = nullptr>
    void create_core(int col, int row) {}

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX < CORES)>::type* = nullptr>
    void create_core(int col, int row) {
        k[CORE_IDX] = kernel::create_object<DemosaicRunner>(std::vector<int16_t>({0}), std::vector<int16_t>({0}));

        std::stringstream ssi;
        ssi << "DataIn" << CORE_IDX;
        in1[CORE_IDX] = input_plio::create(ssi.str().c_str(), adf::plio_64_bits, "data/input.txt");

        std::stringstream sso_r;
        sso_r << "DataOutR" << CORE_IDX;
        outr[CORE_IDX] = output_plio::create(sso_r.str().c_str(), adf::plio_64_bits, "data/outputr.txt");

        std::stringstream sso_g;
        sso_g << "DataOutG" << CORE_IDX;
        outg[CORE_IDX] = output_plio::create(sso_g.str().c_str(), adf::plio_64_bits, "data/outputg.txt");

        std::stringstream sso_b;
        sso_b << "DataOutB" << CORE_IDX;
        outb[CORE_IDX] = output_plio::create(sso_b.str().c_str(), adf::plio_64_bits, "data/outputb.txt");

        // create nets to connect kernels and IO ports
        connect<window<TILE_WINDOW_SIZE> >(in1[CORE_IDX].out[0], k[CORE_IDX].in[0]);
        connect<window<TILE_WINDOW_SIZE> >(k[CORE_IDX].out[0], outr[CORE_IDX].in[0]);
        connect<window<TILE_WINDOW_SIZE> >(k[CORE_IDX].out[1], outg[CORE_IDX].in[0]);
        connect<window<TILE_WINDOW_SIZE> >(k[CORE_IDX].out[2], outb[CORE_IDX].in[0]);

        // specify kernel sources
        source(k[CORE_IDX]) = "xf_demosaicing.cc";

        // location constraints
        location<kernel>(k[CORE_IDX]) = tile(col, row);

        runtime<ratio>(k[CORE_IDX]) = 1.0;

        create_core<CORE_IDX + 1>(col, (row + 1));
    }
};

#endif
