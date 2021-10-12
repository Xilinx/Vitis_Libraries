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

#ifndef _XF_GRAPH_DIAMETER_HPP_
#define _XF_GRAPH_DIAMETER_HPP_

#include <queue>
#include <stdlib.h>
#include <limits>
#include <ctime>
#include <hls_stream.h>
#include <cfloat>
#include <ap_int.h>
#include <stdint.h>

namespace xf {
namespace graph {
namespace internal {
namespace diameter {
#define CONTROL_LTNCY // if there is a control of FADD latency

const int FADD_LTNCY = 5;
const int PARA_NUM = 2;        // Degree of paralleism
const int LOOP_NUM = 1;        // Interator size
const int RANDOM_OFFSET = 380; // Start point

template <typename MType>
union f_cast;

template <>
union f_cast<float> {
    float f;
    uint32_t i;
};

struct IdWeight {
    unsigned column;
    float weight;
};

void my_sssp(unsigned numVert,
             unsigned numEdge,
             unsigned source,
             unsigned* offset,
             IdWeight* idw,
             unsigned& dst,
             float& dst_distance) {
    hls::stream<unsigned, 4096> q;
#pragma HLS BIND_STORAGE variable = q type = fifo
    float distance[4096];

    unsigned source_true = source + RANDOM_OFFSET;

    for (int i = 0; i < numVert; i++) {
        distance[i] = FLT_MAX;
    }
    distance[source_true] = 0;
    q.write(source_true);

    int loop_cnt;
    while (!q.empty()) {
#pragma HLS dependence variable = distance intra true
        unsigned tmp = q.read();
        int start = offset[tmp], end = offset[tmp + 1];
        for (int i = start; i < end; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = distance inter false
            float fromDist = distance[tmp];
            float toDist = distance[idw[i].column];
            float curDist = fromDist + idw[i].weight;
#ifdef CONTROL_LTNCY
            float subDist = curDist - toDist;
#pragma HLS BIND_OP variable = curDist op = fadd impl = fabric latency = FADD_LTNCY
#pragma HLS BIND_OP variable = subDist op = fsub impl = fabric latency = FADD_LTNCY

            f_cast<float> cast;
            cast.f = subDist;
            ap_int<32> ap_subDist = cast.i;
            if (ap_subDist(31, 31))
#else
            if (curDist < toDist)
#endif
            {
                distance[idw[i].column] = curDist;
                q.write(idw[i].column);
            }
            loop_cnt++;
        }
    }

    float max_distance = 0;
    unsigned max_dist = 0;
    for (int i = 0; i < numVert; i++) {
#ifdef CONTROL_LTNCY
#pragma HLS PIPELINE ii = FADD_LTNCY
        float sub;
        if (distance[i] != FLT_MAX) {
            sub = max_distance - distance[i];
        } else {
            sub = 1;
        }
#pragma HLS BIND_OP variable = sub op = fsub impl = fabric latency = FADD_LTNCY
        f_cast<float> cast;
        cast.f = sub;
        ap_int<32> ap_sub = cast.i;
        if (ap_sub(31, 31))
#else
        if (max_distance < distance[i] && distance[i] != FLT_MAX)
#endif
        {
            max_distance = distance[i];
            max_dist = i;
        }
    }

    dst = max_dist;
    dst_distance = max_distance;
}

void diameter(unsigned numVert,
              unsigned numEdge,
              unsigned* offset,
              unsigned* column,
              float* weight,
              float* max_dist,
              unsigned* src,
              unsigned* des) {
#pragma HLS DATAFLOW
    static IdWeight idw[PARA_NUM][42000];
    static unsigned offset_cpy[PARA_NUM][4096];
#pragma HLS ARRAY_PARTITION variable = idw dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = offset_cpy dim = 1 complete

    static unsigned source[PARA_NUM];
    static unsigned destination[PARA_NUM];
    static float distance[PARA_NUM];
#pragma HLS ARRAY_PARTITION variable = source dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = destination dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = distance dim = 1 complete

    for (int i = 0; i < numEdge; i++) {
        for (int n = 0; n < PARA_NUM; n++) {
#pragma HLS UNROLL
            idw[n][i].column = column[i];
            idw[n][i].weight = weight[i];
            source[n] = (numVert - 1) / PARA_NUM * n;
        }
    }

    for (int i = 0; i < numVert + 1; i++) {
        for (int n = 0; n < PARA_NUM; n++) {
#pragma HLS UNROLL
            offset_cpy[n][i] = offset[i];
        }
    }

    for (int n = 0; n < PARA_NUM; n++) {
#pragma HLS UNROLL

        //              destination[n] = (numVert-1) / (n+1);

        xf::graph::internal::diameter::my_sssp(numVert, numEdge, source[n], offset_cpy[n], idw[n], destination[n],
                                               distance[n]);
    }

    float max_distance = 0;
    unsigned max_des = 0;
    unsigned max_src = 0;
    for (int i = 0; i < PARA_NUM; i++) {
#pragma HLS PIPELINE ii = FADD_LTNCY
#ifdef CONTROL_LTNCY
        float sub = max_distance - distance[i];
#pragma HLS BIND_OP variable = sub op = fsub impl = fabric latency = FADD_LTNCY
        f_cast<float> cast;
        cast.f = sub;
        ap_int<32> ap_sub = cast.i;
        if (ap_sub(31, 31))
#else
        if (max_distance < distance[i])
#endif
        {
            max_distance = distance[i];
            max_des = destination[i];
            max_src = source[i];
        }
    }

    src[0] = max_src + RANDOM_OFFSET;
    des[0] = max_des;
    max_dist[0] = max_distance;
}

} // namespace diameter
} // namespace internal

/**
 * @brief diameter estimate based on the sssp algorithm
 *
 * @param numVert vertex number of the input graph
 * @param numEdge edge number of the input graph
 * @param offset row offset of CSR format
 * @param column column index of CSR format
 * @param weight weight value of CSR format
 * @param max_distance the result of max distance
 * @param src the result of source vertex
 * @param des the result of destination vertex
 *
 */
void estimated_diameter(unsigned numVert,
                        unsigned numEdge,
                        unsigned* offset,
                        unsigned* column,
                        float* weight,
                        float* max_dist,
                        unsigned* src,
                        unsigned* des) {
    const int FADD_LTNCY = 5;
    const int PARA_NUM = 8;        // Degree of paralleism
    const int LOOP_NUM = 1;        // Interator size
    const int RANDOM_OFFSET = 380; // Start point

    internal::diameter::diameter(numVert, numEdge, offset, column, weight, max_dist, src, des);
}

} // namespace graph
} // namespace xf

#endif
