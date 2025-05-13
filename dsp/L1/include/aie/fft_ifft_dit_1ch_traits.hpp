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
#ifndef _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_
#define _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_

#include "device_defs.h"

/*
FFT traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
//-------------------------------------
// app-specific constants
static constexpr unsigned int kPointSizeMin = 16;
static constexpr unsigned int kPointSizeMax = 4096;
static constexpr unsigned int kMaxColumns = 2;
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kMaxPointSize = 4096;
static constexpr unsigned int kMaxPointLog = 12;

// return accumulator data type for aie-ml
template <typename T_D>
struct t_accType {
    using type = cacc64;
};
template <>
struct t_accType<cfloat> {
    using type = cfloat;
};
//-------------------------------
// I/O types
template <typename T_D>
struct T_inputIF {};
template <>
struct T_inputIF<int16> {
    input_window<int16>* inWindow;
};
template <>
struct T_inputIF<int32> {
    input_window<int32>* inWindow;
};
template <>
struct T_inputIF<cint16> {
    input_window<cint16>* inWindow;
};
template <>
struct T_inputIF<cint32> {
    input_window<cint32>* inWindow;
};
template <>
struct T_inputIF<float> {
    input_window<float>* inWindow;
};
template <>
struct T_inputIF<cfloat> {
    input_window<cfloat>* inWindow;
};

template <typename T_D>
struct T_outputIF {};
template <>
struct T_outputIF<int16> {
    output_window<int16>* outWindow;
};
template <>
struct T_outputIF<int32> {
    output_window<int32>* outWindow;
};
template <>
struct T_outputIF<cint16> {
    output_window<cint16>* outWindow;
};
template <>
struct T_outputIF<cint32> {
    output_window<cint32>* outWindow;
};
template <>
struct T_outputIF<float> {
    output_window<float>* outWindow;
};
template <>
struct T_outputIF<cfloat> {
    output_window<cfloat>* outWindow;
};

//---------------------------------------
// Configuration Defensive check functions
template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDataType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cfloat>() {
    return true;
};

template <typename TT_IN_DATA, typename TT_OUT_DATA>
INLINE_DECL constexpr bool fnCheckDataIOType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint32, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cfloat, cfloat>() {
    return true;
};

template <typename TT_TWIDDLE>
INLINE_DECL constexpr bool fnCheckTwiddleType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckTwiddleType<cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckTwiddleType<cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckTwiddleType<cfloat>() {
    return true;
};

template <typename TT_DATA, typename TT_TWIDDLE>
INLINE_DECL constexpr bool fnCheckDataTwiddleType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint32, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cfloat, cfloat>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr bool fnCheckPointSize() {
    return (
        // AIE1 outputs vectors of 4, so radix4 outputs 16 samples.
        // AIEML outputs vectors of 8, so the minimum point size is 32.
        TP_POINT_SIZE >= __FFT_MIN_VECTORIZATION__ &&
        (TP_POINT_SIZE == 16 || TP_POINT_SIZE == 32 || TP_POINT_SIZE == 64 || TP_POINT_SIZE == 128 ||
         TP_POINT_SIZE == 256 || TP_POINT_SIZE == 512 || TP_POINT_SIZE == 1024 || TP_POINT_SIZE == 2048 ||
         TP_POINT_SIZE == 4096));
};

template <unsigned int TP_SHIFT>
INLINE_DECL constexpr bool fnCheckShift() {
    return (TP_SHIFT >= 0) && (TP_SHIFT <= 60);
};

template <typename TT_DATA, unsigned int TP_SHIFT>
INLINE_DECL constexpr bool fnCheckShiftFloat() {
    return !(std::is_same<TT_DATA, cfloat>::value) || // This check traps shift != 0 when data = cfloat
           (TP_SHIFT == 0);
};

template <typename TT_DATA, unsigned int RANKS, unsigned int TP_CASC_LEN>
INLINE_DECL constexpr bool fnCheckCascLen() {
    // equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
    return (TP_CASC_LEN > 0) &&
           (std::is_same<TT_DATA, cfloat>::value ? (TP_CASC_LEN <= RANKS) : (TP_CASC_LEN <= (RANKS + 1) / 2));
}

template <typename TT_DATA, unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN>
INLINE_DECL constexpr bool fnCheckCascLen2() {
    return true;
    // The worry here was that since cfloat 16pt requires special buffering, it will not yield to cascade, but
    // all cascade configurations possible will not run into the issue of buffer overwrite involved.
    return (TP_CASC_LEN == 1) || (!std::is_same<TT_DATA, cfloat>::value) || (TP_POINT_SIZE != 16);
}

// End of Defensive check functions

//---------------------------------------------------
// Functions

template <typename TT_DATA, typename TT_TWIDDLE, unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr int fnHeapSize() {
    int retVal = 0;
    int buffsize = 0;
    // cfloat twiddles are cfloat size and cam not use half table trick.
    if (std::is_same<TT_DATA, cfloat>::value) {
        switch (TP_POINT_SIZE) {
            case 16:
                retVal += 5 * 32;
                break; // tw1, tw2, tw4, tw8 all 32 byte aligned. tw 8 is 64 long.
            case 32:
                retVal += 5 * 32 + 128;
                break; // as above, plus tw16 (128)
            case 64:
                retVal += 32 + 4 * 32 + 128 + 256;
                break; // Note the pattern. This is 512 +32.
            case 128:
                retVal += 32 + 1024;
                break; // and so on.
            case 256:
                retVal += 32 + 2048;
                break;
            case 512:
                retVal += 32 + 4096;
                break;
            case 1024:
                retVal += 32 + 8192;
                break;
            case 2048:
                retVal += 32 + 16384;
                break;
            case 4096:
                retVal += 32 + 32768;
                break; // clearly not possible as this is > 32k.
            default:
                retVal = 0;
                break;
        }
    } else {
        switch (TP_POINT_SIZE) {
            case 16:
                retVal += 4 * 32;
                break; // tw1, tw2, tw4, tw8 all 32 byte aligned.
            case 32:
                retVal += 4 * 32 + 64 / 2;
                break; // as above, +tw16(half)
            case 64:
                retVal += 4 * 32 + 64 + 128 / 2;
                break; // as above, but with tw16(full) and tw32(half)
            case 128:
                retVal += 4 * 32 + 192 + 256 / 2;
                break; // and so on.
            case 256:
                retVal += 4 * 32 + 512 - 64 + 256;
                break;
            case 512:
                retVal += 4 * 32 + 768 - 64 + 512;
                break;
            case 1024:
                retVal += 4 * 32 + 1536 - 64 + 1024;
                break;
            case 2048:
                retVal += 4 * 32 + 3072 - 64 + 2048;
                break;
            case 4096:
                retVal += 4 * 32 + 6144 - 64 + 4096;
                break;
            default:
                retVal = 0;
                break;
        }
    }
    retVal += 1024; // sundry items

    buffsize = TP_POINT_SIZE > 128 ? TP_POINT_SIZE : 128; // 128 is minimum point size
    retVal += sizeof(cint32) * buffsize;                  // internal buffer - always 8 bytes per samples
    if (std::is_same<TT_DATA, cint16>::value) {
        retVal += sizeof(cint32) * buffsize; // internal buffer - cint16 requires 2 copies
    }

    return retVal;
};

// To reduce Data Memory required, the output iobuffer can be re-used as a temporary buffer of samples,
// but only when the internal type size is the same as the input type size and when the number of stages for the kernel
// is even
template <typename TT_D, int T_START_RANK, int T_END_RANK, int T_DYN>
INLINE_DECL constexpr bool fnReuseOutputBuffer() {
    if (T_DYN == 1) {
        return false;
    }
    if (std::is_same<TT_D, cint16>::value) {
        return false;
    } else if (std::is_same<TT_D, cfloat>::value) {
        return (T_END_RANK - T_START_RANK) % 2 == 1; // cfloat stages are radix2, stages = ranks.
    } else if (std::is_same<TT_D, cint32>::value) {
        return (T_END_RANK / 2 - T_START_RANK / 2) % 2 == 1; // cint32 stages are radix4, so stages = ranks/2
    }
};

// To reduce Data Memory required, the input window can be re-used as a temporary buffer of samples,
// but only when the internal type size is the same as the input type size
template <typename TT_DATA>
INLINE_DECL constexpr bool fnUsePingPongIntBuffer() {
    return false;
}; // only cint16 requires second internal buffer
template <>
INLINE_DECL constexpr bool fnUsePingPongIntBuffer<cint16>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr int fnPointSizePower() {
    return 0;
};
template <>
INLINE_DECL constexpr int fnPointSizePower<16>() {
    return 4;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<32>() {
    return 5;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<64>() {
    return 6;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<128>() {
    return 7;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<256>() {
    return 8;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<512>() {
    return 9;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<1024>() {
    return 10;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<2048>() {
    return 11;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<4096>() {
    return 12;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<8192>() {
    return 13;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<16384>() {
    return 14;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<32768>() {
    return 15;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<65536>() {
    return 16;
}

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr int fnOddPower() {
    return 0;
};
template <>
INLINE_DECL constexpr int fnOddPower<32>() {
    return 1;
}
template <>
INLINE_DECL constexpr int fnOddPower<128>() {
    return 1;
}
template <>
INLINE_DECL constexpr int fnOddPower<512>() {
    return 1;
}
template <>
INLINE_DECL constexpr int fnOddPower<2048>() {
    return 1;
}
template <>
INLINE_DECL constexpr int fnOddPower<8192>() {
    return 1;
}
template <>
INLINE_DECL constexpr int fnOddPower<32768>() {
    return 1;
}

template <int T_X, int T_Y>
INLINE_DECL constexpr int fnCeil() {
    return ((T_X + T_Y - 1) / T_Y) * T_Y;
};

//----------------------------------------------------------------------
// nullElem
template <typename T_RDATA>
INLINE_DECL T_RDATA nullElem() {
    return 0;
};

// Null cint16_t element
template <>
INLINE_DECL cint16_t nullElem() {
    cint16_t d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null cint32 element
template <>
INLINE_DECL cint32 nullElem() {
    cint32 d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null float element
template <>
INLINE_DECL float nullElem() {
    return 0.0;
};

// Null cint32 element
template <>
INLINE_DECL cfloat nullElem() {
    cfloat retVal = {0.0, 0.0};
    return retVal;
};
}
}
}
}
}

#endif // _DSPLIB_fft_ifft_dit_1ch_TRAITS_HPP_
