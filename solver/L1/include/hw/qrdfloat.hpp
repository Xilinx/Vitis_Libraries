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
#ifndef _QRDFLOAT_HPP_
#define _QRDFLOAT_HPP_

#ifndef __SYNTHESIS__
// For debug
#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdio>
#include "utils/x_matrix_utils.hpp"
#endif

#include "hls_x_complex.h"
//#include <complex>
#include "hls_stream.h"
#include "hls_math.h"

//#define DEBUG_QRD (1)
#define _XF_SOLVER_VOID_CAST static_cast<void>
// XXX toggle here to debug this file
#ifdef DEBUG_QRD
#ifndef __SYNTHESIS__
#define _XF_SOLVER_PRINT(msg...) \
    do {                         \
        printf(msg);             \
    } while (0)
#else
#define _XF_SOLVER_PRINT(msg...) (_XF_SOLVER_VOID_CAST(0))
#endif
#else
#define _XF_SOLVER_PRINT(msg...) (_XF_SOLVER_VOID_CAST(0))
#endif

// namespace
namespace xf {
namespace solver {

// for qrd
#define ram_latency (3)
#define coreII (1)

//---------------------------------SQRT---------------------------------//
template <class T>
void sqrt_layer(T in, T& res) {
#pragma HLS INLINE OFF
#pragma HLS bind_op variable = res op = fsqrt impl = dsp
    res = hls::sqrt(in); // latency=28 when inline off
}

template <class T>
void gen_rsqrt(T in, T& res) {
#pragma HLS INLINE
    T tmp;
#pragma HLS bind_op variable = tmp op = fsqrt impl = dsp
    // sqrt_layer<T>(in, tmp);
    // res = 1.0f / tmp;// latency=28 when inline off
    res = hls::rsqrt(in); // latency=15 when inline off
    // res = InvSqrt(in);// latency=18 when inline off
}

//---------------------------------DSP basic---------------------------------//

// A*B + C
template <class T>
void hls_float_fma( // latency==ii==5
    T& fmaRes,
    T inA,
    T inB,
    T inC) {
#pragma HLS INLINE OFF

    T t_fma;
#pragma HLS BIND_OP variable = t_fma impl = primitivedsp
    t_fma = inA * inB + inC;

    fmaRes = t_fma;
}

//---------------------------------dot---------------------------------//

template <int Rows, int NUMADD, class T>
void adder_layer(T dotRes_i[Rows], T dotRes_o[Rows / 2]) {
Function_qrd_adder_layer_real:;
#pragma HLS INLINE

    for (int i = 0; i < NUMADD; i++) { // 1024/2
#pragma HLS UNROLL
        T tmp_add;
#pragma HLS bind_op variable = tmp_add op = fadd impl = primitivedsp
        tmp_add = dotRes_i[2 * i] + dotRes_i[2 * i + 1];
        dotRes_o[i] = tmp_add;
    }
}

// Tree complex float adder, configable by POW=log2(RowsA)
// max input size is 1024 in one cycle, min is 3. [3, 1024, 2^n] = 3, 4, 8 ,16 ...
template <int RowsA, int POW, class T>
void reduce_add(T dotRes_i[RowsA], T& dotRes) {
Function_qrd_reduce_add_real:;
#pragma HLS INLINE
    // input for each layer adder
    T dotRes_i9[512]; // 1024/2 //sum 1024 to 512
#pragma HLS ARRAY_PARTITION variable = dotRes_i9 complete
    T dotRes_i8[256];
#pragma HLS ARRAY_PARTITION variable = dotRes_i8 complete
    T dotRes_i7[128];
#pragma HLS ARRAY_PARTITION variable = dotRes_i7 complete
    T dotRes_i6[64];
#pragma HLS ARRAY_PARTITION variable = dotRes_i6 complete
    T dotRes_i5[32];
#pragma HLS ARRAY_PARTITION variable = dotRes_i5 complete
    T dotRes_i4[16];
#pragma HLS ARRAY_PARTITION variable = dotRes_i4 complete
    T dotRes_i3[8]; // sum 16 to 8
#pragma HLS ARRAY_PARTITION variable = dotRes_i3 complete
    T dotRes_i2[4];
#pragma HLS ARRAY_PARTITION variable = dotRes_i2 complete
    T dotRes_i1[2];
#pragma HLS ARRAY_PARTITION variable = dotRes_i1 complete

    // set the input level by sel POW=log2(RowsA)
    if (POW == 10) { // 512<RowsA<=1024
        // sum 1024 to 512
        adder_layer<RowsA, 512, T>(dotRes_i, dotRes_i9);
    }

    if (POW > 9) {
        // sum 512 to 256
        adder_layer<512, 256, T>(dotRes_i9, dotRes_i8);
    } else if (POW == 9) { // 256<RowsA<=512
        adder_layer<RowsA, 256, T>(dotRes_i, dotRes_i8);
    }

    if (POW > 8) {
        adder_layer<256, 128, T>(dotRes_i8, dotRes_i7);
    } else if (POW == 8) { // 128<RowsA<=256
        adder_layer<RowsA, 128, T>(dotRes_i, dotRes_i7);
    }

    if (POW > 7) {
        adder_layer<128, 64, T>(dotRes_i7, dotRes_i6);
    } else if (POW == 7) { // RowsA<=128
        adder_layer<RowsA, 64, T>(dotRes_i, dotRes_i6);
    }

    if (POW > 6) {
        adder_layer<64, 32, T>(dotRes_i6, dotRes_i5);
    } else if (POW == 6) { // RowsA<=64
        adder_layer<RowsA, 32, T>(dotRes_i, dotRes_i5);
    }

    if (POW > 5) {
        adder_layer<32, 16, T>(dotRes_i5, dotRes_i4);
    } else if (POW == 5) { // RowsA<=32
        adder_layer<RowsA, 16, T>(dotRes_i, dotRes_i4);
    }

    if (POW > 4) {
        adder_layer<16, 8, T>(dotRes_i4, dotRes_i3);
    } else if (POW == 4) { // RowsA<=16
        adder_layer<RowsA, 8, T>(dotRes_i, dotRes_i3);
    }

    if (POW > 3) {
        adder_layer<8, 4, T>(dotRes_i3, dotRes_i2);
    } else if (POW == 3) { // RowsA<=8
        adder_layer<RowsA, 4, T>(dotRes_i, dotRes_i2);
    }

    if (POW > 2) { // sum 4 to 2
        adder_layer<4, 2, T>(dotRes_i2, dotRes_i1);
    } else if (POW == 2) { // RowsA<=8
        adder_layer<RowsA, 2, T>(dotRes_i, dotRes_i1);
    }

    if (POW == 1) {
        dotRes_i1[0] = dotRes_i[0];
        dotRes_i1[1] = dotRes_i[1];
    }

    // if(RowsA == 3) dotRes_i1[1] = dotRes_i2[2];// corner case when RowsA == 3
    dotRes = dotRes_i1[0] + dotRes_i1[1];
}

template <class T>
void complex_mul_conj(T& mulRes,
                      T lhs, // left hand-side
                      T rhs  // right hand-side
                      ) {
#pragma HLS INLINE

    // mothed 3
    T tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, Res[4];
    T mulA1, mulB1, addC1, mulA2, mulB2, addC2;

    // method 5
    hls_float_fma<T>(tmp1, lhs, rhs, 0);
    mulRes = tmp1;

#ifdef DEBUG_QRD
#ifndef __SYNTHESIS__
// std::cout << x_conj(lhs) << " conj complex mul with " << rhs << " is " << mulRes << std::endl ;
#endif
#endif
}

//---------------------------------fmadd  ---------------------------------//
template <class T>
void complex_mul_main_fma_TEST6(T& mulRes,
                                T add, // add for fma = add + lhs*rhs
                                T lhs, // left hand-side
                                T rhs, // right hand-side
                                const bool is_mul) {
#pragma HLS INLINE

    T tmp1, tmp2;
    // mothed 5
    hls_float_fma<T>(tmp1, is_mul ? lhs : (-lhs), rhs, add);
    mulRes = tmp1;
#ifdef DEBUG_QRD
    tmp2 = is_mul ? lhs : (-lhs);
    std::cout << tmp2 << " det with " << rhs << ", add " << add << "=" << tmp1 << std::endl;
#endif
}

//---------------------------------update proj ---------------------------------//
// latency = M
template <int ColsA, class T>
void qrd_update_proj(hls::stream<T>& tempR_strm,
                     T projOut[ColsA],           // gen use dot
                     hls::stream<T>& proj0_strm, // set to 0 for dot
                     int k) {
#pragma HLS INLINE OFF

    T div, inv_div;

LOOP_buffer_proj:
    for (int j = k + 1; j < ColsA; j++) {
#pragma HLS loop_tripcount min = ColsA / 2 max = ColsA / 2 avg = ColsA / 2
#pragma HLS pipeline II = 1
#pragma HLS dependence array inter false
        T proj = proj0_strm.read();
        if (j == k + 1) { // update dot_self   //k=-1, j=0
            div = proj;
            if (div == 0) {
                _XF_SOLVER_PRINT("ERROR: div is 0! \n");
            }
            inv_div = 1 / div;
        } else {                         // update proj
            projOut[j] = proj * inv_div; // div
        }
        tempR_strm.write(proj);
    } // j
}
//---------------------------------qrd loop R ---------------------------------//
template <int RowsA, int ColsA, class T>
void qrd_write_R(T& norm, hls::stream<T>& R_strm, hls::stream<T>& tempR_strm, int k) {
#pragma HLS INLINE OFF
LOOP_R_row:
    for (int j = k + 1; j < ColsA; j++) {
#pragma HLS loop_tripcount min = ColsA / 2 max = ColsA / 2 avg = ColsA / 2
#pragma HLS pipeline II = 1
        T dataR;
        T tempR = tempR_strm.read();
        if (j == k + 1) {
            dataR = hls::sqrt(tempR);  // dataR[k+1][k+1] parallel-step2
            gen_rsqrt<T>(tempR, norm); // parallel-step2
            _XF_SOLVER_PRINT("===dot adder check : %7f, norm : %7f\n", tempR, norm);
        } else {
            dataR = tempR * norm; // mul seperate  [k+1][j]
        }
        R_strm.write(dataR);
    }
}

//---------------------------------dot---------------------------------//
template <int Rows, int NUMADD, class T>
void adder_layer_strm(hls::stream<T> dotRes_in[Rows], T dotRes_o[Rows / 2]) {
#pragma HLS INLINE

    T dotRes_i[Rows];

    for (int i = 0; i < NUMADD; i++) { // 1024/2
#pragma HLS UNROLL
        T tmp_add_real;
#pragma HLS bind_op variable = tmp_add_real op = fadd impl = primitivedsp

        dotRes_i[2 * i] = dotRes_in[2 * i].read();
        dotRes_i[2 * i + 1] = dotRes_in[2 * i + 1].read();
        tmp_add_real = dotRes_i[2 * i] + dotRes_i[2 * i + 1];

        dotRes_o[i] = tmp_add_real;
    }
}

// Tree complex float adder, configable by POW=log2(RowsA)
// max input size is 1024 in one cycle, min is 3. [3, 1024, 2^n] = 3, 4, 8 ,16 ...
template <int RowsA, int POW, class T>
void reduce_add_strm(hls::stream<T> dotRes_i[RowsA], T& dotRes) {
#pragma HLS INLINE
    // input for each layer adder
    T dotRes_i7[128]; // 256/2 //sum 256 to 128
#pragma HLS ARRAY_PARTITION variable = dotRes_i7 complete
    T dotRes_i6[64];
#pragma HLS ARRAY_PARTITION variable = dotRes_i6 complete
    T dotRes_i5[32];
#pragma HLS ARRAY_PARTITION variable = dotRes_i5 complete
    T dotRes_i4[16];
#pragma HLS ARRAY_PARTITION variable = dotRes_i4 complete
    T dotRes_i3[8]; // sum 16 to 8
#pragma HLS ARRAY_PARTITION variable = dotRes_i3 complete
    T dotRes_i2[4];
#pragma HLS ARRAY_PARTITION variable = dotRes_i2 complete
    T dotRes_i1[2];
#pragma HLS ARRAY_PARTITION variable = dotRes_i1 complete

    // set the input level by sel POW=log2(RowsA)
    if (POW == 8) { // 128<RowsA<=256
        adder_layer_strm<RowsA, 128, T>(dotRes_i, dotRes_i7);
    }

    if (POW > 7) {
        adder_layer<128, 64, T>(dotRes_i7, dotRes_i6);
    } else if (POW == 7) { // RowsA<=128
        adder_layer_strm<RowsA, 64, T>(dotRes_i, dotRes_i6);
    }

    if (POW > 6) {
        adder_layer<64, 32, T>(dotRes_i6, dotRes_i5);
    } else if (POW == 6) { // RowsA<=64
        adder_layer_strm<RowsA, 32, T>(dotRes_i, dotRes_i5);
    }

    if (POW > 5) {
        adder_layer<32, 16, T>(dotRes_i5, dotRes_i4);
    } else if (POW == 5) { // RowsA<=32
        adder_layer_strm<RowsA, 16, T>(dotRes_i, dotRes_i4);
    }

    if (POW > 4) {
        adder_layer<16, 8, T>(dotRes_i4, dotRes_i3);
    } else if (POW == 4) { // RowsA<=16
        adder_layer_strm<RowsA, 8, T>(dotRes_i, dotRes_i3);
    }

    if (POW > 3) {
        adder_layer<8, 4, T>(dotRes_i3, dotRes_i2);
    } else if (POW == 3) { // RowsA<=8
        adder_layer_strm<RowsA, 4, T>(dotRes_i, dotRes_i2);
    }

    if (POW > 2) { // sum 4 to 2
        adder_layer<4, 2, T>(dotRes_i2, dotRes_i1);
    } else if (POW == 2) { // RowsA<=8
        adder_layer_strm<RowsA, 2, T>(dotRes_i, dotRes_i1);
    }

    if (POW == 1) {
        dotRes_i1[0] = dotRes_i[0].read();
        dotRes_i1[1] = dotRes_i[1].read();
    }

    // if(RowsA == 3) dotRes_i1[1] = dotRes_i2[2];//checkme corner case when RowsA == 3
    if (POW == 0) {
        dotRes = dotRes_i[0].read();
    } else {
        dotRes = dotRes_i1[0] + dotRes_i1[1];
    }
}

//---------------------------------qrd loop A ---------------------------------//
template <int RowsA, int ColsA, int PowUnroll, int POWFoldRow, int NCU, class T>
void qrd_update_A(hls::stream<T>& dotRes_out,          // output
                  T dataKOut[(RowsA + NCU - 1) / NCU], // a better coding style : (RowsA + UnrollSize - 1)/UnrollSize
                  T A[(RowsA + NCU - 1) / NCU][ColsA], // inout
                  hls::stream<T>& projIn_strm,         // input
                  T dataKIn[(RowsA + NCU - 1) / NCU],
                  T normIn,
                  int k) {
#pragma HLS INLINE OFF

    const int UnrollSize = 1 << (PowUnroll - POWFoldRow);
    T dot[4];
#pragma HLS ARRAY_PARTITION variable = dot complete dim = 1
#pragma HLS BIND_STORAGE variable = dot type = RAM_2P impl = LUTRAM latency = 1
    // update every t
    T dotRes_i[UnrollSize];
#pragma HLS ARRAY_PARTITION variable = dotRes_i complete dim = 1
    // update every t
    T tempA[UnrollSize];
#pragma HLS ARRAY_PARTITION variable = tempA complete dim = 1
    T tempA2[4]; // low finout maybe more resource !
#pragma HLS ARRAY_PARTITION variable = tempA2 complete dim = 1
#pragma HLS BIND_STORAGE variable = tempA2 type = RAM_2P impl = LUTRAM latency = ram_latency

LOOP_A_col:
    // for (int j = k; j < ColsA; j++) {
    for (int j0 = 0; j0 < ColsA - k; j0++) {
#pragma HLS loop_tripcount min = ColsA / 2 max = ColsA / 2 avg = ColsA / 2
#pragma HLS dependence array inter false
        int j = j0 + k;
        // LOOP_update_A_row:
        for (int t = 0; t < 4; t++) { // k=0 // latency = 4 ,II = 1 //depend on proj
#pragma HLS loop_flatten
#pragma HLS pipeline II = coreII

            T tmp_rhs = 0;
            tmp_rhs = (k >= 0) ? projIn_strm.read() : tmp_rhs;
            for (int r = 0; r < UnrollSize; r++) {
                // A[r][j] = A[r][j] - proj[0][j] * dataK0[r];
                T add, lhs, rhs;
                bool is_mul = true;
                if (k >= 0) {
                    rhs = dataKIn[r + UnrollSize * t];
                    if (j == k) { // calculate one column Q
                        // The commutative law of multiplication must be applied, will lower the accuracy
                        add = 0;
                        lhs = normIn; /*rhs = dataKIn[r + UnrollSize*t];*/
                        is_mul = true;
                        // std::cout << lhs << " det with " << rhs << std::endl ;
                    } else { // calculate one column A
                        add = A[r + UnrollSize * t][j];
                        lhs = tmp_rhs;  /*rhs = dataKIn[r + UnrollSize*t];*/
                        is_mul = false; // col_new(k/j) = col_old(j) - proj * col_old(k)
                        // std::cout << lhs << " update with " << rhs << std::endl ;
                    }

                    //_XF_SOLVER_PRINT("projIn[%d] = %f  \n", j, projIn[j]);
                    complex_mul_main_fma_TEST6(tempA[r], add, lhs, rhs, is_mul);

                    if (j == k + 1) {                            // update dataK only in the beginning j
                        dataKOut[r + UnrollSize * t] = tempA[r]; // A[r + UnrollSize*t][j];
                    } else {
                        // lock the data regs
                    }

                    A[r + UnrollSize * t][j] = tempA[r]; // write back A

                } else { // when k<0, init
                    if (j > k) {
                        if (j == k + 1) {
                            dataKOut[r + UnrollSize * t] = A[r + UnrollSize * t][j];
                        } else {
                            // lock the data regs
                        }
                        tempA[r] = A[r + UnrollSize * t][j];
                    } else {
                        // calculate one column Q cycles
                    }
                }

                if (j > k) {
                    complex_mul_conj<T>(dotRes_i[r], dataKOut[r + UnrollSize * t],
                                        tempA[r]); // dot (col_new(k), col_new(j))
#ifdef DEBUG_QRD
#ifndef __SYNTHESIS__
// std::cout << x_conj(dataKOut[r + UnrollSize*t]) << " det with " << A[r + UnrollSize*t][j] << " is " << dotRes_i[r] <<
// std::endl ;
#endif
#endif
                } else { /* calculate one column Q cycles */
                }
            } // end unroll

            T addRes;
            if (j > k) {
                reduce_add<UnrollSize, PowUnroll - POWFoldRow, T>(dotRes_i, addRes);
            } else { /* calculate one column Q cycles */
            }
            // dot += addRes;
            dot[0] = (j > k && t == 0) ? addRes : dot[0];
            dot[1] = (j > k && t == 1) ? (addRes + dot[0]) : dot[1];
            dot[2] = (j > k && t == 2) ? (addRes + dot[1]) : dot[2];
            dot[3] = (j > k && t == 3) ? (addRes + dot[2]) : dot[3];

            if (j > k && t == 3) dotRes_out.write(dot[3]); // proj0_strm.write(dot[3]);

        } // end t or row
#ifndef __SYNTHESIS__
// if(j>k) printf("proj0(%d) = %f + %f i\n", j, dot[3]);
#endif

    } // end j
}

//---------------------------------qrd_dot_addpart---------------------------------//
template <int RowsA, int ColsA, int NCU, int PowNCU, class T>
void qrd_dot_addpart(hls::stream<T>& proj0_strm, // output
                     hls::stream<T> dotRes_i[NCU],
                     int k) {
#pragma HLS INLINE OFF

// update every k
LOOP_dot_addpart:
    for (int j0 = 0; j0 < ColsA - k; j0++) {
#pragma HLS loop_tripcount min = ColsA / 2 max = ColsA / 2 avg = ColsA / 2
#pragma HLS dependence array inter false
#pragma HLS pipeline II = coreII
        int j = j0 + k;

        if (j > k) {
            T addRes;
            reduce_add_strm<NCU, PowNCU, T>(dotRes_i, addRes);
            proj0_strm.write(addRes);
        } else { /* calculate one column Q cycles */
        }

#ifndef __SYNTHESIS__
// const int lastT = (RowsA + UnrollSize - 1)/UnrollSize - 1;
// printf("proj0(%d) = %f + %f i\n", j, dot[lastT].real(), dot[lastT].imag());
#endif
    } // end j
}

//---------------------------------qrd_read_projIn ---------------------------------//
template <int RowsA, int ColsA, int POWFoldRow, int NCU, class T>
void qrd_read_projIn(hls::stream<T> projIn_strm[NCU], T projIn[ColsA], int k) {
#pragma HLS INLINE OFF

LOOP_read_projIn:
    if (k >= 0)
        for (int j0 = 0; j0 < ColsA - k; j0++) {
#pragma HLS loop_tripcount min = ColsA / 2 max = ColsA / 2 avg = ColsA / 2
#pragma HLS dependence array inter false
            int j = j0 + k;
            // LOOP_update_A_row:
            for (int t = 0; t < (1 << POWFoldRow); t++) {
#pragma HLS loop_flatten
#pragma HLS pipeline II = coreII
                for (int r = 0; r < NCU; r++) {
#pragma HLS UNROLL
                    projIn_strm[r].write(projIn[j]);
                }
            }
        }
}

//---------------------------------dataflow col k ---------------------------------//
template <int RowsA, int ColsA, int PowUnroll, int POWFoldRow, int NCU, int PowNCU, class T>
void qrd_col_dataflow_wrapper_vector(T A[NCU][(RowsA + NCU - 1) / NCU][ColsA],
                                     hls::stream<T>& R_strm,
                                     T projOut[ColsA],
                                     T dataKOut[NCU][(RowsA + NCU - 1) / NCU],
                                     T& normOut,
                                     T projIn[ColsA],
                                     T dataKIn[NCU][(RowsA + NCU - 1) / NCU],
                                     T normIn,
                                     int k) {
#pragma HLS inline off
#pragma HLS dataflow

    // stream for dataflow
    hls::stream<T> projIn_strm[NCU];
#pragma HLS ARRAY_PARTITION variable = projIn_strm complete dim = 1
#pragma HLS stream variable = projIn_strm depth = 2
#pragma HLS bind_storage variable = projIn_strm type = FIFO impl = SRL

    hls::stream<T> tempR_strm;
#pragma HLS stream variable = tempR_strm depth = 2
#pragma HLS bind_storage variable = tempR_strm type = FIFO impl = SRL
    hls::stream<T> proj_strm;
#pragma HLS stream variable = proj_strm depth = 2
#pragma HLS bind_storage variable = proj_strm type = FIFO impl = SRL
    hls::stream<T> dotRes_i[NCU]; // depth = 4 ?
#pragma HLS ARRAY_PARTITION variable = dotRes_i complete dim = 1
#pragma HLS stream variable = dotRes_i depth = 2
#pragma HLS bind_storage variable = dotRes_i type = FIFO impl = SRL

    qrd_read_projIn<RowsA, ColsA, POWFoldRow, NCU, T>(projIn_strm, projIn, k);

    for (int n = 0; n < NCU; n++) {
#pragma HLS UNROLL
        qrd_update_A<RowsA / NCU, ColsA, PowUnroll - PowNCU, POWFoldRow, NCU, T>(dotRes_i[n], dataKOut[n], A[n],
                                                                                 projIn_strm[n], dataKIn[n], normIn, k);
    }

    qrd_dot_addpart<RowsA / NCU, ColsA, NCU, PowNCU, T>(proj_strm, dotRes_i, k);

    qrd_update_proj<ColsA, T>(tempR_strm, projOut, proj_strm, k);

    qrd_write_R<RowsA, ColsA, T>(normOut, R_strm, tempR_strm, k);
}

//---------------------------------core min fanout version  ---------------------------------//
/**
 * @brief Level 1 : high throughput version for Complex Float QR decompression 1024*256
 *
 * @tparam RowsA Row numbers of matrix A. "enum":["1024","512","256"].
 * @tparam ColsA Column numbers of matrix A. "enum":["256","128","64"].
 * @tparam PowUnroll Power2 of RowsA Size. "enum":["10","9","8"].
 *                   Set the appropriate input matrix rows and columns,
 *                   kernel top will automatically deduce the right configuration.
 * @tparam PowFoldRow Power2 of fold Rows Size. "enum":["2"].
 *                   Set the appropriate input matrix rows and columns,
 *                   kernel top will automatically deduce the right configuration.
 * @tparam NCU Number of Compute Unit. "enum":["32","4","1"].
 *                   Set the appropriate input matrix rows and columns,
 *                   kernel top will automatically deduce the right configuration.
 * @tparam PowNCU Power2 of compute unit(CU) number. "enum":["5","2","1"].
 *                   Set the appropriate input matrix rows and columns,
 *                   kernel top will automatically deduce the right configuration.
 * @tparam T Input/output data type.
 *
 * @param dataA Inout port, Matrix A as input and output matrix Q.
 *              For multi-cu design, expand Row into 2 dimensions[NCU][RowsA / NCU], NCU is related to PowNCU.
 * @param R_strm Output port, Matrix R, non-zero numbers in the upper triangular matrix.
 *              The effective numbers can be placed in appropriate position of the 0 matrix to restore the R matrix.
 */
template <int RowsA, int ColsA, int PowUnroll, int PowFoldRow, int NCU, int PowNCU, class T>
void qrd_float_core(T dataA[NCU][RowsA / NCU][ColsA], hls::stream<T>& R_strm) {
    const int UnrollSize_t = 1 << (PowUnroll - PowFoldRow);
//#pragma HLS BIND_STORAGE variable = dataA type=RAM_T2P impl=URAM latency = 1
#pragma HLS ARRAY_PARTITION variable = dataA complete dim = 1
#pragma HLS ARRAY_PARTITION variable = dataA type = cyclic factor = UnrollSize_t / NCU dim = 2

    T proj[2][ColsA];
#pragma HLS ARRAY_PARTITION variable = proj complete dim = 1
//#pragma HLS ARRAY_PARTITION variable = proj complete dim = 2
#pragma HLS BIND_STORAGE variable = proj type = RAM_2P impl = BRAM latency = ram_latency
    T dataK[2][NCU][RowsA / NCU];
#pragma HLS ARRAY_PARTITION variable = dataK type = cyclic factor = UnrollSize_t / NCU dim = 3
#pragma HLS ARRAY_PARTITION variable = dataK complete dim = 2
#pragma HLS ARRAY_PARTITION variable = dataK complete dim = 1
#pragma HLS BIND_STORAGE variable = dataK type = RAM_2P impl = LUTRAM latency = ram_latency
    T norm[2];
#pragma HLS ARRAY_PARTITION variable = norm complete dim = 1
#pragma HLS BIND_STORAGE variable = norm type = RAM_2P impl = LUTRAM latency = ram_latency

    bool ping_pong_flag = true;

LOOP_out_k:
    for (int k = -1; k < ColsA - 1; ++k) { // ColsA - 1 to ColsA for last col Q
#pragma HLS loop_tripcount min = ColsA max = ColsA avg = ColsA
#pragma HLS PIPELINE off
        if (ping_pong_flag) {
            qrd_col_dataflow_wrapper_vector<RowsA, ColsA, PowUnroll, PowFoldRow, NCU, PowNCU, T>(
                dataA, R_strm, proj[0], dataK[0], norm[0], proj[1], dataK[1], norm[1], k);
        } else {
            qrd_col_dataflow_wrapper_vector<RowsA, ColsA, PowUnroll, PowFoldRow, NCU, PowNCU, T>(
                dataA, R_strm, proj[1], dataK[1], norm[1], proj[0], dataK[0], norm[0], k);
        }
        ping_pong_flag = !ping_pong_flag;

    } // end k
    for (int r = 0; r < RowsA; r++) {
        T tmp = norm[(ColsA + 1) % 2];
        T zero = 0;
        complex_mul_main_fma_TEST6<T>(dataA[r % NCU][r / NCU][ColsA - 1], zero,
                                      dataK[(ColsA + 1) % 2][r % NCU][r / NCU], tmp, true);
    }

#ifndef __SYNTHESIS__
    printf("[kernel] basic config : ncu is %d, PowNCU is %d,  core ii is %d\n", NCU, PowNCU, coreII);
#endif
#ifdef DEBUG_QRD
#ifndef __SYNTHESIS__
    _XF_SOLVER_PRINT("===Q matrix\n");
    for (int n = 0; n < NCU; n++) {
        for (int r = 0; r < RowsA / NCU; r++) {
            for (int c = 0; c < ColsA; c++) {
                printf("%f, ", dataA[n][r][c]);
            }
            printf("\n");
        }
    }
#endif
#endif
}

// top template for IP
template <int RowsA, int ColsA, int PowUnroll, int PowFoldRow, int PowNCU, class T>
void qrd_ip_ncu_float_top(T* A, hls::stream<T>& R_strm) {
    const int NCU_t = 1 << PowNCU;
    const int UnrollSize_t = 1 << (PowUnroll - PowFoldRow);
    // Verify that template parameters are correct in simulation
    if (RowsA < ColsA) {
#ifndef __SYNTHESIS__
        printf(
            "ERROR: hls_qrd.h: Template parameter error - RowsA must be greater than ColsA; currently RowsA = %d ColsA "
            "= %d\n",
            RowsA, ColsA);
#endif
        exit(1);
    }
    // Buffers // ping-pong A buffer can get best throughput
    T dataA[NCU_t][RowsA / NCU_t][ColsA];
#pragma HLS BIND_STORAGE variable = dataA type = RAM_T2P impl = BRAM latency = ram_latency
#pragma HLS ARRAY_PARTITION variable = dataA complete dim = 1
#pragma HLS ARRAY_PARTITION variable = dataA type = cyclic factor = UnrollSize_t / NCU_t dim = 2

OUT_LOOP_LOAD_A:
    for (int r = 0; r < RowsA; r++) {
        for (int c = 0; c < ColsA; c++) {
#pragma HLS PIPELINE
            dataA[r % NCU_t][r / NCU_t][c] = A[r * ColsA + c];
        }
    } // xf::solver::print_matrix<RowsA, ColsA, T, xf::solver::NoTranspose>(dataA, " ", 6);

    xf::solver::qrd_float_core<RowsA, ColsA, PowUnroll, PowFoldRow, NCU_t, PowNCU, T>(dataA, R_strm);

OUT_LOOP_WRITE_Q:
    for (int r = 0; r < RowsA; r++) {
        for (int c = 0; c < ColsA; c++) {
#pragma HLS PIPELINE
            A[r * ColsA + c] = dataA[r % NCU_t][r / NCU_t][c];
        }
    }
}

} // namespace
} // namespace

#endif
