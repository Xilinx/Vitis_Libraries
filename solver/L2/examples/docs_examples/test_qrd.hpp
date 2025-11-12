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
#include "qrd_graph.hpp"

using namespace adf;
namespace qrd_example {

#define DATA_TYPE_QRD float
#define DIM_ROWS_QRD 16
#define DIM_COLS_QRD 16
#define NUM_FRAMES_QRD 2 
#define CASC_LEN_QRD 1
#define DIM_A_LEADING_QRD 0
#define DIM_Q_LEADING_QRD 0 
#define DIM_R_LEADING_QRD 0


class test_qrd: public adf::graph {
   public:
    port<input> inA;
    port<output> outQ;
    port<output> outR;
    xf::solver::aie::qrd::qrd_graph<DATA_TYPE_QRD, 
                                    DIM_ROWS_QRD, 
                                    DIM_COLS_QRD, 
                                    NUM_FRAMES_QRD, 
                                    CASC_LEN_QRD,
                                    DIM_A_LEADING_QRD,
                                    DIM_Q_LEADING_QRD,
                                    DIM_R_LEADING_QRD>
        qrdGraph;

    test_qrd() {
        connect<>(inA, qrdGraph.inA[0]);
        connect<>(qrdGraph.outQ[0], outQ);
        connect<>(qrdGraph.outR[0], outR);
    };
};
};