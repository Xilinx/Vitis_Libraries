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
#ifndef _DSPLIB_CONV_CORR_TRAITS_HPP_
#define _DSPLIB_CONV_CORR_TRAITS_HPP_

/*
    Convolution and Correlation traits.
    This file contains sets of overloaded, templatized and specialized templatized functions which
    encapsulate properties of the intrinsics used by the main kernal class. Specifically,
    this file does not contain any vector types or intrinsics since it is required for construction
    and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include <stdio.h>
#include <type_traits>
#include <adf.h>
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "fir_utils.hpp"
#include "device_defs.h"

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

// conv/corr compute modes
#define FULL_MODE 0
#define SAME_MODE 1
#define VALID_MODE 2

// Function_Type
#define CONV 1
#define CORR 0

// RTP related flags
#define USE_RTP_VECTOR_LENGTHS_FALSE 0
#define USE_RTP_VECTOR_LENGTHS_TRUE 1

// Stream Related defines
#define SINGLE_PHASE 1
#define TWO_PHASES 2
#define THREE_PHASE 3
#define FOUR_PHASES 4

#define SINGLE_STREAM 1
#define TWO_STREAMS 2
#define MAC4ROTDELAY 3

// Different Buffer offsets for different num of phases
#define BASE_OFFSET_OF_SELECT 0x0u
#define TWO_PHASES_TWO_STREAMS_OFFSET_OF_SELECT 0x5555AAAAu
#define FOUR_PHASES_TWO_STREAMS_OFFSET_OF_SELECT 0x55AA55AAu
#define ONLY_TWO_STREAMS_OFFSET_OF_SELECT 0x5A5A5A5Au

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

#define INT16_ONE_PHASE_XBUFFOFFSET_LOW_CONV 0x06040200u
#define INT16_ONE_PHASE_XBUFFOFFSET_LOW_CORR 0x04060002u
#define INT16_ONE_PHASE_XBUFFOFFSET_HIGH_CONV 0x0E0C0A08u
#define INT16_ONE_PHASE_XBUFFOFFSET_HIGH_CORR 0x0C0E080Au
#define INT16_ONE_PHASE_XSQUARE_CONV 0x3210u
#define INT16_ONE_PHASE_XSQUARE_CORR 0x0123u

#define INT16_TWO_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM 0x33323130
#define INT16_TWO_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM 0x32333031
#define INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM 0x37363534
#define INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM 0x36373435

#define INT16_TWO_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x1C181410
#define INT16_TWO_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x191D1115
#define INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x1D191511
#define INT16_TWO_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x181C1014
#define INT16_XSQUARE_CORR 0x1032u

#define INT16_TWO_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x1D191511
#define INT16_TWO_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x181C1014
#define INT16_TWO_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x1C181410
#define INT16_TWO_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x191D1115
#define INT16_YSQUARE_CONV 0x2301u
#define INT16_YSQUARE_CORR 0x0123u

#define INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM 0x76747270
#define INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM 0x74767072
#define INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM 0x77757371
#define INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM 0x75777173

#define INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x3A323830
#define INT16_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0X3038323A
#define INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x3B333931
#define INT16_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x3139333B

#define INT16_FOUR_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x38303A32
#define INT16_FOUR_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x323A3038
#define INT16_FOUR_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x39313B33
#define INT16_FOUR_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x333B3139

#define INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_TWO_STREAM 0x39313830
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_TWO_STREAM 0x31393038
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_TWO_STREAM 0x3B333A32
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_TWO_STREAM 0x333B323A

#define INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x75717470
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x71757074
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x77737672
#define INT16_G_FOUR_PHASE_XBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x73777276

#define INT16_G_FOUR_PHASE_YBUFFOFFSET_LOW_CONV_SINGLE_STREAM 0x71757074
#define INT16_G_FOUR_PHASE_YBUFFOFFSET_LOW_CORR_SINGLE_STREAM 0x75717470
#define INT16_G_FOUR_PHASE_YBUFFOFFSET_HIGH_CONV_SINGLE_STREAM 0x73777276
#define INT16_G_FOUR_PHASE_YBUFFOFFSET_HIGH_CORR_SINGLE_STREAM 0x77737672

#define INT16_G_FOUR_PHASE_SELECT32_SINGLE_STREAM_BASE 0x02020000
#define INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CONV 0xF0F0F0F0
#define INT16_G_FOUR_PHASE_SINGLE_STREAM_LANE_SELECT_CORR 0x0F0F0F0F
#define INT16_G_FOUR_PHASE_SELECT32_OFFSET 0x04040404

#define INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CONV_BASE 0x03020100
#define INT16_G_FOUR_PHASE_SELECT32_TWO_STREAM_CORR_BASE 0x01000302
#define INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CONV 0xCCCCCCCC
#define INT16_G_FOUR_PHASE_TWO_STREAM_LANE_SELECT_CORR 0x33333333

#define ROUND(X, Y) (((X % Y) > ((Y + 1) / 2)) ? ((int)(X + Y - 1) / (int)Y) : ((int)X / (int)Y))

// using namespace std;
namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

static constexpr unsigned int kMaxBitsRegY = 1024;                            // Max number of bits on Y Register.
static constexpr unsigned int kMaxBitsRegX = 512;                             // Max number of bits on X Register.
static constexpr unsigned int kMaxBitsRegW = 256;                             // Max number of bits on W Register.
static constexpr unsigned int kMaxBitsRegV = 128;                             // Max number of bits on V Register.
static constexpr unsigned int kMaxBitsLoadOnAie = 256;                        // Max number of bits Load on AIE
static constexpr unsigned int kMaxBytesLoadOnAie = 32;                        // Max number of bytes (256/8) Load on AIE
static constexpr unsigned int kMaxBufferLenOnAieInBytes = __DATA_MEM_BYTES__; // Max buffer Length
static constexpr unsigned int kBuffSize16Byte = 16;                           // 128-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;                           // 256-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;                           // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize128Byte = 128;                         // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffIndx1 = 1;                                 // updW index to update the vector
static constexpr unsigned int kBuffIndx2 = 2;                                 // updW index to update the vector

//**************************************************************************************/
//****   STREAM Related traits which were used when TP_API=1 and AIE_VARIANT=1      ****/
//**************************************************************************************/
static constexpr unsigned int kMaxNumOfPhases =
    16; // Max number of Phases supported by the design of stream processing conv/corr
static constexpr unsigned int kDataBuffLenFactor = 4; // DataBuffLenFactor for stream based conv/corr on AIE-1
static constexpr unsigned int kMinDataBuffLen = 16;   // MinDataBuffLen for stream based conv/corr on AIE-1
static constexpr unsigned int kShiftFactor2 = 2;      // MulFactor2
static constexpr unsigned int kMaxNumOfStreams = 2;   // Maximum Number Of Streams is 2 on AIE-1 for conv/corr
static constexpr unsigned int kStreamSupportsOnlyValidMode =
    2; // Stream Processing supports on AIE-1 only with valid(2) mode only.
static constexpr unsigned int kMaxSamplesInShuffleVec = 16; // Max number of samples to use shuffle() operation is 16
static constexpr unsigned int kMinSamplesInShuffleVec = 4;  // Min number of samples to use shuffle() operation is 4
static constexpr unsigned int kStreamPhaseOffset = 0x3210u; // Base offset of Phase is 0x3210U
static constexpr unsigned int kMaxIndexOf32ByteVector = 4;  // |4|3|2|1| ===> 32byte vector -- Max index number is 4
static constexpr unsigned int kMaxIndexOf16ByteVector = 2;  // |2|1|     ===> 16byte vector -- Max index number is 2
static constexpr unsigned int kNumPoints2 = 2;              // Num of points is 2 for stream processing.
static constexpr unsigned int kNumPoints4 = 4;              // Num of points is 4 for stream processing
static constexpr int kXstepOfMac4Rot = -4;               // xstep is -4 for mac4_rot() intrinsic as per current design.
static constexpr unsigned int kMinLenOfGforStream = 8;   // Min Len of G should be 8 while doing stream processing
static constexpr unsigned int kMaxLenOfGforStream = 256; // Max Len of G should be 8 while doing stream processing

// Function to check Len of G is in valid range or not while doing stream processing
template <unsigned int gLen>
INLINE_DECL constexpr bool fnCheckLenOfStreamforSigG() {
    return ((gLen >= kMinLenOfGforStream) && (gLen <= kMaxLenOfGforStream));
};

// Function to return the Lanes of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnNumOfLanesForMac4Rot() {
    return 4;
}; // defualt Lanes are 4

template <>
INLINE_DECL constexpr unsigned int fnNumOfLanesForMac4Rot<cint16>() {
    return 4;
};

// Function to return the Points of MAC4_ROT Intrinsic on AIE-1
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnNumOfPointsForMac4Rot() {
    return 2;
}; // defualt Points are 2

template <>
INLINE_DECL constexpr unsigned int fnNumOfPointsForMac4Rot<cint16>() {
    return 2;
};

// ************************************** ************ //
// ****      END of stream related traits      ******* //
// ************************************** ************ //

// Function to return the default lanes based on choosen architecture
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnNumOfLanes() {
    return 32; // defualt return for AIE2
};

// Function to return the default MULS/MACS based on choosen architecture
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnNumOfMuls() {
    return 256; // defualt return for AIE2
};

// Function to return the default sample size of F Sig
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF() {
    return 8; // defualt sample size is 8
};

// Function to return the default sample size of G Sig
template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG() {
    return 8; // defualt sample size is 8
};

// Function to return the max sample size of G Signal
template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnSizeOfSample() {
    return 32; // defualt multiple factor is 32 for both AIE-1 and AIE-2
};

// Function to return the max sample size of G Signal
template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int fnSizeOfDataType() {
    return 2; // defualt multiple factor is 32 for both AIE-1 and AIE-2
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int8, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint32, int16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cfloat, cfloat>() {
    return 4;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int8, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int16, int8>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int16, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<float, float>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<float, cfloat>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cfloat, float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cfloat, cfloat>() {
    return 4;
}; //

#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
// AIE-2
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int8, int8>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int16, int8>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<int32, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, int32>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint16, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfLanes<cint32, cint16>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int8, int8>() {
    return 256;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int16, int8>() {
    return 128;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<int32, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<bfloat16, bfloat16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, int16>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, int32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint16, cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint32, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumOfMuls<cint32, cint16>() {
    return 8;
}; //

#endif

// function to return number of samples for given data type of F Sig
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<cfloat>() {
    return 64;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigF<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type of G Sig
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<cfloat>() {
    return 64;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfSigG<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<int8>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<float>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<cint16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<cint32>() {
    return 64;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<cfloat>() {
    return 64;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSizeOfSample<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<int8>() {
    return 1;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<int16>() {
    return 2;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<int32>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<float>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<cint32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<cfloat>() {
    return 8;
}; //
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSizeOfDataType<bfloat16>() {
    return 2;
}; //
#endif

// Function returns number of columns used by MUL/MACs in Conv_Corr
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int getNumofMULs() {
    return fnNumOfMuls<TT_DATA_F, TT_DATA_G>();
};

// Function to return the number of lanes based on given data types
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int getNumofLanes() {
    return fnNumOfLanes<TT_DATA_F, TT_DATA_G>();
};

// Function to return the number of points based on given data types
template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr unsigned int getNumofPoints() {
    return (fnNumOfMuls<TT_DATA_F, TT_DATA_G>() / fnNumOfLanes<TT_DATA_F, TT_DATA_G>());
};

// Function to return the number of points based on given data types
template <typename TT_DATA_F>
INLINE_DECL constexpr unsigned int getNumSamplesFsig() {
    return fnSampleSizeOfSigF<TT_DATA_F>();
};

// Function to return the number of G Sig. samples to read based on given data types
template <typename TT_DATA_G>
INLINE_DECL constexpr unsigned int getNumSamplesGsig() {
    return fnSampleSizeOfSigG<TT_DATA_G>();
};

// Function to return the resultant value of Log base 2
template <unsigned int inLength>
INLINE_DECL constexpr unsigned int getLog2() {
    switch (inLength) {
        case 4:
            return 2;
            break;
        case 8:
            return 3;
            break;
        case 16:
            return 4;
            break;
        case 32:
            return 5;
            break;
        case 64:
            return 6;
            break;
        case 128:
            return 7;
            break;
        case 256:
            return 8;
            break;
        case 512:
            return 9;
            break;
        case 1024:
            return 10;
            break;
        case 2048:
            return 11;
            break;
        case 4096:
            return 12;
            break;
        case 8192:
            return 13;
            break;
        case 16384:
            return 14;
            break;
        case 32768:
            return 15;
            break;
        case 65536:
            return 16;
            break;
        default:
            printf("Error in PowerofLen\n");
            return 0;
            break;
    }
};

// Function to return the offset to move G Sig Pointer.
template <typename TT_DATA_G, unsigned int dataLenG>
INLINE_DECL constexpr unsigned int getOffsetGsig() {
    unsigned int gLoad = (kMaxBitsLoadOnAie / fnSampleSizeOfSigG<TT_DATA_G>());
    return ((dataLenG / gLoad) - 1);
};

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return (kMaxBufferLenOnAieInBytes >> kShiftFactor2 / sizeof(TT_DATA));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return (((kMaxBitsLoadOnAie << 1) / fnSizeOfSample<TT_DATA>()));
};

// Function to return true or false by checking given length is in range or not
template <typename TT_DATA, unsigned int sigLen, unsigned int TP_API>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (kMaxBitsLoadOnAie / fnSizeOfSample<TT_DATA>());
    bool checkLen = false;

    if ((sigLen >= getMinLen<TT_DATA>()) && (sigLen <= getMaxLen<TT_DATA>())) {
        checkLen = (((sigLen % minDataLoad) == 0) ? true : false);
    }
    return (TP_API == USE_STREAM_API) ? true : checkLen;
};

// Function to return the paddedLength based on given data length and compute mode
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int computeMode,
          unsigned int dataLenF,
          unsigned int dataLenG>
INLINE_DECL constexpr unsigned int getPaddedLength() {
    unsigned int paddedLength = 0;
    unsigned int lanes = fnNumOfLanes<TT_DATA_F, TT_DATA_G>();
    unsigned int dataSamples = fnSampleSizeOfSigF<TT_DATA_F>();
    unsigned int dataLoad = kMaxBitsLoadOnAie / dataSamples;

    if (computeMode == FULL_MODE) // Full
    {
        paddedLength = ((dataLenG - 1) + dataLenF + (dataLenG - 1));
        paddedLength = CEIL(paddedLength, dataLoad);
    } else if (computeMode == SAME_MODE) // Same
    {
        paddedLength = (((dataLenG >> 1) - 1) + dataLenF + ((dataLenG >> 1) - 1));
        paddedLength = CEIL(paddedLength, dataLoad);
    } else if (computeMode == VALID_MODE) {
        if (lanes > dataLoad) // Valid
        {
            paddedLength = (dataLenF + (dataLoad << 1));
            paddedLength = CEIL(paddedLength, dataLoad);
        } else {
            paddedLength = (dataLenF + dataLoad);
            paddedLength = CEIL(paddedLength, dataLoad);
        }
    } else {
        // do nothing
    }

    return paddedLength;
};

// Function to return the LoopCount based on given data length and compute mode
template <unsigned int computeMode, unsigned int dataLenF, unsigned int dataLenG>
INLINE_DECL constexpr unsigned int getLoopCount() {
    unsigned int loopCount = 0;
    if (computeMode == FULL_MODE) // Full
    {
        loopCount = ((dataLenF + dataLenG) - 1);
    } else if (computeMode == SAME_MODE) // Same
    {
        loopCount = dataLenF;

    } else if (computeMode == VALID_MODE) // Valid
    {
        loopCount = ((dataLenF - dataLenG) + 1);
    } else {
        loopCount = ((dataLenF + dataLenG) - 1); // default Full mode
    }

    return loopCount;
};

// 8x8 16x8 16x16 32x16 c16x16 c16x32 c16xc16 c32x16 c32xc16
// Configuration Defensive check functions
//----------------------------------------------------------------------

// Check Output data type whether its complex if any input is complex
template <typename TT_DATA_F, typename TT_DATA_G, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataOutType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int8, int8, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int16, int8, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int16, int16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<int32, int16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, cfloat>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, cfloat>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, int32, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint16, cint16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, int16, int16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cint32, cint16, int64>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, float>() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, bfloat16, float>() {
    return true;
};
#endif

template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer() {
    return false;
};

template <typename TT_DATA_F, typename TT_DATA_G>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int8, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, cfloat>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, int16>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, cint16>() {
    return true;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int8, int8>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<int32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<bfloat16, bfloat16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, float>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfIoBuffer<cfloat, cfloat>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, int16>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckInDataTypesOfStream<cint16, cint16>() {
    return false;
};
#endif

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckInputDataTypes() {
    return ((TP_API == USE_WINDOW_API) ? fnCheckInDataTypesOfIoBuffer<TT_DATA_F, TT_DATA_G>()
                                       : fnCheckInDataTypesOfStream<TT_DATA_F, TT_DATA_G>());
}

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_F, typename TT_DATA_G, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataTypeOfOutput() {
    return (fnCheckDataOutType<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>() ? true : false);
}

// Configuration Defensive check function to check Length of F Signal and G Signal
template <typename TT_DATA, unsigned int sigLen, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckLenOfData() {
    return (isLenInRange<TT_DATA, sigLen, TP_API>() ? true : false);
}

// Configuration Defensive check function to check whether strem process supported by AIE-1 or AIE-2
template <unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsStreamSupportedbyArch() {
    if (TP_API == USE_STREAM_API) {
#if (__HAS_ACCUM_PERMUTES__ == 1)
        return true;
#elif (__HAS_ACCUM_PERMUTES__ == 0)
        return false;
#endif
    } else {
        return true;
    }
};

// Function which return true or false if Given Number is power of 2 or not
template <unsigned int inData>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((inData) && !(inData & (inData - 1))) ? true : false);
};

// Configuration Defensive check function to check numPhases which should be power of 2
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int gLen,
          unsigned int cascLen,
          unsigned int numPhases,
          unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckPhases() {
    bool isphasesValid = false;
    unsigned int muls = fnNumOfMuls<TT_DATA_F, TT_DATA_G>();
    unsigned int numOfCores4 =
        (gLen / muls); // muls are 8 Complex MACS for eg: 32(G_Len)/8(CMACS) = 4 is the Max CascLen (4 cores)

    // --------------------------------------------
    // IO-Buffer : Always Num of Phases must be 1.
    // --------------------------------------------
    // Stream Processing: Num of Phases can be increased only when each cascade stream has maximum data rate i.e with
    // maximum cascade length (4)
    //                    To achieve data rate less than 1GSPS, Cascade length parameter can be decreased in a single
    //                    phase design
    //                    To achieve data rate more than 1GSPS, NUM_PHASES parameter should be increased by keeping
    //                        the CASC_LEN parameter to its maximum possible value (i.e. the value required to achieve
    //                        1GSPS when NUM_PHASES=1)
    //---------------------------------------------
    if (TP_API == USE_STREAM_API && numPhases <= kMaxNumOfPhases && isPowerOfTwo<numPhases>()) {
        isphasesValid = ((cascLen == numOfCores4) && (numPhases > 1)) ? true : ((numPhases == 1) ? true : false);
    } else {
        isphasesValid = ((TP_API == USE_WINDOW_API) && (numPhases == 1)) ? true : false;
    }
    return isphasesValid;
};

// Configuration Defensive check function to check whether computeMode is VALID or not for stream processing
template <unsigned int computeMode, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckIsComputeModeValid() {
    return (((TP_API == USE_STREAM_API) && (computeMode != kStreamSupportsOnlyValidMode))
                ? false
                : ((computeMode > kStreamSupportsOnlyValidMode) ? false : true));
};

// Configuration Defensive check function to check Length of G Signal should be multiple
// of ((phases*lanes)*(Points/streamsPerCore)) when stream processing happening
template <typename TT_DATA_F, unsigned int fLen, unsigned int gLen, unsigned int numPhases, unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckGLen() {
    bool isLenOfGvalid = true;
    bool isGlenGreaterThanFlen = (gLen > fLen) ? true : false;

    if (TP_API == USE_STREAM_API) {
        if ((isGlenGreaterThanFlen) && (!(fnCheckLenOfStreamforSigG<gLen>()))) {
            isLenOfGvalid = false;

        } else {
            if ((isGlenGreaterThanFlen) && (gLen < (numPhases << kShiftFactor2))) {
                isLenOfGvalid = false;

            } else if (gLen > (numPhases << kShiftFactor2)) {
                if ((isGlenGreaterThanFlen) && (gLen % (numPhases << (kShiftFactor2 + 1)) != 0)) {
                    isLenOfGvalid = false;
                }
            }
        }
    } else // Check for gLen when TP_API = USE_WINDOW_API (I/O BUFFER)
    {
        if (isGlenGreaterThanFlen) // gLen <= fLen
        {
            isLenOfGvalid = false;
        }
    }
    return isLenOfGvalid;
};

//  Configuration Defensive check function to check whether Glen/casc_len should be multiple of lanes*points
template <typename TT_DATA_F,
          typename TT_DATA_G,
          unsigned int gLen,
          unsigned int cascLen,
          unsigned int numPhases,
          unsigned int TP_API>
INLINE_DECL constexpr bool fnCheckCascLen() {
    bool isCascLenValid = false;
    unsigned int muls = fnNumOfMuls<TT_DATA_F, TT_DATA_G>(); // Num of CMACS are 8 per core on AIE-1.

    // CASCLEN is the parameter which will tell that how many cores are required to achieve maximum throughput i.e.,
    // 1GSPS
    unsigned int numOfCores4 =
        (gLen / muls); // muls are 8 Complex MACS for eg: 32(G_Len)/8(CMACS) = 4 is the Max CascLen (4 cores)
    unsigned int numOfCores2 =
        (gLen / (muls << 1)); // muls are 8 Complex MACS for eg: 32(G_Len)/16(CMACS) = 2 is the CascLen (2 cores)
    unsigned int numOfCores1 =
        (gLen /
         (muls << kShiftFactor2)); // muls are 8 Complex MACS for eg: 32(G_Len)/32(CMACS) = 1 is the CascLen (1 core)
    isCascLenValid =
        (TP_API == USE_STREAM_API)
            ? (((cascLen == (numOfCores4)) || (cascLen == (numOfCores2)) || (cascLen == (numOfCores1))) ? true : false)
            : true;

    return isCascLenValid;
};

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_TRAITS_HPP_