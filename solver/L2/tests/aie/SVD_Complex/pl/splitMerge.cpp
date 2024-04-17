/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "splitMerge.hpp"

extern "C" void splitMerge(int DATANUM,
                           int LOOP,
                           hls::stream<ap_axiu<WDATA, 0, 0, 0> >& in_axis_strm,
                           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm1,
                           hls::stream<ap_axiu<WDATA, 0, 0, 0> >& out_axis_strm,
                           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& from_aie_strm0,
                           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& from_aie_strm1) {
#pragma HLS interface s_axilite bundle = control port = DATANUM
#pragma HLS interface s_axilite bundle = control port = LOOP

#pragma HLS interface axis port = in_axis_strm
#pragma HLS interface axis port = to_aie_strm0
#pragma HLS interface axis port = to_aie_strm1
#pragma HLS interface axis port = out_axis_strm
#pragma HLS interface axis port = from_aie_strm0
#pragma HLS interface axis port = from_aie_strm1

#pragma HLS interface s_axilite bundle = control port = return
#pragma HLS dataflow
    split(DATANUM, LOOP, in_axis_strm, to_aie_strm0, to_aie_strm1);
    merge(DATANUM, LOOP, out_axis_strm, from_aie_strm0, from_aie_strm1);
}