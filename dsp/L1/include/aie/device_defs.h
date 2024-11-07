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
#ifndef __DEVICE_DEFS__
#define __DEVICE_DEFS__

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#include <type_traits>

// The following include is the tools device traits library. The device_defs.h file is a stop-gap means to derive device
// traits before
// the tools library existed
//#include <adf/arch/aie_arch_properties.hpp> //This cannot be included here because this file is used by ref models

// Preamble
// This file exists to make preprocessor clauses more readable by abstracting the trait of the device that the clause
// depends on
// rather than some oblique reference to the device.
// This file is used by both UUT and REF.
// Glossary:
// __AIE_ARCH__ can take values 10 (AIE1), 20 (AIE2, aka AIE-ML), 21 (AIE_PS), 22 (AIE_PS2)
// Even though most of these could be expressed in a boolean fashion, the convention is to use int
// so that in the UUT code you can write #if __X__ == 1 || __Y__ ==2 which becomes awkward with defined(__Y__).

// Error trap for unidentified arch
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10) || (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || \
    (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
// all ok
#else
//#error Unexpected __AIE_ARCH__ or __AIEARCH__ encountered //removed because compilation of gen_cfo.cpp fails here
#endif

#ifndef __AIE_ARCH__
// AIE Graph compilation graph_preprocessor is unaware of the ARCH.
// #define __AIE_ARCH__ 10
#endif

//----------------------------------
// FFT R4 stage support. AIE2 supports true Radix4, but AIE1 spoofs this with 2 stages of radix2.
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __FFT_R4_IMPL__ 0
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __FFT_R4_IMPL__ 1
#endif

//----------------------------------
// FFT vectorization.
#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIEARCH__ == 20) || (__AIEARCH__ == 21)
#define __FFT_MIN_VECTORIZATION__ 32
#elif (__AIE_ARCH__ == 22) || (__AIEARCH__ == 22)
#define __FFT_MIN_VECTORIZATION__ 64
#else
#define __FFT_MIN_VECTORIZATION__ 16
#endif

//----------------------------------
// FFT cfloat support (native)
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_CFLOAT__ 1
// #warning Supports cfloats

#else
#define __SUPPORTS_CFLOAT__ 0
// #warning AIE Architecture does not supports cfloats
#endif

// cfloat support (emulated)
#if (__AIE_ARCH__ == 20) || (__AIEARCH__ == 20)
#define __SUPPORTS_EMULATED_CFLOAT__ 1
// #warning Supports emulated cfloats

#else
#define __SUPPORTS_EMULATED_CFLOAT__ 0
// #warning AIE Architecture does not supports cfloats
#endif

// FFT 32 bit twiddle support
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_32B_TW__ 1
#else
#define __SUPPORTS_32B_TW__ 0
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __HAS_SYM_PREADD__ 1
#else
#define __HAS_SYM_PREADD__ 0
#endif

//-----------------------------------------
// Mixed radix FFT vectorization
// The final stage of mixed radix FFT has to be radix4 for high performance. The butterfly has 4 legs
// each of 4 (AIE1) or 8 (AIE-ML) meaning the atom size for optimal performance is defined as follows.
// Failing this, lower performance, but function can be achieved using radix2 in place of radix4.
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __MRFFT_ATOM__ 16
#else
// Ideally this would NOT be an else. It is best if an unknown value of AIE_ARCH leaves MRFFT_ATOM
// undefined, but curiously, 2 independent ifs malfunctions for AIE1.

//#endif
//#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
//    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __MRFFT_ATOM__ 32
#endif

//----------------------------------
// Accumulator widths
// reference to accumulator widths which do not exist on a device leads to a compile time error. These definitions allow
// such code to be claused out prior to that stage of compilation.
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_ACC48__
#define __SUPPORTS_ACC80__
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __SUPPORTS_ACC32__
#define __SUPPORTS_ACC64__
#endif

//----------------------------------
// Stream support
// clauses getc_wss
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_GETC_WSS__
#endif

//----------------------------------
// Cascade support
// clauses put_mcd and get_scd_v8int32
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_PUT_MCD__
#endif

//__STEAMS_PER_TILE__ is not recommended for use.
// The recommendation is to use get_input_streams_core_module() from #include <adf/arch/aie_arch_properties.hpp>
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __STREAMS_PER_TILE__ 2
#elif (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __STREAMS_PER_TILE__ 1
#else
// error trap
#define __STREAMS_PER_TILE__ -1
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define MCD_SIZE 256
#define SCD_SIZE 256
#else
#define MCD_SIZE 512
#define SCD_SIZE 512
#endif

//----------------------------------
// Misc
// not equal
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_NE32__
#endif

//----------------------------------
// SINCOS scalar intrinsic present in hw in AIE1 but not in AIE2
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SINCOS_IN_HW__ 1
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __SINCOS_IN_HW__ 0
#endif

//----------------------------------
// v8int16 supported in AIE1, not AIE2
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_V8INT16__ 1
#define __SUPPORTS_V16INT16__ 1
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __SUPPORTS_V8INT16__ 0
#define __SUPPORTS_V16INT16__ 0
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __MIN_REGSIZE__ 128
#endif
#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __MIN_REGSIZE__ 256
#endif

#define __MIN_READ_WRITE__ __MIN_REGSIZE__

#if (__AIE_ARCH__ == 10) || (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIEARCH__ == 10) || \
    (__AIEARCH__ == 20) || (__AIEARCH__ == 21)
#define __MAX_READ_WRITE__ 256
#elif (__AIE_ARCH__ == 22) || (__AIEARCH__ == 22)
#define __MAX_READ_WRITE__ 512
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __HAS_ACCUM_PERMUTES__ 1
#else
#define __HAS_ACCUM_PERMUTES__ 0
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10) || (__AIE_ARCH__ == 20) || (__AIEARCH__ == 20) || \
    (__AIE_ARCH__ == 21) || (__AIEARCH__ == 21)
#define __ALIGN_BYTE_SIZE__ 32
#elif (__AIE_ARCH__ == 22) || (__AIEARCH__ == 22)
#define __ALIGN_BYTE_SIZE__ 64
#else
#define __ALIGN_BYTE_SIZE__ 32
#endif

#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_DMA_FIFO__ 1
#else
#define __SUPPORTS_DMA_FIFO__ 0
#endif

//----------------------------------
// Comprehensive addressing for shuffles in AIE1 but not AIE2
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __SUPPORTS_COMPREHENSIVE_SHUFFLES__ 1
#elif (__AIE_ARCH__ == 20) || (__AIEARCH__ == 20)
#define __SUPPORTS_COMPREHENSIVE_SHUFFLES__ 0
#else
#define __SUPPORTS_COMPREHENSIVE_SHUFFLES__ 0
#endif

// TODO: Change such that it aligns with codebase (more than one underscore)
// conv or corr support
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define _SUPPORTS_FLOAT_CFLOAT_
#endif
#if (__AIE_ARCH__ == 20) || (__AIEARCH__ == 20)
#define _SUPPORTS_BFLOAT16_
#endif

// data memory in bytes
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __DATA_MEM_BYTES__ 32768
#elif (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __DATA_MEM_BYTES__ 65536
#endif

//------------------SHIFTING--------------------------
#if (__AIE_ARCH__ == 10) || (__AIEARCH__ == 10)
#define __MAX_SHIFT__ 62
// ! Possible AIE compiler bug. If compiling aiesim AIE1, then this macro is not set if no else clause, resulting in a..
// ! 'not defined in this scope' bug in the fnValidateShiftRange function further down this file. This means that..
// ! right now effectively all __MAX_SHIFT__s are set to 59 to circumvent compilation fail, regardless of device.
#elif (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
#define __MAX_SHIFT__ 59
#else
#define __MAX_SHIFT__ 62
#endif

//----------SATURATION and ROUNDING MODES-------------------
// AIE1 and 2 offer 3 saturation modes
#define __SATURATION_MODES__ 3

// saturation modes
#ifndef s_none
//! @brief none: No saturation is performed and the value is truncated on the MSB side.
#define s_none 0
#endif
#ifndef s_saturate
//! @brief saturate: rounds an n-bit signed value in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ]. For example if n=8, the
//! range would be [-128:127].
#define s_saturate 1
#endif
#ifndef s_symmetric
//! @brief symmetric: rounds an n-bit signed value in the range [-( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. For example if n=8,
//! the range would be [-127:127]
#define s_symmetric 3
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22) || (__AIEARCH__ == 20) || \
    (__AIEARCH__ == 21) || (__AIEARCH__ == 22)
// AIE-ML variants offers 3 additional rounding modes, restricting some values for future use.
#define __ROUNDING_MODES__ 13
#define __SUPPORTS_ML_ROUND_MODES__

#ifndef rnd_floor
//! @brief No rounding - Truncate LSB, always round down (towards negative infinity)
#define rnd_floor 0
#endif
#ifndef rnd_ceil
//! @brief No rounding - Always round up (towards positive infinity)
#define rnd_ceil 1
#endif
#ifndef rnd_sym_floor
//! @brief No rounding - Truncate LSB, always round towards 0
#define rnd_sym_floor 2
#endif
#ifndef rnd_sym_ceil
//! @brief No rounding - Always round up towards infinity
#define rnd_sym_ceil 3
#endif
#ifndef rnd_neg_inf
//! @brief Round halfway towards negative infinity
#define rnd_neg_inf 8
#endif
#ifndef rnd_pos_inf
//! @brief Round halfway towards positive infinity
#define rnd_pos_inf 9
#endif
#ifndef rnd_sym_zero
//! @brief Round halfway towards zero (away from infinity)
#define rnd_sym_zero 10
#endif
#ifndef rnd_sym_inf
//! @brief Round halfway towards infinity (away from zero)
#define rnd_sym_inf 11
#endif
#ifndef rnd_conv_even
//! @brief Round halfway towards nearest even number
#define rnd_conv_even 12
#endif
#ifndef rnd_conv_odd
//! @brief Round halfway towards nearest odd number
#define rnd_conv_odd 13
#endif

#else

// AIE1 offers 8 rounding modes
#define __ROUNDING_MODES__ 7

#ifndef rnd_floor
//! @brief No rounding - Truncate LSB, always round down (towards negative infinity)
#define rnd_floor 0
#endif
#ifndef rnd_ceil
//! @brief No rounding - Always round up (towards positive infinity)
#define rnd_ceil 1
#endif
#ifndef rnd_pos_inf
//! @brief Round halfway towards positive infinity
#define rnd_pos_inf 2
#endif
#ifndef rnd_neg_inf
//! @brief Round halfway towards negative infinity
#define rnd_neg_inf 3
#endif
#ifndef rnd_sym_inf
//! @brief Round halfway towards infinity (away from zero)
#define rnd_sym_inf 4
#endif
#ifndef rnd_sym_zero
//! @brief Round halfway towards zero (away from infinity)
#define rnd_sym_zero 5
#endif
#ifndef rnd_conv_even
//! @brief Round halfway towards nearest even number
#define rnd_conv_even 6
#endif
#ifndef rnd_conv_odd
//! @brief Round halfway towards nearest odd number
#define rnd_conv_odd 7
#endif

#ifndef rnd_sym_floor
//! @brief Not available
#define rnd_sym_floor 8
#endif
#ifndef rnd_sym_ceil
//! @brief Not available
#define rnd_sym_ceil 9
#endif

#endif

// UTILITY FUNCTIONS COMMONLY USED THROUGHOUT CODEBASE
//----------------------------------------------------------------------
template <typename T>
INLINE_DECL constexpr bool isComplex() {
    if
        constexpr(std::is_same<T, cint16>::value) { return true; }
    else if
        constexpr(std::is_same<T, cint32>::value) { return true; }
    else if
        constexpr(std::is_same<T, cfloat>::value) { return true; }
#ifdef _SUPPORTS_BFLOAT16_
    else if
        constexpr(std::is_same<T, cbfloat16>::value) { return true; }
#endif // _SUPPORTS_BFLOAT16_
    else {
        return false;
    }
};

template <typename T>
INLINE_DECL constexpr bool isFloat() {
    if
        constexpr(std::is_same<T, float>::value) { return true; }
    else if
        constexpr(std::is_same<T, cfloat>::value) { return true; }
#ifdef _SUPPORTS_BFLOAT16_
    else if
        constexpr(std::is_same<T, bfloat16>::value) { return true; }
    else if
        constexpr(std::is_same<T, cbfloat16>::value) { return true; }
#endif // _SUPPORTS_BFLOAT16_
    else {
        return false;
    }
};

template <unsigned int shift, typename T>
INLINE_DECL constexpr bool fnValidateShiftFloat() {
    if
        constexpr(!isFloat<T>()) { return true; } // if not float type return true.
    else if
        constexpr(shift == 0) { return true; } // else if shift is 0 return true
    else {
        return false;
    }
};

template <unsigned int shift>
INLINE_DECL constexpr bool fnValidateShiftRange() {
    return (shift <= __MAX_SHIFT__) ? true : false;
};

#ifdef __SUPPORTS_ML_ROUND_MODES__
template <unsigned int roundMode>
INLINE_DECL constexpr bool fnValidateRoundMode() {
    if
        constexpr(roundMode == rnd_floor) { return true; }
    else if
        constexpr(roundMode == rnd_ceil) { return true; }
    else if
        constexpr(roundMode == rnd_sym_floor) { return true; }
    else if
        constexpr(roundMode == rnd_sym_ceil) { return true; }
    else if
        constexpr(roundMode == rnd_neg_inf) { return true; }
    else if
        constexpr(roundMode == rnd_pos_inf) { return true; }
    else if
        constexpr(roundMode == rnd_sym_zero) { return true; }
    else if
        constexpr(roundMode == rnd_sym_inf) { return true; }
    else if
        constexpr(roundMode == rnd_conv_even) { return true; }
    else if
        constexpr(roundMode == rnd_conv_odd) { return true; }
    else {
        return false;
    }
};
#else
template <unsigned int roundMode>
INLINE_DECL constexpr bool fnValidateRoundMode() {
    if
        constexpr(roundMode == rnd_floor) { return true; }
    else if
        constexpr(roundMode == rnd_ceil) { return true; }
    else if
        constexpr(roundMode == rnd_pos_inf) { return true; }
    else if
        constexpr(roundMode == rnd_neg_inf) { return true; }
    else if
        constexpr(roundMode == rnd_sym_inf) { return true; }
    else if
        constexpr(roundMode == rnd_sym_zero) { return true; }
    else if
        constexpr(roundMode == rnd_conv_even) { return true; }
    else if
        constexpr(roundMode == rnd_conv_odd) { return true; }
    else {
        return false;
    }
};
#endif // __SUPPORTS_ML_ROUND_MODES__

template <unsigned int satMode>
INLINE_DECL constexpr bool fnValidateSatMode() {
    if
        constexpr(satMode == s_none) { return true; }
    else if
        constexpr(satMode == s_saturate) { return true; }
    else if
        constexpr(satMode == s_symmetric) { return true; }
    else {
        return false;
    }
};

#endif // __DEVICE_DEFS__
