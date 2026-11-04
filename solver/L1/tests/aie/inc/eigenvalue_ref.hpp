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
#ifndef _SOLVERLIB_EIGENVALUE_REF_HPP_
#define _SOLVERLIB_EIGENVALUE_REF_HPP_

/*
Reference (golden model) kernel for the eigenvalue solver.

Deliberately uses a DIFFERENT algorithm from the production UUT
(hessenberg + shifted/deflated Givens-rotation QR, see hessenberg.hpp /
eig_qr.hpp) for genuine cross-validation, following the same rationale as
mp_pseudoinverse_ref (Gauss-Jordan vs. the UUT's SVD): a plain, unshifted,
undeflated, non-Hessenberg-preconditioned QR-algorithm iteration --

    repeat a large fixed number of sweeps:
        A = Q * R          (full dense Householder QR decomposition of A)
        A := R * Q         (recombine)

For a matrix whose eigenvalues have distinct magnitudes, this is guaranteed
by classical numerical-analysis results to converge (the off-diagonal
entries shrink towards zero and the diagonal towards the eigenvalues,
ordered by decreasing magnitude) -- just far more slowly than the
shift-accelerated, deflation-shortened UUT algorithm. Since this reference
only ever runs under x86sim (never on hardware), using a generously large
fixed sweep count is cheap and requires no shift/deflation logic to get
right independently of the UUT.

This is a scalar, non-vectorized, AIE-compilable kernel (like
ls_cholesky_solver_ref / mp_pseudoinverse_ref), so it compiles for both
x86sim and the AIE device without depending on the production kernels'
vector-API code at all.
*/

#include <adf.h>

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

/**
 * @brief Reference eigenvalue kernel (plain unshifted QR-algorithm iteration).
 *
 * @tparam TT_DATA Data type: float or cfloat.
 * @tparam TP_DIM  Dimension of the square matrix A (= number of eigenvalues).
 **/
template <typename TT_DATA, unsigned int TP_DIM>
class eigenvalue_ref {
   public:
    static constexpr unsigned int kN = TP_DIM;

    // Input: A matrix (kN x kN)
    static constexpr unsigned int kInWindowVsize = kN * kN;
    // Output: E, eigenvalues of A (kN elements)
    static constexpr unsigned int kOutWindowVsize = kN;

    static void registerKernelClass() { REGISTER_FUNCTION(eigenvalue_ref::eigenvalue_ref_main); }

    /**
     * @brief Compute the eigenvalues of A via plain unshifted QR iteration.
     *
     * @param in_A  Input buffer: A matrix (kN x kN).
     * @param out_E Output buffer: eigenvalues of A (kN elements).
     **/
    void eigenvalue_ref_main(input_buffer<TT_DATA>& __restrict in_A, output_buffer<TT_DATA>& __restrict out_E);
};

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_EIGENVALUE_REF_HPP_
