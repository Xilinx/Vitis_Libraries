/*
 * Copyright 2022 Xilinx, Inc.
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
#include "ap_int.h"
#include <hls_stream.h>
#include "merge.hpp"
#include <hls_burst_maxi.h>

#define DWIDTHS (256)
#define CSRWIDTHS (256)
#define COLORWIDTHS (32)
//#define NUM (DWIDTHS / 32)
#define NUM (1)
#define MAXNV (1 << 26)
#define MAXNE (1 << 27)
#define VERTEXS (MAXNV / NUM)
#define EDGES (MAXNE / NUM)
#define DEGREES (1 << 17)
#define COLORS (4096)
#ifndef HLS_TEST

const int depthVertex = VERTEXS;
const int depthEdge = EDGES;

extern "C" {
#endif
void merge_kernel(int num_v,
                  int num_e,
                  int num_c_out,
                  int* num_e_out,
                  DF_V_T* offset_in,
                  DF_E_T* edges_in,
                  DF_W_T* weights_in,
                  DF_V_T* c,
                  DF_V_T* offset_out,
                  DF_E_T* edges_out,
                  DF_W_T* weights_out,
                  DF_V_T* count_c_single,
#ifdef MANUALLY_BURST
                  hls::burst_maxi<DF_V_T> jump,
#else
                  DF_V_T* jump,
#endif
                  DF_V_T* count_c,
                  DF_V_T* index_c) {
#pragma HLS INTERFACE m_axi port = num_e_out bundle = gmem0 depth = 1 latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = offset_in bundle = gmem1 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = edges_in bundle = gmem2 depth = depthEdge latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = weights_in bundle = gmem3 depth = depthEdge latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = c bundle = gmem4 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = offset_out bundle = gmem5 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = edges_out bundle = gmem6 depth = depthEdge latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = weights_out bundle = gmem7 depth = depthEdge latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = count_c_single bundle = gmem8 depth = depthVertex latency = \
    32 num_read_outstanding = 64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = jump bundle = gmem9 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 2
#pragma HLS INTERFACE m_axi port = count_c bundle = gmem10 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE m_axi port = index_c bundle = gmem11 depth = depthVertex latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 16 num_write_outstanding = 32 max_write_burst_length = 16
#pragma HLS INTERFACE s_axilite port = num_v bundle = control
#pragma HLS INTERFACE s_axilite port = num_e bundle = control
#pragma HLS INTERFACE s_axilite port = num_c_out bundle = control
#pragma HLS INTERFACE s_axilite port = num_e_out bundle = control
#pragma HLS INTERFACE s_axilite port = offset_in bundle = control
#pragma HLS INTERFACE s_axilite port = edges_in bundle = control
#pragma HLS INTERFACE s_axilite port = weights_in bundle = control
#pragma HLS INTERFACE s_axilite port = c bundle = control
#pragma HLS INTERFACE s_axilite port = offset_out bundle = control
#pragma HLS INTERFACE s_axilite port = edges_out bundle = control
#pragma HLS INTERFACE s_axilite port = weights_out bundle = control
#pragma HLS INTERFACE s_axilite port = count_c_single bundle = control
#pragma HLS INTERFACE s_axilite port = jump bundle = control
#pragma HLS INTERFACE s_axilite port = count_c bundle = control
#pragma HLS INTERFACE s_axilite port = index_c bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::graph::merge_kernel_core(num_v, num_e, num_c_out, num_e_out, offset_in, edges_in, weights_in, c, offset_out,
                                 edges_out, weights_out, count_c_single, jump, count_c, index_c);
}
#ifndef HLS_TEST
}
#endif
