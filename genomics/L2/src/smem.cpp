/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "smemmodules.hpp"
#include "smem.hpp"
extern "C" {
void mem_collect_intv_core(ap_uint<512>* bwt,
                           ap_uint<256>* mem,
                           ap_uint<64>* bwt_para,
                           ap_uint<32>* mem_num,
                           uint8_t* seq,
                           uint8_t* seq_len,
                           int batch_size) {
#pragma HLS INTERFACE m_axi port = bwt_para offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = seq offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = seq_len offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = mem offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = mem_num offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = bwt offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = bwt_para
#pragma HLS INTERFACE s_axilite port = seq
#pragma HLS INTERFACE s_axilite port = seq_len
#pragma HLS INTERFACE s_axilite port = mem
#pragma HLS INTERFACE s_axilite port = mem_num
#pragma HLS INTERFACE s_axilite port = batch_size
#pragma HLS INTERFACE s_axilite port = bwt
#pragma HLS INTERFACE ap_ctrl_chain port = return
    xf::genomics::bwtint_t bwt_primary = bwt_para[0];
    xf::genomics::bwtint_t L2[5];
#pragma HLS array_partition variable = L2 complete
    for (int i = 0; i < 5; i++) {
#pragma HLS unroll
        L2[i] = bwt_para[i + 1];
    }
    xf::genomics::bwtint_t bwt_size = bwt_para[6];

    xf::genomics::ddr_streaming(bwt_primary, L2, seq, seq_len, mem, mem_num, batch_size, bwt, bwt_size);
}
}
