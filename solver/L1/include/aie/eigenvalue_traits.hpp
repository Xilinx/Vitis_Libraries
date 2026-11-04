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
#ifndef _SOLVERLIB_EIGENVALUE_TRAITS_HPP_
#define _SOLVERLIB_EIGENVALUE_TRAITS_HPP_

#include "device_defs.h"

/*
Eigenvalue solver traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernel classes (hessenberg, eig_qr).
Specifically, this file does not contain any vector types or intrinsics since it is required for
construction and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

//---------------------------------------
// Data type traits
template <typename TT_DATA>
INLINE_DECL constexpr bool fnIsComplexDataType() {
    return false;
}
template <>
INLINE_DECL constexpr bool fnIsComplexDataType<cfloat>() {
    return true;
}

//---------------------------------------
// IO vector size: fixed 256-bit width (8 float / 4 cfloat lanes), matching the
// same fixed-256-bit convention used elsewhere in this library for AIE-variant
// -independent stream/IO sizing (see fnVecSampleNumIO() in svd_traits.hpp).
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnVecSampleNumIO() {
    return 256 / 8 / sizeof(TT_DATA);
}

//---------------------------------------
// constexpr-compatible integer square root (Newton-Raphson), needed to derive
// kMaxDimCols in eigenvalue_graph.hpp from __DATA_MEM_BYTES__ at compile
// time. Mirrors qrd_load_split.hpp's xf::solver::aie::qrd::isqrt() -- kept as
// a local copy rather than a cross-core include, since it's a generic
// integer-math utility unrelated to qrd specifically.
constexpr unsigned int isqrt(unsigned int n) {
    unsigned int x = n;
    unsigned int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_EIGENVALUE_TRAITS_HPP_
