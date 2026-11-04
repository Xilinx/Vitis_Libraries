/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#include <adf.h>
#include "transpose_graph.hpp"

using namespace adf;
namespace transpose_example {

#define TT_DATA_TPOSE cint16
#define TP_DIM_ROWS_TPOSE 32
#define TP_DIM_COLS_TPOSE 32
#define TP_SSR_TPOSE 1
#define TP_CASC_LEN_TPOSE 1
#define TP_SPLIT_MODE_TPOSE 0
#define TP_CONJUGATE_TPOSE 0

class test_transpose : public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::dsp::aie::transpose::transpose_graph<TT_DATA_TPOSE,
                                             TP_DIM_ROWS_TPOSE,
                                             TP_DIM_COLS_TPOSE,
                                             TP_SSR_TPOSE,
                                             TP_CASC_LEN_TPOSE,
                                             TP_SPLIT_MODE_TPOSE,
                                             TP_CONJUGATE_TPOSE>
        transposeGraph;

    test_transpose() {
        connect<>(in, transposeGraph.in[0]);
        connect<>(transposeGraph.out[0], out);
    };
};
};
