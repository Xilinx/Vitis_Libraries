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
#define FOUR 4
#define EIGHT 8
#define SIXTEEN 16
#define THIRTYTWO 32
#define V4SIZE 4
#define V8SIZE 8
#define V16SIZE 16
#define V32SIZE 32
#define UNROLL_4 4
#define UNROLL_8 8
#define UNROLL_16 16
#define MAC4ROTDELAY 3
#define ONE_PHASE_BUFFOFFSET_LOW_CONV 0x76543210u
#define ONE_PHASE_BUFFOFFSET_LOW_CORR 0x45670123u
#define ONE_PHASE_BUFFOFFSET_HI_CONV 0xFEDCBA98u
#define ONE_PHASE_BUFFOFFSET_HI_CORR 0xCDEF89ABu
#define TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM 0xECA86420u
#define TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x8ACE0246u
#define TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0xFDB97531u
#define TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x9BDF1357u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_TWO_STREAM 0xDC985410u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_TWO_STREAM 0x89CD0145u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM 0xD951C840u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x159D048Cu
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_TWO_STREAM 0xFEBA7632u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_TWO_STREAM 0xABEF2367u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0xFB73EA62u
#define GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x37BF26AEu
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

//----------------------------------------------------------------------
// isComplex
template <typename T>
constexpr bool isComplex() {
#if __SUPPORTS_CFLOAT__ == 1
    return (std::is_same<T, cint16>::value || std::is_same<T, cint32>::value || std::is_same<T, cfloat>::value) ? true
                                                                                                                : false;
#endif
#if __SUPPORTS_CFLOAT__ == 0
    return (std::is_same<T, cint16>::value || std::is_same<T, cint32>::value) ? true : false;
#endif
};

// isFloat
template <typename T>
constexpr bool isFloat() {
#if __SUPPORTS_CFLOAT__ == 1
    return (std::is_same<T, float>::value || std::is_same<T, cfloat>::value) ? true : false;
#endif
#if __SUPPORTS_CFLOAT__ == 0
    return (std::is_same<T, float>::value) ? true : false;
#endif
};

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
    using v_type = ::aie::accum<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>, aie_CC_NumLanes<TT_DATA_F, TT_DATA_G>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return aie_CC_NumLanes<TT_DATA_F, TT_DATA_G>(); };
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

template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL void gdata_rearrange(::aie::vector<TT_DATA_G, V4SIZE>* g_buff_ptr,
                                 ::aie::vector<TT_DATA_G, V4SIZE>* g_rearrange_ptr,
                                 unsigned int streams_per_core,
                                 unsigned int TP_PHASES,
                                 unsigned int TP_G_LEN,
                                 unsigned int TP_FUNCT_TYPE) {
    // constexpr int buffsize = (TP_G_LEN > 16) ? 16 : (TP_G_LEN) ;
    using buff_vector = ::aie::vector<TT_DATA_G, V16SIZE>;
    using t_vect = ::aie::vector<TT_DATA_G, V4SIZE>;
    t_vect __aie_dm_resource_a* ppin = (t_vect __aie_dm_resource_a*)g_buff_ptr;
    t_vect __aie_dm_resource_b* ppout0 = (t_vect __aie_dm_resource_b*)g_rearrange_ptr;
    t_vect __aie_dm_resource_a* ppin_alias = (t_vect __aie_dm_resource_a*)g_buff_ptr;

    static constexpr unsigned int kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>();
    unsigned int num_buff_req =
        ((TP_PHASES > TWO) ? ((TP_G_LEN / TP_PHASES) / kLanes) : (CEIL(TP_G_LEN, minDataBuffLen()) / minDataBuffLen()));
    unsigned int buff_ptr_inc = (CEIL(TP_PHASES, kLanes) / kLanes);
    unsigned int max_buf_idx = (TP_G_LEN < minDataBuffLen()) ? TWO : FOUR;

    // offset to shuffle for phases = 1
    unsigned int buff_offset_1ph = (TP_FUNCT_TYPE == 1) ? ONE_PHASE_BUFFOFFSET_LOW_CONV : ONE_PHASE_BUFFOFFSET_LOW_CORR;
    unsigned int buff_offset_hi_1ph =
        (TP_FUNCT_TYPE == 1) ? ONE_PHASE_BUFFOFFSET_HI_CONV : ONE_PHASE_BUFFOFFSET_HI_CORR;

    // offset to shuffle for phases = 2
    unsigned int buff_offset_2ph =
        (streams_per_core == TWO)
            ? ((TP_FUNCT_TYPE == 1) ? ONE_PHASE_BUFFOFFSET_LOW_CONV : ONE_PHASE_BUFFOFFSET_LOW_CORR)
            : ((TP_FUNCT_TYPE == 1) ? TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                    : TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM);
    unsigned int buff_offset_hi_2ph =
        (streams_per_core == TWO) ? ((TP_FUNCT_TYPE == 1) ? ONE_PHASE_BUFFOFFSET_HI_CONV : ONE_PHASE_BUFFOFFSET_HI_CORR)
                                  : ((TP_FUNCT_TYPE == 1) ? TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                          : TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM);

    // offset to shuffle for phases > 2
    unsigned int buff_offset_gt_2ph =
        (streams_per_core == TWO) ? ((TP_FUNCT_TYPE == 1) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_TWO_STREAM
                                                          : GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_TWO_STREAM)
                                  : ((TP_FUNCT_TYPE == 1) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CONV_SINGLE_STREAM
                                                          : GREATERTHAN_TWO_PHASE_BUFFOFFSET_LOW_CORR_SINGLE_STREAM);
    unsigned int buff_offset_hi_gt_2ph =
        (streams_per_core == TWO) ? ((TP_FUNCT_TYPE == 1) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_TWO_STREAM
                                                          : GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_TWO_STREAM)
                                  : ((TP_FUNCT_TYPE == 1) ? GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CONV_SINGLE_STREAM
                                                          : GREATERTHAN_TWO_PHASE_BUFFOFFSET_HIGH_CORR_SINGLE_STREAM);

    unsigned int buff_offset =
        (TP_PHASES > TWO) ? buff_offset_gt_2ph : ((TP_PHASES == TWO) ? buff_offset_2ph : buff_offset_1ph);
    unsigned int buff_offset_hi =
        (TP_PHASES > TWO) ? buff_offset_hi_gt_2ph : ((TP_PHASES == TWO) ? buff_offset_hi_2ph : buff_offset_hi_1ph);
    unsigned int numrd = (TP_PHASES > TWO) ? (TWO / streams_per_core) : 1;

    buff_vector buff[num_buff_req];
    buff_vector shuffle_buff[num_buff_req];
    t_vect gdata_temp;

    for (int i = 0; i < buff_ptr_inc; i++) chess_prepare_for_pipelining chess_loop_range(EIGHT, ) {
            if (TP_FUNCT_TYPE == 1) {
                ppin = ppin_alias + i;
            } else {
                ppin = ppin_alias + (buff_ptr_inc - 1 - i);
            }
            for (int j = 0; j < num_buff_req; j++) {
                buff[j].insert(0, *ppin);
                ppin = ppin + buff_ptr_inc;
                buff[j].insert(1, *ppin);
                ppin = ppin + buff_ptr_inc;
                buff[j].insert(TWO, *ppin);
                ppin = ppin + buff_ptr_inc;
                buff[j].insert(THREE, *ppin);
                ppin = ppin + buff_ptr_inc;
                shuffle_buff[j] = shuffle16(buff[j], 0, buff_offset, buff_offset_hi);
                if (TP_FUNCT_TYPE == 0) {
                    shuffle_buff[j] = (::aie::conj(shuffle_buff[j]));
                }
            }

            for (int j = 0; j < num_buff_req / numrd; j++) {
                for (int k = 0; k < max_buf_idx; k++) {
                    for (int l = 0; l < numrd; l++) {
                        if (TP_FUNCT_TYPE == 1) {
                            gdata_temp = shuffle_buff[j * numrd + l].template extract<FOUR>(k);
                        } else {
                            gdata_temp = shuffle_buff[(num_buff_req / numrd - 1 - j) * numrd + (numrd - 1 - l)]
                                             .template extract<FOUR>((max_buf_idx - 1) - k);
                        }
                        *ppout0++ = gdata_temp;
                    }
                }
            }
        }
};
#endif

} // namespace conv_corr {
} // namespace aie {
} // namespace dsp {
} // namespace xf {

#endif // _DSPLIB_CONV_CORR_UTILS_HPP_