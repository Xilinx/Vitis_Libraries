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

template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int TP_PHASES,
          unsigned int TP_G_LEN,
          unsigned int TP_FUNCT_TYPE,
          unsigned int streamsPerCore,
          unsigned int TP_CASC_LEN>
INLINE_DECL void gDataReArrange(
    ::aie::vector<TT_DATA_G, (kMaxSamplesInShuffleVec / sizeof(TT_DATA_G))>* g_buff_ptr,
    ::aie::vector<TT_DATA_G, (kMaxSamplesInShuffleVec / sizeof(TT_DATA_G))>* g_rearrange_ptr) {
    static constexpr int kVsize = kBuffSize64Byte / sizeof(TT_DATA_G); // element count of a 512-bit (64-byte) vector
    static constexpr int kVsize128Elems =
        kBuffSize16Byte /
        sizeof(TT_DATA_G); // element count of a 128-bit (16-byte) vector; used as the G-data read granularity
    // kHalfVecElems128: half the 128-bit vector element count; threshold for TP_PHASES to select
    // between the per-phase/lane buffer layout and the ceiling-over-kVsize buffer layout.
    static constexpr int kHalfVecElems128 = kVsize128Elems / 2;

    // Alias for for different size of vectors to define.
    using buff_vector = ::aie::vector<TT_DATA_G, kVsize>;
    using t_vect = ::aie::vector<TT_DATA_G, kVsize128Elems>;
    using t_VectInt16 = ::aie::vector<int16, (kMinDataBuffLen << 1)>;
    using t_VectCint16 = ::aie::vector<cint16, kMinDataBuffLen>;

    t_vect __aie_dm_resource_a* gDataReadPtr = (t_vect __aie_dm_resource_a*)g_buff_ptr;
    t_vect __aie_dm_resource_b* gDataWritePtr = (t_vect __aie_dm_resource_b*)g_rearrange_ptr;
    t_vect __aie_dm_resource_a* gDataBasePtr = (t_vect __aie_dm_resource_a*)g_buff_ptr;

    static constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>();
    // kReqNumBuffers: total buff[] entries needed — one per G-data vector group.
    // kVsize128Elems/2: threshold = half the 128-bit vector element count; selects the per-phase/lane
    // formula when TP_PHASES exceeds it, otherwise the ceiling-over-kVsize formula.
    static constexpr unsigned int kReqNumBuffers =
        ((TP_PHASES > kHalfVecElems128) ? ((TP_G_LEN / TP_PHASES) / kLanes) : (CEIL(TP_G_LEN, kVsize) / kVsize));
    // kNumPhaseChunks: number of kVsize128Elems-sized chunks needed to cover one phase's G data;
    // also the outer-loop count (buffering passes) and the gDataReadPtr stride between phase groups.
    static constexpr unsigned int kNumPhaseChunks = (CEIL(TP_PHASES, kVsize128Elems) / kVsize128Elems);
    // kMaxPhaseChunks: compiler hint for chess_loop_range — maximum value kNumPhaseChunks can reach
    // (= kBuffSize16Byte / 2 = 8, corresponding to the largest supported TP_PHASES / kVsize128Elems).
    static constexpr unsigned int kMaxPhaseChunks = kBuffSize16Byte >> 1;
    // kNumSlotsPerBuff: number of kVsize128Elems sub-slots per buff[] entry (loop bound for k).
    static constexpr unsigned int kNumSlotsPerBuff =
        (TP_G_LEN < kVsize) ? kMaxIndexOf16ByteVector : kMaxIndexOf32ByteVector;
    // kHalfNumBuffers: midpoint of buff[]; split-point and loop bound for buffer-pair processing.
    static constexpr unsigned int kHalfNumBuffers = kReqNumBuffers / 2;
    // kPairStride = 2: step when indexing shuffledBuff in consecutive pairs (even index = j*2).
    static constexpr int kPairStride = 2;
    // kInterleaveFactor = 2: number of coefficient halves in the interleave split; also the stride
    // in the outGroupIdx permutation and the modulus/divisor used to compute kNumHalfGroups.
    static constexpr int kInterleaveFactor = 2;
    auto[xbuffOffset, xbuffOffsetHi, ybuffOffset, ybuffOffsetHi, select, xsquare, ysquare] =
        getRearrangeOffsets<TT_DATA_G>(streamsPerCore, TP_PHASES, TP_FUNCT_TYPE);
    // kNumBuffsPerGroup: number of buff[] entries consumed per output group iteration.
    // For TP_PHASES > 2: TWO_PHASES / streamsPerCore (2 for single-stream, 1 for dual-stream).
    // For TP_PHASES <= 2: always 1.
    static constexpr unsigned int kNumBuffsPerGroup = (TP_PHASES > TWO_PHASES) ? (TWO_PHASES / streamsPerCore) : 1;

    buff_vector buff[kReqNumBuffers];
    buff_vector shuffledBuff[kReqNumBuffers];
    t_vect tempGdata;
    t_VectInt16 int16LowerHalf;       // first (lower) output half from select32 re-interleave split
    t_VectInt16 int16UpperHalf;       // second (upper) output half from select32 re-interleave split
    t_VectCint16 cint16ShuffleResult; // temporary result of shuffle16 for complex G-data lane reorder

    for (int i = 0; i < kNumPhaseChunks; i++) chess_prepare_for_pipelining chess_loop_range(kMaxPhaseChunks, ) {
            gDataReadPtr = (TP_FUNCT_TYPE == CONV) ? (gDataBasePtr + i) : (gDataBasePtr + (kNumPhaseChunks - 1 - i));

            for (int j = 0; j < kReqNumBuffers; j++) {
                for (int k = 0; k < kMaxIndexOf32ByteVector /* 4 */; k++) {
                    // Guard: only read valid G samples within TP_G_LEN to avoid
                    // out-of-bounds memory access on AIE hardware when G_LEN is
                    // not a multiple of kVsize (e.g. G_LEN=24 with kVsize=16).
                    if (j * (int)kVsize + k * (int)kVsize128Elems < (int)TP_G_LEN) {
                        buff[j].insert(k, *gDataReadPtr);
                    } else {
                        buff[j].insert(k, t_vect{}); // zero-fill beyond G_LEN
                    }
                    // gDataReadPtr is advanced unconditionally to keep the stride constant
                    // whether or not valid G data was read.  The final gDataReadPtr value
                    // after the loop is never dereferenced.
                    gDataReadPtr = gDataReadPtr + kNumPhaseChunks;
                }

                if (isComplex<TT_DATA_G>()) {
                    cint16ShuffleResult = as_v16cint16(shuffle16(as_v16cint16(buff[j]), 0, xbuffOffset, xbuffOffsetHi));
                    shuffledBuff[j] = ::aie::vector_cast<TT_DATA_G>(cint16ShuffleResult);
                    if (TP_FUNCT_TYPE == CORR) {
                        shuffledBuff[j] =
                            ::aie::vector_cast<TT_DATA_G>(::aie::conj(::aie::vector_cast<cint16>(shuffledBuff[j])));
                    }
                } else {
                    int16LowerHalf = as_v32int16(select32(select, as_v32int16(buff[j]), 0, xbuffOffset, xbuffOffsetHi,
                                                          xsquare, 0, ybuffOffset, ybuffOffsetHi, ysquare));
                    shuffledBuff[j] = ::aie::vector_cast<TT_DATA_G>(int16LowerHalf);
                }
            }

            if (((kVsize / TP_PHASES) < kVsize128Elems) && (!(isComplex<TT_DATA_G>()))) {
                for (int j = 0; j < kHalfNumBuffers; j++) {
                    if (streamsPerCore == SINGLE_STREAM) {
                        if (TP_FUNCT_TYPE == CONV) {
                            int16LowerHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            int16UpperHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                    (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                 MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
                                 MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                        } else {
                            int16LowerHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            int16UpperHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, (INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE +
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
                            int16LowerHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x1032));
                            int16UpperHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CONV,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                    (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                 MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210, kBuffSize32Byte, (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                                          (INT16_G_FOUR_PHASE_SELECT32_OFFSET << 1)),
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE +
                                 MAC4ROTDELAY * INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x1032));
                        } else {
                            int16LowerHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x1032, kBuffSize32Byte, INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE,
                                (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE + INT16_G_FOUR_PHASE_SELECT32_OFFSET),
                                0x3210));
                            int16UpperHalf = as_v32int16(select32(
                                INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CORR,
                                as_v64int16(concat(shuffledBuff[j * kPairStride], shuffledBuff[j * kPairStride + 1])),
                                0, (INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE +
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
                    buff[j] = ::aie::vector_cast<TT_DATA_G>(int16LowerHalf);
                    buff[kHalfNumBuffers + j] = ::aie::vector_cast<TT_DATA_G>(int16UpperHalf);
                }
            } else {
                for (int j = 0; j < kReqNumBuffers; j++) {
                    buff[j] = shuffledBuff[j];
                }
            }

            // kValidChunks: actual G-data chunks; kPadChunks: surplus from CEIL() rounding.
            // CORR reversal would place padding at the front — kUseValidChunkReverse corrects this.
            static constexpr int kValidChunks = (int)((TP_G_LEN + kVsize128Elems - 1) / kVsize128Elems);
            static constexpr int kTotalChunks = (int)kReqNumBuffers * (int)kNumSlotsPerBuff;
            static constexpr int kPadChunks = kTotalChunks - kValidChunks;
            // Apply reversed-chunk fix only for the CEIL()/kNumBuffsPerGroup==1 path with padding;
            // the (TP_G_LEN/TP_PHASES)/kLanes path has a different layout and needs no fix.
            static constexpr bool kUseValidChunkReverse =
                ((TP_PHASES <= kHalfVecElems128) && (kNumBuffsPerGroup == 1) && (kPadChunks != 0));

            // Group-major/pass-minor chunk ordering ensures each cascade core gets one chunk per pass.
            // Natural (pass-major) order mis-assigns coefficients when TP_CASC_LEN > TP_PHASES.
            // Reduces to sequential order when kNumPhaseChunks==1 or kNumGroups==1.
            static constexpr int kNumGroups = (int)kReqNumBuffers / (int)kNumBuffsPerGroup;
            static constexpr int kChunksPerGroup = (int)kNumSlotsPerBuff * (int)kNumBuffsPerGroup;
            // int16: the two interleave halves come from select32 (not a second pass), folding
            // a factor-of-2 into kNumGroups. Transpose operates over (2, kNumGroups/2) pairs;
            // identity when TP_CASC_LEN <= TP_PHASES.
            static constexpr bool kIsHalfBufInterleave =
                (((int)kVsize / (int)TP_PHASES) < (int)kVsize128Elems) && (!(isComplex<TT_DATA_G>()));
            // Reordering is valid only when TP_CASC_LEN > TP_PHASES; test TP_CASC_LEN directly —
            // derived quantities (kNumGroups, kNumPhaseChunks) are unreliable for this check.
            static constexpr bool kMultiCascPerPhase = (TP_CASC_LEN > TP_PHASES);
            static constexpr int kNumHalfGroups =
                (kMultiCascPerPhase && kIsHalfBufInterleave && ((kNumGroups % kInterleaveFactor) == 0))
                    ? (kNumGroups / kInterleaveFactor)
                    : 0;
            for (int j = 0; j < kNumGroups; j++) {
                // outGroupIdx: output destination group index for writing rearranged G-data chunks.
                // Equals loop index j when no interleaving is needed (kNumHalfGroups==0).
                // When kNumHalfGroups > 0, applies a stride-2 permutation of j to interleave
                // the two coefficient halves correctly across cascade passes.
                const int outGroupIdx =
                    (kNumHalfGroups > 0) ? (((j % kNumHalfGroups) * kInterleaveFactor) + (j / kNumHalfGroups)) : j;
                for (int k = 0; k < kNumSlotsPerBuff; k++) {
                    for (int l = 0; l < kNumBuffsPerGroup; l++) {
                        if (TP_FUNCT_TYPE == CONV) {
                            tempGdata = buff[j * kNumBuffsPerGroup + l].template extract<kVsize128Elems>(k);
                        } else if (!kUseValidChunkReverse) {
                            tempGdata = buff[(kReqNumBuffers / kNumBuffsPerGroup - 1 - j) * kNumBuffsPerGroup +
                                             (kNumBuffsPerGroup - 1 - l)]
                                            .template extract<kVsize128Elems>((kNumSlotsPerBuff - 1) - k);
                        } else {
                            // Reverse over the valid chunks only, so the padding is emitted
                            // last (as it is for CONV) instead of first.
                            int outPos = (j * (int)kNumSlotsPerBuff + k) * (int)kNumBuffsPerGroup + l;
                            int srcChunk = kValidChunks - 1 - outPos;
                            if (srcChunk < 0) {
                                tempGdata = t_vect{};
                            } else {
                                tempGdata = buff[srcChunk / (int)kNumSlotsPerBuff].template extract<kVsize128Elems>(
                                    srcChunk % (int)kNumSlotsPerBuff);
                            }
                        }
                        const int outChunk =
                            kMultiCascPerPhase ? (outGroupIdx * ((int)kNumPhaseChunks * kChunksPerGroup) +
                                                  i * kChunksPerGroup) // transposed
                                               : (i * (kNumGroups * kChunksPerGroup) + j * kChunksPerGroup); // original
                        gDataWritePtr[outChunk + k * (int)kNumBuffsPerGroup + l] = tempGdata;
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
