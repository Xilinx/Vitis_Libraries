/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_SVD_TRAITS_HPP_
#define _DSPLIB_SVD_TRAITS_HPP_

#include "device_defs.h"

/*
SVD traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernel class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace solver {
namespace aie {
namespace svd {

//---------------------------------------
// Device-dependent vector size
// Returns the number of samples of type TT that fit in one native vector register.
// AIE1: 256-bit (8 float, 4 cfloat), AIE-ML/AIE22: 512-bit (16 float, 8 cfloat).
template <typename TT>
INLINE_DECL constexpr unsigned int fnVecSampleNum() {
    return __MAX_READ_WRITE__ / 8 / sizeof(TT);
}

// IO vector size: fixed 256-bit on all variants for stream operations and port sizing.
// x86sim for AIE22 does not support 512-bit stream operations (read512bits /
// write512bits segfault), so this must stay 256-bit regardless of AIE variant.
template <typename TT>
INLINE_DECL constexpr unsigned int fnVecSampleNumIO() {
    return 256 / 8 / sizeof(TT);
}

// Storage alignment for V column padding (kStoreCols).
// On AIE22, kStoreCols is rounded to the next multiple of 512-bit width so that
// kVecSizeComp=native (16 for float) is selected. This forces v16float V column
// operations, avoiding the aie2ps chess compiler bug where 256-bit (v8float)
// V column stores produce duplicate columns (both accumulator lanes identical).
// kVecSizeIO (stream IO) remains 256-bit so x86sim stream operations still work.
template <typename TT>
INLINE_DECL constexpr unsigned int fnVecStoreAlign() {
#if __AIE_ARCH__ >= 22
    return __MAX_READ_WRITE__ / 8 / sizeof(TT); // 512-bit alignment on AIE22
#else
    return 256 / 8 / sizeof(TT); // 256-bit on AIE1/AIE-ML
#endif
}

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

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_SVD_TRAITS_HPP_
