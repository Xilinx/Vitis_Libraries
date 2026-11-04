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
#ifndef _SOLVERLIB_EIGENVALUE_UTILS_HPP_
#define _SOLVERLIB_EIGENVALUE_UTILS_HPP_

/*
Vector-API helpers shared by the eigenvalue solver's two kernels (hessenberg,
eig_qr). Kept separate from eigenvalue_traits.hpp because that file must stay
vector-API-free (it is used at graph-construction time); this file requires
aie_api/aie.hpp and is only included by the kernel .hpp/.cpp files themselves.
*/

// Declares the real global ::aie vector/accum API. Not pulled in by <adf.h>
// alone in every build config -- without this, ::aie below would resolve to
// this xf::solver::aie namespace instead of the real one (see the identical
// note in matrix_mult_vecapi.hpp / svd.hpp).
#include "aie_api/aie.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

// ---------------------------------------------------------------------------
// Accumulator type trait (float -> accfloat, cfloat -> caccfloat).
// Mirrors the identical trait already duplicated locally per kernel family
// in this library (see vecapi_acc_t in matrix_mult_vecapi.hpp, acc_t in
// cholesky_traits.hpp).
// ---------------------------------------------------------------------------
template <typename TT>
struct eig_acc_t {
    using type = accfloat;
};
template <>
struct eig_acc_t<cfloat> {
    using type = caccfloat;
};
template <typename TT>
using eigAccType_t = typename eig_acc_t<TT>::type;

// ---------------------------------------------------------------------------
// Scalar type-generic helpers (float <-> cfloat), mirroring the
// getReal/getConj/getScalarAsType pattern already used in
// cholesky_ref_utils.hpp, extended here with getImag()/magnitude() for the
// Givens-rotation / deflation math in eig_qr.cpp.
// ---------------------------------------------------------------------------

/** Real part of a scalar: identity for float, .real for cfloat. */
template <typename TT>
INLINE_DECL float getReal(TT val) {
    return val;
}
template <>
INLINE_DECL float getReal<cfloat>(cfloat val) {
    return val.real;
}

/** Imaginary part of a scalar: always 0 for float, .imag for cfloat. */
template <typename TT>
INLINE_DECL float getImag(TT val) {
    return 0.0f;
}
template <>
INLINE_DECL float getImag<cfloat>(cfloat val) {
    return val.imag;
}

/** Construct a "real-only" TT value from a float scalar. */
template <typename TT>
INLINE_DECL TT makeReal(float val) {
    return val;
}
template <>
INLINE_DECL cfloat makeReal<cfloat>(float val) {
    return {val, 0.0f};
}

/** Scalar complex conjugate: identity for float, negated .imag for cfloat. */
template <typename TT>
INLINE_DECL TT getConj(TT val) {
    return val;
}
template <>
INLINE_DECL cfloat getConj<cfloat>(cfloat val) {
    return {val.real, -val.imag};
}

/** Magnitude (absolute value) of a scalar: |val| for float, sqrt(re^2+im^2) for cfloat. */
template <typename TT>
INLINE_DECL float magnitude(TT val) {
    return val < 0.0f ? -val : val;
}
template <>
INLINE_DECL float magnitude<cfloat>(cfloat val) {
    return ::aie::sqrt(val.real * val.real + val.imag * val.imag);
}

/** Scalar negation: identity-with-sign-flip for float, negate both fields for cfloat.
 *  Written as an explicit helper (rather than relying on unary operator-) to
 *  reconstruct negated complex values field-by-field. */
template <typename TT>
INLINE_DECL TT negate(TT val) {
    return -val;
}
template <>
INLINE_DECL cfloat negate<cfloat>(cfloat val) {
    return {-val.real, -val.imag};
}

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_EIGENVALUE_UTILS_HPP_
