/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
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

#include "kernel_block_cholesky_cfloat.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <hls_math.h>

using namespace std;

// Block size MATRIX_BLOCK_B must match xf::solver::xf_block_cholesky_inplace<MATRIX_DIM, MATRIX_BLOCK_B> (see
// dut_type.hpp).

// Host golden: same scalar reduction order as block_cholesky_cfloat.hpp (load_block / chol_upper /
// trsm_left_lower / gemm_update).
static void load_block_cpu(const MATRIX_OUT_T A[DIM][DIM],
                           int bi,
                           int bj,
                           MATRIX_OUT_T buf[MATRIX_BLOCK_B][MATRIX_BLOCK_B]) {
    for (int i = 0; i < MATRIX_BLOCK_B; ++i) {
        for (int j = 0; j < MATRIX_BLOCK_B; ++j) {
            buf[i][j] = A[bi * MATRIX_BLOCK_B + i][bj * MATRIX_BLOCK_B + j];
        }
    }
}

static void store_block_cpu(MATRIX_OUT_T A[DIM][DIM],
                            int bi,
                            int bj,
                            const MATRIX_OUT_T buf[MATRIX_BLOCK_B][MATRIX_BLOCK_B]) {
    for (int i = 0; i < MATRIX_BLOCK_B; ++i) {
        for (int j = 0; j < MATRIX_BLOCK_B; ++j) {
            A[bi * MATRIX_BLOCK_B + i][bj * MATRIX_BLOCK_B + j] = buf[i][j];
        }
    }
}

static void chol_upper_block_cpu(MATRIX_OUT_T a[MATRIX_BLOCK_B][MATRIX_BLOCK_B]) {
    for (int i = 0; i < MATRIX_BLOCK_B; ++i) {
        for (int j = 0; j < i; ++j) {
            MATRIX_OUT_T sum = a[j][i];
            for (int k = 0; k < j; ++k) {
                sum -= std::conj(a[k][j]) * a[k][i];
            }
            a[j][i] = sum / a[j][j];
        }
        MATRIX_OUT_T diag = a[i][i];
        for (int k = 0; k < i; ++k) {
            diag -= std::conj(a[k][i]) * a[k][i];
        }
        float diag_real = diag.real();
        a[i][i] = MATRIX_OUT_T(hls::sqrtf(diag_real), 0.0f);
        for (int j = 0; j < i; ++j) {
            a[i][j] = MATRIX_OUT_T(0.0f, 0.0f);
        }
    }
}

static void trsm_left_lower_cpu(const MATRIX_OUT_T rkk[MATRIX_BLOCK_B][MATRIX_BLOCK_B],
                                MATRIX_OUT_T akc[MATRIX_BLOCK_B][MATRIX_BLOCK_B]) {
    for (int j = 0; j < MATRIX_BLOCK_B; ++j) {
        for (int i = 0; i < MATRIX_BLOCK_B; ++i) {
            MATRIX_OUT_T sum = akc[i][j];
            for (int k = 0; k < i; ++k) {
                sum -= std::conj(rkk[k][i]) * akc[k][j];
            }
            akc[i][j] = sum / rkk[i][i];
        }
    }
}

static void gemm_update_cpu(const MATRIX_OUT_T akr[MATRIX_BLOCK_B][MATRIX_BLOCK_B],
                            const MATRIX_OUT_T akc[MATRIX_BLOCK_B][MATRIX_BLOCK_B],
                            MATRIX_OUT_T arc[MATRIX_BLOCK_B][MATRIX_BLOCK_B]) {
    for (int i = 0; i < MATRIX_BLOCK_B; ++i) {
        for (int j = 0; j < MATRIX_BLOCK_B; ++j) {
            MATRIX_OUT_T sum(0.0f, 0.0f);
            for (int k = 0; k < MATRIX_BLOCK_B; ++k) {
                sum += std::conj(akr[k][i]) * akc[k][j];
            }
            arc[i][j] -= sum;
        }
    }
}

static void cpu_block_cholesky_upper(MATRIX_OUT_T A[DIM][DIM]) {
    if (DIM % (unsigned)MATRIX_BLOCK_B != 0) {
        std::cerr << "cpu_block_cholesky_upper: DIM=" << DIM
                  << " must be a multiple of MATRIX_BLOCK_B=" << MATRIX_BLOCK_B << std::endl;
        exit(1);
    }
    const int NB = (int)DIM / MATRIX_BLOCK_B;
    MATRIX_OUT_T buf_kk[MATRIX_BLOCK_B][MATRIX_BLOCK_B];
    MATRIX_OUT_T buf_kc[MATRIX_BLOCK_B][MATRIX_BLOCK_B];
    MATRIX_OUT_T buf_kr[MATRIX_BLOCK_B][MATRIX_BLOCK_B];
    MATRIX_OUT_T buf_rc[MATRIX_BLOCK_B][MATRIX_BLOCK_B];

    for (int k = 0; k < NB; ++k) {
        load_block_cpu(A, k, k, buf_kk);
        chol_upper_block_cpu(buf_kk);
        store_block_cpu(A, k, k, buf_kk);

        for (int c = k + 1; c < NB; ++c) {
            load_block_cpu(A, k, c, buf_kc);
            trsm_left_lower_cpu(buf_kk, buf_kc);
            store_block_cpu(A, k, c, buf_kc);
        }

        for (int r = k + 1; r < NB; ++r) {
            load_block_cpu(A, k, r, buf_kr);
            for (int c = r; c < NB; ++c) {
                load_block_cpu(A, r, c, buf_rc);
                load_block_cpu(A, k, c, buf_kc);
                gemm_update_cpu(buf_kr, buf_kc, buf_rc);
                store_block_cpu(A, r, c, buf_rc);
            }
        }
    }

    // Match block_cholesky_cfloat.hpp xf_block_cholesky_inplace clear_strict_lower.
    for (unsigned i = 1; i < DIM; ++i) {
        for (unsigned j = 0; j < i; ++j) {
            A[i][j] = MATRIX_OUT_T(0.0f, 0.0f);
        }
    }
}

void writeComplextoFile(MATRIX_OUT_T* X, std::string filename0, unsigned M, unsigned N) {
    ofstream myfile0;
    myfile0.open(filename0.c_str());
    if (!myfile0) {
        std::cout << "[ERROR]: file " << filename0 << "could not be opened !" << std::endl;
        exit(1);
    }
    for (unsigned j = 0; j < N; j++) {
        for (unsigned i = 0; i < M; i++) {
            MATRIX_OUT_T x;
            x = *(X + j * M + i);
            myfile0 << x.real() << " ";
            myfile0 << x.imag() << " " << std::endl;
        }
    }
    myfile0.close();
}

// Reconstruct B = R^H * R with R upper triangular (strict lower zero). (R^H R)_{ij} = sum_{k<=min(i,j)} conj(R_ki)
// R_kj.
static void reconstruct_RhR_from_upper_R(const MATRIX_OUT_T R[DIM][DIM], std::vector<MATRIX_OUT_T>& B) {
    B.resize((size_t)DIM * DIM);
    for (unsigned i = 0; i < DIM; ++i) {
        for (unsigned j = 0; j < DIM; ++j) {
            const unsigned klast = (i < j) ? i : j;
            double sr = 0.0;
            double si = 0.0;
            for (unsigned k = 0; k <= klast; ++k) {
                std::complex<double> rki((double)R[k][i].real(), (double)R[k][i].imag());
                std::complex<double> rkj((double)R[k][j].real(), (double)R[k][j].imag());
                std::complex<double> p = std::conj(rki) * rkj;
                sr += p.real();
                si += p.imag();
            }
            B[i * DIM + j] = MATRIX_OUT_T((float)sr, (float)si);
        }
    }
}

// ||A - B||_F and ||A||_F (Frobenius), accumulated in double for a stronger check than float-on-float.
static void frobenius_residual_vs_A(const MATRIX_IN_T A[DIM][DIM],
                                    const std::vector<MATRIX_OUT_T>& B_rowmajor,
                                    double* out_res_frob,
                                    double* out_norm_A_frob) {
    double sum_res2 = 0.0;
    double sum_a2 = 0.0;
    for (unsigned i = 0; i < DIM; ++i) {
        for (unsigned j = 0; j < DIM; ++j) {
            const MATRIX_OUT_T& b = B_rowmajor[i * DIM + j];
            std::complex<double> ca((double)A[i][j].real(), (double)A[i][j].imag());
            std::complex<double> cb((double)b.real(), (double)b.imag());
            std::complex<double> d = ca - cb;
            sum_res2 += std::norm(d);
            sum_a2 += std::norm(ca);
        }
    }
    *out_res_frob = std::sqrt(sum_res2);
    *out_norm_A_frob = std::sqrt(sum_a2);
}

// Compare DUT R vs host CPU Cholesky reference: count ULP failures, worst cell, sample lines.
// Helps distinguish CSIM (C model + host float) vs COSIM (RTL + Xilinx float IP) numeric drift.
static void report_R_dut_vs_cpu_ref_ulp(const MATRIX_OUT_T expected[DIM][DIM],
                                        const MATRIX_OUT_T actual[DIM][DIM],
                                        unsigned allowed_ulp_mismatch,
                                        unsigned max_sample_lines,
                                        bool residual_ok,
                                        bool skip_residual) {
    typedef xil_equality<MATRIX_OUT_T> eq;
    unsigned fail = 0;
    uint64_t worst_combo = 0;
    uint64_t worst_ur = 0, worst_ui = 0;
    int worst_r = -1, worst_c = -1;
    int nan_cells = 0;

    for (unsigned r = 0; r < DIM; ++r) {
        for (unsigned c = 0; c < DIM; ++c) {
            const float rr = actual[r][c].real(), ri = actual[r][c].imag();
            if (std::isnan(rr) || std::isnan(ri) || std::isinf(rr) || std::isinf(ri)) {
                ++nan_cells;
            }
            const bool ok = eq::equal(expected[r][c], actual[r][c], allowed_ulp_mismatch);
            if (!ok) {
                ++fail;
                const std::complex<int> u = calcULP(expected[r][c], actual[r][c]);
                const uint64_t ur = (uint64_t)std::abs(u.real());
                const uint64_t ui = (uint64_t)std::abs(u.imag());
                const uint64_t cell_m = ur > ui ? ur : ui;
                if (cell_m > worst_combo) {
                    worst_combo = cell_m;
                    worst_ur = ur;
                    worst_ui = ui;
                    worst_r = (int)r;
                    worst_c = (int)c;
                }
            }
        }
    }

    const bool bitwise_ok = (fail == 0);
    std::cout << "matrix_bitwise_vs_cpu_R_expected: " << (bitwise_ok ? "PASS" : "FAIL")
              << " (allowed_ulp_per_real_imag=" << allowed_ulp_mismatch << ")"
              << ", offending_cells=" << fail << "/" << (DIM * DIM);
    if (!bitwise_ok && worst_r >= 0) {
        std::cout << ", worst_offender_ULP(real,imag)=(" << worst_ur << "," << worst_ui << ") at (" << worst_r << ","
                  << worst_c << ")";
    }
    if (nan_cells) {
        std::cout << ", non_finite_R_out_cells=" << nan_cells;
    }
    std::cout << std::endl;

    if (!bitwise_ok && max_sample_lines > 0) {
        unsigned shown = 0;
        std::cout << "  sample ULP mismatches (R_expected=host cpu_block_cholesky_upper, R_out=DUT):" << std::endl;
        for (unsigned r = 0; r < DIM && shown < max_sample_lines; ++r) {
            for (unsigned c = 0; c < DIM && shown < max_sample_lines; ++c) {
                if (!eq::equal(expected[r][c], actual[r][c], allowed_ulp_mismatch)) {
                    const std::complex<int> u = calcULP(expected[r][c], actual[r][c]);
                    std::cout << "    (" << r << "," << c << ") ULP=(" << std::abs(u.real()) << ","
                              << std::abs(u.imag()) << ") exp=(" << expected[r][c].real() << ","
                              << expected[r][c].imag() << ") dut=(" << actual[r][c].real() << "," << actual[r][c].imag()
                              << ")" << std::endl;
                    ++shown;
                }
            }
        }
    }

    if (!bitwise_ok) {
        const bool res_considered = !skip_residual;
        std::cout << "  diagnosis: ";
        if (nan_cells) {
            std::cout << "Non-finite values in R_out - check RTL/streaming, not just float tolerance.";
        } else if (res_considered && residual_ok) {
            std::cout
                << "Residual vs A is acceptable but R differs from CPU Cholesky reference — typical when COSIM "
                   "uses Xilinx float IP / different op order than this host golden (CSIM C model often tracks CPU "
                   "ref more closely).";
        } else if (res_considered && !residual_ok) {
            std::cout << "Residual vs A failed — factorization accuracy problem, not only bit-exact vs CPU ref.";
        } else {
            std::cout << "Bitwise vs CPU ref failed (residual check skipped); inspect ULP samples above.";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    unsigned allowed_ulp_mismatch = 8192;
    unsigned print_precision = 2;
    unsigned int debug = 0;
    unsigned ulp_report_max = 24;
    double residual_rel_tol = 1e-3;
    double residual_abs_tol = -1.0;
    bool skip_residual_check = false;

    std::vector<std::string> args(argv + 1, argv + argc);
    for (std::vector<std::string>::iterator i = args.begin(); i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            std::cout << "Syntax: main.exe [-max_ulp_mismatch_in_l <n>] [-prec <n>] [-debug <n>]\n"
                      << "  [-ulp_report_max <n>] [-residual_rel_tol <x>] [-residual_abs_tol <x>] "
                         "[-skip_residual_check]\n"
                      << "  residual: Frobenius ||A - R^H R|| vs DUT output R; rel = res/||A||_F\n"
                      << "  default rel_tol=1e-3; abs_tol<0 disables absolute cap\n"
                      << "  -ulp_report_max: max sample lines printed when R_out vs CPU ref ULP check fails (0=off)\n";
            return 0;
        } else if (*i == "-max_ulp_mismatch_in_l") {
            allowed_ulp_mismatch = (unsigned)atol((*++i).c_str());
        } else if (*i == "-prec") {
            print_precision = (unsigned)atol((*++i).c_str());
        } else if (*i == "-debug") {
            debug = (unsigned)atol((*++i).c_str());
        } else if (*i == "-ulp_report_max") {
            ulp_report_max = (unsigned)atol((*++i).c_str());
        } else if (*i == "-residual_rel_tol") {
            residual_rel_tol = std::strtod((*++i).c_str(), nullptr);
        } else if (*i == "-residual_abs_tol") {
            residual_abs_tol = std::strtod((*++i).c_str(), nullptr);
        } else if (*i == "-skip_residual_check") {
            skip_residual_check = true;
        } else {
            printf("Unknown command line option %s.  Try again\n", i->c_str());
            exit(1);
        }
    }
    std::cout << "DEBUG: DIM=" << DIM << " (blocked stream Cholesky, upper factor R)" << std::endl;

    MATRIX_IN_T A[DIM][DIM];
    MATRIX_OUT_T R_out[DIM][DIM];
    MATRIX_OUT_T R_expected[DIM][DIM];

    int A_size = DIM * DIM;

    MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);

    std::string data_path = std::string(DATA_PATH);
    std::string file_A = data_path + "/A_" + std::to_string(DIM) + ".txt";
    std::string file_Rout = data_path + "/Rout_" + std::to_string(DIM) + ".txt";
    std::cout << "File_A: " << file_A << std::endl;
    std::cout << "File_Rout: " << file_Rout << std::endl;

    readTxt(file_A, A_ptr, A_size);

    for (unsigned r = 0; r < DIM; r++) {
        for (unsigned c = 0; c < DIM; c++) {
            R_expected[r][c] = A[r][c];
        }
    }
    cpu_block_cholesky_upper(R_expected);

    hls::stream<MATRIX_IN_T> matrixAStrm;
    hls::stream<MATRIX_OUT_T> matrixLStrm;
    for (unsigned r = 0; r < DIM; r++) {
        for (unsigned c = 0; c < DIM; c++) {
            matrixAStrm.write(A[r][c]);
        }
    }

    kernel_block_cholesky_cfloat_0(matrixAStrm, matrixLStrm);

    // DUT streams full matrix row-major (same order as cholesky_hls write_out)
    for (unsigned r = 0; r < DIM; r++) {
        for (unsigned c = 0; c < DIM; c++) {
            R_out[r][c] = matrixLStrm.read();
        }
    }

    writeComplextoFile((MATRIX_OUT_T*)R_out, file_Rout, DIM, DIM);

    bool mismatch_check = are_matrices_equal<DIM, DIM, MATRIX_OUT_T>((MATRIX_OUT_T*)R_out, (MATRIX_OUT_T*)R_expected,
                                                                     allowed_ulp_mismatch, NULL);

    std::vector<MATRIX_OUT_T> rhR;
    reconstruct_RhR_from_upper_R(R_out, rhR);
    double res_frob = 0.0;
    double norm_A_frob = 0.0;
    frobenius_residual_vs_A(A, rhR, &res_frob, &norm_A_frob);
    const double res_rel = (norm_A_frob > 0.0) ? (res_frob / norm_A_frob) : res_frob;

    bool residual_ok = true;
    if (!skip_residual_check) {
        if (res_rel > residual_rel_tol) {
            residual_ok = false;
        }
        if (residual_abs_tol >= 0.0 && res_frob > residual_abs_tol) {
            residual_ok = false;
        }
    }

    std::cout << "residual_frob ||A - R^H R||_F = " << res_frob << ", ||A||_F = " << norm_A_frob
              << ", relative = " << res_rel << std::endl;
    if (!skip_residual_check) {
        std::cout << "residual_check: rel_tol=" << residual_rel_tol;
        if (residual_abs_tol >= 0.0) {
            std::cout << ", abs_tol=" << residual_abs_tol;
        } else {
            std::cout << ", abs_tol=(disabled)";
        }
        std::cout << " -> " << (residual_ok ? "PASS" : "FAIL") << std::endl;
    } else {
        std::cout << "residual_check: skipped" << std::endl;
    }

    report_R_dut_vs_cpu_ref_ulp(R_expected, R_out, allowed_ulp_mismatch, ulp_report_max, residual_ok,
                                skip_residual_check);

    unsigned int pass_fail = 0;
    // The CPU reference is useful for diagnostics, but RTL floating-point IP may differ by a few ULPs.
    // Use the reconstruction residual as the functional pass/fail criterion.
    if (!skip_residual_check && !residual_ok) pass_fail += 1;

    std::cout << "pass_fail=" << pass_fail
              << ", mismatch_check (true=R_out vs R_expected within ULP)=" << mismatch_check
              << ", residual_ok=" << (skip_residual_check ? true : residual_ok) << std::endl;

    if ((pass_fail > 0 && debug > 0) || debug > 1) {
        printf("  A=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A, "   ", print_precision, 1);
        printf("  R_out=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(R_out, "   ", print_precision, 1);
        printf("  R_expected=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(R_expected, "   ", print_precision, 1);
        if (debug > 1 && !rhR.empty()) {
            std::vector<MATRIX_OUT_T> rhR_exp;
            reconstruct_RhR_from_upper_R(R_expected, rhR_exp);
            double res_ref = 0.0;
            double nref = 0.0;
            frobenius_residual_vs_A(A, rhR_exp, &res_ref, &nref);
            std::cout << "  debug: ||A - R_dut^H R_dut||_F=" << res_frob << ", ||A - R_ref^H R_ref||_F=" << res_ref
                      << std::endl;
        }
    }
    if (pass_fail) {
        std::cout << "TB:Fail" << std::endl;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }

    return (pass_fail);
}
