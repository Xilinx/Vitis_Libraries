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
// Common testbench for Cholesky functions.
// ======================================================================

#include "test_cholesky.hpp"
#include "kernel_cholesky.hpp"
#include "src/utils.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
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
    // long unsigned num_tests = (DIM >= 16 ? 5 : 20); // 100;    // Small default for HLS
    unsigned allowed_ulp_mismatch = 0;
    unsigned int debug = 0;
    double ratio_threshold = 30.0; // Defined in lapack-X.Y.Z/TESTING/[s|d|c|x]test.in
    unsigned int mat_type = 0;     // Specify to only run a single matrix type.
    unsigned int skip_mat_type = 0;
    unsigned print_precision = 10;

    // Parse command line options
    // ====================================================================
    std::vector<std::string> args(argv + 1, argv + argc);
    for (std::vector<std::string>::iterator i = args.begin(); i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            std::cout << "Syntax: main.exe [-num_tests <long unsigned> -mat_type <unsigned> -skip_mat_type <unsigned>]"
                      << std::endl;
            return 0;
        } else if (*i == "-num_tests") {
            num_tests = (long unsigned)atol((*++i).c_str());
            printf("num_tests as long unsigned = %lu\n", num_tests);
        } else if (*i == "-max_ulp_mismatch_in_l") {
            allowed_ulp_mismatch = (unsigned)atol((*++i).c_str());
        } else if (*i == "-prec") {
            print_precision = (unsigned)atol((*++i).c_str());
        } else if (*i == "-ratio_threshold") {
            ratio_threshold = (double)atol((*++i).c_str());
        } else if (*i == "-mat_type") {
            mat_type = (unsigned)atol((*++i).c_str());
        } else if (*i == "-skip_mat_type") {
            skip_mat_type = (unsigned)atol((*++i).c_str());
        } else if (*i == "-debug") {
            debug = (unsigned)atol((*++i).c_str());
        } else {
            printf("Unknown command line option %s.  Try again\n", i->c_str());
            exit(1);
        }
    }

    // Matrix arrays
    MATRIX_IN_T A[DIM][DIM];           // The input array.  Cast from A_generated
    MATRIX_OUT_T L[DIM][DIM];          // The L result from the DUT
    MATRIX_OUT_T L_expected[DIM][DIM]; // The L result from LAPACK in target format

    L_TYPE L_cast[DIM][DIM];
    L_TYPE L_expected_cast[DIM][DIM];
    L_TYPE A_restored[DIM][DIM];        // A recreated from L*Lt
    L_TYPE A_restored_lapack[DIM][DIM]; // A recreated from LAPACK's L*Lt

    L_TYPE A_cast[DIM][DIM];                  // Cast A back to LAPACK compatible type for analysis
    MATRIX_OUT_T L_delta_vs_lapack[DIM][DIM]; // The difference between the DUT's L and LAPACK's L

    L_TYPE A_delta[DIM][DIM];        // The delta values will be passed to xLANSY so need to use LAPACK compatible types
    L_TYPE A_delta_lapack[DIM][DIM]; //

    // Non-complex type
    L_BASE_TYPE A_norm;
    L_BASE_TYPE A_delta_norm;
    L_BASE_TYPE A_delta_lapack_norm;
    L_BASE_TYPE DUT_ratio, LAPACK_ratio;

    float imat_max_ratio_diff[NUM_MAT_TYPES];
    float imat_min_ratio_diff[NUM_MAT_TYPES];
    float imat_max_ratio[NUM_MAT_TYPES];
    float imat_min_ratio[NUM_MAT_TYPES];

    unsigned int ratio_same = 0;
    unsigned int ratio_better = 0;
    unsigned int ratio_worse = 0;
    unsigned int imat_ratio_same[NUM_MAT_TYPES];
    unsigned int imat_ratio_better[NUM_MAT_TYPES];
    unsigned int imat_ratio_worse[NUM_MAT_TYPES];
    // Zero values
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        imat_max_ratio_diff[i] = 0;
        imat_min_ratio_diff[i] = 0;
        imat_ratio_same[i] = 0;
        imat_ratio_better[i] = 0;
        imat_ratio_worse[i] = 0;
        imat_max_ratio[i] = 0;
        imat_min_ratio[i] = ratio_threshold; // 0; Work backwards from worse case?
    }

    double ratio_difference = 0;
    unsigned int pass_fail = 0; // Pass=0 Fail =1

    bool matched_lapack;

    printf("Running %lu %s tests per matrix type on %d x %d matrices with LowerTriangular set to %d\n", num_tests,
           x_is_float(L_expected[0][0])
               ? "single precision"
               : x_is_double(L_expected[0][0]) ? "double precision"
                                               : x_is_fixed(L_expected[0][0]) ? "fixed point" : "Unknown type",
           DIM, DIM, LOWER_TRIANGULAR);

    char lapack_U_L = 'L';
    if (LOWER_TRIANGULAR == false) lapack_U_L = 'U';

    // Generate results table header
    std::cout << "RESULTS_TABLE,Test,IMAT,L Matching,DUT Ratio,LAPACK Ratio,Relative Ratio Difference" << std::endl;

    for (unsigned int imat = 0; imat < NUM_MAT_TYPES; imat++) {
        if ((mat_type == 0 || imat + 1 == mat_type) && skip_mat_type - 1 != imat) {
            for (long unsigned i = 0; i < num_tests; i++) {
                if (imat == 7) {
                    // this matrix type is used to test abnormal situation, the element of matrix is very small
                    // HLS do not support abnormal situation.
                    continue;
                }

                std::string data_path = std::string(DATA_PATH);
                std::string base_path;

                if (x_is_complex(A[0][0]) == true) {
                    if (x_is_fixed(A[0][0]) == true) {
                        base_path = data_path.append("/complex_fixed/");
                    } else {
                        base_path = data_path.append("/complex_float/");
                    }
                } else if (x_is_fixed(A[0][0]) == true) {
                    base_path = data_path.append("/fixed/");
                } else {
                    base_path = data_path.append("/float/");
                }
                std::string file_A =
                    base_path + "A_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                std::string file_L =
                    base_path + "L_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";

                int A_size = DIM * DIM;
                int L_size = DIM * DIM;

                MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);
                MATRIX_OUT_T* L_ptr = reinterpret_cast<MATRIX_OUT_T*>(L_expected);

                readTxt(file_A, A_ptr, A_size);
                readTxt(file_L, L_ptr, L_size);

                for (int r = 0; r < DIM; r++) {
                    for (int c = 0; c < DIM; c++) {
                        A_cast[r][c] = A[r][c];
                    }
                }

                hls::stream<MATRIX_IN_T> matrixAStrm;
                hls::stream<MATRIX_OUT_T> matrixLStrm;
                for (int r = 0; r < DIM; r++) {
                    for (int c = 0; c < DIM; c++) {
                        matrixAStrm.write(A[r][c]);
                    }
                }

                // Get Actual results
                // ====================================================================
                kernel_cholesky_0(matrixAStrm, matrixLStrm);

                for (int r = 0; r < DIM; r++) {
                    for (int c = 0; c < DIM; c++) {
                        L[r][c] = matrixLStrm.read();
                    }
                }

                // Test results
                // ====================================================================
                // Populate arrays used for test criteria calc.
                for (int r = 0; r < DIM; r++) {
                    for (int c = 0; c < DIM; c++) {
                        L_expected_cast[r][c] = L_expected[r][c];
                        L_cast[r][c] = L[r][c];
                    }
                }

                // Basic check cell by cell check based on allowed_ulp_mismatch value.
                matched_lapack =
                    are_matrices_equal<DIM, DIM, MATRIX_OUT_T>((MATRIX_OUT_T*)L, (MATRIX_OUT_T*)L_expected,
                                                               allowed_ulp_mismatch, (MATRIX_OUT_T*)L_delta_vs_lapack);

                // Test Criteria
                // - Uses a norm based test against a threshold:
                //   norm( L*L' - A ) / ( N * norm(A) * EPS )
                // L*L'
                mmult<!LOWER_TRIANGULAR, LOWER_TRIANGULAR, DIM, DIM, DIM, DIM, DIM, DIM>(L_cast, L_cast, A_restored);
                mmult<!LOWER_TRIANGULAR, LOWER_TRIANGULAR, DIM, DIM, DIM, DIM, DIM, DIM>(
                    L_expected_cast, L_expected_cast, A_restored_lapack);
                // L*L' - A
                msub<DIM, DIM, L_TYPE, L_TYPE>(A_restored, A_cast, A_delta);
                msub<DIM, DIM, L_TYPE, L_TYPE>(A_restored_lapack, A_cast, A_delta_lapack);

                A_norm = norm1_dbl<DIM, DIM, L_TYPE, L_BASE_TYPE>(A_cast);
                A_delta_norm = norm1_dbl<DIM, DIM, L_TYPE, L_BASE_TYPE>(A_delta);
                A_delta_lapack_norm = norm1_dbl<DIM, DIM, L_TYPE, L_BASE_TYPE>(A_delta_lapack);

                DUT_ratio = A_delta_norm / (DIM * A_norm * eps);
                LAPACK_ratio = A_delta_lapack_norm / (DIM * A_norm * eps);

                // Check ratio value for invalid values
                int ratio_check = check_ratio(DUT_ratio);
                if (ratio_check) {
                    std::cout << "ERROR: Check ratio failed." << std::endl;
                    pass_fail = 1;
                }

                // Compare DUT vs LAPACK ratios
                ratio_difference = (DUT_ratio - LAPACK_ratio) * 100.0;

                if (ratio_difference > imat_max_ratio_diff[imat]) {
                    imat_max_ratio_diff[imat] = ratio_difference;
                }
                if (ratio_difference < imat_min_ratio_diff[imat]) {
                    imat_min_ratio_diff[imat] = ratio_difference;
                }
                // NOTE: Not tolerance just now.
                if (ratio_difference < 0) {
                    imat_ratio_better[imat]++;
                } else if (ratio_difference > 0) {
                    imat_ratio_worse[imat]++;
                } else {
                    imat_ratio_same[imat]++;
                }
                // Log max and min ratio values, use DUT value
                if (DUT_ratio > imat_max_ratio[imat]) {
                    imat_max_ratio[imat] = DUT_ratio;
                }
                if (DUT_ratio < imat_min_ratio[imat]) {
                    imat_min_ratio[imat] = DUT_ratio;
                }

                std::cout << "RESULTS_TABLE," << i << "," << imat + 1 << "," << matched_lapack << "," << DUT_ratio
                          << "," << LAPACK_ratio << "," << ratio_difference << std::endl;

                // Determine if pass or fail.
                // o Check DUT ratio against test threshold, default taken from LAPACK
                if (DUT_ratio > ratio_threshold) {
                    std::cout << "ERROR: DUT_ratio(" << DUT_ratio << ") > ratio_threshold(" << ratio_threshold
                              << "). LAPACK ratio = " << LAPACK_ratio << std::endl;
                    pass_fail = 1; // Test run fails
                }

                // Print matrix's for debug when worse than LAPACK or cholesky returns non-zero
                if ((ratio_difference > 0 && debug > 0) || debug > 1 || DUT_ratio > ratio_threshold) {
                    printf("  A=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A_cast, "   ", print_precision,
                                                                                        1);
                    printf("  L=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(L_cast, "   ", print_precision,
                                                                                        1);
                    printf("  L_expected=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(L_expected_cast, "   ",
                                                                                        print_precision, 1);
                    printf("  A_restored=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A_restored, "   ",
                                                                                        print_precision, 1);
                    printf("  A_restored_lapack=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A_restored_lapack, "   ",
                                                                                        print_precision, 1);
                    printf("  A_delta=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A_delta, "   ", print_precision,
                                                                                        1);
                    printf("  A_delta_lapack=\n");
                    xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A_delta_lapack, "   ",
                                                                                        print_precision, 1);
                }

            } // End of test loop
        }     // Type test
    }         // Matrix type loop

    // Summary table, catagorize by input matrix (imat) type
    std::cout << "SUMMARY_TABLE,imat,Min Ratio, Max Ratio,Min Diff (Smaller),Max Diff (Larger),Same,Better,Worse"
              << std::endl;
    double max_ratio_diff = 0;
    double min_ratio_diff = 0;
    double max_ratio = 0;
    double min_ratio = ratio_threshold; // as per the imat array starting point
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        std::cout << "SUMMARY_TABLE," << i + 1 << "," << imat_min_ratio[i] << "," << imat_max_ratio[i] << ","
                  << imat_min_ratio_diff[i] << "," << imat_max_ratio_diff[i] << "," << imat_ratio_same[i] << ","
                  << imat_ratio_better[i] << "," << imat_ratio_worse[i] << std::endl;
        // Calc totals
        if (imat_max_ratio_diff[i] > max_ratio_diff) {
            max_ratio_diff = imat_max_ratio_diff[i];
        }
        if (imat_min_ratio_diff[i] < min_ratio_diff) {
            min_ratio_diff = imat_min_ratio_diff[i];
        }
        if (imat_max_ratio[i] > max_ratio) {
            max_ratio = imat_max_ratio[i];
        }
        if (imat_min_ratio[i] < min_ratio) {
            min_ratio = imat_min_ratio[i];
        }
        ratio_better += imat_ratio_better[i];
        ratio_worse += imat_ratio_worse[i];
        ratio_same += imat_ratio_same[i];
    }
    std::cout << "SUMMARY_TABLE,all," << min_ratio << "," << max_ratio << "," << min_ratio_diff << "," << max_ratio_diff
              << "," << ratio_same << "," << ratio_better << "," << ratio_worse << std::endl;

    if (pass_fail) {
        std::cout << "TB:Fail" << std::endl;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }
    std::cout << "" << std::endl;
    return (pass_fail);
}
