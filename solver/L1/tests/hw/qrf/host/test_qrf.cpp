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
// Common testbench for QR Factorisation function.
//
// Golden data is generated from reference Model LAPACKe
//
// ======================================================================

#include "test_qrf.hpp"
#include "kernel_qrf.hpp"

#include "src/utils.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>     // for abs
#include <algorithm> //for find

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}

// Compute time difference
unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

// Arguments parser
class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

// ---------------------------------------------------------------------------------------------
// Main test program
// ---------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Variables set by command line
    // ====================================================================
    long unsigned num_tests = 1;
    // long unsigned num_tests            = (A_ROWS >= 16 ? 4 : 20);    // Small default for HLS
    unsigned allowed_ulp_mismatch = 0;
    unsigned int debug = 0;
    float R_ratio_threshold = 30.0; // Defined in lapack-X.Y.Z/TESTING/[s|d|c|x]test.in
    float Q_ratio_threshold = 30.0; // Defined in lapack-X.Y.Z/TESTING/[s|d|c|x]test.in
    float mat_type = 0;             // Specify to only run a single matrix type.
    unsigned int skip_mat_type = 7;
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
            // This loses the MSB so 18446744073709551615 gets converted into 9223372036854775807.
            // It's probably not a big issue though.
            num_tests = (long unsigned)atol((*++i).c_str());
            printf("num_tests as long unsigned = %lu\n", num_tests);
        } else if (*i == "-max_ulp_mismatch_in_l") {
            allowed_ulp_mismatch = (unsigned)atol((*++i).c_str());
        } else if (*i == "-prec") {
            print_precision = (unsigned)atol((*++i).c_str());
        } else if (*i == "-q_ratio_threshold") {
            Q_ratio_threshold = (float)atol((*++i).c_str());
        } else if (*i == "-r_ratio_threshold") {
            R_ratio_threshold = (float)atol((*++i).c_str());
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
    MATRIX_IN_T A[A_ROWS][A_COLS] = {0}; // The input array.  Cast from A_generated
                                         // Note that this needs to be maximally-sized to support the largest
                                         // Q, otherwise we get strange LAPACK results when ROWS > COLS

    MATRIX_OUT_T Q[A_ROWS][A_ROWS] = {0};          // The Q result from the DUT
    MATRIX_OUT_T Q_expected[A_ROWS][A_ROWS] = {0}; // The Q result from LAPACK in target format

    MATRIX_OUT_T R[A_ROWS][A_COLS] = {0};          // The R result from the DUT
    MATRIX_OUT_T R_expected[A_ROWS][A_COLS] = {0}; // The R result from LAPACK in target format

    MATRIX_OUT_T I[A_ROWS][A_ROWS] = {0}; // The identity matrix to compare against

    // Test variables
    QR_TYPE A_cast[A_ROWS][A_COLS] = {0};                 // Cast A back to LAPACK compatible type for analysis
    MATRIX_OUT_T Q_delta_vs_lapack[A_ROWS][A_ROWS] = {0}; // The difference between the DUT's Q and LAPACK's Q
    MATRIX_OUT_T R_delta_vs_lapack[A_ROWS][A_COLS] = {0}; // The difference between the DUT's R and LAPACK's R
    MATRIX_IN_T R_restored[A_ROWS][A_COLS] = {0};         // R recreated from Q'*A
    MATRIX_IN_T R_restored_lapack[A_ROWS][A_COLS] = {0};  // R recreated from LAPACK's Q'*A
    MATRIX_IN_T I_restored[A_ROWS][A_ROWS] = {0};         // I recreated from Q'*Q
    MATRIX_IN_T I_restored_lapack[A_ROWS][A_ROWS] = {0};  // I recreated from LAPACK's Q'*Q

    QR_TYPE R_delta[A_ROWS][A_COLS] = {
        0}; // The delta values will be passed to xLANSY so need to use LAPACK compatible types
    QR_TYPE R_delta_lapack[A_ROWS][A_COLS] = {0}; //
    QR_TYPE I_delta[A_ROWS][A_ROWS] = {
        0}; // The delta values will be passed to xLANSY so need to use LAPACK compatible types
    QR_TYPE I_delta_lapack[A_ROWS][A_ROWS] = {0}; //

    // Non-complex type
    double A_norm = 0;
    QR_BASE_TYPE R_delta_norm = 0;
    QR_BASE_TYPE R_delta_lapack_norm = 0;
    QR_BASE_TYPE R_DUT_ratio = 0;
    QR_BASE_TYPE R_LAPACK_ratio = 0;
    QR_BASE_TYPE I_delta_norm = 0;
    QR_BASE_TYPE I_delta_lapack_norm = 0;
    QR_BASE_TYPE I_DUT_ratio = 0;
    QR_BASE_TYPE I_LAPACK_ratio = 0;

    float R_imat_max_ratio_diff[NUM_MAT_TYPES] = {0};
    float R_imat_min_ratio_diff[NUM_MAT_TYPES] = {0};
    float R_imat_max_ratio[NUM_MAT_TYPES] = {0};
    float R_imat_min_ratio[NUM_MAT_TYPES] = {0};
    float I_imat_max_ratio_diff[NUM_MAT_TYPES] = {0};
    float I_imat_min_ratio_diff[NUM_MAT_TYPES] = {0};
    float I_imat_max_ratio[NUM_MAT_TYPES] = {0};
    float I_imat_min_ratio[NUM_MAT_TYPES] = {0};

    unsigned int R_ratio_same = 0;
    unsigned int R_ratio_better = 0;
    unsigned int R_ratio_worse = 0;
    unsigned int R_imat_ratio_same[NUM_MAT_TYPES] = {0};
    unsigned int R_imat_ratio_better[NUM_MAT_TYPES] = {0};
    unsigned int R_imat_ratio_worse[NUM_MAT_TYPES] = {0};
    unsigned int I_ratio_same = 0;
    unsigned int I_ratio_better = 0;
    unsigned int I_ratio_worse = 0;
    unsigned int I_imat_ratio_same[NUM_MAT_TYPES] = {0};
    unsigned int I_imat_ratio_better[NUM_MAT_TYPES] = {0};
    unsigned int I_imat_ratio_worse[NUM_MAT_TYPES] = {0};

    // Zero values
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        R_imat_max_ratio_diff[i] = 0;
        R_imat_min_ratio_diff[i] = 0;
        R_imat_ratio_same[i] = 0;
        R_imat_ratio_better[i] = 0;
        R_imat_ratio_worse[i] = 0;
        R_imat_max_ratio[i] = 0;
        R_imat_min_ratio[i] = R_ratio_threshold;
        I_imat_max_ratio_diff[i] = 0;
        I_imat_min_ratio_diff[i] = 0;
        I_imat_ratio_same[i] = 0;
        I_imat_ratio_better[i] = 0;
        I_imat_ratio_worse[i] = 0;
        I_imat_max_ratio[i] = 0;
        I_imat_min_ratio[i] = Q_ratio_threshold;
    }

    float R_ratio_difference = 0;
    float I_ratio_difference = 0;
    unsigned int R_pass_fail = 0; // Pass=0 Fail =1
    unsigned int I_pass_fail = 0; // Pass=0 Fail =1
    unsigned int pass_fail = 0;   // Pass=0 Fail =1

    bool matched_lapack_Q = false;
    bool matched_lapack_R = false;

    printf("Running %lu %s tests per matrix type on %d x %d matrices with TransposedQ %d\n", num_tests,
           x_is_float(R_expected[0][0])
               ? "single precision"
               : x_is_double(R_expected[0][0]) ? "double precision"
                                               : x_is_fixed(R_expected[0][0]) ? "fixed point" : "Unknown type",
           A_ROWS, A_COLS, TRANSPOSED_Q);

    std::cout << " MIN: " << hls::numeric_limits<MATRIX_IN_BASE_T>::min() << std::endl;
    std::cout << " MAX: " << hls::numeric_limits<MATRIX_IN_BASE_T>::max() << std::endl;

    // Generate results table header
    std::cout << "RESULTS_TABLE,Test,IMAT,kl,ku,cond,anorm,R Matching,DUT Ratio,LAPACK Ratio,Relative Ratio "
                 "Difference,Q Matching,DUT Ratio,LAPACK Ratio,Relative Ratio Difference"
              << std::endl;

    // Create I to compare against later
    for (int r = 0; r < A_ROWS; r++) {
        for (int c = 0; c < A_ROWS; c++) {
            if (r == c) {
                I[r][c] = 1.0;
            } else {
                I[r][c] = 0.0;
            }
        }
    }

    //---------------- New code post-test review ----------
    std::cout << "Post-Test: NUM_MAT_TYPES is " << NUM_MAT_TYPES << ", num_tests is " << num_tests << "\n";
    for (unsigned int imat = 0; imat < NUM_MAT_TYPES; imat++) {
        // Test which matrix type to run
        if ((mat_type == 0 || imat + 1 == mat_type) && skip_mat_type != imat + 1) {
            for (long unsigned i = 0; i < num_tests; i++) {
                if (imat == 6) {
                    // this matrix type is used to test abnormal situation, the element of matrix is very small
                    // HLS do not support abnormal situation.
                    continue;
                }

                if (imat == 8 && i > 0) {
                    // Test only one zero matrix
                    break;
                }

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
                std::string file_Q =
                    base_path + "Q_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";
                std::string file_R =
                    base_path + "R_matType_" + std::to_string(imat + 1) + "_" + std::to_string(i) + ".txt";

                int A_size = A_ROWS * A_COLS;
                int Q_size = A_ROWS * A_ROWS;
                int R_size = A_ROWS * A_COLS;

                MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);
                MATRIX_OUT_T* Q_ptr = reinterpret_cast<MATRIX_OUT_T*>(Q_expected);
                MATRIX_OUT_T* R_ptr = reinterpret_cast<MATRIX_OUT_T*>(R_expected);

                readTxt(file_A, A_ptr, A_size);
                readTxt(file_Q, Q_ptr, Q_size);
                readTxt(file_R, R_ptr, R_size);

                for (int r = 0; r < A_ROWS; r++) {
                    for (int c = 0; c < A_COLS; c++) {
                        // Cast back to pick up quantization. Used in test criteria
                        A_cast[r][c] = A[r][c];
                    }
                }

                hls::stream<MATRIX_IN_T> matrixAStrm;
                hls::stream<MATRIX_IN_T> matrixQStrm;
                hls::stream<MATRIX_IN_T> matrixRStrm;
                for (int r = 0; r < A_ROWS; r++) {
                    for (int c = 0; c < A_COLS; c++) {
                        matrixAStrm.write(A[r][c]);
                    }
                }

                // Get Actual results
                //
                // Q is output in (conjugate) transposed form, specified in generic_wrapper.cpp,
                // which means Q*A = R
                // ====================================================================
                kernel_qrf_0(matrixAStrm, matrixQStrm, matrixRStrm);

                // Force zeros in lower triangle of our R, as we did that for LAPACK as well
                for (int r = 0; r < A_ROWS; r++) {
                    for (int c = 0; c < A_ROWS; c++) {
                        Q[r][c] = matrixQStrm.read();
                    }
                }

                for (int r = 0; r < A_ROWS; r++) {
                    for (int c = 0; c < A_COLS; c++) {
                        R[r][c] = matrixRStrm.read();
                        if (r > c) {
                            R[r][c] = 0;
                        }
                    }
                }

                // Check for NaNs
                if (anyNaN<A_ROWS, A_COLS>(R) == 1) {
                    printf("ERROR: Caught NaN in R\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R, "   ",
                                                                                                    print_precision, 1);
                    printf("TB:Fail\n");
                    return (32);
                }
                if (anyNaN<A_ROWS, A_ROWS>(Q) == 1) {
                    printf("ERROR: Caught NaN in Q\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(Q, "   ",
                                                                                                    print_precision, 1);
                    printf("TB:Fail\n");
                    return (33);
                }
                // Validation checks (i.e. are the matrices correctly formed)
                // ====================================================================

                // Check 1: Complex matrices should have a real-valued diagonal (for QR inverse, and also to match
                // Matlab/LAPACK results)
                if (x_is_complex(R_expected[0][0]) == true) {
                    // printf("INFO: Checking for real-valued diagonal entries\n");
                    for (int r = 0; r < A_ROWS; r++) {
                        for (int c = 0; c < A_COLS; c++) {
                            if (r == c) {
                                if (hls::x_imag(R[r][c]) != 0) {
                                    printf("ERROR: Complex-valued diagonal element detected in R\n");
                                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(
                                        R, "   ", print_precision, 1);
                                    printf("TB:Fail\n");
                                    return (22);
                                }
                            }
                        }
                    }
                }
                /*
                // Check 2: Verify that the lower-triangular portion of R is zeroed
                // Note this is disabled for now because I haven't decided whether to force zero values when applying
                the Givens
                // rotations or not.  If I don't force zeros, then there's the chance that catastrophic cancellation can
                occur and
                // some lower-triangular values can be non-zero (although we would document for users that these
                elements are not
                // explicitly zeroed).
                for(int r = 0; r < A_ROWS; r++){
                  for(int c = 0; c < A_COLS; c++){
                    if (r > c) {
                      if (R[r][c] != 0) {
                        printf("ERROR: Lower-triangular element R[%d][%d] is not zero\n",r,c);
                        xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R, "   ",
                print_precision,1);
                        printf("TB:Fail\n");
                        return(23);
                      }
                    }
                  }
                }
                */

                // Test results
                // ====================================================================

                // Basic check cell by cell check based on allowed_ulp_mismatch value.
                matched_lapack_Q = are_matrices_equal<A_ROWS, A_ROWS, MATRIX_OUT_T>(
                    (MATRIX_OUT_T*)Q, (MATRIX_OUT_T*)Q_expected, allowed_ulp_mismatch,
                    (MATRIX_OUT_T*)Q_delta_vs_lapack);
                matched_lapack_R = are_matrices_equal<A_ROWS, A_COLS, MATRIX_OUT_T>(
                    (MATRIX_OUT_T*)R, (MATRIX_OUT_T*)R_expected, allowed_ulp_mismatch,
                    (MATRIX_OUT_T*)R_delta_vs_lapack);

                // Test Criteria
                // - Uses a norm based test against a threshold for Q and R:
                //   norm(R - Q'*A)/(rows * norm(A) * EPS)
                //   norm(I - Q'*Q)/(rows * EPS)
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
                mmult<!TRANSPOSED_Q, false, A_ROWS, A_ROWS, A_ROWS, A_COLS, A_ROWS, A_COLS>(Q, A_cast, R_restored);
                mmult<true, false, A_ROWS, A_ROWS, A_ROWS, A_COLS, A_ROWS, A_COLS>(
                    Q_expected, A_cast, R_restored_lapack); // LAPACK produces Q such that Q*R=A, so we (conjugate)
                                                            // transpose Q to compute Q*A = R
                msub<A_ROWS, A_COLS, MATRIX_IN_T, QR_TYPE>(R, R_restored, R_delta);
                msub<A_ROWS, A_COLS, MATRIX_IN_T, QR_TYPE>(R_expected, R_restored_lapack, R_delta_lapack);

                // norm1 as used in SPOT01
                // This norm needs to be computed in double precision to avoid overflow, as the elements can be quite
                // large for the 1/Min norm case (imat=8)
                A_norm = norm1_dbl<A_ROWS, A_COLS, QR_TYPE, QR_BASE_TYPE>(A_cast);

                if (isinf(A_norm)) {
                    // Should never be Inf - if it is, we've probably overflowed
                    printf("ERROR: Caught unexpected Inf for A_norm\n");
                    printf("TB:Fail\n");
                    return (3);
                }

                // norm1 as used in SPOT01
                R_delta_norm = norm1<A_ROWS, A_COLS, QR_TYPE, QR_BASE_TYPE>(R_delta);

                // norm1 as used in SPOT01
                R_delta_lapack_norm = norm1<A_ROWS, A_COLS, QR_TYPE, QR_BASE_TYPE>(R_delta_lapack);

                //----------------------------------------------------------------------------------------------------------------------------------------
                // printf("A_norm %.15f R_delta_norm %.15f R_delta_lapack_norm %.15f\n",A_norm , R_delta_norm ,
                // R_delta_lapack_norm );

                // printf("A \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(A, "   ",
                // print_precision,1);

                // printf("A_cast \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(A_cast, "   ",
                // print_precision,1);

                // printf("Q \n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(Q, "   ",
                // print_precision,1);
                // printf("Q_expected\n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(Q_expected, "   ",
                // print_precision,1);

                // printf("Q' \n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::ConjugateTranspose>(Q, "   ",
                // print_precision,1);
                // printf("Q'_expected\n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::ConjugateTranspose>(Q_expected, "
                // ",
                // print_precision,1);

                // printf("R \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R, "   ",
                // print_precision,1);
                // printf("R_expected\n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R_expected, "   ",
                // print_precision,1);

                // printf("R_delta \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R_delta, "   ",
                // print_precision,1);
                // printf("R_delta_lapack\n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R_delta_lapack, " ",
                // print_precision,1);

                // printf("R_restored \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R_restored, "   ",
                // print_precision,1);
                // printf("R_restored_lapack \n");
                // xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_OUT_T, xf::solver::NoTranspose>(R_restored_lapack, "
                // ",
                // print_precision,1);
                //----------------------------------------------------------------------------------------------------------------------------------------

                R_DUT_ratio = (double)R_delta_norm / ((double)A_ROWS * (double)A_norm * (double)eps);
                std::cout << R_DUT_ratio << " " << R_delta_norm << " " << A_ROWS << " " << A_norm << " " << eps
                          << std::endl;
                R_LAPACK_ratio = (double)R_delta_lapack_norm / ((double)A_ROWS * (double)A_norm * (double)eps);

                // Check that the norm values are OK and we are not comparing two bad ratios
                //
                if (isnan(R_DUT_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for R_DUT_ratio\n");
                        printf("TB:Fail\n");
                        return (4);
                    }
                }

                if (isnan(R_LAPACK_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for R_LAPACK_ratio\n");
                        printf("TB:Fail\n");
                        return (5);
                    }
                }

                if (isinf(R_DUT_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for R_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (6);
                }

                if (isinf(R_LAPACK_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for R_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (7);
                }

                if (R_DUT_ratio == 0) {
                    if (!(imat == 0 || imat == 1)) {
                        // Neither diagonal nor upper-triangular, so there should be error in the reconstruction, but we
                        // didn't detect that, so fail
                        printf("ERROR: Caught unexpected Zero for R_DUT_ratio\n");
                        printf("TB:Fail\n");
                        return (8);
                    }
                }

                if (R_LAPACK_ratio == 0) {
                    if (!(imat == 0 || imat == 1)) {
                        // Neither diagonal nor upper-triangular, so there should be error in the reconstruction, but we
                        // didn't detect that, so fail
                        printf("ERROR: Caught unexpected Zero for R_LAPACK_ratio\n");
                        printf("TB:Fail\n");
                        return (9);
                    }
                }

                if (R_DUT_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative R_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (10);
                }

                if (R_LAPACK_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative R_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (11);
                }

                // mmult performs conjugate transpose via x_conj
                mmult<true, false, A_ROWS, A_ROWS, A_ROWS, A_ROWS, A_ROWS, A_ROWS>(Q, Q, I_restored);
                mmult<true, false, A_ROWS, A_ROWS, A_ROWS, A_ROWS, A_ROWS, A_ROWS>(Q_expected, Q_expected,
                                                                                   I_restored_lapack);
                msub<A_ROWS, A_ROWS, MATRIX_IN_T, QR_TYPE>(I, I_restored, I_delta);
                msub<A_ROWS, A_ROWS, MATRIX_IN_T, QR_TYPE>(I, I_restored_lapack, I_delta_lapack);

                I_delta_norm = norm1<A_ROWS, A_ROWS, QR_TYPE, QR_BASE_TYPE>(I_delta);
                I_delta_lapack_norm = norm1<A_ROWS, A_ROWS, QR_TYPE, QR_BASE_TYPE>(I_delta_lapack);

                ////----------------------------------------------------------------------------------------------------------------------------------------
                // printf("I_delta \n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(I_delta, "   ",
                // print_precision,1);
                // printf("I_delta_lapack\n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(I_delta_lapack, " ",
                // print_precision,1);

                // printf("I_restored \n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(I_restored, "   ",
                // print_precision,1);
                // printf("I_restored_lapack \n");
                // xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_OUT_T, xf::solver::NoTranspose>(I_restored_lapack, "
                // ",
                // print_precision,1);

                // printf("A_norm %f R_delta_norm %f R_delta_lapack_norm %f I_delta_norm %f I_delta_lapack_norm
                // %f\n",A_norm, R_delta_norm , R_delta_lapack_norm , I_delta_norm , I_delta_lapack_norm);
                ////----------------------------------------------------------------------------------------------------------------------------------------

                I_DUT_ratio = (double)I_delta_norm / ((double)A_ROWS * (double)eps);
                I_LAPACK_ratio = (double)I_delta_lapack_norm / ((double)A_ROWS * (double)eps);

                // Check that the norm values are OK and we are not comparing two bad ratios
                //
                if (isnan(I_DUT_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for I_DUT_ratio\n");
                        printf("TB:Fail\n");
                        return (12);
                    }
                }

                if (isnan(I_LAPACK_ratio)) {
                    // Should only be NaN if A_norm was zero, so check that
                    if (A_norm != 0) {
                        printf("ERROR: Caught unexpected NaN for I_LAPACK_ratio\n");
                        printf("TB:Fail\n");
                        return (13);
                    }
                }

                if (isinf(I_DUT_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for I_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (14);
                }

                if (isinf(I_LAPACK_ratio)) {
                    // Should never be Inf
                    printf("ERROR: Caught unexpected Inf for I_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (15);
                }

                if (I_DUT_ratio == 0) {
                    if (!(imat == 0 || imat == 1 || imat == 8)) {
                        // Neither diagonal nor upper-triangular nor zero matrix, so there should be error in the
                        // reconstruction, but we didn't detect that, so fail
                        printf("ERROR: Caught unexpected Zero for I_DUT_ratio\n");
                        printf("TB:Fail\n");
                        return (16);
                    }
                }

                // The LAPACK result might reconstruct exactly
                // if (I_LAPACK_ratio == 0) {
                //  if (!(imat == 0 || imat == 1)) {
                //    // Neither diagonal nor upper-triangular, so there should be error in the reconstruction, but we
                //    didn't detect that, so fail
                //    printf("ERROR: Caught unexpected Zero for I_LAPACK_ratio\n");
                //    printf("TB:Fail\n");
                //    return(17);
                //  }
                //}

                if (I_DUT_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative I_DUT_ratio\n");
                    printf("TB:Fail\n");
                    return (18);
                }

                if (I_LAPACK_ratio < 0) {
                    // Should never be less than zero - if it is, it's either an error code or something went badly
                    // wrong
                    printf("ERROR: Caught unexpected negative I_LAPACK_ratio\n");
                    printf("TB:Fail\n");
                    return (19);
                }

                // Compare DUT vs LAPACK ratios
                R_ratio_difference = (R_DUT_ratio - R_LAPACK_ratio) * 100.0;

                if (R_ratio_difference > R_imat_max_ratio_diff[imat]) {
                    R_imat_max_ratio_diff[imat] = R_ratio_difference;
                }
                if (R_ratio_difference < R_imat_min_ratio_diff[imat]) {
                    R_imat_min_ratio_diff[imat] = R_ratio_difference;
                }

                // NOTE: Not tolerance just now.
                if (R_ratio_difference < 0) {
                    R_imat_ratio_better[imat]++;
                } else if (R_ratio_difference > 0) {
                    R_imat_ratio_worse[imat]++;
                } else {
                    R_imat_ratio_same[imat]++;
                }
                // Log max and min ratio values, use DUT value
                if (R_DUT_ratio > R_imat_max_ratio[imat]) {
                    R_imat_max_ratio[imat] = R_DUT_ratio;
                }
                if (R_DUT_ratio < R_imat_min_ratio[imat]) {
                    R_imat_min_ratio[imat] = R_DUT_ratio;
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

                // Determine if pass or fail.
                // o Check DUT ratio against test threshold, default taken from LAPACK
                if (R_DUT_ratio > R_ratio_threshold) {
                    std::cout << "ERROR: R_DUT_ratio(" << R_DUT_ratio << ") > R_ratio_threshold(" << R_ratio_threshold
                              << "). R_LAPACK ratio = " << R_LAPACK_ratio << std::endl;
                    R_pass_fail = 1; // Test run fails
                }
                if (I_DUT_ratio > Q_ratio_threshold) {
                    std::cout << "ERROR: I_DUT_ratio(" << I_DUT_ratio << ") > Q_ratio_threshold(" << Q_ratio_threshold
                              << "). I LAPACK ratio = " << I_LAPACK_ratio << std::endl;
                    I_pass_fail = 1; // Test run fails
                }

                // Print matrices for debug when worse than LAPACK
                if ((R_ratio_difference > 0 && debug > 0) || debug > 1 || R_DUT_ratio > R_ratio_threshold) {
                    printf("  A=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(A, "   ",
                                                                                                   print_precision, 1);
                    printf("  Q=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_IN_T, xf::solver::NoTranspose>(Q, "   ",
                                                                                                   print_precision, 1);
                    printf("  Q_expeced=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_IN_T, xf::solver::NoTranspose>(Q_expected, "   ",
                                                                                                   print_precision, 1);

                    printf("  R=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(R, "   ",
                                                                                                   print_precision, 1);
                    printf("  R_expeced=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(R_expected, "   ",
                                                                                                   print_precision, 1);
                    printf("  R_restored=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(R_restored, "   ",
                                                                                                   print_precision, 1);
                    printf("  R_restored_lapack=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(
                        R_restored_lapack, "   ", print_precision, 1);
                    printf("  R_delta=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, QR_TYPE, xf::solver::NoTranspose>(R_delta, "   ",
                                                                                               print_precision, 1);
                    printf("  R_delta_lapack=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, QR_TYPE, xf::solver::NoTranspose>(R_delta_lapack, "   ",
                                                                                               print_precision, 1);
                }
                if ((I_ratio_difference > 0 && debug > 0) || debug > 1 || I_DUT_ratio > Q_ratio_threshold) {
                    printf("  A=\n");
                    xf::solver::print_matrix<A_ROWS, A_COLS, MATRIX_IN_T, xf::solver::NoTranspose>(A, "   ",
                                                                                                   print_precision, 1);
                    printf("  I_restored=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_IN_T, xf::solver::NoTranspose>(I_restored, "   ",
                                                                                                   print_precision, 1);
                    printf("  I_restored_lapack=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, MATRIX_IN_T, xf::solver::NoTranspose>(
                        I_restored_lapack, "   ", print_precision, 1);
                    printf("  I_delta=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, QR_TYPE, xf::solver::NoTranspose>(I_delta, "   ",
                                                                                               print_precision, 1);
                    printf("  I_delta_lapack=\n");
                    xf::solver::print_matrix<A_ROWS, A_ROWS, QR_TYPE, xf::solver::NoTranspose>(I_delta_lapack, "   ",
                                                                                               print_precision, 1);
                }

            } // End of test loop
              // printf("\n");
        }     // Type test
    }         // Matrix type loop

    // Summary table, catagorize by input matrix (imat) type
    std::cout << "" << std::endl;
    std::cout << "SUMMARY_TABLE R,imat,Min Ratio,Max Ratio,Min Diff (Smaller),Max Diff (Larger),Same,Better,Worse"
              << std::endl;
    float R_max_ratio_diff = 0;
    float R_min_ratio_diff = 0;
    float R_max_ratio = 0;
    float R_min_ratio = R_ratio_threshold; // as per the imat array starting point
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        std::cout << "SUMMARY_TABLE R," << i + 1 << "," << R_imat_min_ratio[i] << "," << R_imat_max_ratio[i] << ","
                  << R_imat_min_ratio_diff[i] << "," << R_imat_max_ratio_diff[i] << "," << R_imat_ratio_same[i] << ","
                  << R_imat_ratio_better[i] << "," << R_imat_ratio_worse[i] << std::endl;
        // Calc totals
        if (R_imat_max_ratio_diff[i] > R_max_ratio_diff) {
            R_max_ratio_diff = R_imat_max_ratio_diff[i];
        }
        if (R_imat_min_ratio_diff[i] < R_min_ratio_diff) {
            R_min_ratio_diff = R_imat_min_ratio_diff[i];
        }
        if (R_imat_max_ratio[i] > R_max_ratio) {
            R_max_ratio = R_imat_max_ratio[i];
        }
        if (R_imat_min_ratio[i] < R_min_ratio) {
            R_min_ratio = R_imat_min_ratio[i];
        }
        R_ratio_better += R_imat_ratio_better[i];
        R_ratio_worse += R_imat_ratio_worse[i];
        R_ratio_same += R_imat_ratio_same[i];
    }
    std::cout << "SUMMARY_TABLE R,all," << R_min_ratio << "," << R_max_ratio << "," << R_min_ratio_diff << ","
              << R_max_ratio_diff << "," << R_ratio_same << "," << R_ratio_better << "," << R_ratio_worse << std::endl;

    std::cout << "" << std::endl;

    std::cout << "SUMMARY_TABLE Q,imat,Min Ratio,Max Ratio,Min Diff (Smaller),Max Diff (Larger),Same,Better,Worse"
              << std::endl;
    float I_max_ratio_diff = 0;
    float I_min_ratio_diff = 0;
    float I_max_ratio = 0;
    float I_min_ratio = Q_ratio_threshold; // as per the imat array starting point
    for (int i = 0; i < NUM_MAT_TYPES; i++) {
        std::cout << "SUMMARY_TABLE Q," << i + 1 << "," << I_imat_min_ratio[i] << "," << I_imat_max_ratio[i] << ","
                  << I_imat_min_ratio_diff[i] << "," << I_imat_max_ratio_diff[i] << "," << I_imat_ratio_same[i] << ","
                  << I_imat_ratio_better[i] << "," << I_imat_ratio_worse[i] << std::endl;
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
    std::cout << "SUMMARY_TABLE Q,all," << I_min_ratio << "," << I_max_ratio << "," << I_min_ratio_diff << ","
              << I_max_ratio_diff << "," << I_ratio_same << "," << I_ratio_better << "," << I_ratio_worse << std::endl;
    if (R_pass_fail == 1 || I_pass_fail == 1) {
        std::cout << "TB:Fail" << std::endl;
        pass_fail = 1;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }
    std::cout << "" << std::endl;
    return (pass_fail);
}
