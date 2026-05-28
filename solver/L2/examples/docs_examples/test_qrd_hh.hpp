/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include "qrd_hh_graph.hpp"

using namespace adf;
namespace qrd_hh_example {

#define DATA_TYPE_QRD_HH float
#define DIM_ROWS_QRD_HH 16
#define DIM_COLS_QRD_HH 16
#define NUM_FRAMES_QRD_HH 2 
#define CASC_LEN_QRD_HH 1
#define DIM_A_LEADING_QRD_HH 0
#define DIM_Q_LEADING_QRD_HH 0 
#define DIM_R_LEADING_QRD_HH 0


class test_qrd_hh: public adf::graph {
   public:
    port<input> inA;
    port<output> outQ;
    port<output> outR;
    xf::solver::aie::qrd_hh::qrd_hh_graph<DATA_TYPE_QRD_HH, 
                                    DIM_ROWS_QRD_HH, 
                                    DIM_COLS_QRD_HH, 
                                    NUM_FRAMES_QRD_HH, 
                                    CASC_LEN_QRD_HH,
                                    DIM_A_LEADING_QRD_HH,
                                    DIM_Q_LEADING_QRD_HH,
                                    DIM_R_LEADING_QRD_HH>
        qrdHHGraph;

    test_qrd_hh() {
        connect<>(inA, qrdHHGraph.inA[0]);
        connect<>(qrdHHGraph.outQ[0], outQ);
        connect<>(qrdHHGraph.outR[0], outR);
    };
};
};