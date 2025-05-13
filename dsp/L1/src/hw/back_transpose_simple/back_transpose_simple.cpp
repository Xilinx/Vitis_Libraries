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
#ifndef BACK_TRANSPOSE_CPP
#define BACK_TRANSPOSE_CPP

#include "back_transpose_simple.h"

using namespace std;
using namespace back_transpose_simple;

void back_transpose_simple_wrapper(backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_IN sig_i[SSR],
                                   backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_OUT sig_o[SSR]) { // 2
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS DATAFLOW
    static backTransposeSimpleCls<POINT_SIZE, SSR> uut;
    uut.back_transpose_simple_top(sig_i, sig_o);
}

#endif