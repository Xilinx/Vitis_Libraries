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

enum PORT_t { IN1, OUT1, OUT2 };
template <int CORES>
class TopPipelineGraph : public adf::graph {
   private:
    kernel k[CORES]; // Comb

   public:
    port<input> rgain;
    port<input> bgain;
    port<input> ggain;
    port<input> blk_val;
    port<input> mul_val;
    port<input> coeffs;

    input_plio in1[CORES];
    output_plio out1[CORES];
    output_plio out2[CORES];

    TopPipelineGraph(int start_col, int start_row, int start_core_idx) {
        create_core<0>(start_col, start_row, start_core_idx);
    }

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX >= CORES)>::type* = nullptr>
    void create_core(int col, int row, int start_core_idx) {}

    template <int CORE_IDX, typename std::enable_if<(CORE_IDX < CORES)>::type* = nullptr>
    void create_core(int col, int row, int start_core_idx) {
        k[CORE_IDX] = kernel::create(comb<DATA_TYPE, 32, 0>);

        std::stringstream ssi;
        ssi << "DataIn" << (start_core_idx + CORE_IDX);
        std::stringstream di;
        di << "data/input_" << (start_core_idx + CORE_IDX) << ".txt";
        in1[CORE_IDX] = input_plio::create(ssi.str().c_str(), adf::plio_128_bits, di.str().c_str());

        std::stringstream sso, ss1;
        sso << "DataOut" << (start_core_idx + CORE_IDX);
        ss1 << "Data-Out" << (start_core_idx + CORE_IDX);
        std::stringstream do0, do1;
        do0 << "data/outputDEM_" << (start_core_idx + CORE_IDX) << ".txt";
        do1 << "data/outputAWB_" << (start_core_idx + CORE_IDX) << ".txt";
        out1[CORE_IDX] = output_plio::create(sso.str().c_str(), adf::plio_128_bits, do0.str().c_str());
        out2[CORE_IDX] = output_plio::create(ss1.str().c_str(), adf::plio_128_bits, do1.str().c_str());

        // create nets to connect kernels and IO ports
        connect<>(in1[CORE_IDX].out[0], k[CORE_IDX].in[0]);

        connect<adf::parameter>(coeffs, async(k[0].in[1]));
        connect<adf::parameter>(blk_val, async(k[0].in[2]));
        connect<adf::parameter>(mul_val, async(k[0].in[3]));
        connect<adf::parameter>(rgain, async(k[0].in[4]));
        connect<adf::parameter>(bgain, async(k[0].in[5]));
        connect<adf::parameter>(ggain, async(k[0].in[6]));

        connect<>(k[CORE_IDX].out[0], out1[CORE_IDX].in[0]);
        connect<>(k[CORE_IDX].out[1], out2[CORE_IDX].in[0]);

        adf::dimensions(k[CORE_IDX].in[0]) = {TILE_WINDOW_SIZE};
        adf::dimensions(k[CORE_IDX].out[0]) = {TILE_WINDOW_SIZE_RGBA};
        adf::dimensions(k[CORE_IDX].out[1]) = {TILE_WINDOW_SIZE_RGBA};

        // specify kernel sources
        source(k[CORE_IDX]) = "xf_comb.cc";

        // location constraints

        location<kernel>(k[CORE_IDX]) = tile(col, row);

        runtime<ratio>(k[CORE_IDX]) = 0.7;

        create_core<CORE_IDX + 1>((col + 2), row, start_core_idx);
    }
};

#endif
