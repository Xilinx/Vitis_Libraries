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
#ifndef IFFT_BACK_TRANSPOSE_CPP
#define IFFT_BACK_TRANSPOSE_CPP

#include "ifft_back_transpose.h"

using namespace std;
using namespace back_transpose;

void ifft_back_transpose_wrapper(backTransposeCls<POINT_SIZE, SSR, VSS_MODE>::TT_STREAM sig_i[SSR],
                                 backTransposeCls<POINT_SIZE, SSR, VSS_MODE>::TT_STREAM sig_o[SSR]) {
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS DATAFLOW
    static backTransposeCls<POINT_SIZE, SSR, VSS_MODE> uut;
    uut.ifft_back_transpose_top(sig_i, sig_o);
}

#endif