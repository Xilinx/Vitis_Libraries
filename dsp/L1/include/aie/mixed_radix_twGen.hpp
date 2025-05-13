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
#ifndef _DSPLIB_mixed_radix_twGenGEN_HPP_
#define _DSPLIB_mixed_radix_twGenGEN_HPP_

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

#define M_PI 3.14159265358979323846 /* pi */

// MIXED_RADIX_FFT Twiddle Generation base of specialization .
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, typename TT_TWIDDLE, unsigned int TP_POINT_SIZE, unsigned int TP_RND, unsigned int TP_SAT>
class kernel_MRFFTTwiddleGenerationClass {
   private:
#if __FFT_R4_IMPL__ == 0
    static constexpr int kMinPtSizeGranularity = 8;
#endif //__FFT_R4_IMPL__
#if __FFT_R4_IMPL__ == 1
    // Use __FFT_MIN_VECTORIZATION__
    static constexpr int kMinPtSizeGranularity = 16; // AIE-ML is not supported yet, but this will apply when it does
#endif                                               //__FFT_R4_IMPL__
                                                     // Parameter value defensive and legality checks
    static_assert(std::is_same<TT_TWIDDLE, cint16>::value, "ERROR: only TT_TWIDDLE=cint16 is currently supported.");
    static_assert(TP_POINT_SIZE % kMinPtSizeGranularity == 0,
                  "ERROR. TP_POINT_SIZE must be a multiple of 16 on AIE1 and 32 on AIE2");
    static_assert(TP_SAT == 0 || TP_SAT == 1 || TP_SAT == 3, "ERROR: unsupported value of TP_SAT");

    // TODO DUPLICATION of declarations - see around line 165 of mixed_radix_twGen.hpp and line 685 of
    // mixed_radix_fft_graph.hpp
    static constexpr int kFanVectorSize =
        16; // number of cint16's in fan array (kTotalVectors step arrays) - maximum possible in 32KB
    static constexpr int kStepVectorSize =
        132; // number of cint16's in step array (kTotalVectors step arrays) - maximum possible in 32KB
    // static constexpr int eightyPercentMaxPointsize = 0.8*TP_POINT_SIZE;
    // static constexpr int kStepVectorSize = eightyPercentMaxPointsize;
    static constexpr int kTotalVectors = 62; // 62 possible pointsizes <= maximum pointsize of 3300
    static constexpr int kfanLUTsize = kFanVectorSize * kTotalVectors;
    static constexpr int kstepLUTsize = kStepVectorSize * kTotalVectors;
    static constexpr int m_knRmultiple = (kMaxStagesR2 - minNumR2) * (kMaxStagesR3) * (kMaxStagesR5);
    static constexpr int coeffSize = (kStepVectorSize + 1) * kFanVectorSize; // +1 because coeff is made up of fan_array
                                                                             // (+1) then fan_array*each step element
                                                                             // (+kStepVectorSize)

    alignas(__ALIGN_BYTE_SIZE__) int16 (&m_kIndicesLUT)[m_knRmultiple];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_kFanLUT)[kfanLUTsize];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_kStepLUT)[kstepLUTsize];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&coeff)[coeffSize];

    // static constexpr int kHeaderSize = 16; // 16 since 16 int16's in 256 bits
    // static constexpr int kFanVectorSize = 16; // 16 since 16 int16's in 256 bits
    // static constexpr int kTwVectorSize = 8; // 8 since 8 cint16's in 256 bits
    static constexpr int kHeaderSize = 8; // 8 since 8 int32's in 256 bits
    // static constexpr int kFanVectorSize = 16; // 16 since 16 int16's in 256 bits
    static constexpr int kVectorSize = kNumAncillaryPerRow;
    static constexpr int kTwVectorSize = 256 / (8 * sizeof(TT_TWIDDLE)); // 8 since 8 cint16's in 256 bits
    static constexpr int kTwVectorSizeShift = fnGetNumStages<kTwVectorSize, 2, TT_TWIDDLE>(); // log2(kTwVectorSize);
    static constexpr TT_TWIDDLE firstTwiddle = {32767, 0};                                    // W(0, N)
    static constexpr TT_TWIDDLE blankTwiddle = {0, 0};
    static constexpr unsigned int kTwShift =
        std::is_same<TT_TWIDDLE, cfloat>::value
            ? 0
            : 8 * sizeof(TT_TWIDDLE) / 2 - 1; // TODO what is the correct function here?

   public:
    // Constructor
    kernel_MRFFTTwiddleGenerationClass(int16 (&indicesLUT)[m_knRmultiple],
                                       TT_TWIDDLE (&fanLUT)[kfanLUTsize],
                                       TT_TWIDDLE (&stepLUT)[kstepLUTsize],
                                       TT_TWIDDLE (&intCoeffArr)[coeffSize])
        : // initializer list
          m_kIndicesLUT(indicesLUT),
          m_kFanLUT(fanLUT),
          m_kStepLUT(stepLUT),
          coeff(intCoeffArr) {}

    // MIXED_RADIX_FFT
    void kernelMixedRadixFFTtwiddleGen(input_buffer<TT_DATA>* headerInWindow,
                                       output_buffer<TT_DATA>* headerOutWindow,
                                       output_buffer<TT_TWIDDLE>* outWindow);
};

template <typename TT_DATA, typename TT_TWIDDLE, unsigned int TP_POINT_SIZE, unsigned int TP_RND, unsigned int TP_SAT>
class mixed_radix_twGen
    : public kernel_MRFFTTwiddleGenerationClass<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_RND, TP_SAT> {
   private:
    // TODO DUPLICATION of declarations - see around line 165 of mixed_radix_twGen.hpp and line 685 of
    // mixed_radix_fft_graph.hpp
    static constexpr int kFanVectorSize =
        16; // number of cint16's in fan array (kTotalVectors step arrays) - maximum possible in 32KB
    static constexpr int kStepVectorSize =
        132; // number of cint16's in step array (kTotalVectors step arrays) - maximum possible in 32KB
    // static constexpr int eightyPercentMaxPointsize = 0.8*TP_POINT_SIZE;
    // static constexpr int kStepVectorSize = eightyPercentMaxPointsize;
    static constexpr int kTotalVectors = 62; // 62 possible pointsizes <= maximum pointsize of 3300
    static constexpr int kfanLUTsize = kFanVectorSize * kTotalVectors;
    static constexpr int kstepLUTsize = kStepVectorSize * kTotalVectors;
    static constexpr int m_knRmultiple = (kMaxStagesR2 - minNumR2 + 0) * (kMaxStagesR3 + 0) * (kMaxStagesR5 + 0);
    static constexpr unsigned int kTwShift =
        std::is_same<TT_TWIDDLE, cfloat>::value
            ? 0
            : 8 * sizeof(TT_TWIDDLE) / 2 - 1;                                // TODO what is the correct function here?
    static constexpr int coeffSize = (kStepVectorSize + 1) * kFanVectorSize; // +1 because coeff is made up of fan_array
                                                                             // (+1) then fan_array*each step element
                                                                             // (+kStepVectorSize)

    alignas(__ALIGN_BYTE_SIZE__) int16 (&m_kIndicesLUT)[m_knRmultiple];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_kFanLUT)[kfanLUTsize];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&m_kStepLUT)[kstepLUTsize];
    alignas(__ALIGN_BYTE_SIZE__) TT_TWIDDLE (&coeff)[coeffSize];

   public:
    // Constructor
    using thisKernelClass = kernel_MRFFTTwiddleGenerationClass<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_RND, TP_SAT>;
    mixed_radix_twGen(int16 (&indicesLUT)[m_knRmultiple],
                      TT_TWIDDLE (&fanLUT)[kfanLUTsize],
                      TT_TWIDDLE (&stepLUT)[kstepLUTsize],
                      TT_TWIDDLE (&intCoeffArr)[coeffSize])
        : // Initializer list
          m_kIndicesLUT(indicesLUT),
          m_kFanLUT(fanLUT),
          m_kStepLUT(stepLUT),
          coeff(intCoeffArr),
          thisKernelClass(indicesLUT, fanLUT, stepLUT, intCoeffArr) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(mixed_radix_twGen::mixed_radix_twGenMain);
        REGISTER_PARAMETER(m_kIndicesLUT);
        REGISTER_PARAMETER(m_kFanLUT);
        REGISTER_PARAMETER(m_kStepLUT);
        REGISTER_PARAMETER(coeff);
    }

    // MIXED_RADIX_FFT TW
    void mixed_radix_twGenMain(input_buffer<TT_DATA>& __restrict headerInWindow,
                               output_buffer<TT_DATA>& __restrict headerOutWindow,
                               output_buffer<TT_TWIDDLE>& __restrict outWindow);
};
}
}
}
}
}
#endif // _DSPLIB_mixed_radix_twGenGEN_HPP_
