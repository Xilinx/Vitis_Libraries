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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_UTILS_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_UTILS_HPP_

/*
    Euclidean Distance Utilities
    This file contains sets of overloaded, templatized and specialized templatized functions for use
    by the main kernel class and run-time function. These functions are separate from the traits file
    because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "device_defs.h"
#include "kernel_api_utils.hpp"
#include "euclidean_distance_traits.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

// Return type of default accumulator used in euclidean_distance
template <typename TT_DATA>
struct tEDAccType {
    using type = accfloat;
};

// Retun type of accumulator and output data type for AIE-1
#if (__HAS_ACCUM_PERMUTES__ == 1)
// float
template <>
struct tEDAccType<float> {
    using type = accfloat;
};

#endif // __HAS_ACCUM_PERMUTES__ == 1 for AIE

// Retun type of accumulator and output data type for AIE-2
#if (__HAS_ACCUM_PERMUTES__ == 0)
// float
template <>
struct tEDAccType<float> {
    using type = accfloat;
};

// bfloat16
template <>
struct tEDAccType<bfloat16> {
    using type = accfloat;
};

#endif // __HAS_ACCUM_PERMUTES__ == 0 for AIE-ML

template <typename T_D>
using tEDAccType_t = typename tEDAccType<T_D>::type;

// Function to return the 1024 bit vector i.e. Y reg.
template <typename TT_DATA>
struct T_InOut_Y_buff {
    using v_type = ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>;
    v_type val;
    static constexpr unsigned int size = 1024;
    static constexpr unsigned getSizeOfVec() { return 1024 / 8 / sizeof(TT_DATA); };
};

// Function to return the 512 bit vector i.e. X reg.
template <typename TT_DATA>
struct T_InOut_X_buff {
    using v_type = ::aie::vector<TT_DATA, 512 / 8 / sizeof(TT_DATA)>;
    v_type val;
    static constexpr unsigned int size = 512;
    static constexpr unsigned getSizeOfVec() { return 512 / 8 / sizeof(TT_DATA); };
};

// Function to return the 256 bit vector i.e. W reg.
template <typename TT_DATA>
struct T_InOut_W_buff {
    using v_type = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    v_type val;
    static constexpr unsigned int size = 256;
    static constexpr unsigned getSizeOfVec() { return 256 / 8 / sizeof(TT_DATA); };
};

// Function to return the 128 bit vector i.e. V reg.
template <typename TT_DATA>
struct T_InOut_V_buff {
    using v_type = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
    v_type val;
    static constexpr unsigned int size = 128;
    static constexpr unsigned getSizeOfVec() { return 128 / 8 / sizeof(TT_DATA); };
};

// Update 256-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// Update 512-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA, 512 / 8 / sizeof(TT_DATA)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// Update 1024-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// T_acc struct with ::aie::accum
template <typename TT_DATA>
struct T_acc_ED {
    using v_type = ::aie::accum<tEDAccType_t<TT_DATA>, fnEDNumLanes<TT_DATA>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return (fnEDNumLanes<TT_DATA>()); };
    static constexpr unsigned getSize() { return getAccSize<TT_DATA>(); };
};

} // namespace euclidean_distance {
} // namespace aie {
} // namespace dsp {
} // namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_UTILS_HPP_