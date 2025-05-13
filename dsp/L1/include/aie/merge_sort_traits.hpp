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
#ifndef _DSPLIB_MERGE_SORT_TRAITS_HPP_
#define _DSPLIB_MERGE_SORT_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

// TP_API values
static constexpr unsigned int kWindowAPI = 0;
static constexpr unsigned int kStreamAPI = 1;
static constexpr unsigned int kCascAPI = 2;
static constexpr unsigned int kStreamCascAPI = 3;

// Input Interfaces
template <typename T_D>
struct T_inputIF {
    T_D* __restrict inWindow0;
    T_D* __restrict inWindow1;
    input_stream<T_D>* __restrict inStream0;
    input_stream<T_D>* __restrict inStream1;
    input_cascade<T_D>* inCascade = {};
};
// CASC_OUT_FALSE
template <typename T_D>
struct T_outputIF {
    output_stream<T_D>* __restrict outStream;
    output_cascade<T_D>* outCascade{};
};
}
}
}
} // closing namespaces
#endif // _DSPLIB_MERGE_SORT_TRAITS_HPP_
