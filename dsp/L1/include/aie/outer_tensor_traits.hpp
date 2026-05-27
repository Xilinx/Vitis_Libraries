/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_OUTER_TENSOR_TRAITS_HPP_
#define _DSPLIB_OUTER_TENSOR_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {

template <typename outType>
INLINE_DECL constexpr unsigned int getUnrollBudget() {
    return 4;
}
#if (__SUPPORTS_CFLOAT__ == 0) // not enough program memory for 16 unrolls.
template <>
INLINE_DECL constexpr unsigned int getUnrollBudget<cfloat>() {
    return 4;
}
#endif // __SUPPORTS_CFLOAT__ == 1
}
}
}
} // closing namespaces
#endif // _DSPLIB_OUTER_TENSOR_TRAITS_HPP_
