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

#include "axis_split_join.h"
#define NBITS 128
typedef hls::stream<ap_uint<NBITS> > TT_STREAM_EXT;
typedef hls::stream<ap_uint<SAMPLE_SIZE> > TT_STREAM_INT;
#define SSR_INT (NBITS / SAMPLE_SIZE) * SSR

void splitter_wrapper(TT_STREAM_EXT sig_i[SSR], TT_STREAM_INT sig_i_int[SSR_INT]) {
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS DATAFLOW
    static axisSplitJoin<NBITS, SSR, SSR_INT> uut_split;
    uut_split.stream_splitter(sig_i, sig_i_int);
}

void joiner_wrapper(TT_STREAM_INT sig_o_int[SSR_INT], TT_STREAM_EXT sig_o[SSR]) {
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS DATAFLOW
    static axisSplitJoin<SAMPLE_SIZE, SSR_INT, SSR> uut_join;
    uut_join.stream_joiner(sig_o_int, sig_o);
}