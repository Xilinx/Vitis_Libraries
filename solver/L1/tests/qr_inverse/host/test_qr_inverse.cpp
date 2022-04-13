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
// Common testbench for QR Inverse function.
//
// Golden data is generated from reference Model LAPACKe
//
// ======================================================================

#include "test_qr_inverse.hpp"
#include "kernel_qr_inverse.hpp"

#include "src/utils.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <cmath> // for abs

// ---------------------------------------------------------------------------------------------
// Main test program
// ---------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Variables set by command line
    // ====================================================================
    long unsigned num_tests = 1;
    // long unsigned num_tests = (ROWSCOLSA >= 16 ? 5 : 20); // Small default for HLS
    unsigned allowed_ulp_mismatch = 0;
    unsigned int debug = 0;
    double ratio_threshold = 30.0;
    double mat_type = 0; // Specify to only run a single matrix type.
    unsigned int skip_mat_type[2] = {0};
    int skip_mat_type_itr = 0;
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
            skip_mat_type[skip_mat_type_itr] = (unsigned)atol((*++i).c_str());
            skip_mat_type_itr++;
        } else if (*i == "-debug") {
            debug = (unsigned)atol((*++i).c_str());
        } else {
            printf("Unknown command line option %s.  Try again\n", i->c_str());
            exit(1);
        }
    }

    if (skip_mat_type[0] != 0) {
        std::cout << "INFO: Skipping matrix type " << skip_mat_type[0] << std::endl;
    }
    if (skip_mat_type[1] != 0) {
        std::cout << "INFO: Skipping matrix type " << skip_mat_type[1] << std::endl;
    }

    int qr_inverse_return = 0; // Return code from hls::qr_inverse
    bool testing_singular_matrix = false;

    // Matrix arrays
    MATRIX_IN_T A[ROWSCOLSA][ROWSCOLSA]; // The input array.  Cast from A_generated

    MATRIX_OUT_T InverseA[ROWSCOLSA][ROWSCOLSA];          // The inverse result from the DUT
    MATRIX_OUT_T InverseA_expected[ROWSCOLSA][ROWSCOLSA]; // The inverse result from LAPACK in target format

    MATRIX_OUT_T I[ROWSCOLSA][ROWSCOLSA]; // The identity matrix to compare against

    // Test variables
    QR_INV_TYPE A_cast[ROWSCOLSA][ROWSCOLSA]; // Cast A back to LAPACK compatible type for analysis
    MATRIX_OUT_T InverseA_delta_vs_lapack[ROWSCOLSA]
                                         [ROWSCOLSA]; // The difference between the DUT's InverseA and LAPACK's InverseA
    MATRIX_IN_T I_restored[ROWSCOLSA][ROWSCOLSA];     // I recreated from InverseA*A
    MATRIX_IN_T I_restored_lapack[ROWSCOLSA][ROWSCOLSA]; // I recreated from LAPACK's InverseA*A

    QR_INV_TYPE I_delta[ROWSCOLSA]
                       [ROWSCOLSA]; // The delta values will be passed to xLANSY so need to use LAPACK compatible types
    QR_INV_TYPE I_delta_lapack[ROWSCOLSA][ROWSCOLSA]; //

    // Non-complex type
    double A_norm;
    double InverseA_norm;
    QR_INV_BASE_TYPE I_delta_norm;
    QR_INV_BASE_TYPE I_delta_lapack_norm;
    QR_INV_BASE_TYPE I_DUT_ratio, I_LAPACK_ratio;

    double I_imat_max_ratio_diff[NUM_MAT_TYPES];
    double I_imat_min_ratio_diff[NUM_MAT_TYPES];
    double I_imat_max_ratio[NUM_MAT_TYPES];
    double I_imat_min_ratio[NUM_MAT_TYPES];

    unsigned int I_ratio_same = 0;
    unsigned int I_ratio_better = 0;
    unsigned int I_ratio_worse = 0;
    unsigned int I_imat_ratio_same[NUM_MAT_TYPES];
    unsigned int I_imat_ratio_better[NUM_MAT_TYPES];
    unsigned int I_imat_ratio_worse[NUM_MAT_TYPES];

    // Zero values
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        I_imat_max_ratio_diff[i] = 0;
        I_imat_min_ratio_diff[i] = 0;
        I_imat_ratio_same[i] = 0;
        I_imat_ratio_better[i] = 0;
        I_imat_ratio_worse[i] = 0;
        I_imat_max_ratio[i] = 0;
        I_imat_min_ratio[i] = ratio_threshold;
    }

    double I_ratio_difference = 0;
    unsigned int pass_fail = 0; // Pass=0 Fail =1

    bool matched_lapack_InverseA;

    printf("Running %lu %s tests per matrix type on %d x %d matrices\n", num_tests,
           x_is_float(InverseA_expected[0][0])
               ? "single precision"
               : x_is_double(InverseA_expected[0][0])
                     ? "double precision"
                     : x_is_fixed(InverseA_expected[0][0]) ? "fixed point" : "Unknown type",
           ROWSCOLSA, ROWSCOLSA);

    // Generate results table header
    std::cout << "RESULTS_TABLE,Test,IMAT,InverseA Matching,DUT Ratio,LAPACK Ratio,Relative Ratio Difference"
              << std::endl;

    // Create I to compare against later
    for (int r = 0; r < ROWSCOLSA; r++) {
        for (int c = 0; c < ROWSCOLSA; c++) {
            if (r == c) {
                I[r][c] = 1.0;
            } else {
                I[r][c] = 0.0;
            }
        }
    }

    //---------------- New code post-test review ----------
    for (unsigned int imat = 0; imat < NUM_MAT_TYPES; imat++) {
        // Test which matrix type to run
        if ((mat_type == 0 || imat + 1 == mat_type) && (skip_mat_type[0] - 1 != imat && skip_mat_type[1] - 1 != imat)) {
            for (long unsigned i = 0; i < num_tests; i++) {
                if (imat >= 4) {
                    testing_singular_matrix = true;
                } else {
                    testing_singular_matrix = false;
                }
                if ((imat == 4 || imat == 5 || imat == 6) && i > 0) {
                    // Test only one singular matrix
                    break;
                }

                // Get reference results
                // ====================================================================
                // Read input matrix and golden matrix from files
                std::string data_path = std::string(DATA_PATH);
                std::string base_path;

                if (x_is_complex(A[0][0]) == true) {
                    base_path = data_path.append("/complex/");
                } else {
                    base_path = data_path.append("/float/");
                }
                std::string file_A =
                    base_path + "A_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                std::string file_InverseA =
                    base_path + "InverseA_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";

                int A_size = ROWSCOLSA * ROWSCOLSA;
                int InverseA_size = ROWSCOLSA * ROWSCOLSA;

                MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);
                MATRIX_OUT_T* InverseA_ptr = reinterpret_cast<MATRIX_OUT_T*>(InverseA_expected);

                readTxt(file_A, A_ptr, A_size);
                readTxt(file_InverseA, InverseA_ptr, InverseA_size);

                for (int r = 0; r < ROWSCOLSA; r++) {
                    for (int c = 0; c < ROWSCOLSA; c++) {
                        A_cast[r][c] = A[r][c];
                    }
                }

                hls::stream<MATRIX_IN_T> matrixAStrm;
                hls::stream<MATRIX_OUT_T> matrixInverseAStrm;
                for (int r = 0; r < ROWSCOLSA; r++) {
                    for (int c = 0; c < ROWSCOLSA; c++) {
                        matrixAStrm.write(A[r][c]);
                    }
                }

                // Get Actual results
                // ====================================================================
                qr_inverse_return = kernel_qr_inverse_0(matrixAStrm, matrixInverseAStrm);

                for (int r = 0; r < ROWSCOLSA; r++) {
                    for (int c = 0; c < ROWSCOLSA; c++) {
                        matrixInverseAStrm.read(InverseA[r][c]);
                    }
                }

                if (qr_inverse_return != 0 && !testing_singular_matrix) {
                    printf("ERROR: Input matrix was not singular, but QR Inverse thinks it is!\n");
                    printf("TB:Fail\n");
                    return (1);
                } else if (qr_inverse_return != 0 && testing_singular_matrix) {
                    printf("INFO: Singular matrix was correctly detected\n");
                }

                // Check for NaNs in result
                if (anyNaN<ROWSCOLSA, ROWSCOLSA>(InverseA) == 1 && !testing_singular_matrix) {
                    printf("ERROR: Caught NaN in InverseA\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(
                        InverseA, "   ", print_precision, 1);
                    printf("TB:Fail\n");
                    return (32);
                }

                // Test results
                // ====================================================================

                // Basic check cell by cell check based on allowed_ulp_mismatch value.
                matched_lapack_InverseA = are_matrices_equal<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T>(
                    (MATRIX_OUT_T*)InverseA, (MATRIX_OUT_T*)InverseA_expected, allowed_ulp_mismatch,
                    (MATRIX_OUT_T*)InverseA_delta_vs_lapack);

                // Test Criteria
                // - Uses a norm based test against a threshold for the reconstructed identity matrix:
                //   norm(I - InverseA*A)/(rows * norm(A) * norm(InverseA) * EPS)
                // - Our intention is to compare the LAPACK result ratio against our function ratio and compare those
                // figures.
                // - Can remove the divisor as both ratios will be scaled in the same way. Removes uncertainity about
                // what EPS should be
                //   - SO just compare norms of delta?
                //   - Might be usefull to consider the relative values when trying to determine a threshold for a
                //   "fail"
                // - Will use the LAPACK norm function.....yet another horrible function call.....
                // TODO/WARNING: Using implementation types. When fixed complex will have bit growth issues....should we
                // cast everything to double...

                mmult<false, false, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA>(InverseA, A_cast,
                                                                                                      I_restored);
                mmult<false, false, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA, ROWSCOLSA>(
                    InverseA_expected, A_cast, I_restored_lapack);
                msub<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, QR_INV_TYPE>(I, I_restored, I_delta);
                msub<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, QR_INV_TYPE>(I, I_restored_lapack, I_delta_lapack);

                // REVISIT: is A_cast the appropriate format to use here?
                // norm1 as used in SPOT01
                A_norm = norm1_dbl<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, QR_INV_BASE_TYPE>(A_cast);

                if (isinf(A_norm)) {
                    // Should never be Inf - if it is, we've probably overflowed
                    printf("ERROR: Caught unexpected Inf for A_norm\n");
                    printf("TB:Fail\n");
                    return (4);
                }

                // REVISIT: which version of InverseA should we use here?  Probably LAPACK's, since it's more likely to
                // be correct - hopefully!
                // norm1 as used in SPOT01
                InverseA_norm = norm1_dbl<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, QR_INV_BASE_TYPE>(InverseA_expected);

                if (isinf(InverseA_norm)) {
                    // Should never be Inf - if it is, we've probably overflowed
                    printf("ERROR: Caught unexpected Inf for InverseA_norm\n");
                    printf("TB:Fail\n");
                    return (5);
                }

                // norm1 as used in SPOT01
                I_delta_norm = norm1<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, QR_INV_BASE_TYPE>(I_delta);

                // norm1 as used in SPOT01
                I_delta_lapack_norm = norm1<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, QR_INV_BASE_TYPE>(I_delta_lapack);

                /*
                //----------------------------------------------------------------------------------------------------------------------------------------
                printf("A \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, xf::solver::NoTranspose>(A, "   ",
                print_precision, 1);

                printf("InverseA \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(InverseA, "   ",
                print_precision, 1);
                printf("InverseA_expected \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(InverseA_expected,
                "   ", print_precision, 1);

                printf("I_delta \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(I_delta, "   ",
                print_precision, 1);
                printf("I_delta_lapack\n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(I_delta_lapack, "
                ", print_precision,1);

                printf("I_restored \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(I_restored, "   ",
                print_precision,1);
                printf("I_restored_lapack \n");
                xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(I_restored_lapack,
                "   ", print_precision,1);

                printf("A_norm %.60f I_delta_norm %.60f I_delta_lapack_norm %.60f InverseA_norm %.60f\n",A_norm,
                I_delta_norm , I_delta_lapack_norm, InverseA_norm);

                //----------------------------------------------------------------------------------------------------------------------------------------
                */

                // Cast to double for the operations here - there is not enough precision with float, and we get silent
                // underflow.
                I_DUT_ratio =
                    (double)I_delta_norm / ((double)ROWSCOLSA * (double)A_norm * (double)InverseA_norm * (double)eps);
                I_LAPACK_ratio = (double)I_delta_lapack_norm /
                                 ((double)ROWSCOLSA * (double)A_norm * (double)InverseA_norm * (double)eps);

                // Check that the norm values are OK and we are not comparing two bad ratios
                //
                if (isnan(I_DUT_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for I_DUT_ratio\n");
                        printf("TB:Fail\n");
                        return (6);
                    }
                }

                if (isnan(I_LAPACK_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for I_LAPACK_ratio\n");
                        printf("TB:Fail\n");
                        return (7);
                    }
                }

                if (isinf(I_DUT_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for I_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (8);
                }

                if (isinf(I_LAPACK_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for I_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (9);
                }

                if (I_DUT_ratio == 0) {
                    if (!(imat == 0 || imat == 1 || testing_singular_matrix)) {
                        // Neither diagonal nor upper-triangular, so there should be error in the reconstruction, but we
                        // didn't detect that, so fail
                        printf("ERROR: Caught unexpected Zero for I_DUT_ratio\n");
                        std::cout << "RESULTS_TABLE," << i << "," << imat + 1 << "," << matched_lapack_InverseA << ","
                                  << I_DUT_ratio << "," << I_LAPACK_ratio << "," << I_ratio_difference << std::endl;
                        printf("TB:Fail\n");
                        return (10);
                    }
                }

                /*
                // LAPACK's result can sometimes reconstruct exactly (to machine precision) so don't run this test.
                // Generally this only occurs for small (~3x3) matrices
                if (I_LAPACK_ratio == 0) {
                if (!(imat == 0 || imat == 1)) {
                // Neither diagonal nor upper-triangular, so there should be error in the reconstruction, but we didn't
                detect that, so fail
                printf("ERROR: Caught unexpected Zero for I_LAPACK_ratio\n");
                std::cout << "RESULTS_TABLE," << i << "," << imat+1 << "," << latms_kl << "," << latms_ku << "," <<
                latms_cond << "," << latms_anorm << "," << matched_lapack_InverseA << "," << I_DUT_ratio << "," <<
                I_LAPACK_ratio << "," << I_ratio_difference << std::endl;
                printf("TB:Fail\n");
                return(11);
                }
                }
                */

                if (I_DUT_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative I_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (12);
                }

                if (I_LAPACK_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative I_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (13);
                }

                I_ratio_difference = (I_DUT_ratio - I_LAPACK_ratio) * 100.0;
                if (I_ratio_difference > I_imat_max_ratio_diff[imat]) {
                    I_imat_max_ratio_diff[imat] = I_ratio_difference;
                }
                if (I_ratio_difference < I_imat_min_ratio_diff[imat]) {
                    I_imat_min_ratio_diff[imat] = I_ratio_difference;
                }
                // NOTE: Not tolerance just now.
                if (I_ratio_difference < 0) {
                    I_imat_ratio_better[imat]++;
                } else if (I_ratio_difference > 0) {
                    I_imat_ratio_worse[imat]++;
                } else {
                    I_imat_ratio_same[imat]++;
                }
                // Log max and min ratio values, use DUT value
                if (I_DUT_ratio > I_imat_max_ratio[imat]) {
                    I_imat_max_ratio[imat] = I_DUT_ratio;
                }
                if (I_DUT_ratio < I_imat_min_ratio[imat]) {
                    I_imat_min_ratio[imat] = I_DUT_ratio;
                }

                std::cout << "RESULTS_TABLE," << i << "," << imat + 1 << "," << matched_lapack_InverseA << ","
                          << I_DUT_ratio << "," << I_LAPACK_ratio << "," << I_ratio_difference << std::endl;

                // Determine if pass or fail.
                // o Check DUT ratio against test threshold, default taken from LAPACK
                if (I_DUT_ratio > ratio_threshold && !testing_singular_matrix) {
                    std::cout << "ERROR: I_DUT_ratio(" << I_DUT_ratio << ") > ratio_threshold(" << ratio_threshold
                              << "). I LAPACK ratio = " << I_LAPACK_ratio << std::endl;
                    pass_fail = 1; // Test run fails
                }

                // Print matrices for debug when worse than LAPACK
                if ((I_ratio_difference > 0 && debug > 0) || debug > 1 || (!testing_singular_matrix) ||
                    I_DUT_ratio > ratio_threshold) {
                    printf("testing_singular_matrix = %s \n", testing_singular_matrix ? "true" : "false");
                    printf("  A=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, xf::solver::NoTranspose>(
                        A, "   ", print_precision, 1);
                    printf("  InverseA=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(
                        InverseA, "   ", print_precision, 1);
                    printf("  InverseA_expected=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_OUT_T, xf::solver::NoTranspose>(
                        InverseA_expected, "   ", print_precision, 1);
                    printf("  I_restored=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, xf::solver::NoTranspose>(
                        I_restored, "   ", print_precision, 1);
                    printf("  I_restored_lapack=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, MATRIX_IN_T, xf::solver::NoTranspose>(
                        I_restored_lapack, "   ", print_precision, 1);
                    printf("  I_delta=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, xf::solver::NoTranspose>(
                        I_delta, "   ", print_precision, 1);
                    printf("  I_delta_lapack=\n");
                    xf::solver::print_matrix<ROWSCOLSA, ROWSCOLSA, QR_INV_TYPE, xf::solver::NoTranspose>(
                        I_delta_lapack, "   ", print_precision, 1);
                }

            } // End of test loop
            printf("\n");
        } // Type test
    }     // Matrix type loop

    // Summary table, catagorize by input matrix (imat) type
    std::cout << "" << std::endl;
    std::cout << "SUMMARY_TABLE,imat,Min Ratio,Max Ratio,Min Diff (Smaller),Max Diff (Larger),Same,Better,Worse"
              << std::endl;
    double I_max_ratio_diff = 0;
    double I_min_ratio_diff = 0;
    double I_max_ratio = 0;
    double I_min_ratio = ratio_threshold; // as per the imat array starting point
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        if ((mat_type == 0 || i + 1 == mat_type) && (skip_mat_type[0] - 1 != i && skip_mat_type[1] - 1 != i)) {
            std::cout << "SUMMARY_TABLE," << i + 1 << "," << I_imat_min_ratio[i] << "," << I_imat_max_ratio[i] << ","
                      << I_imat_min_ratio_diff[i] << "," << I_imat_max_ratio_diff[i] << "," << I_imat_ratio_same[i]
                      << "," << I_imat_ratio_better[i] << "," << I_imat_ratio_worse[i] << std::endl;
            // Calc totals
            if (I_imat_max_ratio_diff[i] > I_max_ratio_diff) {
                I_max_ratio_diff = I_imat_max_ratio_diff[i];
            }
            if (I_imat_min_ratio_diff[i] < I_min_ratio_diff) {
                I_min_ratio_diff = I_imat_min_ratio_diff[i];
            }
            if (I_imat_max_ratio[i] > I_max_ratio) {
                I_max_ratio = I_imat_max_ratio[i];
            }
            if (I_imat_min_ratio[i] < I_min_ratio) {
                I_min_ratio = I_imat_min_ratio[i];
            }
            I_ratio_better += I_imat_ratio_better[i];
            I_ratio_worse += I_imat_ratio_worse[i];
            I_ratio_same += I_imat_ratio_same[i];
        }
    }
    std::cout << "SUMMARY_TABLE,all," << I_min_ratio << "," << I_max_ratio << "," << I_min_ratio_diff << ","
              << I_max_ratio_diff << "," << I_ratio_same << "," << I_ratio_better << "," << I_ratio_worse << std::endl;

    if (pass_fail == 1) {
        std::cout << "TB:Fail" << std::endl;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }
    std::cout << "" << std::endl;
    return (pass_fail);
}
