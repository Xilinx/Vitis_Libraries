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
#ifndef _SOLVERLIB_EIGENVALUE_REF_UTILS_HPP_
#define _SOLVERLIB_EIGENVALUE_REF_UTILS_HPP_

#include <cmath>

/*
Scalar type-generic helpers (float <-> cfloat) for the eigenvalue reference
model. Deliberately independent of eigenvalue_utils.hpp (used by the
production hessenberg/eig_qr kernels): this file uses only plain std::
arithmetic, no AIE vector-API calls, matching the same
reference-kernel-independence convention already used by
cholesky_ref_utils.hpp for the ls_cholesky_solver reference.
*/

template <typename TT>
inline float eigRefGetReal(TT val) {
    return val;
}
template <>
inline float eigRefGetReal<cfloat>(cfloat val) {
    return val.real;
}

template <typename TT>
inline float eigRefGetImag(TT val) {
    return 0.0f;
}
template <>
inline float eigRefGetImag<cfloat>(cfloat val) {
    return val.imag;
}

template <typename TT>
inline TT eigRefMakeReal(float val) {
    return val;
}
template <>
inline cfloat eigRefMakeReal<cfloat>(float val) {
    return {val, 0.0f};
}

template <typename TT>
inline TT eigRefGetConj(TT val) {
    return val;
}
template <>
inline cfloat eigRefGetConj<cfloat>(cfloat val) {
    return {val.real, -val.imag};
}

template <typename TT>
inline float eigRefMagnitude(TT val) {
    return std::abs(val);
}
template <>
inline float eigRefMagnitude<cfloat>(cfloat val) {
    return std::sqrt(val.real * val.real + val.imag * val.imag);
}

#endif // _SOLVERLIB_EIGENVALUE_REF_UTILS_HPP_
