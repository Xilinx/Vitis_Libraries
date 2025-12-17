/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/

#include "hw/qrdfloat.hpp"

typedef float Type;
const int M = QRF_A_ROWS; // rows
const int N = QRF_A_COLS; // columns

#if (QRF_A_ROWS <= 1024 && QRF_A_ROWS > 512)
const int PowUnroll_t = 10;
const int PowNCU_t = 5;
#elif (QRF_A_ROWS <= 512 && QRF_A_ROWS > 256)
const int PowUnroll_t = 9;
const int PowNCU_t = 5;
#elif (QRF_A_ROWS <= 256 && QRF_A_ROWS > 128)
const int PowUnroll_t = 8;
const int PowNCU_t = 5;
#elif (QRF_A_ROWS <= 128 && QRF_A_ROWS > 64)
const int PowUnroll_t = 7;
const int PowNCU_t = 2;
#elif (QRF_A_ROWS <= 64 && QRF_A_ROWS > 32)
const int PowUnroll_t = 6;
const int PowNCU_t = 2;
#elif (QRF_A_ROWS <= 32 && QRF_A_ROWS > 16)
const int PowUnroll_t = 5;
const int PowNCU_t = 2;
#elif (QRF_A_ROWS <= 16 && QRF_A_ROWS > 8)
const int PowUnroll_t = 4;
const int PowNCU_t = 2;
#elif (QRF_A_ROWS <= 8 && QRF_A_ROWS > 4)
const int PowUnroll_t = 3;
const int PowNCU_t = 1;
//#else
// std::cout<<"ERROR input support is max 1024 rows, min 2 rows, [2, 3 ,4, ... 1023, 1024]"<< std::endl;
#endif

const int POWFoldRow_t = 2;
const int NCU_t = 1 << PowNCU_t;
const int UnrollSize_t = 1 << (PowUnroll_t - POWFoldRow_t);
const int CUSize_t = 1 << (PowUnroll_t - POWFoldRow_t - PowNCU_t);

// List two top ip for easy use
extern "C" void qrd_top(Type dataA[NCU_t][M / NCU_t][N], hls::stream<Type>& R_strm) {
#pragma HLS INTERFACE axis port = R_strm
#pragma HLS INTERFACE s_axilite port = return bundle = control
#ifndef __SYNTHESIS__
    printf("[kernel] qrd_float float estimate performance\n");
#endif
    xf::solver::qrd_float_core<M, N, PowUnroll_t, POWFoldRow_t, NCU_t, PowNCU_t, Type>(dataA, R_strm);
}

extern "C" void qrd_ip_top(Type* A, hls::stream<Type>& R_strm) {
    const uint32_t depthA = M * N;
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = A latency = 128 num_read_outstanding = \
    32 max_read_burst_length = 32 depth = depthA
#pragma HLS INTERFACE axis port = R_strm
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#ifndef __SYNTHESIS__
    printf("[kernel] qrd_ip_float float estimate resource\n");
#endif
    // Verify that template parameters are correct in simulation
    if (M < N) {
#ifndef __SYNTHESIS__
        printf("ERROR: qrd_float/src/qrd.cpp: parameter error - M must be greater than N; M = %d N = %d\n", M, N);
#endif
        exit(1);
    }
    // Buffers
    Type dataA[NCU_t][M / NCU_t][N];
#pragma HLS BIND_STORAGE variable = dataA type = RAM_T2P impl = BRAM latency = 1
#pragma HLS ARRAY_PARTITION variable = dataA complete dim = 1
#pragma HLS ARRAY_PARTITION variable = dataA type = cyclic factor = CUSize_t dim = 2

OUT_LOOP_LOAD_A:
    for (int r = 0; r < M; r++) {
        for (int c = 0; c < N; c++) {
#pragma HLS PIPELINE
            dataA[r % (NCU_t)][r / (NCU_t)][c] = A[r * N + c];
        }
    }

    qrd_top(dataA, R_strm);

OUT_LOOP_WRITE_Q:
    for (int r = 0; r < M; r++) {
        for (int c = 0; c < N; c++) {
            A[r * N + c] = dataA[r % (NCU_t)][r / (NCU_t)][c];
        }
    }
}

// for ping-pong buffer IP

// extern "C" void qrd_ip_full_bandwidth(
// #ifndef __SYNTHESIS__
//     Type** A,
// #else
//     Type A[M][N],
// #endif
//     hls::stream<Type >& R_strm
// ){
//     const uint32_t depthA = M*N;
// //#pragma HLS INTERFACE mode = bram port = A storage_type=RAM_T2P latency = 1
// //#pragma HLS ARRAY_PARTITION variable = A type=cyclic factor =M/4 dim = 1
// #pragma HLS INTERFACE axis port = R_strm
// #pragma HLS INTERFACE s_axilite port = return bundle = control
// #ifndef __SYNTHESIS__
//     printf("POW is %d\n", POW);
// #endif
//     xf::solver::qrd_ip_top_full_bandwidth<M, N , POW, M/4, Type>(A, R_strm);
// }