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
#ifndef IFFT_BACK_TRANSPOSE_H
#define IFFT_BACK_TRANSPOSE_H

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <hls_streamofblocks.h>
#include "back_transpose_simple_class.h"

using namespace back_transpose_simple;

void back_transpose_simple_wrapper(backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_IN sig_i[SSR],
                                   backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_OUT sig_o[SSR]);

#endif