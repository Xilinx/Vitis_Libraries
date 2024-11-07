/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_FUNC_APPROX_TRAITS_HPP_
#define _DSPLIB_FUNC_APPROX_TRAITS_HPP_

/*
Function Approximation traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {

#ifdef __SUPPORTS_ACC64__
template <typename T_D>
struct accType {
    using type = acc64;
};
template <>
struct accType<int16> {
    using type = acc64;
};
template <>
struct accType<float> {
    using type = accfloat;
};
template <>
struct accType<bfloat16> {
    using type = accfloat;
};
#else
template <typename T_D>
struct accType {
    using type = acc80;
};
template <>
struct accType<int16> {
    using type = acc48;
};
template <>
struct accType<float> {
    using type = accfloat;
};
#endif //__SUPPORTS_ACC64__
template <typename T_D_A>
using accType_t = typename accType<T_D_A>::type;

template <typename T_D>
struct slopeOffset_t {
    T_D slope;
    T_D offset;
};

template <typename TT>
struct complex_tt_data {
    using type = cint16;
};
template <>
struct complex_tt_data<int32> {
    using type = cint32;
};
template <>
struct complex_tt_data<float> {
    using type = cfloat;
};
template <typename TT>
using complex_tt_data_t = typename complex_tt_data<TT>::type;
}
}
}
}
#endif // _DSPLIB_FUNC_APPROX_TRAITS_HPP_
