/*
 * Copyright 2020-2021 Xilinx, Inc.
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

#include "bfs_acc.hpp"

void bfs_acc::hls_kernel(const int srcID,
                         const int numEdges,
                         const int vertexNum,

                         ap_uint<512>* column,
                         ap_uint<512>* offset,

                         ap_uint<512>* queue512,
                         ap_uint<32>* queue,
                         ap_uint<512>* color512,

                         ap_uint<32>* result_dt,
                         ap_uint<32>* result_ft,
                         ap_uint<32>* result_pt,
                         ap_uint<32>* result_lv) {
    xf::graph::bfs<MAXDEGREE>(srcID, vertexNum, offset, column, queue512, queue, color512, result_dt, result_ft,
                              result_pt, result_lv);
}
