/*
 * Copyright 2019-2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 olver_L2.hpp    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "vpp_acc.hpp"
#include "xf_graph_L2.hpp"

#include <ap_int.h>
#include <hls_stream.h>

#define MAXDEGREE (10 * 4096)

class bfs_acc : public VPP_ACC<bfs_acc, /*NCU=*/1> {
    ZERO_COPY(column, column[(numEdges + 15) / 16]);
    ZERO_COPY(offset, offset[(vertexNum + 1 + 15) / 16]);
    ZERO_COPY(queue, queue[vertexNum]);
    ZERO_COPY(queue512, queue512[(vertexNum + 15) / 16]);
    ZERO_COPY(color512, color512[(vertexNum + 15) / 16]);
    ZERO_COPY(result_dt, result_dt[((vertexNum + 15) / 16) * 16]);
    ZERO_COPY(result_ft, result_ft[((vertexNum + 15) / 16) * 16]);
    ZERO_COPY(result_pt, result_pt[((vertexNum + 15) / 16) * 16]);
    ZERO_COPY(result_lv, result_lv[((vertexNum + 15) / 16) * 16]);

    SYS_PORT(column, DDR[0]);
    SYS_PORT(offset, DDR[0]);
    SYS_PORT(queue, DDR[0]);
    SYS_PORT(queue512, DDR[0]);
    SYS_PORT(color512, DDR[0]);
    SYS_PORT(result_dt, DDR[0]);
    SYS_PORT(result_ft, DDR[0]);
    SYS_PORT(result_pt, DDR[0]);
    SYS_PORT(result_lv, DDR[0]);

    SYS_PORT_PFM(u50, column, HBM[0]);
    SYS_PORT_PFM(u50, offset, HBM[0]);
    SYS_PORT_PFM(u50, queue, HBM[0]);
    SYS_PORT_PFM(u50, queue512, HBM[0]);
    SYS_PORT_PFM(u50, color512, HBM[0]);
    SYS_PORT_PFM(u50, result_dt, HBM[0]);
    SYS_PORT_PFM(u50, result_ft, HBM[0]);
    SYS_PORT_PFM(u50, result_pt, HBM[0]);
    SYS_PORT_PFM(u50, result_lv, HBM[0]);

   public:
    static void compute(const int srcID,
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
                        ap_uint<32>* result_lv);

    static void hls_kernel(const int srcID,
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
                           ap_uint<32>* result_lv);
};
