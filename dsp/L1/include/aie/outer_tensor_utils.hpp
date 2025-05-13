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
#ifndef _DSPLIB_OUTER_TENSOR_UTILS_HPP_
#define _DSPLIB_OUTER_TENSOR_UTILS_HPP_

//#define _DSPLIB_OUTER_TENSOR_UTILS_DEBUG_

/*
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "outer_tensor_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {

template <typename T_A, typename T_B>
struct dataVect_t {
    using A = ::aie::vector<T_A, vecSampleNum<T_A, T_B>().A>;
    using B = ::aie::vector<T_B, vecSampleNum<T_A, T_B>().B>;
    using Acc = ::aie::accum<accTypeMult_t<T_A, T_B>, vecSampleNum<T_A, T_B>().Acc>;
    using TempOut = ::aie::vector<outTypeMult_t<T_A, T_B>, vecSampleNum<T_A, T_B>().TempOut>;
    using Out = ::aie::vector<outTypeMult_t<T_A, T_B>, vecSampleNum<T_A, T_B>().Out>;
};
}
}
}
}

#endif // _DSPLIB_OUTER_TENSOR_UTILS_HPP_
