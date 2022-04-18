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

#ifndef _XF_GRAPH_MIS_HPP_
#define _XF_GRAPH_MIS_HPP_

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include "L2_utils.hpp"

namespace xf {
namespace graph {
namespace internal {
namespace mis {

static ap_uint<32> count_S;

// get_rand: Get pesuo-random number by hash function
template <long int HASH_RANGE>
unsigned int get_rand(unsigned int key) {
#pragma HLS INLINE

    // paras init
    const unsigned int FNV_PRIME = 16777619;
    const unsigned int OFFSET_BASIS = 2166136261;
    const unsigned int HASH_CODE_LEN = 4;

    // Hash generator: FNV hash-1
    unsigned char* buf = (unsigned char*)&key;
    unsigned int hash = OFFSET_BASIS;

    for (int i = 0; i < HASH_CODE_LEN; i++) {
#pragma HLS UNROLL
        hash = (hash * FNV_PRIME) % HASH_RANGE;
        hash = hash ^ (unsigned int)buf[i];
    }

    return hash;
}

template <int V_NULL>
void Get_Unselect_V(int m, int* S_group, hls::stream<int>& vertex_out) {
#pragma HLS INLINE off

    // output select vertex
    for (int v = 0; v < m + 1; v++) {
#pragma HLS PIPELINE II = 1
        int v_status = S_group[v];

        if (v == m) {
            vertex_out.write(V_NULL);
        } else if (v_status == 0) {
            vertex_out.write(v);
        }
    }
}

template <int V_NULL>
void Get_degree(const int* offset,            // HBM memory(read): offset of graph
                hls::stream<int>& str_v1,     // stream_in: unselected in C_group[MAX_V]
                hls::stream<int>& off1_out,   // stream_out: offset of v
                hls::stream<int>& vertex_out, // stream_out: vertex queue
                hls::stream<int>& degree_out  // stream_out: degree queue
                ) {
#pragma HLS INLINE off

    bool stop_flag = 0;
    int current_d = 0;
    int offset_1 = 0;
    int offset_2 = 0;
    int pre_offset = 0;
    int degree = 0;
    int cnt = 0;

    while (!stop_flag) {
#pragma HLS PIPELINE II = 2
        int v = str_v1.read();

        if (v == V_NULL) {
            stop_flag = 1;

            vertex_out.write(V_NULL);
            degree_out.write(V_NULL);
            off1_out.write(V_NULL);
        } else {
            int d = offset[v + 1] - offset[v];
            int offset_1 = offset[v];

            vertex_out.write(v);
            degree_out.write(d);
            off1_out.write(offset_1);
        }
    }
}

template <int V_NULL, long int HASH_RANGE>
void v_edgstr_e3(const int* indices,
                 // input
                 hls::stream<int>& offset_1_str,
                 hls::stream<int>& str_v4,
                 hls::stream<int>& d_str,
                 // output
                 hls::stream<int>& str_v4put,
                 hls::stream<int>& e_output,
                 hls::stream<int>& d_output,
                 hls::stream<unsigned int>& hash_e_output,
                 hls::stream<unsigned int>& hash_str_v4put) {
#pragma HLS INLINE off

    bool stop_flag = 0;
    int v = 0;
    int d = 0;
    int cnt = 0;
    int idx = 0;
    int offset_1 = 0;

    while (!stop_flag) {
#pragma HLS PIPELINE II = 1
        // Load v,d,offset every vertex
        if (cnt == 0) {
            v = str_v4.read();
            d = d_str.read();
            offset_1 = offset_1_str.read();

            if (d == 0) {
                cnt = 0;
            } else {
                cnt = d - 1;
            }
        } else {
            cnt--;
        }

        if (v == V_NULL) {
            stop_flag = 1;

            // output stream ending flag
            str_v4put.write(V_NULL);
            e_output.write(V_NULL);
            d_output.write(V_NULL);
            hash_e_output.write(HASH_RANGE - 1);
            hash_str_v4put.write(HASH_RANGE - 1);
        } else {
            int idx = offset_1 + cnt;
            int e = indices[idx];
            unsigned int hash_e = get_rand<HASH_RANGE>(e);
            unsigned int hash_v = get_rand<HASH_RANGE>(v);

            // output generated stream to interface
            str_v4put.write(v);
            e_output.write(e);
            d_output.write(d);
            hash_e_output.write(hash_e);
            hash_str_v4put.write(hash_v);
        }
    }
}

template <int V_NULL, long int HASH_RANGE>
void Check_V(int* C_group,
             // input
             hls::stream<int>& vertex_input,
             hls::stream<int>& edge_input,
             hls::stream<int>& degree,
             hls::stream<unsigned int>& hash_e_input,
             hls::stream<unsigned int>& hash_v_input,
             // stream output to Update CS
             hls::stream<int>& v_edge_out,
             hls::stream<int>& v_edge_d_out,
             hls::stream<int>& str_v4,
             hls::stream<int>& str_status_4) {
#pragma HLS INLINE off

    int cnt = 0;
    bool stop_flag = 0;
    unsigned int min_val = HASH_RANGE - 1;

    while (!stop_flag) {
#pragma HLS PIPELINE II = 1
        int v = vertex_input.read();
        int e = edge_input.read();
        int d = degree.read();
        unsigned int hash_v = hash_v_input.read();
        unsigned int hash_e = hash_e_input.read();
        int v_status = 0;
        int e_status = C_group[e];

        if (v == V_NULL) {
            stop_flag = 1;

            v_edge_d_out.write(V_NULL);
            str_status_4.write(V_NULL);
            str_v4.write(V_NULL);
            v_edge_out.write(V_NULL);
        } else {
            v_edge_out.write(e); // stream out

            if (cnt == d - 1) {
                if (e_status < 0) {
                    if (hash_v <= min_val) {
                        v_status = -1;
                    }
                } else {
                    if (hash_v <= min_val && hash_v <= hash_e) {
                        v_status = -1;
                    }
                }

                cnt = 0;
                min_val = HASH_RANGE - 1;

                // sub_status
                str_status_4.write(v_status);
                str_v4.write(v);
                v_edge_d_out.write(d);
            } else {
                if (e_status < 0) {
                    min_val = min_val;
                } else if (hash_e <= min_val) {
                    min_val = hash_e;
                }

                cnt++;
            }
        }
    }
}

template <int V_NULL>
void Update_CS(
    // memory
    int* S_group,
    int* C_group,
    int* res_out,
    // pointer
    int* select_cnt,
    // stream in
    hls::stream<int>& edge_in_str,
    hls::stream<int>& degree_in_str,
    hls::stream<int>& v_in,
    hls::stream<int>& v_status_in) {
#pragma HLS INLINE off

    bool stop_flag = 0;
    int cnt = 0;
    int v_status = 0;
    int v = 0;
    int d = 0;
    int e = 0;

    while (!stop_flag) {
#pragma HLS PIPELINE II = 1
        if (cnt == 0) {
            v = v_in.read();
            v_status = v_status_in.read();
            d = degree_in_str.read();
            e = 0;
            if (v_status == -1) {
                S_group[v] = -1;
                C_group[v] = -1;
                res_out[count_S] = v;
                count_S++;
                *select_cnt = *select_cnt + 1;
            }

            cnt = d;
        } else {
            e = edge_in_str.read();

            if (v_status == -1) {
                S_group[e] = -2;
                C_group[e] = -2;
            }

            cnt--;
        }

        // counter & flag update
        if (e == V_NULL) {
            stop_flag = 1;
            res_out[count_S] = V_NULL;
        }
    }
}

inline void mem_sync(int m, int* S_group_0, int* S_group_1, int* C_group_0, int* C_group_1) {
#pragma HLS INLINE off

    for (int i = 0; i < m; i++) {
#pragma HLS PIPELINE II = 1
        S_group_0[i] = S_group_1[i];
        C_group_0[i] = C_group_1[i];
    }
}

template <int V_NULL, long int HASH_RANGE, int M_DEPTH>
void MIS_dataflow(int* C_group_0,
                  int* C_group_1,
                  int* S_group_0,
                  int* S_group_1,
                  int* res_out,
                  const int* offset,
                  const int* indices,
                  int m,
                  int* select_cnt) {
#pragma HLS INLINE off

#pragma HLS DATAFLOW

    hls::stream<int> str_v1("str_v1");
#pragma HLS STREAM variable = str_v1 depth = M_DEPTH
    hls::stream<int> str_off1("str_off1");
#pragma HLS STREAM variable = str_off1 depth = M_DEPTH
    hls::stream<int> str_v2("str_v2");
#pragma HLS STREAM variable = str_v2 depth = M_DEPTH
    hls::stream<int> str_d2("str_d2");
#pragma HLS STREAM variable = str_d2 depth = M_DEPTH
    hls::stream<int> str_v3("str_v3");
#pragma HLS STREAM variable = str_v3 depth = M_DEPTH
    hls::stream<int> str_e3("str_e3");
#pragma HLS STREAM variable = str_e3 depth = M_DEPTH
    hls::stream<int> str_d3("str_d3");
#pragma HLS STREAM variable = str_d3 depth = M_DEPTH
    hls::stream<unsigned int> str_hash_e3("str_hash_e3");
#pragma HLS STREAM variable = str_hash_e3 depth = M_DEPTH
    hls::stream<unsigned int> str_hash_v3("str_hash_v3");
#pragma HLS STREAM variable = str_hash_v3 depth = M_DEPTH
    hls::stream<int> str_v4("str_v4");
#pragma HLS STREAM variable = str_v4 depth = M_DEPTH
    hls::stream<int> str_status_4("str_status_4");
#pragma HLS STREAM variable = str_status_4 depth = M_DEPTH
    hls::stream<int> str_d4("str_d4");
#pragma HLS STREAM variable = str_d4 depth = M_DEPTH
    hls::stream<int> str_e4("str_e4");
#pragma HLS STREAM variable = str_e4 depth = M_DEPTH

    // check status in S_group: select vertex with status-0
    Get_Unselect_V<V_NULL>(m, S_group_0, str_v1);

    // Get degree for selected vertex
    Get_degree<V_NULL>(offset, str_v1, str_off1, str_v2, str_d2);

    // Get edge according to each vertex: ignore removed edges
    v_edgstr_e3<V_NULL, HASH_RANGE>(indices, str_off1, str_v2, str_d2, str_v3, str_e3, str_d3, str_hash_e3,
                                    str_hash_v3);

    // Check each vertex and its' neighbors
    Check_V<V_NULL, HASH_RANGE>(C_group_0, str_v3, str_e3, str_d3, str_hash_e3, str_hash_v3, str_e4, str_d4, str_v4,
                                str_status_4);

    // Remove selected vertex & edge, output result
    Update_CS<V_NULL>(S_group_1, C_group_1, res_out, select_cnt, str_e4, str_d4, str_v4, str_status_4);
}

} // namespace mis
} // namespace internel

/**
 * @brief mis Implement the algorithm which for finding a maximal set of independent vertices
 *
 * @tparam MAX_ROUND the max round number for mis algorithm iterations
 *
 * @param m the vertex number of current graph
 * @param offset offset for undirect-graph
 * @param indices indices for undirect-graph
 * @param C_group_0 status field, C-0
 * @param C_group_1 status field, C-1
 * @param S_group_0 status field, S-0
 * @param S_group_1 status field, S-1
 * @param res_out selected mis result(output)
 *
 */
template <int MAX_ROUND>
void mis(int m,              // vertex number of current graph, REG: axi-lite
         const int* offset,  // graph offset, read-only, HBM: Read
         const int* indices, // graph indices, read-only HBM: Read
         int* C_group_0,     // status field, C-0, HBM: Read & Write
         int* C_group_1,     // status field, C-1, HBM: Read & Write
         int* S_group_0,     // status field, S-0, HBM: Read & Write
         int* S_group_1,     // status field, S-1, HBM: Read & Write
         int* res_out        // selected mis result(output), HBM: Write
         ) {
#pragma HLS INLINE off

    // paras init
    const long int HASH_RANGE = 4294967296;
    const int V_NULL = -256;
    const int M_DEPTH = 40000;

    // MIS Impl
    for (int round = 0; round < MAX_ROUND; round++) {
#pragma HLS PIPELINE off
        int select_cnt = 0;
        xf::graph::internal::mis::MIS_dataflow<V_NULL, HASH_RANGE, M_DEPTH>(C_group_0, C_group_1, S_group_0, S_group_1,
                                                                            res_out, offset, indices, m, &select_cnt);
        xf::graph::internal::mis::mem_sync(m, C_group_0, C_group_1, S_group_0, S_group_1);

        if (select_cnt == 0) {
            break;
        }
    }
}

} // namespace graph
} // namespace xf
#endif
