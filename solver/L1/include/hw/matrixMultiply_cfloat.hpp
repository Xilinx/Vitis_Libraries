/*
 * Copyright 2021 Xilinx, Inc.
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

/**
 * @file matrixMultiply.hpp
 * @brief This file contains Matrix Multiply functions
 *   - matrixMultiply           : Entry point function.
 *   - matrixMultiplyBasic      : basic matrix multiply function
 *   - matrixMultiplyTiled      : blocked matrix myultiply function
 */

#ifndef _XF_SOLVER_MATRIX_MULTIPLY_CFLOAT_HPP_
#define _XF_SOLVER_MATRIX_MULTIPLY_CFLOAT_HPP_

#include "hls_x_complex.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"
#include <complex>
#include <assert.h>

namespace xf {
namespace solver {

// ===================================================================================================================
// matrixMultiplyBasic: for small matrix multiplication
template <int RowsA, int ColsA, int RowsB, int ColsB, int RowsC, int ColsC, typename InputType, typename OutputType>
void matrixMultiplyBasic(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC]) {
#pragma HLS INTERFACE bram port = A
#pragma HLS INTERFACE bram port = B
#pragma HLS INTERFACE bram port = C
#pragma HLS ARRAY_PARTITION variable = A cyclic factor = 16 dim = 2
#pragma HLS ARRAY_PARTITION variable = B cyclic factor = 16 dim = 1
#pragma HLS ARRAY_PARTITION variable = C complete dim = 2

a_row_loop:
    for (int i = 0; i < RowsA; i++) {
    b_col_loop:
        for (int j = 0; j < ColsB; j++) {
#pragma HLS PIPELINE II = 1
            OutputType sum = 0;
        a_col_loop:
            for (int k = 0; k < ColsA; k += 4) {
#pragma HLS UNROLL factor = 4
                sum += A[i][k] * B[k][j];
                sum += A[i][k + 1] * B[k + 1][j];
                sum += A[i][k + 2] * B[k + 2][j];
                sum += A[i][k + 3] * B[k + 3][j];
            }
            C[i][j] = sum;
        }
    }
}
template <int Rows, int Cols, int TILE_SIZE, typename DType>
void load_block(const DType A[Rows][Cols], DType tile[TILE_SIZE][TILE_SIZE], unsigned row, unsigned col) {
#pragma HLS INLINE
    for (int i = 0; i < TILE_SIZE; i++) {
        for (int k = 0; k < TILE_SIZE; k++) {
#pragma HLS PIPE LINE II = 1
            tile[i][k] = A[row + i][col + k];
        }
    }
}
template <int Rows, int Cols, int TILE_SIZE, typename DType>
void store_block(DType C[Rows][Cols], DType tile[TILE_SIZE][TILE_SIZE], unsigned row, unsigned col) {
#pragma HLS INLINE
    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            C[row + i][col + j] = tile[i][j];
        }
    }
}
template <int TILE_SIZE, typename DType>
void init_block(DType tile[TILE_SIZE][TILE_SIZE]) {
#pragma HLS INLINE
    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            tile[i][j] = 0;
        }
    }
}
template <int TILE_SIZE, typename InputType, typename OutputType>
void compute_block(InputType tileA[TILE_SIZE][TILE_SIZE],
                   InputType tileB[TILE_SIZE][TILE_SIZE],
                   OutputType tileC[TILE_SIZE][TILE_SIZE]) {
matmul_tile_a_row_loop:
    for (int i = 0; i < TILE_SIZE; i++) {
    matmul_tile_b_col_loop:
        for (int j = 0; j < TILE_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            OutputType sum = tileC[i][j];
        matmul_tile_a_col_loop:
            for (int k = 0; k < TILE_SIZE; k++) {
#pragma HLS UNROLL factor = 4
                sum += tileA[i][k] * tileB[k][j];
            }
            tileC[i][j] = sum;
        }
    }
}

// ===================================================================================================================
// matrixMultiplyTiled: for large matrix multiplication
template <int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          int TILE_SIZE,
          typename InputType,
          typename OutputType>
void matrixMultiplyTiled(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC], int startRow, int endRow) {
// AXI Interface config
#pragma HLS INTERFACE m_axi port = A offset = slave bundle = gmem0 num_read_outstanding = 32 max_read_burst_length = 256
#pragma HLS INTERFACE m_axi port = B offset = slave bundle = gmem1 num_read_outstanding = 32 max_read_burst_length = 256
#pragma HLS INTERFACE m_axi port = C offset = slave bundle = gmem2 num_write_outstanding = 32 max_write_burst_length = \
    256
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Double Buffering
    InputType tileA[2][TILE_SIZE][TILE_SIZE];
    InputType tileB[2][TILE_SIZE][TILE_SIZE];
    OutputType tileC[TILE_SIZE][TILE_SIZE];
#pragma HLS ARRAY_PARTITION variable = tileA complete dim = 1
#pragma HLS ARRAY_PARTITION variable = tileA complete dim = 2
#pragma HLS ARRAY_PARTITION variable = tileB complete dim = 1
#pragma HLS ARRAY_PARTITION variable = tileB complete dim = 3
#pragma HLS ARRAY_PARTITION variable = tileC complete dim = 0

    unsigned load_flag = 0;
    // preload A and B
    load_block<RowsA, ColsA, TILE_SIZE, InputType>(A, tileA[0], 0, 0);
    load_block<RowsB, ColsB, TILE_SIZE, InputType>(B, tileB[0], 0, 0);

// Block Loop
blockA_row_loop:
    for (int ii = 0; ii < RowsA; ii += TILE_SIZE) {
    blockB_col_loop:
        for (int jj = 0; jj < ColsB; jj += TILE_SIZE) {
            init_block<TILE_SIZE, OutputType>(tileC);
        blockA_col_loop:
            for (int kk = 0; kk < ColsA; kk += TILE_SIZE) {
                // calculate block matrix multiplication on current buffer
                compute_block<TILE_SIZE, InputType, OutputType>(tileA[load_flag], tileB[load_flag], tileC);

                if ((kk + TILE_SIZE) < ColsA) {
                    load_block<RowsA, ColsA, TILE_SIZE, InputType>(A, tileA[1 - load_flag], ii, TILE_SIZE + kk);
                    load_block<RowsB, ColsB, TILE_SIZE, InputType>(B, tileB[1 - load_flag], TILE_SIZE + kk, jj);
                } else if (((ii + TILE_SIZE) < RowsA) || ((jj + TILE_SIZE) < ColsB)) {
                    int next_ii = (jj + TILE_SIZE >= ColsB) ? ii + TILE_SIZE : ii;
                    int next_jj = (jj + TILE_SIZE >= ColsB) ? 0 : jj + TILE_SIZE;
                    if (next_ii < RowsA) {
                        load_block<RowsA, ColsA, TILE_SIZE, InputType>(A, tileA[1 - load_flag], next_ii, 0);
                        load_block<RowsB, ColsB, TILE_SIZE, InputType>(B, tileB[1 - load_flag], 0, next_jj);
                    }
                }
                load_flag = 1 - load_flag;
            }
            // update matC results with tileC
            store_block<RowsC, ColsC, TILE_SIZE, OutputType>(C, tileC, ii, jj);
        }
    }
}

/**
 * @brief matrixMultiply entry point function.
 *
 *  @tparam RowsA             Defines the matrixA rows number 
 *  @tparam ColsA             Defines the matrixA columns number 
 *  @tparam RowsB             Defines the matrixB rows number 
 *  @tparam ColsB             Defines the matrixB column number
 *  @tparam RowsC             Defines the matrixC rows number
 *  @tparam ColsC             Defines the matrixC columns number
 *  @tparam TILE_SIZE         Defines the block dimensition in the block matrix multiplication algorithm
 *  @tparam BLK               Defines the number of rows when loading matrix into memory.
 *  @tparam InputType         Input data type
 *  @tparam OutputType        Output data type
 *
 *  @param matrixAStrm        Stream of First input matrix
 *  @param matrixBStrm        Stream of Second input matrix
 *  @param matrixCStrm        Stream of AB product output matrix
 */
template <int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          int TILE_SIZE,
          int BLK,
          typename InputType,
          typename OutputType>
void matrixMultiply(hls::stream<InputType>& matrixAStrm,
                    hls::stream<InputType>& matrixBStrm,
                    hls::stream<OutputType>& matrixCStrm) {
    InputType A[BLK][ColsA];
    InputType B[RowsB][ColsB];
    OutputType C[BLK][ColsC];

    for (int r = 0; r < RowsB; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsB; c++) {
            matrixBStrm.read(B[r][c]);
        }
    }
    for (int n = 0; n < RowsA; n += BLK) {
        for (int r = 0; r < BLK; r++) {
#pragma HLS PIPELINE
            for (int c = 0; c < ColsA; c++) {
                matrixAStrm.read(A[r][c]);
            }
        }
        matrixMultiplyTiled<BLK, ColsA, RowsB, ColsB, BLK, ColsC, TILE_SIZE, InputType, OutputType>(A, B, C, n, n+BLK);
        for (int r = 0; r < BLK; r++) {
#pragma HLS PIPELINE
            for (int c = 0; c < ColsC; c++) {
                matrixCStrm.write(C[r][c]);
            }
        }
    }
}
} // end namespace solver
} // end namespace xf

#endif
