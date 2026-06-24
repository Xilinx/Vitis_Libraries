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

/**
 * @file block_cholesky_cfloat.hpp
 * @brief Blocked Cholesky (outer-product form) for NxN Hermitian SPD matrices using std::complex<float>
 *        (cfloat). Vitis HLS in-place upper-triangular factorization A = R^H * R.
 *        Dimensions are compile-time template parameters: matrix order @a N and block size @a B (N % B == 0).
 *   - xf_block_cholesky_inplace<N, B>:   Entry point function (explicit template arguments at call site).
 *   - xf_block_cholesky_128_inplace:     Convenience wrapper for N=128, B=32 (legacy default).
 *
 * @note Call xf::solver::xf_block_cholesky_inplace<N,B>() from a stream/AXI wrapper top; do not add HLS INTERFACE
 *       pragmas on the array parameter here. Instantiations are header-only (inline templates).
 */
#ifndef _XF_BLOCK_CHOLESKY_CFLOAT_HPP_
#define _XF_BLOCK_CHOLESKY_CFLOAT_HPP_

#include <hls_math.h>
#include <complex>

namespace xf {
namespace solver {

using xf_block_cholesky_cfloat = std::complex<float>;

// ===================================================================================================================
// Block I/O: copy one BxB tile between global matrix A and local buffers.

template <int N, int B>
void load_block(const xf_block_cholesky_cfloat A[N][N], int bi, int bj, xf_block_cholesky_cfloat buf[B][B]) {
load_i:
    for (int i = 0; i < B; ++i) {
    load_j:
        for (int j = 0; j < B; ++j) {
#pragma HLS PIPELINE II = 1
            buf[i][j] = A[bi * B + i][bj * B + j];
        }
    }
}

template <int N, int B>
void store_block(xf_block_cholesky_cfloat A[N][N], int bi, int bj, const xf_block_cholesky_cfloat buf[B][B]) {
store_i:
    for (int i = 0; i < B; ++i) {
    store_j:
        for (int j = 0; j < B; ++j) {
#pragma HLS PIPELINE II = 1
            A[bi * B + i][bj * B + j] = buf[i][j];
        }
    }
}

// ===================================================================================================================
// BxB upper Cholesky: factorize diagonal tile in place (upper triangle); clear strict lower in tile.

template <int B>
void chol_upper(xf_block_cholesky_cfloat a[B][B]) {
    static_assert(B > 0, "block_cholesky: B must be positive");
chol_i:
    for (int i = 0; i < B; ++i) {
    chol_j:
        for (int j = 0; j < i; ++j) {
            xf_block_cholesky_cfloat sum = a[j][i];
        chol_k1:
            for (int k = 0; k < j; ++k) {
                sum -= std::conj(a[k][j]) * a[k][i];
            }
            a[j][i] = sum / a[j][j];
        }
        xf_block_cholesky_cfloat diag = a[i][i];
    chol_k2:
        for (int k = 0; k < i; ++k) {
            diag -= std::conj(a[k][i]) * a[k][i];
        }
        float diag_real = diag.real();
        a[i][i] = xf_block_cholesky_cfloat(hls::sqrtf(diag_real), 0.0f);
    clear_lower:
        for (int j = 0; j < i; ++j) {
            a[i][j] = xf_block_cholesky_cfloat(0.0f, 0.0f);
        }
    }
}

// ===================================================================================================================
// Triangular solve: rkk^H * X = akc with rkk upper; overwrites akc with X.

template <int B>
void trsm_left_lower(const xf_block_cholesky_cfloat rkk[B][B], xf_block_cholesky_cfloat akc[B][B]) {
    static_assert(B > 0, "block_cholesky: B must be positive");
trsm_col:
    for (int j = 0; j < B; ++j) {
    trsm_row:
        for (int i = 0; i < B; ++i) {
            xf_block_cholesky_cfloat sum = akc[i][j];
        trsm_k:
            for (int k = 0; k < i; ++k) {
                sum -= std::conj(rkk[k][i]) * akc[k][j];
            }
            akc[i][j] = sum / rkk[i][i];
        }
    }
}

// ===================================================================================================================
// Rank-k update: arc -= akr^H * akc.

template <int B>
void gemm_update(const xf_block_cholesky_cfloat akr[B][B],
                 const xf_block_cholesky_cfloat akc[B][B],
                 xf_block_cholesky_cfloat arc[B][B]) {
    static_assert(B > 0, "block_cholesky: B must be positive");
gemm_i:
    for (int i = 0; i < B; ++i) {
    gemm_j:
        for (int j = 0; j < B; ++j) {
            xf_block_cholesky_cfloat sum = xf_block_cholesky_cfloat(0.0f, 0.0f);
        gemm_k:
            for (int k = 0; k < B; ++k) {
                sum += std::conj(akr[k][i]) * akc[k][j];
            }
            arc[i][j] -= sum;
        }
    }
}

/**
 * @brief xf_block_cholesky_inplace
 *
 * Entry point function: in-place blocked upper Cholesky on the full NxN matrix A.
 *
 * @tparam N Matrix order (positive; N % B == 0).
 * @tparam B Block size (positive tile dimension).
 *
 * @param A Hermitian positive definite matrix in/out. On exit, upper triangle holds R for A = R^H * R;
 *          strict lower triangle is cleared for TB / stream I/O.
 */
template <int N, int B>
void xf_block_cholesky_inplace(xf_block_cholesky_cfloat A[N][N]) {
    static_assert(N > 0 && B > 0 && N % B == 0, "block_cholesky: require N > 0, B > 0, and N divisible by B");
    constexpr int NB = N / B;

    xf_block_cholesky_cfloat buf_kk[B][B];
    xf_block_cholesky_cfloat buf_kc[B][B];
    xf_block_cholesky_cfloat buf_kr[B][B];
    xf_block_cholesky_cfloat buf_rc[B][B];

blk_k:
    for (int k = 0; k < NB; ++k) {
        load_block<N, B>(A, k, k, buf_kk);
        chol_upper<B>(buf_kk);
        store_block<N, B>(A, k, k, buf_kk);

    blk_c:
        for (int c = k + 1; c < NB; ++c) {
            load_block<N, B>(A, k, c, buf_kc);
            trsm_left_lower<B>(buf_kk, buf_kc);
            store_block<N, B>(A, k, c, buf_kc);
        }

    blk_r:
        for (int r = k + 1; r < NB; ++r) {
            load_block<N, B>(A, k, r, buf_kr);
        blk_c2:
            for (int c = r; c < NB; ++c) {
                load_block<N, B>(A, r, c, buf_rc);
                load_block<N, B>(A, k, c, buf_kc);
                gemm_update<B>(buf_kr, buf_kc, buf_rc);
                store_block<N, B>(A, r, c, buf_rc);
            }
        }
    }

clear_strict_lower:
    for (int i = 1; i < N; ++i) {
        for (int j = 0; j < i; ++j) {
#pragma HLS PIPELINE II = 1
            A[i][j] = xf_block_cholesky_cfloat(0.0f, 0.0f);
        }
    }
}

/** @brief Convenience wrapper: N=128, B=32 (historical default testcase). */
inline void xf_block_cholesky_128_inplace(xf_block_cholesky_cfloat A[128][128]) {
    xf_block_cholesky_inplace<128, 32>(A);
}

} // end namespace solver
} // end namespace xf

#endif // _XF_BLOCK_CHOLESKY_CFLOAT_HPP_
