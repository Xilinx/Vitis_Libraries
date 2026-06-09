/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
//
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Author: Mark Rollins

#pragma once

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <cstring>
#define Q(x) #x
#define QUOTE(x) Q(x)
namespace mm2s {
struct cint16 {};
struct cint32 {};
struct cfloat {};

#ifndef POINT_SIZE_D1
#define POINT_SIZE_D1 1
#endif

template <unsigned int TP_POINT_SIZE>
static constexpr unsigned int fnPtSizeD1() {
    unsigned int sqrtVal =
        TP_POINT_SIZE == 65536
            ? 256
            : TP_POINT_SIZE == 32768
                  ? 256
                  : TP_POINT_SIZE == 16384
                        ? 128
                        : TP_POINT_SIZE == 8192
                              ? 128
                              : TP_POINT_SIZE == 4096
                                    ? 64
                                    : TP_POINT_SIZE == 2048
                                          ? 64
                                          : TP_POINT_SIZE == 1024
                                                ? 32
                                                : TP_POINT_SIZE == 512
                                                      ? 32
                                                      : TP_POINT_SIZE == 256
                                                            ? 16
                                                            : TP_POINT_SIZE == 128
                                                                  ? 16
                                                                  : TP_POINT_SIZE == 64
                                                                        ? 8
                                                                        : TP_POINT_SIZE == 32
                                                                              ? 8
                                                                              : TP_POINT_SIZE == 16 ? 4 : 0;
    return sqrtVal;
}

template <unsigned int len, unsigned int rnd>
static constexpr unsigned int fnCeil() {
    return (len + rnd - 1) / rnd * rnd;
}

static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz
typedef ap_uint<NBITS> TT_DATA;        // Equals two 'cint32' samples
typedef hls::stream<TT_DATA> TT_STREAM;
static constexpr unsigned LOOP_CNT = NITER;
// static constexpr int DATAWIDTH = std::is_same<DATA_TYPE, cint16>::value ? 32:64;
static constexpr unsigned samplesPerRead = NBITS / DATAWIDTH;
typedef ap_uint<DATAWIDTH> TT_SAMPLE; // Samples are 'cint32'

// DUAL_STREAMS feature: if DUAL_STREAMS=1, effectively double the number of output streams
// This allows operating as if SSR was doubled
#ifndef API_IO
static constexpr unsigned API_IO = 0;
#endif
// API_IO will only be allowed for AIE1, so dual streams is inferred.
static constexpr unsigned NSTREAM_INT = (API_IO == 1) ? (NSTREAM * 2) : NSTREAM;
static constexpr unsigned ptSizeD1 = (POINT_SIZE_D1 == 1) ? fnPtSizeD1<POINT_SIZE>() : POINT_SIZE_D1;
static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM_INT>();
static constexpr unsigned ptSizeD2 = POINT_SIZE / ptSizeD1;
static constexpr unsigned ptSizeD2Ceil = fnCeil<ptSizeD2, NSTREAM_INT>();
static constexpr unsigned memSizeAct =
    (ptSizeD2Ceil * ptSizeD1) / samplesPerRead; // fnCeil<POINT_SIZE, NSTREAM*samplesPerRead>()/samplesPerRead;
                                                // //(ptSizeD2Ceil * ptSizeD1Ceil)/2; 4100/2 = 2050
};

// Run:
void mm2s_wrapper(mm2s::TT_DATA mem[NITER][mm2s::memSizeAct], mm2s::TT_STREAM sig_o[mm2s::NSTREAM_INT]);