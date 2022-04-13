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

// ======================================================================
// Common testbench for SVD function.
//
// Golden data is generated from reference Model LAPACKe
//
// ======================================================================

#include "test_svd.hpp"
#include "kernel_svd.hpp"
#include "src/utils.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>

// ---------------------------------------------------------------------------------------------
// Main test program
// ---------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Variables set by command line
    // ====================================================================
    long unsigned num_tests = 1;
    // long unsigned num_tests            = 3;//20;//40;//100;    // Small default for HLS
    unsigned allowed_ulp_mismatch = 0;
    unsigned int debug = 0;
    float ratio_threshold = 50.0; // Defined in LAPACK test file
    float mat_type = 0;           // Specify to only run a single matrix type.
    float skip_mat_type = 0;
    unsigned int s_to_diag = 1;      // WARNING: By default do this but it makes the results worse!!!!
    unsigned int reduce_complex = 1; // Run a reduced set of matrix types for the complex case
    // - Trying to get AVE tests not to time out
    unsigned print_precision = 10;

    // Parse command line options
    // ====================================================================
    std::vector<std::string> args(argv + 1, argv + argc);
    for (std::vector<std::string>::iterator i = args.begin(); i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            std::cout << "Syntax: main.exe [-num_tests <long unsigned> -mat_type <unsigned> -skip_mat_type <unsigned> "
                         "-reduce_complex <unsigned>]"
                      << std::endl;
            return 0;
        } else if (*i == "-num_tests") {
            // This loses the MSB so 18446744073709551615 gets converted into 9223372036854775807.
            // It's probably not a big issue though.
            num_tests = (long unsigned)atol((*++i).c_str());
            printf("num_tests as long unsigned = %lu\n", num_tests);
        } else if (*i == "-max_ulp_mismatch_in_l") {
            allowed_ulp_mismatch = (unsigned)atol((*++i).c_str());
        } else if (*i == "-prec") {
            print_precision = (unsigned)atol((*++i).c_str());
        } else if (*i == "-ratio_threshold") {
            ratio_threshold = (float)atol((*++i).c_str());
        } else if (*i == "-mat_type") {
            mat_type = (unsigned)atol((*++i).c_str());
        } else if (*i == "-skip_mat_type") {
            skip_mat_type = (unsigned)atol((*++i).c_str());
        } else if (*i == "-debug") {
            debug = (unsigned)atol((*++i).c_str());
        } else if (*i == "-s_to_diag") {
            s_to_diag = (unsigned)atol((*++i).c_str());
        } else if (*i == "-reduce_complex") {
            reduce_complex = (unsigned)atol((*++i).c_str());
        } else {
            printf("Unknown command line option %s.  Try again\n", i->c_str());
            exit(1);
        }
    }

    // Matrix arrays
    MATRIX_IN_T A[ROWS][COLS];  // The input array.  Cast from A_generated
    MATRIX_OUT_T U[ROWS][ROWS]; // The U result from the DUT
    MATRIX_OUT_T S[ROWS][COLS]; // The S result from the DUT
    MATRIX_OUT_T V[COLS][COLS]; // The V result from the DUT

    MATRIX_OUT_T U_expected[ROWS][ROWS]; // The U result from LAPACK in target format
    MATRIX_OUT_T S_expected[ROWS][COLS]; // The S result from LAPACK in target format
    MATRIX_OUT_T V_expected[COLS][COLS]; // The V result from LAPACK in target format

    // Test variables
    L_TYPE A_cast[ROWS][COLS];                  // Cast A back to LAPACK compatible type for analysis
    MATRIX_OUT_T U_delta_vs_lapack[ROWS][ROWS]; // The difference between the DUT's U and LAPACK's U
    MATRIX_OUT_T S_delta_vs_lapack[ROWS][COLS]; // The difference between the DUT's S and LAPACK's S
    MATRIX_OUT_T V_delta_vs_lapack[COLS][COLS]; // The difference between the DUT's V and LAPACK's V
    L_TYPE U_cast[ROWS][ROWS];
    L_TYPE S_cast[ROWS][COLS];
    L_TYPE V_cast[COLS][COLS];
    L_TYPE U_expected_cast[ROWS][ROWS];
    L_TYPE S_expected_cast[ROWS][COLS];
    L_TYPE V_expected_cast[COLS][COLS];
    L_TYPE SV[ROWS][COLS];                // A recreated from U*S*V
    L_TYPE A_restored[ROWS][COLS];        // A recreated from U*S*V
    L_TYPE SV_lapack[ROWS][COLS];         // A recreated from LAPACK's U*S*V
    L_TYPE A_restored_lapack[ROWS][COLS]; // A recreated from LAPACK's U*S*V
    L_TYPE I_from_U[ROWS][ROWS];          // Identity created from U'*U
    L_TYPE I_from_U_lapack[ROWS][ROWS];   // Identity created from LAPACK's U'*U
    L_TYPE I_from_V[COLS][COLS];          // Identity created from V*V'
    L_TYPE I_from_V_lapack[COLS][COLS];   // Identity created from LAPACK's V*V'
    L_TYPE I_ref_U[ROWS][ROWS];           // Reference Identity matrix
    L_TYPE I_ref_V[COLS][COLS];           // Reference Identity matrix

    // Constant to simplify the implementation of reducing matrix types for complex data types
    const bool reduce_matrix_types = (x_is_complex(S[0][0]) && reduce_complex);

    // Populate I_ref_x
    for (int irows = 0; irows < ROWS; irows++) {
        for (int icols = 0; icols < ROWS; icols++) {
            I_ref_U[irows][icols] = 0.0;
            if (irows == icols) {
                I_ref_U[irows][icols] = 1.0;
            }
        }
    }
    for (int irows = 0; irows < COLS; irows++) {
        for (int icols = 0; icols < COLS; icols++) {
            I_ref_V[irows][icols] = 0.0;
            if (irows == icols) {
                I_ref_V[irows][icols] = 1.0;
            }
        }
    }

    L_TYPE A_delta[ROWS][COLS];
    L_TYPE A_delta_lapack[ROWS][COLS];
    L_TYPE I_from_U_delta[ROWS][ROWS];
    L_TYPE I_from_U_delta_lapack[ROWS][ROWS];
    L_TYPE I_from_V_delta[COLS][COLS];
    L_TYPE I_from_V_delta_lapack[COLS][COLS];

    // Non-complex type
    L_BASE_TYPE A_delta_norm;
    L_BASE_TYPE A_delta_lapack_norm;
    L_BASE_TYPE I_from_V_delta_norm;
    L_BASE_TYPE I_from_V_delta_lapack_norm;
    L_BASE_TYPE I_from_U_delta_norm;
    L_BASE_TYPE I_from_U_delta_lapack_norm;
    L_BASE_TYPE A_norm;
    L_BASE_TYPE A_DUT_ratio, A_LAPACK_ratio;
    L_BASE_TYPE I_from_U_DUT_ratio, I_from_U_LAPACK_ratio;
    L_BASE_TYPE I_from_V_DUT_ratio, I_from_V_LAPACK_ratio;

    float imat_max_ratio_diff[3][NUM_MAT_TYPES];
    float imat_min_ratio_diff[3][NUM_MAT_TYPES];
    float imat_max_ratio[3][NUM_MAT_TYPES];
    float imat_min_ratio[3][NUM_MAT_TYPES];

    unsigned int ratio_same = 0;
    unsigned int ratio_better = 0;
    unsigned int ratio_worse = 0;
    unsigned int imat_ratio_same[3][NUM_MAT_TYPES];
    unsigned int imat_ratio_better[3][NUM_MAT_TYPES];
    unsigned int imat_ratio_worse[3][NUM_MAT_TYPES];
    // Zero values
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        for (int t = 0; t < 3; t++) {
            imat_max_ratio_diff[t][i] = 0;
            imat_min_ratio_diff[t][i] = 0;
            imat_max_ratio[t][i] = 0;
            imat_min_ratio[t][i] = ratio_threshold; // 0; Work backwards from worse case?
            imat_ratio_same[t][i] = 0;
            imat_ratio_better[t][i] = 0;
            imat_ratio_worse[t][i] = 0;
        }
    }

    float A_ratio_difference = 0;
    float I_from_V_ratio_difference = 0;
    float I_from_U_ratio_difference = 0;
    unsigned int pass_fail = 0; // Pass=0 Fail =1

    bool U_matched_lapack, S_matched_lapack, V_matched_lapack;

    int ratio_check = 0;

    printf("Running %lu %s tests per matrix type on %d x %d matrices\n", num_tests,
           x_is_float(S[0][0])
               ? "single precision"
               : x_is_double(S[0][0]) ? "double precision" : x_is_fixed(S[0][0]) ? "fixed point" : "Unknown type",
           ROWS, COLS);

    // Generate results table header
    std::cout << "RESULTS_TABLE,Test,IMAT,U Matching,S Matching,V Matching,A DUT Ratio,A LAPACK Ratio,I_from_U DUT "
                 "Ratio,I_from_U LAPACK Ratio,I_from_V DUT Ratio,I_from_V LAPACK Ratio"
              << std::endl;

    for (unsigned int imat = 0; imat < NUM_MAT_TYPES; imat++) {
        // Test which matrix type to run or which one to skip
        if ((mat_type == 0 || imat + 1 == mat_type) && skip_mat_type - 1 != imat) {
            // Further test set reduction
            // - Remember imat indexed 0 but Verification doc is indexed 1
            if (!(reduce_matrix_types && (imat == 2 || imat == 4 || imat == 7 || imat == 9))) {
                int tests_to_run = num_tests;
                if (imat == 0 || imat == 1) {
                    // Special cases, only run a single test
                    tests_to_run = 1;
                }
                for (long unsigned i = 0; i < tests_to_run; i++) {
                    // Get Golden Values
                    // ====================================================================

                    std::string data_path = std::string(DATA_PATH);
                    std::string base_path;

                    if (x_is_complex(A[0][0]) == true) {
                        base_path = data_path.append("/complex/");
                    } else {
                        base_path = data_path.append("/float/");
                    }
                    std::string file_A =
                        base_path + "A_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                    std::string file_U =
                        base_path + "U_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                    std::string file_S =
                        base_path + "S_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                    std::string file_V =
                        base_path + "V_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";

                    int A_size = ROWS * COLS;
                    int U_size = ROWS * ROWS;
                    int S_size = ROWS * COLS;
                    int V_size = COLS * COLS;

                    MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);
                    MATRIX_OUT_T* U_ptr = reinterpret_cast<MATRIX_OUT_T*>(U);
                    MATRIX_OUT_T* S_ptr = reinterpret_cast<MATRIX_OUT_T*>(S);
                    MATRIX_OUT_T* V_ptr = reinterpret_cast<MATRIX_OUT_T*>(V);

                    readTxt(file_A, A_ptr, A_size);
                    readTxt(file_S, S_ptr, S_size);
                    readTxt(file_U, U_ptr, U_size);
                    readTxt(file_V, V_ptr, V_size);

                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            // Cast back to pick up quantization. Used in test criteria
                            A_cast[r][c] = A[r][c];
                        }
                    }

                    hls::stream<MATRIX_IN_T> matrixAStrm;
                    hls::stream<MATRIX_OUT_T> matrixSStrm;
                    hls::stream<MATRIX_OUT_T> matrixUStrm;
                    hls::stream<MATRIX_OUT_T> matrixVStrm;
                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            matrixAStrm.write(A[r][c]);
                        }
                    }

                    // Get Actual results
                    // ====================================================================
                    int return_code = kernel_svd_0(matrixAStrm, matrixSStrm, matrixUStrm, matrixVStrm);

                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            matrixSStrm.read(S[r][c]);
                        }
                    }
                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < ROWS; c++) {
                            matrixUStrm.read(U[r][c]);
                        }
                    }
                    for (int r = 0; r < COLS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            matrixVStrm.read(V[r][c]);
                        }
                    }

                    if (return_code) {
                        std::cout << "ERROR: Test function returned a non-zero result" << std::endl;
                    }

                    // Test results
                    // ====================================================================

                    // Populate arrays used for test criteria calc.
                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            S_expected_cast[r][c] = S_expected[r][c];
                            S_cast[r][c] = S[r][c];
                            // Force zero on off-diags....
                            // NOTE: The results become worse
                            if (r != c && s_to_diag > 0) {
                                S_cast[r][c] = 0;
                            }
                        }
                    }
                    for (int r = 0; r < ROWS; r++) {
                        for (int c = 0; c < ROWS; c++) {
                            U_expected_cast[r][c] = U_expected[r][c];
                            U_cast[r][c] = U[r][c];
                        }
                    }
                    for (int r = 0; r < COLS; r++) {
                        for (int c = 0; c < COLS; c++) {
                            V_expected_cast[r][c] = V_expected[r][c];
                            V_cast[r][c] = V[r][c];
                        }
                    }

                    // Basic check cell by cell check based on allowed_ulp_mismatch value.
                    U_matched_lapack = are_matrices_equal<ROWS, ROWS, MATRIX_OUT_T>(
                        (MATRIX_OUT_T*)U, (MATRIX_OUT_T*)U_expected, allowed_ulp_mismatch,
                        (MATRIX_OUT_T*)U_delta_vs_lapack);
                    S_matched_lapack = are_matrices_equal<ROWS, COLS, MATRIX_OUT_T>(
                        (MATRIX_OUT_T*)S, (MATRIX_OUT_T*)S_expected, allowed_ulp_mismatch,
                        (MATRIX_OUT_T*)S_delta_vs_lapack);
                    V_matched_lapack = are_matrices_equal<COLS, COLS, MATRIX_OUT_T>(
                        (MATRIX_OUT_T*)V, (MATRIX_OUT_T*)V_expected, allowed_ulp_mismatch,
                        (MATRIX_OUT_T*)V_delta_vs_lapack);

                    // Test Criteria 1
                    // o norm( A – U * S * V ) / ( norm(A) * max(M,N) * EPS )
                    // mmult<ROWS,COLS,COLS,false,true,L_TYPE,L_TYPE>(S_cast,V_cast,SV);
                    mmult<false, true, ROWS, COLS, COLS, COLS, ROWS, COLS>(S_cast, V_cast, SV);
                    // mmult<ROWS,ROWS,COLS,false,false,L_TYPE,L_TYPE>(U_cast,SV,A_restored);
                    mmult<false, false, ROWS, ROWS, ROWS, COLS, ROWS, COLS>(U_cast, SV, A_restored);
                    msub<ROWS, COLS, L_TYPE, L_TYPE>(A_restored, A_cast, A_delta);

                    // NOTE/TODO: Don't need to transpose LAPACK return value!!!
                    // mmult<ROWS,COLS,COLS,false,false,L_TYPE,L_TYPE>(S_expected_cast,V_expected_cast,SV_lapack);
                    mmult<false, false, ROWS, COLS, COLS, COLS, ROWS, COLS>(S_expected_cast, V_expected_cast,
                                                                            SV_lapack);
                    // mmult<ROWS,ROWS,COLS,false,false,L_TYPE,L_TYPE>(U_expected_cast,SV_lapack,A_restored_lapack);
                    mmult<false, false, ROWS, ROWS, ROWS, COLS, ROWS, COLS>(U_expected_cast, SV_lapack,
                                                                            A_restored_lapack);
                    msub<ROWS, COLS, L_TYPE, L_TYPE>(A_restored_lapack, A_cast, A_delta_lapack);

                    A_norm = norm1_dbl<ROWS, COLS, L_TYPE, L_BASE_TYPE>(A_cast);
                    A_delta_norm = norm1_dbl<ROWS, COLS, L_TYPE, L_BASE_TYPE>(A_delta);
                    A_delta_lapack_norm = norm1_dbl<ROWS, COLS, L_TYPE, L_BASE_TYPE>(A_delta_lapack);

                    A_DUT_ratio = A_delta_norm / (MAX_DIM * A_norm * eps);
                    A_LAPACK_ratio = A_delta_lapack_norm / (MAX_DIM * A_norm * eps);

                    // Check ratio value for invalid values
                    // - Skip for zero matrix
                    if (imat != 0) {
                        ratio_check = check_ratio(A_DUT_ratio);
                        if (ratio_check) {
                            std::cout << "ERROR: Check A_DUT_ratio failed." << std::endl;
                            pass_fail = 1;
                        }
                    }

                    // Compare DUT vs LAPACK ratios
                    A_ratio_difference = (A_DUT_ratio - A_LAPACK_ratio) * 100.0;

                    if (A_ratio_difference > imat_max_ratio_diff[0][imat]) {
                        imat_max_ratio_diff[0][imat] = A_ratio_difference;
                    }
                    if (A_ratio_difference < imat_min_ratio_diff[0][imat]) {
                        imat_min_ratio_diff[0][imat] = A_ratio_difference;
                    }
                    // NOTE: Not tolerance just now.
                    if (A_ratio_difference < 0) {
                        imat_ratio_better[0][imat]++;
                    } else if (A_ratio_difference > 0) {
                        imat_ratio_worse[0][imat]++;
                    } else {
                        imat_ratio_same[0][imat]++;
                    }
                    // Log max and min ratio values, use DUT value
                    if (A_DUT_ratio > imat_max_ratio[0][imat]) {
                        imat_max_ratio[0][imat] = A_DUT_ratio;
                    }
                    if (A_DUT_ratio < imat_min_ratio[0][imat]) {
                        imat_min_ratio[0][imat] = A_DUT_ratio;
                    }

                    // Test Criteria 2
                    // o norm( I – UT * U) / ( M * EPS )
                    // mmult<ROWS,ROWS,ROWS,false,true,L_TYPE,L_TYPE>(U_cast,U_cast,I_from_U);
                    mmult<false, true, ROWS, ROWS, ROWS, ROWS, ROWS, ROWS>(U_cast, U_cast, I_from_U);
                    msub<ROWS, ROWS, L_TYPE, L_TYPE>(I_ref_U, I_from_U, I_from_U_delta);

                    // mmult<ROWS,ROWS,ROWS,false,true,L_TYPE,L_TYPE>(U_expected_cast,U_expected_cast,I_from_U_lapack);
                    mmult<false, true, ROWS, ROWS, ROWS, ROWS, ROWS, ROWS>(U_expected_cast, U_expected_cast,
                                                                           I_from_U_lapack);
                    msub<ROWS, ROWS, L_TYPE, L_TYPE>(I_ref_U, I_from_U_lapack, I_from_U_delta_lapack);

                    I_from_U_delta_norm = norm1_dbl<ROWS, ROWS, L_TYPE, L_BASE_TYPE>(I_from_U_delta);
                    I_from_U_delta_lapack_norm = norm1_dbl<ROWS, ROWS, L_TYPE, L_BASE_TYPE>(I_from_U_delta_lapack);

                    I_from_U_DUT_ratio = I_from_U_delta_norm / (ROWS * eps);
                    I_from_U_LAPACK_ratio = I_from_U_delta_lapack_norm / (ROWS * eps);

                    // Check ratio value for invalid values
                    ratio_check = check_ratio(I_from_U_DUT_ratio);
                    if (ratio_check) {
                        std::cout << "ERROR: Check I_from_U_DUT_ratio failed." << std::endl;
                        pass_fail = 1;
                    }

                    // Compare DUT vs LAPACK ratios
                    I_from_U_ratio_difference = (I_from_U_DUT_ratio - I_from_U_LAPACK_ratio) * 100.0;

                    if (I_from_U_ratio_difference > imat_max_ratio_diff[1][imat]) {
                        imat_max_ratio_diff[1][imat] = I_from_U_ratio_difference;
                    }
                    if (I_from_U_ratio_difference < imat_min_ratio_diff[1][imat]) {
                        imat_min_ratio_diff[1][imat] = I_from_U_ratio_difference;
                    }
                    // NOTE: Not tolerance just now.
                    if (I_from_U_ratio_difference < 0) {
                        imat_ratio_better[1][imat]++;
                    } else if (A_ratio_difference > 0) {
                        imat_ratio_worse[1][imat]++;
                    } else {
                        imat_ratio_same[1][imat]++;
                    }
                    // Log max and min ratio values, use DUT value
                    if (I_from_U_DUT_ratio > imat_max_ratio[1][imat]) {
                        imat_max_ratio[1][imat] = I_from_U_DUT_ratio;
                    }
                    if (I_from_U_DUT_ratio < imat_min_ratio[1][imat]) {
                        imat_min_ratio[1][imat] = I_from_U_DUT_ratio;
                    }

                    // Test Criteria 3
                    // o norm( I – V * VT ) / ( N * EPS )
                    // mmult<COLS,COLS,COLS,false,true,L_TYPE,L_TYPE>(V_cast,V_cast,I_from_V);
                    mmult<false, true, COLS, COLS, COLS, COLS, COLS, COLS>(V_cast, V_cast, I_from_V);
                    msub<COLS, COLS, L_TYPE, L_TYPE>(I_ref_V, I_from_V, I_from_V_delta);

                    // mmult<COLS,COLS,COLS,false,true,L_TYPE,L_TYPE>(V_expected_cast,V_expected_cast,I_from_V_lapack);
                    mmult<false, true, COLS, COLS, COLS, COLS, COLS, COLS>(V_expected_cast, V_expected_cast,
                                                                           I_from_V_lapack);
                    msub<COLS, COLS, L_TYPE, L_TYPE>(I_ref_V, I_from_V_lapack, I_from_V_delta_lapack);

                    I_from_V_delta_norm = norm1_dbl<COLS, COLS, L_TYPE, L_BASE_TYPE>(I_from_V_delta);
                    I_from_V_delta_lapack_norm = norm1_dbl<COLS, COLS, L_TYPE, L_BASE_TYPE>(I_from_V_delta_lapack);

                    I_from_V_DUT_ratio = I_from_V_delta_norm / (COLS * eps);
                    I_from_V_LAPACK_ratio = I_from_V_delta_lapack_norm / (COLS * eps);

                    // Check ratio value for invalid values
                    ratio_check = check_ratio(I_from_V_DUT_ratio);
                    if (ratio_check) {
                        std::cout << "ERROR: Check I_from_V_DUT_ratio failed." << std::endl;
                        pass_fail = 1;
                    }

                    // Compare DUT vs LAPACK ratios
                    I_from_V_ratio_difference = (I_from_V_DUT_ratio - I_from_V_LAPACK_ratio) * 100.0;

                    if (I_from_V_ratio_difference > imat_max_ratio_diff[2][imat]) {
                        imat_max_ratio_diff[2][imat] = I_from_V_ratio_difference;
                    }
                    if (I_from_V_ratio_difference < imat_min_ratio_diff[2][imat]) {
                        imat_min_ratio_diff[2][imat] = I_from_V_ratio_difference;
                    }
                    // NOTE: Not tolerance just now.
                    if (I_from_V_ratio_difference < 0) {
                        imat_ratio_better[2][imat]++;
                    } else if (A_ratio_difference > 0) {
                        imat_ratio_worse[2][imat]++;
                    } else {
                        imat_ratio_same[2][imat]++;
                    }
                    // Log max and min ratio values, use DUT value
                    if (I_from_V_DUT_ratio > imat_max_ratio[2][imat]) {
                        imat_max_ratio[2][imat] = I_from_V_DUT_ratio;
                    }
                    if (I_from_V_DUT_ratio < imat_min_ratio[2][imat]) {
                        imat_min_ratio[2][imat] = I_from_V_DUT_ratio;
                    }

                    // Test Criteria 4
                    // - S contains min(M,N) nonnegative values in decreasing order
                    // - Initially will only test to non-negative values as the current SVD implementation doesn't sort
                    // the singular values
                    int num_neg_values = 0;
                    for (int d = 0; d < MIN_DIM; d++) {
                        // Should be real only
                        if (hls::x_real(S_cast[d][d]) < 0) {
                            num_neg_values++;
                            if (debug > 0) {
                                std::cout << "ERROR: Negative singular value: " << S_cast[d][d] << " Index: " << d
                                          << std::endl;
                            }
                        }
                    }
                    if (num_neg_values > 0) {
                        std::cout << "ERROR: Negative singular values generated. " << num_neg_values << " out of "
                                  << MIN_DIM << " values." << std::endl;
                        pass_fail = 1;
                    }

                    // Test Criteria 5
                    // - Non lapack test but add a check that the singular values are real only
                    int num_imag_values = 0;
                    for (int d = 0; d < MIN_DIM; d++) {
                        // Should be real only
                        if (hls::x_imag(S_cast[d][d]) != 0) {
                            num_imag_values++;
                        }
                    }
                    if (num_imag_values > 0) {
                        std::cout << "ERROR: Non-real singular values generated. " << num_imag_values << " out of "
                                  << MIN_DIM << " values." << std::endl;
                        pass_fail = 1;
                    }

                    // Test Criteria 6
                    // - Non lapack test
                    // - Check for any nans in any of the output arrays
                    // - Andrew had found on the QRF that the ratios could still be OK even though the output matrix
                    // contained nans
                    if (anyNaN<ROWS, COLS>(S_cast) == 1) {
                        std::cout << "ERROR: Nan found in S" << std::endl;
                        pass_fail = 1;
                    }
                    if (anyNaN<ROWS, ROWS>(U_cast) == 1) {
                        std::cout << "ERROR: Nan found in U" << std::endl;
                        pass_fail = 1;
                    }
                    if (anyNaN<COLS, COLS>(V_cast) == 1) {
                        std::cout << "ERROR: Nan found in V" << std::endl;
                        pass_fail = 1;
                    }

                    // Test Criteria 7
                    // - Non lapack test
                    // - Check S is diagonal ???
                    // - Difficult, what consitutes a 0?

                    std::cout << "RESULTS_TABLE," << i << "," << imat + 1 << "," << U_matched_lapack << ","
                              << S_matched_lapack << "," << V_matched_lapack << "," << A_DUT_ratio << ","
                              << A_LAPACK_ratio << "," << I_from_U_DUT_ratio << "," << I_from_U_LAPACK_ratio << ","
                              << I_from_V_DUT_ratio << "," << I_from_V_LAPACK_ratio << std::endl;

                    // Determine if pass or fail.
                    // o Check DUT ratio against test threshold, default taken from LAPACK
                    int individual_pass_fail = 0;
                    if (A_DUT_ratio > ratio_threshold) {
                        std::cout << "ERROR: A_DUT_ratio(" << A_DUT_ratio << ") > ratio_threshold(" << ratio_threshold
                                  << "). LAPACK ratio = " << A_LAPACK_ratio << std::endl;
                        pass_fail = 1; // Test run fails
                        individual_pass_fail = 1;
                    }
                    if (I_from_U_DUT_ratio > ratio_threshold) {
                        std::cout << "ERROR: I_from_U_DUT_ratio(" << I_from_U_DUT_ratio << ") > ratio_threshold("
                                  << ratio_threshold << "). LAPACK ratio = " << I_from_U_LAPACK_ratio << std::endl;
                        pass_fail = 1; // Test run fails
                        individual_pass_fail = 1;
                    }
                    if (I_from_V_DUT_ratio > ratio_threshold) {
                        std::cout << "ERROR: I_from_V_DUT_ratio(" << I_from_V_DUT_ratio << ") > ratio_threshold("
                                  << ratio_threshold << "). LAPACK ratio = " << I_from_V_LAPACK_ratio << std::endl;
                        pass_fail = 1; // Test run fails
                        individual_pass_fail = 1;
                    }

                    // Print matrix's for debug when worse than LAPACK or cholesky returns non-zero
                    if (((A_ratio_difference > 0 || I_from_U_ratio_difference > 0 || I_from_V_ratio_difference > 0) &&
                         debug > 0) ||
                        debug > 1 || return_code || individual_pass_fail > 0) {
                        printf("  A=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(A_cast, "   ",
                                                                                              print_precision, 1);
                        printf("\n");

                        printf("  S=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(S_cast, "   ",
                                                                                              print_precision, 1);
                        printf("  S_lapack=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(S_expected_cast, "   ",
                                                                                              print_precision, 1);
                        printf("  U=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(U_cast, "   ",
                                                                                              print_precision, 1);
                        printf("  U_lapack=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(U_expected_cast, "   ",
                                                                                              print_precision, 1);
                        printf("  V=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(V_cast, "   ",
                                                                                              print_precision, 1);
                        printf("  V_lapack=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(V_expected_cast, "   ",
                                                                                              print_precision, 1);
                        printf("\n");

                        printf("  A_restored=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(A_restored, "   ",
                                                                                              print_precision, 1);
                        printf("  A_restored_lapack=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(A_restored_lapack, "   ",
                                                                                              print_precision, 1);
                        printf("  A_delta=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(A_delta, "   ",
                                                                                              print_precision, 1);
                        printf("  A_delta_lapack=\n");
                        xf::solver::print_matrix<ROWS, COLS, L_TYPE, xf::solver::NoTranspose>(A_delta_lapack, "   ",
                                                                                              print_precision, 1);
                        printf("\n");

                        printf("  I_from_U=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(I_from_U, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_U_lapack=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(I_from_U_lapack, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_U_delta=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(I_from_U_delta, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_U_delta_lapack=\n");
                        xf::solver::print_matrix<ROWS, ROWS, L_TYPE, xf::solver::NoTranspose>(
                            I_from_U_delta_lapack, "   ", print_precision, 1);
                        printf("\n");

                        printf("  I_from_V=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(I_from_V, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_V_lapack=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(I_from_V_lapack, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_V_delta=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(I_from_V_delta, "   ",
                                                                                              print_precision, 1);
                        printf("  I_from_V_delta_lapack=\n");
                        xf::solver::print_matrix<COLS, COLS, L_TYPE, xf::solver::NoTranspose>(
                            I_from_V_delta_lapack, "   ", print_precision, 1);
                        printf("\n");
                    }

                    // Exit when test or LAPACK functions failed.
                    if (return_code) {
                        std::cout << "TB:Fail" << std::endl;
                        return (1);
                    }

                } // End of test loop
            }     // 2nd type test
        }         // Type test
    }             // Matrix type loop

    // Summary table, catagorize by input matrix (imat) type
    std::cout << "SUMMARY_TABLE,Test,imat,Min Ratio, Max Ratio,Min Diff (Smaller),Max Diff (Larger),Same,Better,Worse"
              << std::endl;
    float max_ratio_diff = 0;
    float min_ratio_diff = 0;
    float max_ratio = 0;
    float min_ratio = ratio_threshold; // as per the imat array starting point
    for (int t = 0; t < 3; t++) {
        for (int i = 0; i < NUM_MAT_TYPES; i++) {
            std::cout << "SUMMARY_TABLE," << t << "," << i + 1 << "," << imat_min_ratio[t][i] << ","
                      << imat_max_ratio[t][i] << "," << imat_min_ratio_diff[t][i] << "," << imat_max_ratio_diff[t][i]
                      << "," << imat_ratio_same[t][i] << "," << imat_ratio_better[t][i] << "," << imat_ratio_worse[t][i]
                      << std::endl;
            // Calc totals
            if (imat_max_ratio_diff[t][i] > max_ratio_diff) {
                max_ratio_diff = imat_max_ratio_diff[t][i];
            }
            if (imat_min_ratio_diff[t][i] < min_ratio_diff) {
                min_ratio_diff = imat_min_ratio_diff[t][i];
            }
            if (imat_max_ratio[t][i] > max_ratio) {
                max_ratio = imat_max_ratio[t][i];
            }
            if (imat_min_ratio[t][i] < min_ratio) {
                min_ratio = imat_min_ratio[t][i];
            }
            ratio_better += imat_ratio_better[t][i];
            ratio_worse += imat_ratio_worse[t][i];
            ratio_same += imat_ratio_same[t][i];
        }
    }
    std::cout << "SUMMARY_TABLE,all,all," << min_ratio << "," << max_ratio << "," << min_ratio_diff << ","
              << max_ratio_diff << "," << ratio_same << "," << ratio_better << "," << ratio_worse << std::endl;

    if (pass_fail) {
        std::cout << "TB:Fail" << std::endl;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }
    return (pass_fail);
}
