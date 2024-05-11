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
#ifndef _DSPLIB_CONV_CORR_UTILS_HPP_
#define _DSPLIB_CONV_CORR_UTILS_HPP_

/*
    Convolution and Correlation Utilities
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
#include "conv_corr_traits.hpp"

using namespace adf;

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#define Y_REG_BITS 1024
#define X_REG_BITS 512
#define W_REG_BITS 256
#define V_REG_BITS 128
#define ZERO 0
#define ONE 1
#define TWO 2
#define THREE 3
#define UNROLL_4 4
#define UNROLL_8 8
#define UNROLL_16 16
#define ROUND(X, Y) (((X % Y) > ((Y + 1) / 2)) ? ((int)(X + Y - 1) / (int)Y) : ((int)X / (int)Y))

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

// Return type of default accumulator used in conv_corr
template <typename TT_DATA_F, typename TT_DATA_G>
struct tConvCorrAccType {
    using type = cacc48;
};

// Return type of default output data type used in conv_corr
template <typename TT_DATA_F, typename TT_DATA_G>
struct outType {
    using type = cint16;
};

// Retun type of accumulator and output data type for AIE-1
#ifdef _SUPPORTS_DEVICE_AIE_1_
template <>
struct tConvCorrAccType<int8, int8> {
    using type = acc48;
};
template <>
struct outType<int8, int8> {
    using type = int16;
};

template <>
struct tConvCorrAccType<int16, int8> {
    using type = acc48;
};
template <>
struct outType<int16, int8> {
    using type = int16;
};

// int16
template <>
struct tConvCorrAccType<int16, int16> {
    using type = acc48;
};
template <>
struct outType<int16, int16> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int16, int32> {
    using type = acc80;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int16, cint16> {
    using type = cacc48;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};

template <>
struct tConvCorrAccType<int16, cint32> {
    using type = cacc80;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};

// int32
template <>
struct tConvCorrAccType<int32, int16> {
    using type = acc80;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int32, cint16> {
    using type = cacc80;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<int32, int32> {
    using type = acc80;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int32, cint32> {
    using type = cacc80;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};

// float
template <>
struct tConvCorrAccType<float, float> {
    using type = accfloat;
};
template <>
struct outType<float, float> {
    using type = float;
};

template <>
struct tConvCorrAccType<float, cfloat> {
    using type = caccfloat;
};
template <>
struct outType<float, cfloat> {
    using type = cfloat;
};

// cint16
template <>
struct tConvCorrAccType<cint16, int16> {
    using type = cacc48;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};

template <>
struct tConvCorrAccType<cint16, cint16> {
    using type = cacc48;
};
template <>
struct outType<cint16, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint16, int32> {
    using type = cacc48;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint16, cint32> {
    using type = cacc80;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};

// cint32
template <>
struct tConvCorrAccType<cint32, int16> {
    using type = cacc48;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, cint16> {
    using type = cacc80;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, int32> {
    using type = cacc80;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, cint32> {
    using type = cacc80;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};

// cfloat
template <>
struct tConvCorrAccType<cfloat, float> {
    using type = caccfloat;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};

template <>
struct tConvCorrAccType<cfloat, cfloat> {
    using type = caccfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};

#endif // _SUPPORTS_DEVICE_AIE_1_

// Retun type of accumulator and output data type for AIE-2
#ifdef _SUPPORTS_DEVICE_AIE_2_
// int8
template <>
struct tConvCorrAccType<int8, int8> {
    using type = acc32;
};
template <>
struct outType<int8, int8> {
    using type = int16;
};

template <>
struct tConvCorrAccType<int16, int8> {
    using type = acc32;
};
template <>
struct outType<int16, int8> {
    using type = int16;
};

// int16
template <>
struct tConvCorrAccType<int16, int16> {
    using type = acc64;
};
template <>
struct outType<int16, int16> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int16, int32> {
    using type = acc64;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int16, cint16> {
    using type = cacc64;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};

template <>
struct tConvCorrAccType<int16, cint32> {
    using type = cacc64;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};

// int32
template <>
struct tConvCorrAccType<int32, int16> {
    using type = acc64;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int32, cint16> {
    using type = cacc64;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<int32, int32> {
    using type = acc64;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};

template <>
struct tConvCorrAccType<int32, cint32> {
    using type = cacc64;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};

// float
template <>
struct tConvCorrAccType<float, float> {
    using type = accfloat;
};
template <>
struct outType<float, float> {
    using type = float;
};

// bfloat16
template <>
struct tConvCorrAccType<bfloat16, bfloat16> {
    using type = accfloat;
};
template <>
struct outType<bfloat16, bfloat16> {
    using type = float;
};

// cint16
template <>
struct tConvCorrAccType<cint16, int16> {
    using type = cacc64;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};

template <>
struct tConvCorrAccType<cint16, cint16> {
    using type = cacc64;
};
template <>
struct outType<cint16, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint16, int32> {
    using type = cacc64;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint16, cint32> {
    using type = cacc64;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};

// cint32
template <>
struct tConvCorrAccType<cint32, int16> {
    using type = cacc64;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, cint16> {
    using type = cacc64;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, int32> {
    using type = cacc64;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};

template <>
struct tConvCorrAccType<cint32, cint32> {
    using type = cacc64;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};

#endif // _SUPPORTS_DEVICE_AIE_2_

template <typename T_D_A, typename T_D_B>
using tConvCorrAccType_t = typename tConvCorrAccType<T_D_A, T_D_B>::type;

template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

// Function to return the 1024 bit vector i.e. Y reg.
template <typename TT_DATA_F>
struct T_InOut_Y_buff {
    using v_type = ::aie::vector<TT_DATA_F, 1024 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 1024;
    static constexpr unsigned getLanes() { return 1024 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 512 bit vector i.e. X reg.
template <typename TT_DATA_F>
struct T_InOut_X_buff {
    using v_type = ::aie::vector<TT_DATA_F, 512 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 512;
    static constexpr unsigned getLanes() { return 512 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 256 bit vector i.e. W reg.
template <typename TT_DATA_F>
struct T_InOut_W_buff {
    using v_type = ::aie::vector<TT_DATA_F, 256 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 256;
    static constexpr unsigned getLanes() { return 256 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 128 bit vector i.e. V reg.
template <typename TT_DATA_F>
struct T_InOut_V_buff {
    using v_type = ::aie::vector<TT_DATA_F, 128 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 128;
    static constexpr unsigned getLanes() { return 128 / 8 / sizeof(TT_DATA_F); };
};

// Update 256-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA_F>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA_F, 256 / 8 / sizeof(TT_DATA_F)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA_F>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA_F);
    using t_vect = ::aie::vector<TT_DATA_F, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// Update 512-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA_F>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA_F, 512 / 8 / sizeof(TT_DATA_F)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA_F>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA_F);
    using t_vect = ::aie::vector<TT_DATA_F, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// Update 1024-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA_F>
INLINE_DECL void upd_W_buff(::aie::vector<TT_DATA_F, 1024 / 8 / sizeof(TT_DATA_F)>& inbuff,
                            unsigned int index,
                            T_InOut_W_buff<TT_DATA_F>* inDataPtr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA_F);
    using t_vect = ::aie::vector<TT_DATA_F, kVsize>;
    t_vect* vPtr = (t_vect*)&*inDataPtr;
    t_vect vect = *vPtr;
    inbuff.insert(index, vect);
    inDataPtr += kVsize;
};

// T_acc struct with ::aie::accum
template <typename TT_DATA_F, typename TT_DATA_G>
struct T_acc_ConCor {
    using v_type = ::aie::accum<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>, aie_CC_NumLanes<TT_DATA_F, TT_DATA_G>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return aie_CC_NumLanes<TT_DATA_F, TT_DATA_G>(); };
    static constexpr unsigned getSize() { return getAccSize<TT_DATA_F, TT_DATA_G>(); };
};

} // namespace conv_corr {
} // namespace aie {
} // namespace dsp {
} // namespace xf {

#endif // _DSPLIB_CONV_CORR_UTILS_HPP_