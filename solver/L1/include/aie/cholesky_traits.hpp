/*
 * Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CHOLESKY_TRAITS_HPP_
#define _DSPLIB_CHOLESKY_TRAITS_HPP_

#include "device_defs.h"

// Enable independent accumulators across unrolled elimination iterations.
#define XF_SOLVER_CHOLESKY_USE_MULTI_ACCUMULATOR_ELIMINATIONS 0

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

#define IDX(i, j) (i) * kNumVecsPerDim + (j)                            // 2D indexing for 1D inputs.
#define DATA_VECT_T(TT, kVecSampleNum) ::aie::vector<TT, kVecSampleNum> // Shortcut for defining vectors.

// Number of distinct accumulators to use in unrolled k-loops. Bounded by the device's
// floating-point accumulator budget (see __MAX_FLOAT_ACCS__/__MAX_CFLOAT_ACCS__ in
// device_defs.h), but never more than the vector sample count. On AIE1 this keeps the
// accumulator array small enough to fit the vector registers shared with data; on
// AIE-ML/MLv2 the dedicated accumulator file is large, so the full sample count is used.
template <typename TT>
INLINE_DECL constexpr unsigned int fnNumAccs() {
#if XF_SOLVER_CHOLESKY_USE_MULTI_ACCUMULATOR_ELIMINATIONS
    constexpr unsigned int kVecSampleNum = fnVecSampleNum<TT>();
    constexpr unsigned int kMaxAccs = isComplex<TT>() ? __MAX_CFLOAT_ACCS__ : __MAX_FLOAT_ACCS__;
    return (kVecSampleNum < kMaxAccs) ? kVecSampleNum : kMaxAccs;
#else
    return 1;
#endif
}

template <typename TT>
struct acc_t {
    using type = accfloat;
};
template <>
struct acc_t<cfloat> {
    using type = caccfloat;
};
template <typename TT>
using accType_t = typename acc_t<TT>::type;
}
}
}
} // closing namespaces
#endif // _DSPLIB_CHOLESKY_TRAITS_HPP_
