//
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Author: Mark Rollins

#pragma once

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <stdlib.h>
#define Q(x) #x
#define QUOTE(x) Q(x)

namespace s2mm {
struct cint16 {};
struct cint32 {};
struct cfloat {};
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
// static constexpr unsigned DATAWIDTH      =std::is_same<TT_DATA, cint16>::value ? 32 : 64; // it can be one of cint16,
// cint32, cfloat
static constexpr unsigned samplesPerRead = NBITS / DATAWIDTH;
static constexpr unsigned ptSizeD1 = fnPtSizeD1<POINT_SIZE>();
typedef ap_uint<NBITS / samplesPerRead> TT_SAMPLE; // Samples are 'cint32'
typedef hls::stream<TT_DATA> TT_STREAM;
static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM>();
static constexpr unsigned ptSizeD2 = POINT_SIZE / ptSizeD1;
static constexpr unsigned ptSizeD2Ceil = fnCeil<ptSizeD2, NSTREAM>();
static constexpr unsigned memSizeAct = (ptSizeD2Ceil * ptSizeD1) / samplesPerRead;
};

// Run:
void s2mm_wrapper(s2mm::TT_DATA mem[NITER][s2mm::memSizeAct], s2mm::TT_STREAM sig_i[NSTREAM]);