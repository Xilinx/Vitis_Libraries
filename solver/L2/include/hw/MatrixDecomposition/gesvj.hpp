/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WANCUNCUANTIES ONCU CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file gesvj.hpp
 * @brief  This files contain implementation of SVD using one-sided Jacobi method.
 *
 * This file is part of XF Solver Library.
 */

#ifndef _XF_SOLVER_GESVJ_HPP_
#define _XF_SOLVER_GESVJ_HPP_

#include "ap_fixed.h"
#include "hls_stream.h"
#include "hls_math.h"

namespace xf {
namespace solver {
namespace internal {

union double_casting {
    double d;
    uint64_t i;
};

// the 2x2 matrix contains 4 elements
//------beta  gamma-----
//------gamma alpha-----
template <typename DT>
void jacobi_rotation_2x2(DT alpha, DT beta, DT gamma, hls::stream<DT>& s_strm, hls::stream<DT>& c_strm) {
//	double zeta = (beta - alpha) / (2.0 * gamma);
//	double ang_tan = sgn(zeta) / (hls::abs(zeta) + hls::sqrt(1.0 + (zeta*zeta)));//compute tan of angle
//	double ang_cos = 1.0 / (hls::sqrt (1.0 + (ang_tan*ang_tan)));       //cos
//	double ang_sin = ang_cos*ang_tan;              // sin

#pragma HLS inline off

#pragma HLS PIPELINE II = 1
    double m00, m01, m11;

    m00 = beta;
    m01 = gamma;
    m11 = alpha;
    double d;
#pragma HLS RESOURCE variable = d core = DAddSub_nodsp
    d = m00 - m11; // calculate the off-diagonal value
    ap_uint<11> exp1;
    ap_uint<52> sig1;
    union double_casting dc;
    // calculate deno = 2*m01
    dc.d = m01;
    ap_uint<64> data = dc.i;
    exp1(10, 0) = data(62, 52);
    exp1 = exp1 + ap_uint<11>(1);
    data(62, 52) = exp1(10, 0);
    dc.i = data;
    double deno = dc.d;
    ///////////////////////////
    // calculate KK = 2*abs(m00 - m11)
    dc.d = d;
    data = dc.i;
    exp1(10, 0) = data(62, 52);
    exp1 = exp1 + ap_uint<11>(1);
    data(62, 52) = exp1(10, 0);
    data[63] = 0;
    dc.i = data;
    double KK = dc.d;
    ///////////////////////////

    double deno2, d2;
#pragma HLS RESOURCE variable = d2 core = DMul_maxdsp
#pragma HLS RESOURCE variable = deno2 core = DMul_maxdsp
    d2 = d * d;          // d2 = (m00 - m11)^2
    deno2 = deno * deno; // deno2 = 4*(m01)^2
    double m;
#pragma HLS RESOURCE variable = m core = DAddSub_nodsp
    m = deno2 + d2;              // m = (m00 - m11)^2 + 4*(m01)^2
    double sqrtM = hls::sqrt(m); // sqrtM = sqrt((m00-m11)^2 + 4*(m01)^2)
    //////////////////
    // calculate M2
    dc.d = m;
    data = dc.i;
    exp1(10, 0) = data(62, 52);
    exp1 = exp1 + ap_uint<11>(1);
    data(62, 52) = exp1(10, 0);
    dc.i = data;
    double M2 = dc.d; // M2 = 2*m
    ////////////////////////////////////
    double tmpMul, tmpSum, tmpSub;
#pragma HLS RESOURCE variable = tmpMul core = DMul_maxdsp
    tmpMul = KK * sqrtM; // tmpMul = 2*abs(m00 - m11) * sqrt((m00-m11)^2 + 4*(m01)^2)
#pragma HLS RESOURCE variable = tmpSum core = DAddSub_nodsp
    tmpSum = tmpMul + M2;
    double tmpDivider = deno2 / tmpSum;
#pragma HLS RESOURCE variable = tmpSub core = DAddSub_nodsp
    tmpSub = 1 - tmpDivider;
    DT c_right = hls::sqrt(tmpSub);
    double tmp = hls::sqrt(tmpDivider);
    DT s_right = (((d > 0) && (deno > 0)) | ((d < 0) && (deno < 0))) ? tmp : -tmp;

    s_strm.write(s_right);
    c_strm.write(c_right);
}
//! calc the converge of next sweep
template <typename DT>
void calc_converge(DT alpha, DT beta, DT gamma, hls::stream<DT>& conv_strm) {
    DT converge = hls::abs(gamma) / hls::sqrt(alpha * beta); // compute convergence
    conv_strm.write(converge);
}

template <typename DT>
void svd_and_conv(
    DT alpha, DT beta, DT gamma, hls::stream<DT>& conv_strm, hls::stream<DT>& s_strm, hls::stream<DT>& c_strm) {
#pragma HLS DATAFLOW
    jacobi_rotation_2x2<DT>(alpha, beta, gamma, s_strm, c_strm);
    calc_converge(alpha, beta, gamma, conv_strm);
}

template <typename DT, int MAXM, int MAXN, int MCU, int ACUM>
void update_A(
    DT A[MCU][ACUM][MAXN], DT A_i[MCU][ACUM], DT A_j[MCU][ACUM], int rows, int cols, int col_i, int col_j, DT s, DT c) {
#pragma HLS inline off
UPDATE_A:
    for (int k = 0; k < ACUM; k++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
        for (int m = 0; m < MCU; m++) {
#pragma HLS UNROLL
            // tki = A[k][i];
            // tkj = A[k][j];
            DT tki = A_i[m][k];
            DT tkj = A_j[m][k];
            A[m][k][col_i] = c * tki - s * tkj; // A[k][i] = c*tki - s*A[k][j];
            A[m][k][col_j] = s * tki + c * tkj; // A[k][j] = s*tki + c*A[k][j];
        }
    }
}

template <typename DT, int MAXN, int NCU, int ACUN>
void update_V(DT V[NCU][ACUN][MAXN], DT V_i[NCU][ACUN], DT V_j[NCU][ACUN], int cols, int col_i, int col_j, DT s, DT c) {
#pragma HLS inline off
CALC_V:
    // for(int k=0; k<cols; k++){
    for (int k = 0; k < ACUN; k++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = ACUN max = ACUN
        for (int m = 0; m < NCU; m++) {
#pragma HLS UNROLL
            // tki = V[k][i];
            // tkj = V[k][j];
            DT tki = V_i[m][k];
            DT tkj = V_j[m][k];
            V[m][k][col_i] = c * tki - s * tkj; // V[k][i] = c*tki - s*V[k][j];
            V[m][k][col_j] = s * tki + c * tkj; // V[k][j] = s*tki + c*V[k][j];
        }
    }
}

template <typename DT, int MAXM, int MAXN, int MCU, int ACUM, int NCU, int ACUN>
void update_AV(DT A[MCU][ACUM][MAXN],
               DT V[NCU][ACUN][MAXN],
               DT A_i[MCU][ACUM],
               DT A_j[MCU][ACUM],
               DT V_i[NCU][ACUN],
               DT V_j[NCU][ACUN],
               int rows,
               int cols,
               int col_i,
               int col_j,
               DT s,
               DT c) {
#pragma HLS DATAFLOW
    update_A<DT, MAXM, MAXN, MCU, ACUM>(A, A_i, A_j, rows, cols, col_i, col_j, s, c);
    update_V<DT, MAXN, NCU, ACUN>(V, V_i, V_j, cols, col_i, col_j, s, c);
}

//! Read two columns of A into two seperate Bram
template <typename DT, int MAXM, int MAXN, int MCU, int ACUM>
void read_and_gen_2x2(DT A[MCU][ACUM][MAXN],
                      DT A_i[MCU][ACUM],
                      DT A_j[MCU][ACUM],
                      int rows,
                      int cols,
                      int col_i,
                      int col_j,
                      hls::stream<DT>& alpha_strm,
                      hls::stream<DT>& beta_strm,
                      hls::stream<DT>& gamma_strm) {
#pragma HLS inline off
    DT alpha = 0;
    DT beta = 0;
    DT gamma = 0;

    const int DEP = 16;
    // used for accumulate alpha*alpha, beta*beta, gamma*gamma
    DT alpha_acc[MCU][DEP];
#pragma HLS resource variable = alpha_acc core = RAM_2P_BRAM
#pragma HLS ARRAY_PARTITION variable = alpha_acc complete dim = 0
    DT beta_acc[MCU][DEP];
#pragma HLS resource variable = beta_acc core = RAM_2P_BRAM
#pragma HLS ARRAY_PARTITION variable = beta_acc complete dim = 0
    DT gamma_acc[MCU][DEP];
#pragma HLS resource variable = gamma_acc core = RAM_2P_BRAM
#pragma HLS ARRAY_PARTITION variable = gamma_acc complete dim = 0

    DT alpha_sum[DEP];
    DT beta_sum[DEP];
    DT gamma_sum[DEP];

INIT_ACC:
    for (int t = 0; t < DEP; t++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 8
        for (int m = 0; m < MCU; m++) {
#pragma HLS UNROLL
            alpha_acc[m][t] = 0;
            beta_acc[m][t] = 0;
            gamma_acc[m][t] = 0;
        }

        alpha_sum[t] = 0;
        beta_sum[t] = 0;
        gamma_sum[t] = 0;
    }

CALC_ELEMENTS:
    for (int k = 0; k < ACUM; k++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = ACUM max = ACUM
#pragma HLS dependence variable = alpha_acc inter false
#pragma HLS dependence variable = beta_acc inter false
#pragma HLS dependence variable = gamma_acc inter false
        for (int m = 0; m < MCU; m++) {
#pragma HLS UNROLL
            if (k * MCU + m < rows) {
                DT Aki = A[m][k][col_i];
                DT Akj = A[m][k][col_j];
                A_i[m][k] = Aki; // store to extra bram, so no need to read URAM again when updating A
                A_j[m][k] = Akj;
                alpha_acc[m][k % DEP] += Aki * Aki;
                beta_acc[m][k % DEP] += Akj * Akj;
                gamma_acc[m][k % DEP] += Aki * Akj;
            }
        }
    }

    ap_uint<4> idx = 0;
ACCU:
    for (int k = 0; k < DEP; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 8
#pragma HLS PIPELINE II = MCU
        for (int m = 0; m < MCU; m++) {
#pragma HLS LOOP_TRIPCOUNT min = MCU max = MCU
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = alpha_acc inter false
#pragma HLS dependence variable = beta_acc inter false
#pragma HLS dependence variable = gamma_acc inter false
            alpha_sum[idx] += alpha_acc[m][k];
            beta_sum[idx] += beta_acc[m][k];
            gamma_sum[idx] += gamma_acc[m][k];
            idx++;
        }
    }

    // sum 16 data to 8
    DT alpha_sum_tmp0[8];
    DT beta_sum_tmp0[8];
    DT gamma_sum_tmp0[8];
    for (int k = 0; k < 8; k++) {
#pragma HLS PIPELINE
        alpha_sum_tmp0[k] = alpha_sum[2 * k] + alpha_sum[2 * k + 1];
        beta_sum_tmp0[k] = beta_sum[2 * k] + beta_sum[2 * k + 1];
        gamma_sum_tmp0[k] = gamma_sum[2 * k] + gamma_sum[2 * k + 1];
    }

    // sum 8 data to 4
    DT alpha_sum_tmp1[4];
    DT beta_sum_tmp1[4];
    DT gamma_sum_tmp1[4];
    for (int k = 0; k < 4; k++) {
#pragma HLS PIPELINE
        alpha_sum_tmp1[k] = alpha_sum_tmp0[2 * k] + alpha_sum_tmp0[2 * k + 1];
        beta_sum_tmp1[k] = beta_sum_tmp0[2 * k] + beta_sum_tmp0[2 * k + 1];
        gamma_sum_tmp1[k] = gamma_sum_tmp0[2 * k] + gamma_sum_tmp0[2 * k + 1];
    }
    // sum 4 data to 2
    DT alpha_sum_tmp2[2];
    DT beta_sum_tmp2[2];
    DT gamma_sum_tmp2[2];
    for (int k = 0; k < 2; k++) {
#pragma HLS PIPELINE
        alpha_sum_tmp2[k] = alpha_sum_tmp1[2 * k] + alpha_sum_tmp1[2 * k + 1];
        beta_sum_tmp2[k] = beta_sum_tmp1[2 * k] + beta_sum_tmp1[2 * k + 1];
        gamma_sum_tmp2[k] = gamma_sum_tmp1[2 * k] + gamma_sum_tmp1[2 * k + 1];
    }
    // sum 2 data to 1
    alpha = alpha_sum_tmp2[0] + alpha_sum_tmp2[1];
    beta = beta_sum_tmp2[0] + beta_sum_tmp2[1];
    gamma = gamma_sum_tmp2[0] + gamma_sum_tmp2[1];

    alpha_strm.write(alpha);
    beta_strm.write(beta);
    gamma_strm.write(gamma);
}

//! Read two columns (i and j) of V into two seperate Bram V_i[N] and V_j[N]
template <typename DT, int MAXN, int NCU, int ACUN>
void read_V_2cols(DT V[NCU][ACUN][MAXN], DT V_i[NCU][ACUN], DT V_j[NCU][ACUN], int cols, int col_i, int col_j) {
#pragma HLS inline off
    for (int k = 0; k < ACUN; k++) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = ACUN max = ACUN
        for (int m = 0; m < NCU; m++) {
#pragma HLS UNROLL
            if (k * NCU + m < cols) {
                V_i[m][k] = V[m][k][col_i];
                V_j[m][k] = V[m][k][col_j];
            }
        }
    }
}

//! read 2 columns(i and j) of data from A matrix and V matrix
template <typename DT, int MAXM, int MAXN, int MCU, int ACUM, int NCU, int ACUN>
void read_to_2cols(DT A[MCU][ACUM][MAXN],
                   DT V[NCU][ACUN][MAXN],
                   DT A_i[MCU][ACUM],
                   DT A_j[MCU][ACUM],
                   DT V_i[NCU][ACUN],
                   DT V_j[NCU][ACUN],
                   int rows,
                   int cols,
                   int col_i,
                   int col_j,
                   hls::stream<DT>& alpha_strm,
                   hls::stream<DT>& beta_strm,
                   hls::stream<DT>& gamma_strm) {
#pragma HLS DATAFLOW
    read_and_gen_2x2<DT, MAXM, MAXN, MCU, ACUM>(A, A_i, A_j, rows, cols, col_i, col_j, alpha_strm, beta_strm,
                                                gamma_strm);
    read_V_2cols<DT, MAXN, NCU, ACUN>(V, V_i, V_j, cols, col_i, col_j);
}

} // end of namespace internal

/**
 * @brief This function implements singular value decomposition of matrix A using one-sided Jacobi algorihtm.
   \f{equation*} {A = U \Sigma {V}^T, }\f}
   where \f$A\f$ is a dense matrix of size \f$rows \times cols\f$, \f$U\f$ is
   \f$rows \times rows\f$ matrix with orthonormal columns, \f$V\f$ is \f$cols \times cols\f$
   matrix with orthonormal columns, and \f$\Sigma\f$ is diagonal matrix.\n
   The maximum matrix size supported in FPGA is templated by MAXN, MAXM.
 *
 * @tparam DT: the data type of gesvj
 * @tparam MAXM: the maximum size (rows) of supported gesvj
 * @tparam MAXN: the maximum size (cols) of supported gesvj
 * @tparam MCU: the partition number of M
 * @tparam NCU: the partition number of N
 * @param rows: the size of matrix row
 * @param cols: the size of matrix col
 * @param input_mat: input matrix
 * @param output_U: the output U of svd
 * @param output_S: the output S of svd
 * @param output_V: the output V of svd
 **/
template <typename DT, int MAXM, int MAXN, int MCU, int NCU>
void gesvj(int rows,
           int cols,
           DT input_mat[MAXM * MAXN],
           DT output_U[MAXM * MAXM],
           DT output_S[MAXN],
           DT output_V[MAXN * MAXN]) {
    // num of elements in each MCU
    const int ACUM = (MAXM + MCU - 1) / MCU;
    // num of elements in each NCU
    const int ACUN = (MAXN + NCU - 1) / NCU;

    DT A[MCU][ACUM][MAXN];
#pragma HLS RESOURCE variable = A core = RAM_T2P_URAM
#pragma HLS ARRAY_PARTITION variable = A dim = 1
    DT U[MAXM][MAXM];
#pragma HLS RESOURCE variable = U core = RAM_T2P_URAM
    DT V[NCU][ACUN][MAXN];
#pragma HLS RESOURCE variable = V core = RAM_T2P_URAM
#pragma HLS ARRAY_PARTITION variable = V dim = 1
    DT A_i[MCU][ACUM];
#pragma HLS RESOURCE variable = A_i core = RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable = A_i dim = 1
    DT A_j[MCU][ACUM];
#pragma HLS RESOURCE variable = A_j core = RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable = A_j dim = 1
    DT V_i[NCU][ACUN];
#pragma HLS RESOURCE variable = V_i core = RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable = V_i dim = 1
    DT V_j[NCU][ACUN];
#pragma HLS RESOURCE variable = V_j core = RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable = V_j dim = 1

    const int S_SIZE = MAXM > MAXN ? MAXN : MAXM;
    const int ssize = rows > cols ? cols : rows;
    DT S[S_SIZE]; //??what if M>N?@TODO
#pragma HLS RESOURCE variable = S core = RAM_S2P_BRAM

INIT_S:
    for (int j = 0; j < ssize; j++) {
#pragma HLS LOOP_TRIPCOUNT min = S_SIZE max = S_SIZE
#pragma HLS PIPELINE II = 1
        S[j] = 0.0;
    }

INIT_U:
    for (int i = 0; i < rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
        for (int j = 0; j < rows; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
            if (i == j) {
                U[i][j] = 1.0;
            } else {
                U[i][j] = 0.0;
            }
        }
    }
// initialize V matrix to diagonal
INIT_V:
    for (int i = 0; i < cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
        for (int j = 0; j < cols; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
            if (i == j) {
                V[i % NCU][i / NCU][j] = 1.0;
            } else {
                V[i % NCU][i / NCU][j] = 0.0;
            }
        }
    }

// read A from DDR
READ_A:
    for (int i = 0; i < rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
        for (int j = 0; j < cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
#pragma HLS PIPELINE II = 1
            A[i % MCU][i / MCU][j] = input_mat[i * cols + j];
#ifndef __SYNTHESIS__
#ifdef __SOLVER_DEBUG__
            std::cout << A[i % MCU][i / MCU][j] << " ";
#endif
#endif
        }
#ifndef __SYNTHESIS__
#ifdef __SOLVER_DEBUG__
        std::cout << std::endl;
#endif
#endif
    }

    hls::stream<DT> alpha_strm;
#pragma HLS STREAM variable = alpha_strm depth = 16 dim = 1
    hls::stream<DT> beta_strm;
#pragma HLS STREAM variable = beta_strm depth = 16 dim = 1
    hls::stream<DT> gamma_strm;
#pragma HLS STREAM variable = gamma_strm depth = 16 dim = 1
    hls::stream<DT> s_strm;
#pragma HLS STREAM variable = s_strm depth = 8 dim = 1
    hls::stream<DT> c_strm;
#pragma HLS STREAM variable = c_strm depth = 8 dim = 1
    hls::stream<DT> alpha_update_strm;
#pragma HLS STREAM variable = alpha_update_strm depth = 16 dim = 1
    hls::stream<DT> beta_update_strm;
#pragma HLS STREAM variable = beta_update_strm depth = 16 dim = 1
    hls::stream<DT> gamma_update_strm;
#pragma HLS STREAM variable = gamma_update_strm depth = 16 dim = 1
    hls::stream<DT> conv_strm;
#pragma HLS STREAM variable = conv_strm depth = 8 dim = 1

    DT converge = 1.0;

    DT epsilon = 1.e-8;
    if (sizeof(DT) == sizeof(float)) {
        epsilon = 1.e-7;
    }

    int sweep_loop = 0;

CONV_WHILE:
    while (converge > epsilon) {
#pragma HLS LOOP_TRIPCOUNT min = 5 max = 15
        converge = 0.0;
        sweep_loop++;

    LOOP_COLS:
        for (int i = 1; i < cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
        LOOP_i:
            for (int j = 0; j < i; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min = MAXN/2 max = MAXN/2
                // clang-format on
                internal::read_to_2cols<DT, MAXM, MAXN, MCU, ACUM, NCU, ACUN>(A, V, A_i, A_j, V_i, V_j, rows, cols, i,
                                                                              j, alpha_strm, beta_strm, gamma_strm);

                DT alpha = alpha_strm.read();
                DT beta = beta_strm.read();
                DT gamma = gamma_strm.read();
                internal::svd_and_conv(alpha, beta, gamma, conv_strm, s_strm, c_strm);

                DT s = s_strm.read();
                DT c = c_strm.read();
                internal::update_AV<DT, MAXM, MAXN, MCU, ACUM, NCU, ACUN>(A, V, A_i, A_j, V_i, V_j, rows, cols, i, j, s,
                                                                          c);
                DT conv_tmp = conv_strm.read();
                converge = hls::max(converge, conv_tmp);
            }
        }
    }

    // calculate the S diagonal matrix
    const int DEP = 16;
    DT accu_s = 0;
    DT AUS_accu[DEP];
CALC_US:
    for (int j = 0; j < cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
        accu_s = 0;
        for (int i = 0; i < DEP; i++) {
#pragma HLS UNROLL
            AUS_accu[i] = 0;
        }
        for (int i = 0; i < rows; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
            DT Aij = A[i % MCU][i / MCU][j];
            AUS_accu[i % DEP] += Aij * Aij;
        }
        for (int m = 0; m < DEP; m++) {
#pragma HLS PIPELINE
            accu_s += AUS_accu[m];
        }
        accu_s = hls::sqrt(accu_s);
        for (int i = 0; i < rows; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
            U[i][j] = A[i % MCU][i / MCU][j] / accu_s;
            if (i == j) {
                S[i] = accu_s;
            }
        }
    }

//================output================
// output S, U, V and store
OUT_U:
    for (int i = 0; i < rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
        for (int j = 0; j < rows; j++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXM max = MAXM
#pragma HLS PIPELINE II = 1
            output_U[i * rows + j] = U[i][j];
        }
    }

    int outssize = rows > cols ? cols : rows;
OUT_S:
    for (int i = 0; i < outssize; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXM
        output_S[i] = S[i];
    }

OUT_V:
    for (int i = 0; i < cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
        for (int j = 0; j < cols; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAXN max = MAXN
            output_V[i * cols + j] = V[i % NCU][i / NCU][j];
        }
    }

#ifndef __SYNTHESIS__
#ifdef __SOLVER_DEBUG__
    std::cout << "sweep iterations = " << sweep_loop << "--------" << std::endl;
#endif
#endif
}

} // end of namespace sovler
} // end of namespace xf
#endif //#ifndef _XF_SOLVER_GESVJ_HPP_
