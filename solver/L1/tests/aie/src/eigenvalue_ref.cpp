/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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

#include "eigenvalue_ref.hpp"
#include "eigenvalue_ref_utils.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

// Number of unshifted QR sweeps to run. Deliberately generous: this
// reference only ever runs under x86sim (never on hardware), so a large,
// fixed, shift-free/deflation-free sweep count is cheap and needs no
// convergence-detection logic to get right independently of the UUT.
static constexpr unsigned int kNumSweeps = 500;

// A tiny numerical-noise floor below which a vector is treated as zero
// (skip the corresponding Householder reflection rather than dividing by
// ~zero).
static constexpr float kZeroFloor = 1.0e-20f;

template <typename TT_DATA, unsigned int TP_DIM>
void eigenvalue_ref<TT_DATA, TP_DIM>::eigenvalue_ref_main(input_buffer<TT_DATA>& __restrict in_A,
                                                          output_buffer<TT_DATA>& __restrict out_E) {
    constexpr unsigned int N = kN;

    // Awork[i + j*N] = A(i,j). This is a single, self-consistent indexing
    // convention chosen for this function's own internal use -- it does not
    // need to match the graph-level row-major/column-major contract for A,
    // because eigenvalues(A) = eigenvalues(A^T) always (plain transpose, no
    // conjugation), so it makes no difference to the *eigenvalues* produced
    // which convention is used here, only to whether intermediate Q/R
    // factors would match a textbook example.
    TT_DATA Awork[N * N];
    TT_DATA Q[N * N];
    TT_DATA Anext[N * N];
    TT_DATA v[N];

    TT_DATA* pA = in_A.data();
    TT_DATA* pE = out_E.data();

    for (unsigned int i = 0; i < N * N; ++i) Awork[i] = pA[i];

    auto idx = [](unsigned int i, unsigned int j) -> unsigned int { return i + j * N; };

    for (unsigned int sweep = 0; sweep < kNumSweeps; ++sweep) {
        // ------------------------------------------------------------
        // Householder QR decomposition: Awork = Q * R.
        // R is accumulated in-place into Awork; Q is accumulated separately.
        // ------------------------------------------------------------
        for (unsigned int i = 0; i < N * N; ++i) Q[i] = eigRefMakeReal<TT_DATA>(0.0f);
        for (unsigned int i = 0; i < N; ++i) Q[idx(i, i)] = eigRefMakeReal<TT_DATA>(1.0f);

        for (unsigned int k = 0; k < N - 1; ++k) {
            // x = Awork[k:N, k]; compute its norm.
            float normx2 = 0.0f;
            for (unsigned int i = k; i < N; ++i) {
                TT_DATA xi = Awork[idx(i, k)];
                normx2 += eigRefGetReal<TT_DATA>(xi) * eigRefGetReal<TT_DATA>(xi) +
                          eigRefGetImag<TT_DATA>(xi) * eigRefGetImag<TT_DATA>(xi);
            }
            float normx = std::sqrt(normx2);
            if (normx < kZeroFloor) continue; // column already zero below the diagonal

            // Phase-corrected Householder vector v (avoids cancellation):
            //   v = x;  v[k] += phase(x[k]) * ||x||   where phase(z) = z/|z| (or 1 if z ~ 0)
            TT_DATA x0 = Awork[idx(k, k)];
            float x0mag = eigRefMagnitude<TT_DATA>(x0);
            TT_DATA phase =
                (x0mag < kZeroFloor) ? eigRefMakeReal<TT_DATA>(1.0f) : x0 * eigRefMakeReal<TT_DATA>(1.0f / x0mag);

            for (unsigned int i = k; i < N; ++i) v[i] = Awork[idx(i, k)];
            v[k] = v[k] + phase * eigRefMakeReal<TT_DATA>(normx);

            float vnorm2 = 0.0f;
            for (unsigned int i = k; i < N; ++i) {
                vnorm2 += eigRefGetReal<TT_DATA>(v[i]) * eigRefGetReal<TT_DATA>(v[i]) +
                          eigRefGetImag<TT_DATA>(v[i]) * eigRefGetImag<TT_DATA>(v[i]);
            }
            if (vnorm2 < kZeroFloor) continue;
            float beta = 2.0f / vnorm2;

            // Apply the reflection to R (held in Awork): R := (I - beta*v*v^H) * R,
            // affecting columns j = k..N-1.
            for (unsigned int j = k; j < N; ++j) {
                TT_DATA dot = eigRefMakeReal<TT_DATA>(0.0f);
                for (unsigned int i = k; i < N; ++i) {
                    dot = dot + eigRefGetConj<TT_DATA>(v[i]) * Awork[idx(i, j)];
                }
                TT_DATA scale = dot * eigRefMakeReal<TT_DATA>(beta);
                for (unsigned int i = k; i < N; ++i) {
                    Awork[idx(i, j)] = Awork[idx(i, j)] - v[i] * scale;
                }
            }

            // Accumulate the same reflection into Q: Q := Q * (I - beta*v*v^H).
            // (Q*H)(i,j) = Q(i,j) - beta*conj(v_j) * sum_l Q(i,l)*v_l, for j = k..N-1.
            for (unsigned int i = 0; i < N; ++i) {
                TT_DATA dot = eigRefMakeReal<TT_DATA>(0.0f);
                for (unsigned int l = k; l < N; ++l) {
                    dot = dot + Q[idx(i, l)] * v[l];
                }
                TT_DATA dotBeta = dot * eigRefMakeReal<TT_DATA>(beta);
                for (unsigned int j = k; j < N; ++j) {
                    Q[idx(i, j)] = Q[idx(i, j)] - dotBeta * eigRefGetConj<TT_DATA>(v[j]);
                }
            }
        }

        // ------------------------------------------------------------
        // Recombine: Awork := R * Q  (the standard QR-algorithm sweep step).
        // ------------------------------------------------------------
        for (unsigned int j = 0; j < N; ++j) {
            for (unsigned int i = 0; i < N; ++i) {
                TT_DATA acc = eigRefMakeReal<TT_DATA>(0.0f);
                for (unsigned int kk = 0; kk < N; ++kk) {
                    acc = acc + Awork[idx(i, kk)] * Q[idx(kk, j)];
                }
                Anext[idx(i, j)] = acc;
            }
        }
        for (unsigned int i = 0; i < N * N; ++i) Awork[i] = Anext[i];
    }

    // The diagonal of the fully-converged Awork holds the eigenvalues.
    for (unsigned int i = 0; i < N; ++i) pE[i] = Awork[idx(i, i)];
}

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf
