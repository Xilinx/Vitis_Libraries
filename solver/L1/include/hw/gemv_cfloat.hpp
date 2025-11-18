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
 * @file gemv_cfloat.hpp
 * @brief This file contains Matrix vector Multiply functions
 *   - gemv: Entry point function of blocked matrix vector multiply.
 *   - gemv_basic: basic matrix vector multiply function
 */

#ifndef _XF_SOLVER_MATRIX_VECTOR_MULTIPLY_CFLOAT_HPP_
#define _XF_SOLVER_MATRIX_VECTOR_MULTIPLY_CFLOAT_HPP_

#include "hls_x_complex.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"
#include <complex>
#include <assert.h>

namespace xf {
namespace solver {

// ===================================================================================================================
/*
// matrixMultiplyImpl: for small matrix multiplication
template <int RowsA, int ColsA, typename InputType, typename OutputType>
void gemv_basic(const InputType A[RowsA][ColsA], const InputType B[ColsA], OutputType C[RowsA]) {
#pragma HLS INTERFACE bram port = A
#pragma HLS INTERFACE bram port = B
#pragma HLS INTERFACE bram port = C
#pragma HLS ARRAY_PARTITION variable = A cyclic factor = 4 dim = 2
#pragma HLS ARRAY_PARTITION variable = B cyclic factor = 4 dim = 1
#pragma HLS ARRAY_PARTITION variable = C complete

a_row_loop:
    for (int i = 0; i < RowsA; i++) {
#pragma HLS UNROLL factor = 4
        OutputType sum = 0;
        a_col_loop:
            for (int k = 0; k < ColsA; k ++) {
#pragma HLS UNROLL factor = 4
                sum += A[i][k] * B[k];
            }
            C[i] = sum;
    }
}
*/
template <int Rows, int Cols, int BR, int BC, typename DType>
void load_block(hls::stream<DType>& AStrm,
                hls::stream<DType>& xStrm,
                DType A_buf[BR][BC],
                DType x_buf[BC],
                unsigned row,
                unsigned col) {
    for (int i = 0; i < BR && (row + i) < Rows; i++) {
        for (int j = 0; j < BC && (col + j) < Cols; j++) {
#pragma HLS PIPE LINE II = 1
            if (!AStrm.empty()) {
                A_buf[i][j] = AStrm.read();
                DType tmp = A_buf[i][j];
            } else {
#ifndef __SYNTHESIS__
                printf("AStrm is empty \n");
#endif
            }
        }
    }
    for (int j = 0; j < BC && (col + j) < Cols; j++) {
#pragma HLS PIPE LINE II = 1
        if (!xStrm.empty()) {
            x_buf[j] = xStrm.read();
            DType tmpB = x_buf[j];
        } else {
#ifndef __SYNTHESIS__
            printf("xStrm is empty \n");
#endif
        }
    }
}
template <int Rows, int BR, typename DType>
void store_block(hls::stream<DType>& yStrm, DType y_buf[BR], unsigned ii) {
#pragma HLS INLINE
    for (int i = 0; i < BR && (ii + i) < Rows; i++) {
#pragma HLS PIPELINE II = 1
        yStrm.write(y_buf[i]);
    }
}
template <int TILE_SIZE, typename DType>
void init_block(DType block[TILE_SIZE]) {
#pragma HLS INLINE
    for (int i = 0; i < TILE_SIZE; i++) {
#pragma HLS PIPELINE II = 1
        block[i] = 0;
    }
}
template <int BR, int BC, typename InputType, typename OutputType>
void compute_block(InputType A_buf[BR][BC], InputType x_buf[BC], OutputType y_buf[BR]) {
block_compute_row_loop:
    for (int i = 0; i < BR; i++) {
#pragma HLS PIPELINE II = 1
        OutputType sum = y_buf[i];
    block_compute_col_loop:
        for (int j = 0; j < BC; j++) {
#pragma HLS UNROLL factor = 4
            sum += A_buf[i][j] * x_buf[j];
        }
        y_buf[i] = sum;
    }
}

// ===================================================================================================================
/**
 * @brief gemv entry point function.
 *
 *  @tparam RowsA             Defines the matrixA rows number 
 *  @tparam ColsA             Defines the matrixA columns number 
 *  @tparam BR                Defines the rows number of block matrix in the block matrix vector multiplication algorithm
 *  @tparam BC                Defines the columns number of block matrix in the block matrix vector multiplication algorithm
 *  @tparam InputType         Input data type
 *  @tparam OutputType        Output data type
 *
 *  @param AStrm        Stream of input matrix
 *  @param xStrm        Stream of input vector 
 *  @param yStrm        Stream of output vector y=Ax 
 */
template <int RowsA, int ColsA, int BR, int BC, typename InputType, typename OutputType>
void gemv(hls::stream<InputType>& AStrm, hls::stream<InputType>& xStrm, hls::stream<OutputType>& yStrm) {
#pragma HLS INTERFACE axis port = AStrm
#pragma HLS INTERFACE axis port = xStrm
#pragma HLS INTERFACE axis port = yStrm
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATAFLOW

    // Double Buffering
    InputType A_buf[2][BR][BC];
    InputType x_buf[2][BC];
    OutputType y_buf[2][BR];
    OutputType y_acc[BR];
#pragma HLS ARRAY_PARTITION variable = A_buf complete dim = 1
#pragma HLS ARRAY_PARTITION variable = x_buf complete dim = 1
#pragma HLS ARRAY_PARTITION variable = y_buf complete dim = 1
#pragma HLS ARRAY_PARTITION variable = y_acc complete dim = 1

    unsigned load_flag = 0;
    // preload A and x
    load_block<RowsA, ColsA, BR, BC, InputType>(AStrm, xStrm, A_buf[0], x_buf[0], 0, 0);

// Block Loop
block_row_loop:
    for (int ii = 0; ii < RowsA; ii += BR) {
        init_block<BR, OutputType>(y_acc);
    block_col_loop:
        for (int jj = 0; jj < ColsA; jj += BC) {
#pragma HLS LOOP_TRIPCOUNT min = ColsA / BC max = ColsA / BC
#pragma HLS UNROLL factor = 4
            // calculate block matrix multiplication on current buffer
            compute_block<BR, BC, InputType, OutputType>(A_buf[load_flag], x_buf[load_flag], y_acc);

            if ((jj + BC) < ColsA) {
                load_block<RowsA, ColsA, BR, BC, InputType>(AStrm, xStrm, A_buf[1 - load_flag], x_buf[1 - load_flag],
                                                            ii, BC + jj);
            } else if (((ii + BR) < RowsA)) {
                load_block<RowsA, ColsA, BR, BC, InputType>(AStrm, xStrm, A_buf[1 - load_flag], x_buf[1 - load_flag],
                                                            ii + BR, 0);
            }
            load_flag = 1 - load_flag;
        }
        // update matC results with blockC
        store_block<RowsA, BR, OutputType>(yStrm, y_acc, ii);
    }
}

} // end namespace solver
} // end namespace xf

#endif
