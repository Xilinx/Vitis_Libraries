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
 * @file matrix_multiply.hpp
 * @brief This file contains Matrix Multiply functions
 *   - matrixMultiply           : Entry point function.
 *   - matrixMultiplyTop       : Top level function that selects implementation architecture and internal types based
 * on traits class.
 *   - matrixMultiplyDefault   : Default architecture.
 *   - matrixMultiplyAlt1      : Improved throughput for rolled floating point implementations at the expense of an
 * additional memory.
 *   - matrixMultiplyAlt2      : Further rolled floating point throughput improvement for small matrix sizes. Requires
 * larger internal memory.
 *   - matrixMultiplyAddTree  : Architecture using an adder tree for fully unrolled floating point implementations.
 *   - matrixMultiplyFull      : Default architecture including directives to fully unroll inner loop, fixed point
 * implementations
 */

#ifndef _XF_SOLVER_MATRIX_MULTIPLY_HPP_
#define _XF_SOLVER_MATRIX_MULTIPLY_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"
#include <complex>
#include <assert.h>

namespace xf {
namespace solver {

// ===================================================================================================================
// Default traits struct defining variable types and architecture selection
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          typename InputType,
          typename OutputType>
struct matrixMultiplyTraits {
    static const int RowsATrans = (TransposeFormA::TransposeType != 0 ? ColsA : RowsA);
    static const int ColsATrans = (TransposeFormA::TransposeType != 0 ? RowsA : ColsA);
    static const int RowsBTrans = (TransposeFormB::TransposeType != 0 ? ColsB : RowsB);
    static const int ColsBTrans = (TransposeFormB::TransposeType != 0 ? RowsB : ColsB);
    static const int B_UNROLL_DIM =
        (TransposeFormB::TransposeType != 0
             ? 1
             : 2); // Determine B input unroll dimension for matrixMultiply_ADD_ALT 1&2 architectures
    static const int A_FULL_UNROLL_DIM =
        (TransposeFormA::TransposeType != 0 ? 1 : 2); // Determine A input unroll dimension for matrixMultiplyAddTree
                                                      // and matrixMultiplyFull architectures
    static const int B_FULL_UNROLL_DIM =
        (TransposeFormB::TransposeType != 0 ? 2 : 1); // Determine B input unroll dimension for matrixMultiplyAddTree
                                                      // and matrixMultiplyFull architectures
    typedef InputType INPUT_T;
    typedef typename hls::x_traits<InputType, InputType>::MULT_T MULT_T;
    typedef typename hls::x_traits_d<InputType, ColsATrans>::ACCUM_T ACCUM_T;
    static const int ARCH = 2;          // Select implementation:
                                        // 0: matrixMultiplyDefault
                                        // 1: matrixMultiplyAlt1
                                        // 2: matrixMultiplyAlt2
                                        // 3: matrixMultiplyAddTree
                                        // 4: matrixMultiplyFull
    static const int INNER_II = 1;      // Specify the pipelining target for the inner loop
    static const int UNROLL_FACTOR = 1; // Specify the inner loop unrolling factor
};

// Specialization for ap_fixed
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct matrixMultiplyTraits<TransposeFormA,
                            TransposeFormB,
                            RowsA,
                            ColsA,
                            RowsB,
                            ColsB,
                            ap_fixed<W1, I1, Q1, O1, N1>,
                            ap_fixed<W2, I2, Q2, O2, N2> > {
    static const int RowsATrans = (TransposeFormA::TransposeType != 0 ? ColsA : RowsA);
    static const int ColsATrans = (TransposeFormA::TransposeType != 0 ? RowsA : ColsA);
    static const int RowsBTrans = (TransposeFormB::TransposeType != 0 ? ColsB : RowsB);
    static const int ColsBTrans = (TransposeFormB::TransposeType != 0 ? RowsB : ColsB);
    static const int B_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 1 : 2);
    static const int A_FULL_UNROLL_DIM = (TransposeFormA::TransposeType != 0 ? 1 : 2);
    static const int B_FULL_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 2 : 1);
    typedef ap_fixed<W1, I1, Q1, O1, N1> INPUT_T;
    typedef ap_fixed<W1 + W1, I1 + I1, AP_TRN, AP_WRAP, 0> MULT_T;
    typedef ap_fixed<W1 + W1 + BitWidth<ColsATrans>::Value, I1 + I1 + BitWidth<ColsATrans>::Value, AP_TRN, AP_WRAP, 0>
        ACCUM_T;
    static const int ARCH = 0;
    static const int INNER_II = 1;
    static const int UNROLL_FACTOR = 1;
};

// Further specialization for hls::x_complex<ap_fixed>
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct matrixMultiplyTraits<TransposeFormA,
                            TransposeFormB,
                            RowsA,
                            ColsA,
                            RowsB,
                            ColsB,
                            hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                            hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    static const int RowsATrans = (TransposeFormA::TransposeType != 0 ? ColsA : RowsA);
    static const int ColsATrans = (TransposeFormA::TransposeType != 0 ? RowsA : ColsA);
    static const int RowsBTrans = (TransposeFormB::TransposeType != 0 ? ColsB : RowsB);
    static const int ColsBTrans = (TransposeFormB::TransposeType != 0 ? RowsB : ColsB);
    static const int B_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 1 : 2);
    static const int A_FULL_UNROLL_DIM = (TransposeFormA::TransposeType != 0 ? 1 : 2);
    static const int B_FULL_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 2 : 1);
    typedef hls::x_complex<ap_fixed<W1, I1, AP_TRN, AP_WRAP, 0> > INPUT_T;
    typedef hls::x_complex<ap_fixed<W1 + W1, I1 + I1, AP_TRN, AP_WRAP, 0> > MULT_T;
    typedef hls::x_complex<
        ap_fixed<W1 + W1 + BitWidth<ColsATrans>::Value, I1 + I1 + BitWidth<ColsATrans>::Value, AP_TRN, AP_WRAP, 0> >
        ACCUM_T;
    static const int ARCH = 0;
    static const int INNER_II = 1;
    static const int UNROLL_FACTOR = 1;
};

// Further specialization for std::complex<ap_fixed>
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct matrixMultiplyTraits<TransposeFormA,
                            TransposeFormB,
                            RowsA,
                            ColsA,
                            RowsB,
                            ColsB,
                            std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                            std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    static const int RowsATrans = (TransposeFormA::TransposeType != 0 ? ColsA : RowsA);
    static const int ColsATrans = (TransposeFormA::TransposeType != 0 ? RowsA : ColsA);
    static const int RowsBTrans = (TransposeFormB::TransposeType != 0 ? ColsB : RowsB);
    static const int ColsBTrans = (TransposeFormB::TransposeType != 0 ? RowsB : ColsB);
    static const int B_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 1 : 2);
    static const int A_FULL_UNROLL_DIM = (TransposeFormA::TransposeType != 0 ? 1 : 2);
    static const int B_FULL_UNROLL_DIM = (TransposeFormB::TransposeType != 0 ? 2 : 1);
    typedef std::complex<ap_fixed<W1, I1, AP_TRN, AP_WRAP, 0> > INPUT_T;
    typedef std::complex<ap_fixed<W1 + W1, I1 + I1, AP_TRN, AP_WRAP, 0> > MULT_T;
    typedef std::complex<
        ap_fixed<W1 + W1 + BitWidth<ColsATrans>::Value, I1 + I1 + BitWidth<ColsATrans>::Value, AP_TRN, AP_WRAP, 0> >
        ACCUM_T;
    static const int ARCH = 0;
    static const int INNER_II = 1;
    static const int UNROLL_FACTOR = 1;
};

// ===================================================================================================================
// matrixMultiplyDefault: Default architecture
// o Fixed point implementation maps well to DSP48 cascades
// o Floating point adders get used sequentially as an accumulator giving a long latency
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyDefault(const InputType A[RowsA][ColsA],
                           const InputType B[RowsB][ColsB],
                           OutputType C[RowsC][ColsC]) {
    // Check defined array dimensions are compatible
    // - The ROWS and COLS value of the traits considers the transpose operation request for A & B
    assert(MULTIPLIER_TRAITS::ColsATrans == MULTIPLIER_TRAITS::RowsBTrans);
    assert(RowsC == MULTIPLIER_TRAITS::RowsATrans);
    assert(ColsC == MULTIPLIER_TRAITS::ColsBTrans);

    // Use the traits struct to specify the correct type for the intermediate variables
    typename MULTIPLIER_TRAITS::INPUT_T cast_in_a, cast_in_b;
    typename MULTIPLIER_TRAITS::MULT_T mult;
    typename MULTIPLIER_TRAITS::ACCUM_T recast_mult, sum_mult;

a_row_loop:
    for (int r = 0; r < MULTIPLIER_TRAITS::RowsATrans; r++) {
    b_col_loop:
        for (int c = 0; c < MULTIPLIER_TRAITS::ColsBTrans; c++) {
        a_col_loop:
            for (int k = 0; k < MULTIPLIER_TRAITS::ColsATrans; k++) {
#pragma HLS PIPELINE II = MULTIPLIER_TRAITS::INNER_II
                cast_in_a = GetMatrixElement<TransposeFormA, RowsA, ColsA, InputType>(A, r, k);
                cast_in_b = GetMatrixElement<TransposeFormB, RowsB, ColsB, InputType>(B, k, c);
                mult = cast_in_a * cast_in_b;

                // Cast mult to the correct output size before adding.
                recast_mult = mult;
                if (k == 0)
                    sum_mult = recast_mult;
                else
                    sum_mult += recast_mult;

                // Store result
                if (k == MULTIPLIER_TRAITS::ColsATrans - 1) C[r][c] = sum_mult;
            }
        }
    }
}

// ===================================================================================================================
// matrixMultiplyAlt1: Improved throughput for rolled floating point implementations at the expense of an additional
// memory.
// o Moves the inner loop defined in matrixMultiplyDefault up one level of nesting and uses an internal memory to
// store
//   partial results
// o For matrix sizes where the common dimension (A rows & B cols) is greater than the latency of the accumulation adder
//   this implementation will achieve full throughput for the inner loop once it is pipelined.
// o For smaller matrix sizes use matrixMultiplyAlt2 to achieve full throughput in the inner loop.
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyAlt1(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC]) {
    // Check defined array dimensions are compatible
    // - The ROWS and COLS value of the traits considers the transpose operation request for A & B
    assert(MULTIPLIER_TRAITS::ColsATrans == MULTIPLIER_TRAITS::RowsBTrans);
    assert(RowsC == MULTIPLIER_TRAITS::RowsATrans);
    assert(ColsC == MULTIPLIER_TRAITS::ColsBTrans);

    // Use the traits struct to specify the correct type for the intermediate variables
    typename MULTIPLIER_TRAITS::INPUT_T cast_in_a, cast_in_b;
    typename MULTIPLIER_TRAITS::MULT_T mult;
    typename MULTIPLIER_TRAITS::ACCUM_T recast_mult;
    typename MULTIPLIER_TRAITS::ACCUM_T sum_mult[MULTIPLIER_TRAITS::ColsBTrans];

#pragma HLS ARRAY_PARTITION variable = B cyclic dim = MULTIPLIER_TRAITS::B_UNROLL_DIM factor = \
    MULTIPLIER_TRAITS::UNROLL_FACTOR
#pragma HLS ARRAY_PARTITION variable = C cyclic dim = 2 factor = MULTIPLIER_TRAITS::UNROLL_FACTOR
#pragma HLS ARRAY_PARTITION variable = sum_mult cyclic dim = 1 factor = MULTIPLIER_TRAITS::UNROLL_FACTOR

a_row_loop:
    for (int r = 0; r < MULTIPLIER_TRAITS::RowsATrans; r++) {
    a_col_loop:
        for (int k = 0; k < MULTIPLIER_TRAITS::ColsATrans; k++) {
        b_col_loop:
            for (int c = 0; c < MULTIPLIER_TRAITS::ColsBTrans; c++) {
#pragma HLS PIPELINE II = MULTIPLIER_TRAITS::INNER_II
#pragma HLS UNROLL FACTOR = MULTIPLIER_TRAITS::UNROLL_FACTOR
                // Multiply
                cast_in_a = GetMatrixElement<TransposeFormA, RowsA, ColsA, InputType>(A, r, k);
                cast_in_b = GetMatrixElement<TransposeFormB, RowsB, ColsB, InputType>(B, k, c);
                mult = cast_in_a * cast_in_b;
                // Cast mult to the correct output size before adding.
                recast_mult = mult;
                // Sum
                if (k == 0) {
                    // Initialise
                    sum_mult[c] = recast_mult;
                } else if (k == MULTIPLIER_TRAITS::ColsATrans - 1) {
                    // Store result
                    C[r][c] = sum_mult[c] + recast_mult;
                } else {
                    // Accumulate
                    sum_mult[c] = sum_mult[c] + recast_mult;
                }
            }
        }
    }
}

// ===================================================================================================================
// matrixMultiplyAlt2: Further rolled floating point throughput improvement for small matrix sizes. Requires larger
// internal memory.
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyAlt2(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC]) {
    // Check defined array dimensions are compatible
    // - The ROWS and COLS value of the traits considers the transpose operation request for A & B
    assert(MULTIPLIER_TRAITS::ColsATrans == MULTIPLIER_TRAITS::RowsBTrans);
    assert(RowsC == MULTIPLIER_TRAITS::RowsATrans);
    assert(ColsC == MULTIPLIER_TRAITS::ColsBTrans);

    // Use the traits struct to specify the correct type for the intermediate variables
    typename MULTIPLIER_TRAITS::INPUT_T cast_in_a, cast_in_b;
    typename MULTIPLIER_TRAITS::MULT_T mult;
    typename MULTIPLIER_TRAITS::ACCUM_T recast_mult;
    typename MULTIPLIER_TRAITS::ACCUM_T sum_mult[MULTIPLIER_TRAITS::RowsATrans][MULTIPLIER_TRAITS::ColsBTrans];

#pragma HLS ARRAY_PARTITION variable = B cyclic dim = MULTIPLIER_TRAITS::B_UNROLL_DIM factor = \
    MULTIPLIER_TRAITS::UNROLL_FACTOR
#pragma HLS ARRAY_PARTITION variable = C cyclic dim = 2 factor = MULTIPLIER_TRAITS::UNROLL_FACTOR
#pragma HLS ARRAY_PARTITION variable = sum_mult cyclic dim = 2 factor = MULTIPLIER_TRAITS::UNROLL_FACTOR

a_col_loop:
    for (int k = 0; k < MULTIPLIER_TRAITS::ColsATrans; k++) {
    a_row_loop:
        for (int r = 0; r < MULTIPLIER_TRAITS::RowsATrans; r++) {
        b_col_loop:
            for (int c = 0; c < MULTIPLIER_TRAITS::ColsBTrans; c++) {
#pragma HLS PIPELINE II = MULTIPLIER_TRAITS::INNER_II
#pragma HLS UNROLL FACTOR = MULTIPLIER_TRAITS::UNROLL_FACTOR
                // Multiply
                cast_in_a = GetMatrixElement<TransposeFormA, RowsA, ColsA, InputType>(A, r, k);
                cast_in_b = GetMatrixElement<TransposeFormB, RowsB, ColsB, InputType>(B, k, c);
                mult = cast_in_a * cast_in_b;
                // Cast mult to the correct output size before adding.
                recast_mult = mult;
                // Sum
                if (k == 0) {
                    // Initialise
                    sum_mult[r][c] = recast_mult;
                } else if (k == MULTIPLIER_TRAITS::ColsATrans - 1) {
                    // Store result
                    C[r][c] = sum_mult[r][c] + recast_mult;
                } else {
                    // Accumulate
                    sum_mult[r][c] = sum_mult[r][c] + recast_mult;
                }
            }
        }
    }
}

// ===================================================================================================================
// matrixMultiplyAddTree: Architecture using an adder tree
// o Optimized for unrolled floating-point. In fixed-point, this adds overhead.
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyAddTree(const InputType A[RowsA][ColsA],
                           const InputType B[RowsB][ColsB],
                           OutputType C[RowsC][ColsC]) {
    // Check defined array dimensions are compatible
    // - The ROWS and COLS value of the traits considers the transpose operation request for A & B
    assert(MULTIPLIER_TRAITS::ColsATrans == MULTIPLIER_TRAITS::RowsBTrans);
    assert(RowsC == MULTIPLIER_TRAITS::RowsATrans);
    assert(ColsC == MULTIPLIER_TRAITS::ColsBTrans);

#pragma HLS ARRAY_PARTITION variable = A complete dim = MULTIPLIER_TRAITS::A_FULL_UNROLL_DIM
#pragma HLS ARRAY_PARTITION variable = B complete dim = MULTIPLIER_TRAITS::B_FULL_UNROLL_DIM

    // Use the traits struct to specify the correct type for the intermediate variables
    typename MULTIPLIER_TRAITS::INPUT_T cast_in_a, cast_in_b;
    typename MULTIPLIER_TRAITS::MULT_T mult;

    // Determine the number of ranks for the adder tree and declare array
    // o The adder_tree is larger than required as each rank only needs to be half the size of the previous rank
    const unsigned a_cols_log2 = BitWidth<MULTIPLIER_TRAITS::ColsATrans>::Value;
    const unsigned a_cols_sub1_log2 = BitWidth<MULTIPLIER_TRAITS::ColsATrans - 1>::Value;
    const unsigned num_ranks = (a_cols_log2 != a_cols_sub1_log2 ? a_cols_log2 : a_cols_log2 + 1);

    typename MULTIPLIER_TRAITS::ACCUM_T adder_tree[num_ranks][MULTIPLIER_TRAITS::ColsATrans];

a_row_loop:
    for (int i = 0; i < MULTIPLIER_TRAITS::RowsATrans; i++) {
    b_col_loop:
        for (int j = 0; j < MULTIPLIER_TRAITS::ColsBTrans; j++) {
// Fully unrolled inner loop
#pragma HLS PIPELINE II = 1
        a_col_loop:
            for (int k = 0; k < MULTIPLIER_TRAITS::ColsATrans; k++) {
                cast_in_a = GetMatrixElement<TransposeFormA, RowsA, ColsA, InputType>(A, i, k);
                cast_in_b = GetMatrixElement<TransposeFormB, RowsB, ColsB, InputType>(B, k, j);
                mult = cast_in_a * cast_in_b;
                // Assign multiplier output to the first rank of the adder tree array
                adder_tree[num_ranks - 1][k] = mult;
            }

            // Build adder tree
            unsigned rank_size = MULTIPLIER_TRAITS::ColsATrans;
        add_level_loop:
            for (int adder_tree_rank = num_ranks - 2; adder_tree_rank >= 0; adder_tree_rank--) {
                unsigned prev_rank_is_odd = rank_size % 2;
                rank_size = (rank_size + 1) / 2; // rank size
            // Fixed loop size so it can be unrolled.
            add_col_loop:
                for (int jj = 0; jj < ((MULTIPLIER_TRAITS::ColsATrans + 1) / 2); jj++) {
                    if (jj < rank_size) {
                        if (prev_rank_is_odd && jj == rank_size - 1) {
                            // Bypass, no adder required.
                            adder_tree[adder_tree_rank][jj] = adder_tree[adder_tree_rank + 1][(jj * 2)];
                        } else {
                            adder_tree[adder_tree_rank][jj] =
                                adder_tree[adder_tree_rank + 1][jj * 2] + adder_tree[adder_tree_rank + 1][(jj * 2) + 1];
                        }
                    }
                }
            }
            // Store result
            C[i][j] = adder_tree[0][0];
        }
    }
}

// ===================================================================================================================
// matrixMultiplyFull: Default architecture including directives to fully unroll inner loop, fixed point
// implementations
// o Fixed point implementation maps well to DSP48 cascades
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyFull(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC]) {
    // Check defined array dimensions are compatible
    // - The ROWS and COLS value of the traits considers the transpose operation request for A & B
    assert(MULTIPLIER_TRAITS::ColsATrans == MULTIPLIER_TRAITS::RowsBTrans);
    assert(RowsC == MULTIPLIER_TRAITS::RowsATrans);
    assert(ColsC == MULTIPLIER_TRAITS::ColsBTrans);

    // Use the traits struct to specify the correct type for the intermediate variables
    typename MULTIPLIER_TRAITS::INPUT_T cast_in_a, cast_in_b;
    typename MULTIPLIER_TRAITS::MULT_T mult;
    typename MULTIPLIER_TRAITS::ACCUM_T recast_mult, sum_mult;

#pragma HLS ARRAY_PARTITION variable = A complete dim = MULTIPLIER_TRAITS::A_FULL_UNROLL_DIM
#pragma HLS ARRAY_PARTITION variable = B complete dim = MULTIPLIER_TRAITS::B_FULL_UNROLL_DIM

a_row_loop:
    for (int r = 0; r < MULTIPLIER_TRAITS::RowsATrans; r++) {
    b_col_loop:
        for (int c = 0; c < MULTIPLIER_TRAITS::ColsBTrans; c++) {
// Fully unrolled inner loop
#pragma HLS PIPELINE II = 1
        a_col_loop:
            for (int k = 0; k < MULTIPLIER_TRAITS::ColsATrans; k++) {
                cast_in_a = GetMatrixElement<TransposeFormA, RowsA, ColsA, InputType>(A, r, k);
                cast_in_b = GetMatrixElement<TransposeFormB, RowsB, ColsB, InputType>(B, k, c);
                mult = cast_in_a * cast_in_b;

                // Cast mult to the correct output size before adding.
                recast_mult = mult;
                if (k == 0)
                    sum_mult = recast_mult;
                else
                    sum_mult += recast_mult;

                // Store result
                if (k == MULTIPLIER_TRAITS::ColsATrans - 1) C[r][c] = sum_mult;
            }
        }
    }
}

// ===================================================================================================================
// matrixMultiplyTop: Top level function that selects implementation architecture and internal types based on the
// traits class provided via the MULTIPLIER_TRAITS template parameter.
// o Call this function directly if you wish to override the default architecture choice or internal types
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename MULTIPLIER_TRAITS,
          typename InputType,
          typename OutputType>
void matrixMultiplyTop(const InputType A[RowsA][ColsA], const InputType B[RowsB][ColsB], OutputType C[RowsC][ColsC]) {
    switch (MULTIPLIER_TRAITS::ARCH) {
        case 0:
            matrixMultiplyDefault<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                                  MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
            break;
        case 1:
            matrixMultiplyAlt1<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                               MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
            break;
        case 2:
            matrixMultiplyAlt2<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                               MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
            break;
        case 3:
            matrixMultiplyAddTree<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                                  MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
            break;
        case 4:
            matrixMultiplyFull<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                               MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
            break;
        default:
            matrixMultiplyDefault<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC,
                                  MULTIPLIER_TRAITS, InputType, OutputType>(A, B, C);
    }
}

/**
 * @brief matrixMultiply entry point function for calculating matrix power.
 *
 *  @tparam TransposeFormA1   Defines if the first matix is transposed before the multiplication. Valid values are:
 * NoTranspose, Transpose, ConjugateTranspose
 *  @tparam TransposeFormA2   Defines if the second matix is transposed before the multiplication.
 *  @tparam RowsA             Defines the number of rows in the A matrix
 *  @tparam ColsA             Defines the number of columns in the A matrix
 *  @tparam RowsC             Defines the number of rows in the C matrix
 *  @tparam ColsC             Defines the number of columns in the C matrix
 *  @tparam InputType         Input data type
 *  @tparam OutputType        Output data type
 *  @tparam TRAITS            Traits class
 *
 *  @param matrixAStrm        Stream of input matrix
 *  @param matrixCStrm        Stream of A^2 product output matrix
 */
template <
    class TransposeFormA1,
    class TransposeFormA2,
    int RowsA,
    int ColsA,
    int RowsC,
    int ColsC,
    typename InputType,
    typename OutputType,
    typename TRAITS =
        matrixMultiplyTraits<TransposeFormA1, TransposeFormA2, RowsA, ColsA, RowsA, ColsA, InputType, OutputType> >
void matrixMultiply(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixCStrm) {
    InputType A1[RowsA][ColsA];
    InputType A2[RowsA][ColsA];
    OutputType C[RowsC][ColsC];

    for (int r = 0; r < RowsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsA; c++) {
            matrixAStrm.read(A1[r][c]);
            A2[r][c] = A1[r][c];
        }
    }
    matrixMultiplyTop<TransposeFormA1, TransposeFormA2, RowsA, ColsA, RowsA, ColsA, RowsC, ColsC, TRAITS, InputType,
                      OutputType>(A1, A2, C);
    for (int r = 0; r < RowsC; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsC; c++) {
            matrixCStrm.write(C[r][c]);
        }
    }
}
/**
 * @brief matrixMultiply entry point function.
 *
 *  @tparam TransposeFormA    Defines if the A matix is transposed before the multiplication. Valid values are:
 * NoTranspose, Transpose, ConjugateTranspose
 *  @tparam TransposeFormB    Defines if the B matix is transposed before the multiplication.
 *  @tparam RowsA             Defines the number of rows in the A matrix
 *  @tparam ColsA             Defines the number of columns in the A matrix
 *  @tparam RowsB             Defines the number of rows in the B matrix
 *  @tparam ColsB             Defines the number of columns in the B matrix
 *  @tparam RowsC             Defines the number of rows in the C matrix
 *  @tparam ColsC             Defines the number of columns in the C matrix
 *  @tparam InputType         Input data type
 *  @tparam OutputType        Output data type
 *  @tparam TRAITS            Traits class
 *
 *  @param matrixAStrm        Stream of First input matrix
 *  @param matrixBStrm        Stream of Second input matrix
 *  @param matrixCStrm        Stream of AB product output matrix
 */
template <class TransposeFormA,
          class TransposeFormB,
          int RowsA,
          int ColsA,
          int RowsB,
          int ColsB,
          int RowsC,
          int ColsC,
          typename InputType,
          typename OutputType,
          typename TRAITS =
              matrixMultiplyTraits<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, InputType, OutputType> >
void matrixMultiply(hls::stream<InputType>& matrixAStrm,
                    hls::stream<InputType>& matrixBStrm,
                    hls::stream<OutputType>& matrixCStrm) {
    InputType A[RowsA][ColsA];
    InputType B[RowsB][ColsB];
    OutputType C[RowsC][ColsC];

    for (int r = 0; r < RowsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsA; c++) {
            matrixAStrm.read(A[r][c]);
        }
    }
    for (int r = 0; r < RowsB; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsB; c++) {
            matrixBStrm.read(B[r][c]);
        }
    }
    matrixMultiplyTop<TransposeFormA, TransposeFormB, RowsA, ColsA, RowsB, ColsB, RowsC, ColsC, TRAITS, InputType,
                      OutputType>(A, B, C);
    for (int r = 0; r < RowsC; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsC; c++) {
            matrixCStrm.write(C[r][c]);
        }
    }
}
} // end namespace solver
} // end namespace xf

#endif
