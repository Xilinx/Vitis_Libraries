/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_dft_TRAITS_HPP_
#define _DSPLIB_dft_TRAITS_HPP_

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#define TRUNC(x, y) (((x) / y) * y)

/*
DFT traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {
//-------------------------------------
// app-specific constants
static constexpr unsigned int kMaxColumns = 2;
static constexpr unsigned int kUpdWSize = 32; // Upd_w size in Bytes (256bit) - const for all data/coeff types

//-------------------------------
// I/O types
// template<typename T_D> struct T_inputIF   {};
// template<> struct T_inputIF<int16 >  {input_window<int16 >* inWindow;};
// template<> struct T_inputIF<int32 >  {input_window<int32 >* inWindow;};
// template<> struct T_inputIF<cint16>  {input_window<cint16>* inWindow;};
// template<> struct T_inputIF<cint32>  {input_window<cint32>* inWindow;};
// template<> struct T_inputIF<float >  {input_window<float >* inWindow;};
// template<> struct T_inputIF<cfloat>  {input_window<cfloat>* inWindow;};

// template<typename T_D> struct T_outputIF   {};
// template<> struct T_outputIF<int16 >  {output_window<int16 >* outWindow;};
// template<> struct T_outputIF<int32 >  {output_window<int32 >* outWindow;};
// template<> struct T_outputIF<cint16>  {output_window<cint16>* outWindow;};
// template<> struct T_outputIF<cint32>  {output_window<cint32>* outWindow;};
// template<> struct T_outputIF<float >  {output_window<float >* outWindow;};
// template<> struct T_outputIF<cfloat>  {output_window<cfloat>* outWindow;};

//---------------------------------------
// Configuration Defensive check functions
template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDataType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<float>() {
    return true;
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
INLINE_DECL constexpr bool fnCheckTwiddleType<cfloat>() {
    return true;
};

template <typename TT_DATA, typename TT_TWIDDLE>
INLINE_DECL constexpr bool fnCheckDataTwiddleType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<int16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<int32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<float, cfloat>() {
    return true;
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
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cfloat, cfloat>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr bool fnCheckPointSize() {
    return (TP_POINT_SIZE >= 4 && TP_POINT_SIZE <= 128);
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
    cfloat retVal;

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};

// Chop the coeffs
template <typename TT_TWIDDLE, unsigned int TP_CASC_LEN, unsigned int kPaddedCoeffSize>
static std::vector<TT_TWIDDLE> chopping_function(const std::vector<TT_TWIDDLE> coeff,
                                                 unsigned int i,
                                                 unsigned int kernelStepSize) {
    std::vector<TT_TWIDDLE> subCoeff;
    for (unsigned int j = 0; j < kernelStepSize * kPaddedCoeffSize; j++) {
        unsigned int coeffIndex = j + i * kernelStepSize * kPaddedCoeffSize; // kernelStepSize*kPaddedCoeffSize
        subCoeff.push_back(coeff.at(coeffIndex));
    };
    return subCoeff;
}

/**
 * @endcond
 */

// Calculate dft range for cascaded kernel
template <unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN, int TP_Rnd = 1>
unsigned int fnDftRange(int TP_KP) {
    // TP_KP - Kernel Position
    return ((TRUNC(TP_POINT_SIZE, TP_CASC_LEN) / TP_CASC_LEN) +
            ((TP_POINT_SIZE - TRUNC(TP_POINT_SIZE, TP_CASC_LEN)) >= TP_Rnd * (TP_KP + 1) ? TP_Rnd : 0));
}

/**
 * @endcond
 */

// Used by the final kernel
template <unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN, int TP_Rnd = 1>
unsigned int fnDftRangeRem(int TP_KP) {
    // TP_KP - Kernel Position
    return ((TRUNC(TP_POINT_SIZE, TP_CASC_LEN) / TP_CASC_LEN) +
            ((TP_POINT_SIZE - TRUNC(TP_POINT_SIZE, TP_CASC_LEN)) % TP_Rnd));
}

/**
 * @endcond
 */

// Calculate dft range for cascaded kernel
template <unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN, int TP_Rnd = 1, unsigned int TP_KP>
static constexpr unsigned int dftRange() {
    // TP_KP - Kernel Position
    return ((TRUNC(TP_POINT_SIZE, TP_CASC_LEN) / TP_CASC_LEN) +
            ((TP_POINT_SIZE - TRUNC(TP_POINT_SIZE, TP_CASC_LEN)) >= TP_Rnd * (TP_KP + 1) ? TP_Rnd : 0));
}

/**
 * @endcond
 */

// Used by the final kernel
template <unsigned int TP_POINT_SIZE, unsigned int TP_CASC_LEN, int TP_Rnd = 1, unsigned int TP_KP>
static constexpr unsigned int dftRangeRem() {
    // TP_KP - Kernel Position
    return ((TRUNC(TP_POINT_SIZE, TP_CASC_LEN) / TP_CASC_LEN) +
            ((TP_POINT_SIZE - TRUNC(TP_POINT_SIZE, TP_CASC_LEN)) % TP_Rnd));
}
}
}
}
}
}

#endif // _DSPLIB_dft_TRAITS_HPP_
