/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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

// #define _DSPLIB_CHOLESKY_HPP_DEBUG_

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

#define IDX(i, j)  (i)*kNumVecsPerDim + (j) // 2D indexing for 1D inputs.
#define DATA_VECT_T(TT) ::aie::vector<TT, fnVecSampleNum<TT>()>  // Shortcut for defining vectors.


template <typename TT>
INLINE_DECL constexpr unsigned int fnVecSampleNum() {
    return __MAX_READ_WRITE__ / 8 / sizeof(TT);
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
