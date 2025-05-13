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
#ifndef _DSPLIB_MIXED_RADIX_FFT_HPP_
#define _DSPLIB_MIXED_RADIX_FFT_HPP_

/*
MIXED_RADIX_FFT
This file exists to capture the definition of the single channel MIXED_RADIX_FFT/iMIXED_RADIX_FFT filter kernel class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>

#include "device_defs.h"
#include "mixed_radix_fft_traits.hpp"

#ifndef _DSPLIB_MIXED_RADIX_FFT_HPP_DEBUG_
//#define _DSPLIB_MIXED_RADIX_FFT_HPP_DEBUG_
#endif //_DSPLIB_MIXED_RADIX_FFT_HPP_DEBUG_

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

//-----------------------------------------------------------------------------------------------------
// Base class
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE>
class kernel_MixedRadixFFTClass {
   private:
#if __FFT_R4_IMPL__ == 0
    static constexpr int kMinPtSizeGranularity = 8;
#endif //__FFT_R4_IMPL__
#if __FFT_R4_IMPL__ == 1
    static constexpr int kMinPtSizeGranularity = 16; // AIE-ML is not supported yet, but this will apply when it does
#endif                                               //__FFT_R4_IMPL__
    // Parameter value defensive and legality checks
    static_assert(std::is_same<TT_TWIDDLE, cint16>::value || std::is_same<TT_TWIDDLE, cint32>::value ||
                      std::is_same<TT_TWIDDLE, cfloat>::value,
                  "ERROR: TT_TWIDDLE type unrecognised. Must be cint16, cint32 or cfloat");
    static_assert((std::is_same<TT_IN_DATA, cint16>::value && std::is_same<TT_TWIDDLE, cint16>::value) ||
                      (std::is_same<TT_IN_DATA, cint32>::value && std::is_same<TT_TWIDDLE, cint16>::value) ||
                      (std::is_same<TT_IN_DATA, cint32>::value && std::is_same<TT_TWIDDLE, cint32>::value) ||
                      (std::is_same<TT_IN_DATA, cfloat>::value && std::is_same<TT_TWIDDLE, cfloat>::value),
                  "ERROR: TT_DATA/TT_TWIDDLE combination is illegal. Must be cint16/cint16, cint32/cint16, "
                  "cint32/cint32 or cfloat/cfloat");
    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
    static_assert(TP_SHIFT >= 0 && TP_SHIFT <= 60, "ERROR: TP_SHIFT is out of range (0 to 60)");
    static_assert(TP_POINT_SIZE % kMinPtSizeGranularity == 0,
                  "ERROR. TP_POINT_SIZE must be a multiple of 8 on AIE1 and 16 on AIE2");
    static_assert(TP_SAT == 0 || TP_SAT == 1 || TP_SAT == 3, "ERROR: unsupported value of TP_SAT");
    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE");

    static constexpr int m_kR5Stages = fnGetNumStages<TP_POINT_SIZE, 5, TT_TWIDDLE>();
    static constexpr int m_kR3Stages = fnGetNumStages<TP_POINT_SIZE, 3, TT_TWIDDLE>();
    static constexpr int m_kR4Stages = fnGetNumStages<TP_POINT_SIZE, 4, TT_TWIDDLE>();
    static constexpr int m_kR2Stages = fnGetNumStages<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2, TT_TWIDDLE>();
    static constexpr int m_kR5factor = fnGetRadixFactor<TP_POINT_SIZE, 5, TT_TWIDDLE>();
    static constexpr int m_kR3factor = fnGetRadixFactor<TP_POINT_SIZE, 3, TT_TWIDDLE>();
    static constexpr int m_kR4factor = fnGetRadixFactor<TP_POINT_SIZE, 4, TT_TWIDDLE>();
    static constexpr int m_kR2factor = fnGetRadixFactor<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2, TT_TWIDDLE>();

    static_assert(m_kR5Stages >= 0 && m_kR5Stages <= 3, "Error: unsupported number of radix5 stages");
    static_assert(m_kR3Stages >= 0 && m_kR3Stages <= 5, "Error: unsupported number of radix3 stages");
    static_assert(m_kR2Stages >= 0 && m_kR2Stages <= 10, "Error: unsupported number of radix2 stages");
    static_assert(m_kR4Stages >= 0 && m_kR4Stages <= 5, "Error: unsupported number of radix4 stages");
    static_assert(m_kR5factor* m_kR3factor* m_kR2factor* m_kR4factor == TP_POINT_SIZE,
                  "ERROR: TP_POINT_SIZE must be a multiple of 2, 3 and 5 only");

    static constexpr int m_kTotalStages = m_kR5Stages + m_kR3Stages + m_kR2Stages + m_kR4Stages;
    static constexpr int m_ktwiddleTableSize = fnGetTwiddleTableSize<TT_TWIDDLE, TP_POINT_SIZE>();

    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_twTable)[m_ktwiddleTableSize];
    alignas(__ALIGN_BYTE_SIZE__) int (&m_twiddlePtrPtr)[kNumMaxTables];

    typedef typename std::conditional<std::is_same<TT_IN_DATA, cint16>::value, cint32_t, TT_IN_DATA>::type
        T_internalDataType;
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff0)[TP_POINT_SIZE];
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff1)[TP_POINT_SIZE];

   public:
    // Constructor
    kernel_MixedRadixFFTClass(T_internalDataType (&tmpBuff0)[TP_POINT_SIZE],
                              T_internalDataType (&tmpBuff1)[TP_POINT_SIZE],
                              TT_TWIDDLE (&m_twiddleTable)[m_ktwiddleTableSize],
                              int (&m_twiddlePtrs)[kNumMaxTables])
        : // initializer list
          m_tmpBuff0(tmpBuff0),
          m_tmpBuff1(tmpBuff1),
          m_twTable(m_twiddleTable),
          m_twiddlePtrPtr(m_twiddlePtrs) {}

    // MIXED_RADIX_FFT
    void singleFrame(TT_IN_DATA* inFrameStart, TT_OUT_DATA* outFrameStart);
    void kernelMixedRadixFFTmain(input_buffer<TT_IN_DATA>* inWindow, output_buffer<TT_OUT_DATA>* outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Dynamic Base class
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK>
class kernel_MixedRadixFFTClass<TT_IN_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_RND,
                                TP_SAT,
                                TP_WINDOW_VSIZE,
                                TP_START_RANK,
                                TP_END_RANK,
                                1> {
   private:
    typedef typename std::conditional<std::is_same<TT_IN_DATA, cint16>::value, cint32_t, TT_IN_DATA>::type
        T_internalDataType;
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff0)[TP_POINT_SIZE];
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff1)[TP_POINT_SIZE];

   public:
    // Constructor
    kernel_MixedRadixFFTClass(T_internalDataType (&tmpBuff0)[TP_POINT_SIZE],
                              T_internalDataType (&tmpBuff1)[TP_POINT_SIZE])
        : // Initializer list
          m_tmpBuff0(tmpBuff0),
          m_tmpBuff1(tmpBuff1) {}

    static constexpr int16 kVectorSize = kNumAncillaryPerRow;

    // MIXED_RADIX_FFT
    void dynamicSingleFrame(TT_IN_DATA* inFrameStart,
                            TT_TWIDDLE* twiddleInFrameStart,
                            TT_OUT_DATA* outFrameStart,
                            std::array<T_ancillaryFields, kVectorSize> headerData); // twiddleInFrameStart ?
    void kernelMixedRadixFFTmain(input_buffer<TT_IN_DATA>* inWindow,
                                 input_buffer<TT_TWIDDLE>* twInWindow,
                                 output_buffer<TT_OUT_DATA>* outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the mixed radix kernel entry level class, and is also used for the default
// specialization

template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE>
class mixed_radix_fft : public kernel_MixedRadixFFTClass<TT_IN_DATA,
                                                         TT_OUT_DATA,
                                                         TT_TWIDDLE,
                                                         TP_POINT_SIZE,
                                                         TP_FFT_NIFFT,
                                                         TP_SHIFT,
                                                         TP_RND,
                                                         TP_SAT,
                                                         TP_WINDOW_VSIZE,
                                                         TP_START_RANK,
                                                         TP_END_RANK,
                                                         TP_DYN_PT_SIZE> {
   private:
    static constexpr int m_kR5Stages = fnGetNumStages<TP_POINT_SIZE, 5, TT_TWIDDLE>();
    static constexpr int m_kR3Stages = fnGetNumStages<TP_POINT_SIZE, 3, TT_TWIDDLE>();
    static constexpr int m_kR4Stages = fnGetNumStages<TP_POINT_SIZE, 4, TT_TWIDDLE>();
    static constexpr int m_kR2Stages = fnGetNumStages<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2, TT_TWIDDLE>();
    static constexpr int m_kR5factor = fnGetRadixFactor<TP_POINT_SIZE, 5, TT_TWIDDLE>();
    static constexpr int m_kR3factor = fnGetRadixFactor<TP_POINT_SIZE, 3, TT_TWIDDLE>();
    static constexpr int m_kR4factor = fnGetRadixFactor<TP_POINT_SIZE, 4, TT_TWIDDLE>();
    static constexpr int m_kR2factor = fnGetRadixFactor<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2, TT_TWIDDLE>();

    static constexpr int m_kTotalStages = m_kR5Stages + m_kR3Stages + m_kR2Stages + m_kR4Stages;
    static constexpr int m_ktwiddleTableSize = fnGetTwiddleTableSize<TT_TWIDDLE, TP_POINT_SIZE>();

    typedef typename std::conditional<std::is_same<TT_IN_DATA, cint16>::value, cint32_t, TT_IN_DATA>::type
        T_internalDataType;
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff0)[TP_POINT_SIZE];
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff1)[TP_POINT_SIZE];

    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_twTable)[m_ktwiddleTableSize];
    alignas(__ALIGN_BYTE_SIZE__) int (&m_twiddlePtrPtr)[kNumMaxTables];

   public:
    // Constructor
    using thisKernelClass = kernel_MixedRadixFFTClass<TT_IN_DATA,
                                                      TT_OUT_DATA,
                                                      TT_TWIDDLE,
                                                      TP_POINT_SIZE,
                                                      TP_FFT_NIFFT,
                                                      TP_SHIFT,
                                                      TP_RND,
                                                      TP_SAT,
                                                      TP_WINDOW_VSIZE,
                                                      TP_START_RANK,
                                                      TP_END_RANK,
                                                      TP_DYN_PT_SIZE>;
    mixed_radix_fft(T_internalDataType (&tmpBuff0)[TP_POINT_SIZE],
                    T_internalDataType (&tmpBuff1)[TP_POINT_SIZE],
                    TT_TWIDDLE (&twiddleTable)[m_ktwiddleTableSize],
                    int (&twiddlePtrs)[kNumMaxTables])
        : // Initializer list
          m_tmpBuff0(tmpBuff0),
          m_tmpBuff1(tmpBuff1),
          m_twTable(twiddleTable),
          m_twiddlePtrPtr(twiddlePtrs),
          thisKernelClass(tmpBuff0, tmpBuff1, twiddleTable, twiddlePtrs) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(mixed_radix_fft::mixed_radix_fftMain);
        REGISTER_PARAMETER(m_tmpBuff0);
        REGISTER_PARAMETER(m_tmpBuff1);
        REGISTER_PARAMETER(m_twTable);
        REGISTER_PARAMETER(m_twiddlePtrPtr);
    }
    // MIXED_RADIX_FFT
    void mixed_radix_fftMain(input_buffer<TT_IN_DATA>& __restrict inWindow,
                             output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK>
class mixed_radix_fft<TT_IN_DATA,
                      TT_OUT_DATA,
                      TT_TWIDDLE,
                      TP_POINT_SIZE,
                      TP_FFT_NIFFT,
                      TP_SHIFT,
                      TP_RND,
                      TP_SAT,
                      TP_WINDOW_VSIZE,
                      TP_START_RANK,
                      TP_END_RANK,
                      1> : public kernel_MixedRadixFFTClass<TT_IN_DATA,
                                                            TT_OUT_DATA,
                                                            TT_TWIDDLE,
                                                            TP_POINT_SIZE,
                                                            TP_FFT_NIFFT,
                                                            TP_SHIFT,
                                                            TP_RND,
                                                            TP_SAT,
                                                            TP_WINDOW_VSIZE,
                                                            TP_START_RANK,
                                                            TP_END_RANK,
                                                            1> {
   private:
    typedef typename std::conditional<std::is_same<TT_IN_DATA, cint16>::value, cint32_t, TT_IN_DATA>::type
        T_internalDataType;
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff0)[TP_POINT_SIZE];
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType (&m_tmpBuff1)[TP_POINT_SIZE];

   public:
    // Constructor
    using thisKernelClass = kernel_MixedRadixFFTClass<TT_IN_DATA,
                                                      TT_OUT_DATA,
                                                      TT_TWIDDLE,
                                                      TP_POINT_SIZE,
                                                      TP_FFT_NIFFT,
                                                      TP_SHIFT,
                                                      TP_RND,
                                                      TP_SAT,
                                                      TP_WINDOW_VSIZE,
                                                      TP_START_RANK,
                                                      TP_END_RANK,
                                                      1>;
    mixed_radix_fft(T_internalDataType (&tmpBuff0)[TP_POINT_SIZE],
                    T_internalDataType (&tmpBuff1)[TP_POINT_SIZE])
        : // Initializer list
          m_tmpBuff0(tmpBuff0),
          m_tmpBuff1(tmpBuff1),
          thisKernelClass(tmpBuff0, tmpBuff1) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(mixed_radix_fft::mixed_radix_fftMain);
        REGISTER_PARAMETER(m_tmpBuff0);
        REGISTER_PARAMETER(m_tmpBuff1);
    }
    // MIXED_RADIX_FFT
    void mixed_radix_fftMain(input_buffer<TT_IN_DATA>& __restrict inWindow,
                             input_buffer<TT_TWIDDLE>& __restrict twInWindow,
                             output_buffer<TT_OUT_DATA>& __restrict outWindow);
};
}
}
}
}
}
#endif // _DSPLIB_MIXED_RADIX_FFT_HPP_
