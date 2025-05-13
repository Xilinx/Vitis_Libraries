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
#if (__HAS_ACCUM_PERMUTES__ == 1)
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

#endif // __HAS_ACCUM_PERMUTES__ == 1

// Retun type of accumulator and output data type for AIE-2
#if (__HAS_ACCUM_PERMUTES__ == 0)
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

#endif // __HAS_ACCUM_PERMUTES__ == 0

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
    static constexpr unsigned getSizeOfVec() { return 1024 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 512 bit vector i.e. X reg.
template <typename TT_DATA_F>
struct T_InOut_X_buff {
    using v_type = ::aie::vector<TT_DATA_F, 512 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 512;
    static constexpr unsigned getSizeOfVec() { return 512 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 256 bit vector i.e. W reg.
template <typename TT_DATA_F>
struct T_InOut_W_buff {
    using v_type = ::aie::vector<TT_DATA_F, 256 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 256;
    static constexpr unsigned getSizeOfVec() { return 256 / 8 / sizeof(TT_DATA_F); };
};

// Function to return the 128 bit vector i.e. V reg.
template <typename TT_DATA_F>
struct T_InOut_V_buff {
    using v_type = ::aie::vector<TT_DATA_F, 128 / 8 / sizeof(TT_DATA_F)>;
    v_type val;
    static constexpr unsigned int size = 128;
    static constexpr unsigned getSizeOfVec() { return 128 / 8 / sizeof(TT_DATA_F); };
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
    using v_type = ::aie::accum<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>, fnNumOfLanes<TT_DATA_F, TT_DATA_G>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return fnNumOfLanes<TT_DATA_F, TT_DATA_G>(); };
    static constexpr unsigned getSize() { return getAccSize<TT_DATA_F, TT_DATA_G>(); };
};

#if (__HAS_ACCUM_PERMUTES__ == 1)
// function to read 128 bits from stream and store it in a 512bit buffer.
template <typename TT_DATA_F>
INLINE_DECL void readStream(::aie::vector<TT_DATA_F, 512 / 8 / sizeof(TT_DATA_F)>& inbuff,
                            unsigned int index,
                            unsigned int streamno) {
    constexpr int kVsize = 128 / 8 / sizeof(TT_DATA_F);
    using t_vect = ::aie::vector<TT_DATA_F, kVsize>;
    t_vect vect = getc_wss(streamno);
    inbuff.insert(index, vect);
};

// function to read 128 bits from stream and store it in a 1024bit buffer.
template <typename TT_DATA_F>
INLINE_DECL void readStream(::aie::vector<TT_DATA_F, 1024 / 8 / sizeof(TT_DATA_F)>& inbuff,
                            unsigned int index,
                            unsigned int streamno) {
    constexpr int kVsize = 128 / 8 / sizeof(TT_DATA_F);
    using t_vect = ::aie::vector<TT_DATA_F, kVsize>;
    t_vect vect = getc_wss(streamno);
    inbuff.insert(index, vect);
};

struct BuffOffsetShuffle {
    unsigned int xBuffOffsetLow = 0;
    unsigned int xBuffOffsetHi = 0;
    unsigned int yBuffOffsetLow = 0;
    unsigned int yBuffOffsetHi = 0;
    unsigned int select = 0;
    unsigned int xsquare = 0;
    unsigned int ysquare = 0;
};

// function to get offset for shuffle.
template <typename TT_DATA_G>
auto getRearrangeOffsets(unsigned int streamsPerCore, unsigned int TP_PHASES, unsigned int TP_FUNCT_TYPE) {
    unsigned int xBuffOffsetLow = 0;
    unsigned int xBuffOffsetHi = 0;
    unsigned int yBuffOffsetLow = 0;
    unsigned int yBuffOffsetHi = 0;
    unsigned int select = 0;
    unsigned int xsquare = 0;
    unsigned int ysquare = 0;
    if (isComplex<TT_DATA_G>()) {
        if (TP_PHASES == 1) {
            xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? ONE_PHASE_BUFFOFFSET_LOW_CONV : ONE_PHASE_BUFFOFFSET_LOW_CORR;
            xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? ONE_PHASE_BUFFOFFSET_HI_CONV : ONE_PHASE_BUFFOFFSET_HI_CORR;
        } else if (TP_PHASES == TWO_PHASES) {
            if (streamsPerCore == TWO_STREAMS) {
                xBuffOffsetLow =
                    (TP_FUNCT_TYPE == CONV) ? ONE_PHASE_BUFFOFFSET_LOW_CONV : ONE_PHASE_BUFFOFFSET_LOW_CORR;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? ONE_PHASE_BUFFOFFSET_HI_CONV : ONE_PHASE_BUFFOFFSET_HI_CORR;
            } else {
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
            }
        } else {
            if (streamsPerCore == TWO_STREAMS) {
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_TWO_STREAM
                                                         : GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_TWO_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_TWO_STREAM
                                                        : GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_TWO_STREAM;
            } else {
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
            }
        }
    } else {
        if (TP_PHASES == SINGLE_PHASE) {
            select = BASE_OFFSET_OF_SELECT;
            xBuffOffsetLow =
                (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XBUFFOFFSET_LOW_CONV : INT16_ONE_PHASE_XBUFFOFFSET_LOW_CORR;
            xBuffOffsetHi =
                (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XBUFFOFFSET_HIGH_CONV : INT16_ONE_PHASE_XBUFFOFFSET_HIGH_CORR;
            xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_ONE_PHASE_XSQUARE_CORR;
        } else if (TP_PHASES == TWO_PHASES) {
            if (streamsPerCore == TWO_STREAMS) {
                select = BASE_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM
                                                         : INT16_TWO_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM
                                                        : INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM;
                xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_ONE_PHASE_XSQUARE_CORR;
            } else {
                select = TWO_PHASES_TWO_STREAMS_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_TWO_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                yBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_TWO_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                yBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_TWO_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_TWO_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_XSQUARE_CORR;
                ysquare = (TP_FUNCT_TYPE == CONV) ? INT16_YSQUARE_CONV : INT16_YSQUARE_CORR;
            }
        } else if (TP_PHASES == FOUR_PHASES) {
            if (streamsPerCore == TWO_STREAMS) {
                select = BASE_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM
                                                         : INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM
                                                        : INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM;
                xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_ONE_PHASE_XSQUARE_CORR;
            } else {
                select = FOUR_PHASES_TWO_STREAMS_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                yBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_FOUR_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                yBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_FOUR_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_FOUR_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_XSQUARE_CORR;
                ysquare = (TP_FUNCT_TYPE == CONV) ? INT16_YSQUARE_CONV : INT16_YSQUARE_CORR;
            }
        } else {
            if (streamsPerCore == TWO_STREAMS) {
                select = BASE_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM
                                                         : INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM
                                                        : INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM;
                xsquare = (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_ONE_PHASE_XSQUARE_CORR;
            } else {
                select = ONLY_TWO_STREAMS_OFFSET_OF_SELECT;
                xBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                xBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                yBuffOffsetLow = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                         : INT16_G_FOUR_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM;
                yBuffOffsetHi = (TP_FUNCT_TYPE == CONV) ? INT16_G_FOUR_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                        : INT16_G_FOUR_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM;
                xsquare =
                    (TP_FUNCT_TYPE == CONV) ? INT16_ONE_PHASE_XSQUARE_CONV : INT16_XSQUARE_CORR; // INT16_YSQUARE_CORR ;
                ysquare = (TP_FUNCT_TYPE == CONV) ? INT16_YSQUARE_CONV : INT16_YSQUARE_CORR;     // INT16_XSQUARE_CORR;
            }
        }
    }
    return BuffOffsetShuffle{xBuffOffsetLow, xBuffOffsetHi, yBuffOffsetLow, yBuffOffsetHi, select, xsquare, ysquare};
};

template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL void gDataReArrange(
    ::aie::vector<TT_DATA_G, (kMaxSamplesInShuffleVec / sizeof(TT_DATA_G))>* g_buff_ptr,
    ::aie::vector<TT_DATA_G, (kMaxSamplesInShuffleVec / sizeof(TT_DATA_G))>* g_rearrange_ptr,
    unsigned int streamsPerCore,
    unsigned int TP_PHASES,
    unsigned int TP_G_LEN,
    unsigned int TP_FUNCT_TYPE) {
    constexpr int kVsize = kBuffSize64Byte / sizeof(TT_DATA_G);  // vector size with 512-bit buffer interms of Bytes
    constexpr int kVsize1 = kBuffSize16Byte / sizeof(TT_DATA_G); // vector size with 128-bit buffer interms of Bytes

    // Alias for for different size of vectors to define.
    using buff_vector = ::aie::vector<TT_DATA_G, kVsize>;
    using t_vect = ::aie::vector<TT_DATA_G, kVsize1>;
    using t_VectInt16 = ::aie::vector<int16, (kMinDataBuffLen << 1)>;
    using t_VectCint16 = ::aie::vector<cint16, kMinDataBuffLen>;

    t_vect __aie_dm_resource_a* ppin = (t_vect __aie_dm_resource_a*)g_buff_ptr;
    t_vect __aie_dm_resource_b* ppout0 = (t_vect __aie_dm_resource_b*)g_rearrange_ptr;
    t_vect __aie_dm_resource_a* ppin_alias = (t_vect __aie_dm_resource_a*)g_buff_ptr;

    static constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>();
    unsigned int reqNumBuff =
        ((TP_PHASES > ((kBuffSize16Byte >> 1) / sizeof(TT_DATA_G))) ? ((TP_G_LEN / TP_PHASES) / kLanes)
                                                                    : (CEIL(TP_G_LEN, kVsize) / kVsize));
    unsigned int buffPtrInc = (CEIL(TP_PHASES, kVsize1) / kVsize1);
    unsigned int maxBuffIndx = (TP_G_LEN < kVsize) ? kMaxIndexOf16ByteVector : kMaxIndexOf32ByteVector;
    auto[xbuffOffset, xbuffOffsetHi, ybuffOffset, ybuffOffsetHi, select, xsquare, ysquare] =
        getRearrangeOffsets<TT_DATA_G>(streamsPerCore, TP_PHASES, TP_FUNCT_TYPE);
    unsigned int numrd = (TP_PHASES > TWO_PHASES) ? (TWO_PHASES / streamsPerCore) : 1;

    buff_vector buff[reqNumBuff];
    buff_vector aShuffleBuff[reqNumBuff];
    t_vect tempGdata;
    t_VectInt16 vectInt16Buff1, vectInt16Buff2;
    t_VectCint16 VectCint16;

    for (int i = 0; i < buffPtrInc; i++) chess_prepare_for_pipelining chess_loop_range((kBuffSize16Byte >> 1), ) {
            ppin = (TP_FUNCT_TYPE == CONV) ? (ppin_alias + i) : (ppin_alias + (buffPtrInc - 1 - i));

            for (int j = 0; j < reqNumBuff; j++) {
                for (int k = 0; k < kMaxIndexOf32ByteVector /* 4 */; k++) {
                    buff[j].insert(k, *ppin);
                    ppin = ppin + buffPtrInc;
                }

                if (isComplex<TT_DATA_G>()) {
                    VectCint16 = as_v16cint16(shuffle16(as_v16cint16(buff[j]), 0, xbuffOffset, xbuffOffsetHi));
                    aShuffleBuff[j] = ::aie::vector_cast<TT_DATA_G>(VectCint16);
                    if (TP_FUNCT_TYPE == CORR) {
                        aShuffleBuff[j] =
                            ::aie::vector_cast<TT_DATA_G>(::aie::conj(::aie::vector_cast<cint16>(aShuffleBuff[j])));
                    }
                } else {
                    vectInt16Buff1 = as_v32int16(select32(select, as_v32int16(buff[j]), 0, xbuffOffset, xbuffOffsetHi,
                                                          xsquare, 0, ybuffOffset, ybuffOffsetHi, ysquare));
                    aShuffleBuff[j] = ::aie::vector_cast<TT_DATA_G>(vectInt16Buff1);
                }
            }

            if (((kVsize / TP_PHASES) < kVsize1) && (!(isComplex<TT_DATA_G>()))) {
                for (int j = 0; j < (reqNumBuff >> 1); j++) {
                    if (streamsPerCore == SINGLE_STREAM) {
                        if (TP_FUNCT_TYPE == CONV) {
                            vectInt16Buff1 = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            vectInt16Buff2 = as_v32int16(
                                select32(INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CONV,
                                         as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                                                   (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210));
                        } else {
                            vectInt16Buff1 = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            vectInt16Buff2 = as_v32int16(
                                select32(INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CORR,
                                         as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                                                   (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210));
                        }
                    } else {
                        if (TP_FUNCT_TYPE == CONV) {
                            vectInt16Buff1 = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x1032));
                            vectInt16Buff2 = as_v32int16(
                                select32(INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CONV,
                                         as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                                                   (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x1032));
                        } else {
                            vectInt16Buff1 = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x1032, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            vectInt16Buff2 = as_v32int16(
                                select32(INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CORR,
                                         as_v64int16(concat(aShuffleBuff[(j << 1)], aShuffleBuff[(j << 1) + 1])), 0,
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE +
                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x1032, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE +
                                                                   (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                         (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE +
                                          MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                         0x3210));
                        }
                    }
                    buff[j] = ::aie::vector_cast<TT_DATA_G>(vectInt16Buff1);
                    buff[(reqNumBuff >> 1) + j] = ::aie::vector_cast<TT_DATA_G>(vectInt16Buff2);
                }
            } else {
                for (int j = 0; j < reqNumBuff; j++) {
                    buff[j] = aShuffleBuff[j];
                }
            }

            for (int j = 0; j < reqNumBuff / numrd; j++) {
                for (int k = 0; k < maxBuffIndx; k++) {
                    for (int l = 0; l < numrd; l++) {
                        if (TP_FUNCT_TYPE == CONV) {
                            tempGdata = buff[j * numrd + l].template extract<kVsize1>(k);
                        } else {
                            tempGdata =
                                buff[(reqNumBuff / numrd - 1 - j) * numrd + (numrd - 1 - l)].template extract<kVsize1>(
                                    (maxBuffIndx - 1) - k);
                        }
                        *ppout0++ = tempGdata;
                    }
                }
            }

        } // End of Main Loop
};
#endif

} // namespace conv_corr {
} // namespace aie {
} // namespace dsp {
} // namespace xf {

#endif // _DSPLIB_CONV_CORR_UTILS_HPP_