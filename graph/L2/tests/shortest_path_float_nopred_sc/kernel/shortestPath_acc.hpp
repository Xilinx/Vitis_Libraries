/*
 * Copyright 2019-2021 Xilinx, Inc.
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
#pragma once
#include "vpp_acc.hpp"
#include "xf_graph_L2.hpp"
#include <ap_int.h>

#define MAXOUTDEGREE (4096 * 10)

class shortestPath_acc : public VPP_ACC<shortestPath_acc, /*NCU=*/1> {
    ZERO_COPY(config32, config32[6]);
    ZERO_COPY(offset512, offset512[(numVertices + 1 + 15) / 16]);
    ZERO_COPY(column512, column512[(numEdges + 15) / 16]);
    ZERO_COPY(weight512, weight512[(numEdges + 15) / 16]);
    ZERO_COPY(queue512, queue512[(10 * 300 * 4096) / 16]);
    ZERO_COPY(queue32, queue32[10 * 300 * 4096]);
    ZERO_COPY(result512, result512[(((numVertices + 15) / 16 + 63) / 64) * 64]);
    ZERO_COPY(result32, result32[((numVertices + 1023) / 1024) * 1024]);
    ZERO_COPY(info8, info8[4]);

    SYS_PORT(config32, DDR[0]);
    SYS_PORT(offset512, DDR[0]);
    SYS_PORT(column512, DDR[0]);
    SYS_PORT(weight512, DDR[0]);
    SYS_PORT(queue512, DDR[0]);
    SYS_PORT(queue32, DDR[0]);
    SYS_PORT(result512, DDR[0]);
    SYS_PORT(result32, DDR[0]);
    SYS_PORT(info8, DDR[0]);

   public:
    static void compute(int numVertices,
                        int numEdges,
                        ap_uint<32>* config32,
                        ap_uint<512>* offset512,
                        ap_uint<512>* column512,
                        ap_uint<512>* weight512,
                        ap_uint<512>* queue512,
                        ap_uint<32>* queue32,
                        ap_uint<512>* result512,
                        ap_uint<32>* result32,
                        ap_uint<8>* info8);

    static void hls_kernel(int numVertices,
                           int numEdges,
                           ap_uint<32>* config32,
                           ap_uint<512>* offset512,
                           ap_uint<512>* column512,
                           ap_uint<512>* weight512,
                           ap_uint<512>* queue512,
                           ap_uint<32>* queue32,
                           ap_uint<512>* result512,
                           ap_uint<32>* result32,
                           ap_uint<8>* info8);
};
