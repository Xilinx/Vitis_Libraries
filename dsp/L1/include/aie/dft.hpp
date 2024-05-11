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
#ifndef _DSPLIB_DFT_HPP_
#define _DSPLIB_DFT_HPP_

/*
DFT
This file exists to capture the definition of the single channel DFT/iDFT filter kernel class.
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

#include "dft_traits.hpp"
#include "fir_utils.hpp"
#include "aie_api/aie.hpp"
#ifndef _DSPLIB_DFT_HPP_DEBUG_
// #define _DSPLIB_DFT_HPP_DEBUG_
#endif //_DSPLIB_DFT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

struct no_port {};
template <typename T_A, typename T_B>
#ifdef __SUPPORTS_ACC64__
struct accType {
    using type = cacc64;
};
template <>
struct accType<cint16, cint16> {
    using type = cacc64;
};
template <>
struct accType<cint32, cint16> {
    using type = cacc64;
};
template <>
struct accType<cfloat, cfloat> {
    using type = caccfloat;
};
#else
struct accType {
    using type = cacc48;
};
template <>
struct accType<cint16, cint16> {
    using type = cacc48;
};
template <>
struct accType<cint32, cint16> {
    using type = cacc80;
};
template <>
struct accType<cfloat, cfloat> {
    using type = caccfloat;
};
#endif //__SUPPORTS_ACC64__

template <typename T_D, typename T_T>
using accType_t = typename accType<T_D, T_T>::type;

// IF input type-------------------------------------
template <bool T_CASC_IN, typename T_D, typename T_T>
struct T_inputIF {};
// CASC_IN_FALSE
template <typename T_D, typename T_T>
struct T_inputIF<CASC_IN_FALSE, T_D, T_T> {
    T_D* __restrict inWindow;
    input_stream<accType_t<T_D, T_T> >* inCascade = {};
};
// CASC_IN_TRUE
template <typename T_D, typename T_T>
struct T_inputIF<CASC_IN_TRUE, T_D, T_T> {
    T_D* __restrict inWindow;
    input_stream<accType_t<T_D, T_T> >* inCascade;
};
// IF output type ------------------------------------
template <bool T_CASC_OUT, typename T_D, typename T_T>
struct T_outputIF {};
// CASC_OUT_FALSE
template <typename T_D, typename T_T>
struct T_outputIF<CASC_OUT_FALSE, T_D, T_T> {
    T_D* __restrict outWindow;
    output_stream<accType_t<T_D, T_T> >* outCascade{};
};
// CASC_OUT_TRUE
template <typename T_D, typename T_T>
struct T_outputIF<CASC_OUT_TRUE, T_D, T_T> {
    T_D* __restrict outWindow = {};
    output_stream<accType_t<T_D, T_T> >* outCascade;
};

//-----------------------------------------------------------------------------------------------------
// Base class
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class kernelDFTClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(fnCheckDataType<TT_DATA>(), "ERROR: TT_IN_DATA is not a supported type");
    static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
    static_assert(fnCheckDataTwiddleType<TT_DATA, TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
    static_assert(fnCheckPointSize<TP_POINT_SIZE>(), "ERROR: TP_POINT_SIZE is not a supported value {4-128");
    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
    static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
    static_assert(fnCheckShiftFloat<TT_DATA, TP_SHIFT>(),
                  "ERROR: TP_SHIFT is ignored for data type cfloat so must be set to 0");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__

    // frame and data sizes
    static constexpr int paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVectData);
    static constexpr int paddedFrameSize = CEIL(paddedDataSize, (kSamplesInVectData * TP_CASC_LEN));
    // static constexpr int paddedWindowSize = TP_NUM_FRAMES * paddedFrameSize;
    // frame and window sizes per cascade (TP_CASC_LEN, TP_SSR has no affect on data)
    static constexpr int cascFrameSize = paddedFrameSize / TP_CASC_LEN;
    // static constexpr int cascWindowSize = paddedWindowSize / TP_CASC_LEN;
    static constexpr int kVecInFrame = cascFrameSize / kSamplesInVectData;

    // number of data points each cascade will recieve
    // for example POINT_SIZE=29 CASC_LEN=3, casc1 gets 10 samples, casc2 gets 10 samples, casc3 gets 9 samples
    static constexpr int stepSize =
        (TP_KERNEL_POSITION < (TP_POINT_SIZE % TP_CASC_LEN)) + (TP_POINT_SIZE / TP_CASC_LEN);
    static constexpr int coeffColSize = CEIL(TP_POINT_SIZE, (kSamplesInVectData * TP_SSR)) / TP_SSR;
    static constexpr int kTotalCoeffSize = coeffColSize * stepSize;
    static constexpr int kVecInCoeff = coeffColSize / kSamplesInVectData;

    static constexpr int kPairsInCoeff = kVecInCoeff / 2;     // Use Parallel Accumulator/MAC when possible
    static constexpr int singleAccRequired = kVecInCoeff % 2; // Use single if necessary

    // Number of full vectors in a data frame (no zero-padding)
    static constexpr int kVecTotal = (cascFrameSize / kSamplesInVectData);
    // number of elements in the final padded vector of a frame
    static constexpr int kVecNoPad = (stepSize / kSamplesInVectData);
    // number of elements in the final padded vector of a frame
    static constexpr int elemsInPaddedVec = stepSize % (kSamplesInVectData);
    static constexpr int shift = TP_SHIFT + 15;

   public:
    // Constructor
    kernelDFTClass() {}

    // DFT
    void kernelDFT(T_inputIF<TP_CASC_IN, TT_DATA, TT_TWIDDLE> inInterface,
                   TT_TWIDDLE* __restrict coeffPtr,
                   T_outputIF<TP_CASC_OUT, T_outDataType, TT_TWIDDLE> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the dft class, and is also used for the Standalone kernel specialization with no
// cascade ports, a single input and no reload

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class dft {
   private:
    kernelDFTClass<TT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_CASC_LEN,
                   TP_SSR,
                   TP_NUM_FRAMES,
                   TP_KERNEL_POSITION,
                   TP_CASC_IN,
                   TP_CASC_OUT,
                   TP_RND,
                   TP_SAT>
        m_dftKernel;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
    static constexpr int coeffColSize = CEIL(TP_POINT_SIZE, (kSamplesInVectData * TP_SSR)) / TP_SSR;
    static constexpr int stepSize =
        (TP_KERNEL_POSITION < (TP_POINT_SIZE % TP_CASC_LEN)) + (TP_POINT_SIZE / TP_CASC_LEN);
    static constexpr int kTotalCoeffSize = coeffColSize * stepSize;

   public:
    TT_TWIDDLE (&inCoeff)[kTotalCoeffSize]; // Need to change
    // Constructor

    dft(TT_TWIDDLE (&coeff)[kTotalCoeffSize]) : inCoeff(coeff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(dft::dftMain);
        REGISTER_PARAMETER(inCoeff);
    }
    // DFT
    void dftMain(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<T_outDataType>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), single input, no reload
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dft<TT_DATA,
          TT_TWIDDLE,
          TP_POINT_SIZE,
          TP_FFT_NIFFT,
          TP_SHIFT,
          TP_CASC_LEN,
          TP_SSR,
          TP_NUM_FRAMES,
          TP_KERNEL_POSITION,
          CASC_IN_FALSE,
          CASC_OUT_TRUE,
          TP_RND,
          TP_SAT> {
   private:
    kernelDFTClass<TT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_CASC_LEN,
                   TP_SSR,
                   TP_NUM_FRAMES,
                   TP_KERNEL_POSITION,
                   CASC_IN_FALSE,
                   CASC_OUT_TRUE,
                   TP_RND,
                   TP_SAT>
        m_dftKernel;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
    static constexpr int coeffColSize = CEIL(TP_POINT_SIZE, (kSamplesInVectData * TP_SSR)) / TP_SSR;
    static constexpr int stepSize =
        (TP_KERNEL_POSITION < (TP_POINT_SIZE % TP_CASC_LEN)) + (TP_POINT_SIZE / TP_CASC_LEN);
    static constexpr int kTotalCoeffSize = coeffColSize * stepSize;

   public:
    TT_TWIDDLE (&inCoeff)[kTotalCoeffSize];
    // Constructor

    dft(TT_TWIDDLE (&coeff)[kTotalCoeffSize]) : inCoeff(coeff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(dft::dftMain);
        REGISTER_PARAMETER(inCoeff);
    }
    // DFT
    void dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                 output_stream<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), single input, no reload
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dft<TT_DATA,
          TT_TWIDDLE,
          TP_POINT_SIZE,
          TP_FFT_NIFFT,
          TP_SHIFT,
          TP_CASC_LEN,
          TP_SSR,
          TP_NUM_FRAMES,
          TP_KERNEL_POSITION,
          CASC_IN_TRUE,
          CASC_OUT_TRUE,
          TP_RND,
          TP_SAT> {
   private:
    // kernelDFTClass <TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, CASC_IN_TRUE, CASC_OUT_TRUE>
    // m_dftKernel;
    kernelDFTClass<TT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_CASC_LEN,
                   TP_SSR,
                   TP_NUM_FRAMES,
                   TP_KERNEL_POSITION,
                   CASC_IN_TRUE,
                   CASC_OUT_TRUE,
                   TP_RND,
                   TP_SAT>
        m_dftKernel;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
    static constexpr int coeffColSize = CEIL(TP_POINT_SIZE, (TP_SSR * kSamplesInVectData)) / TP_SSR;
    static constexpr int stepSize =
        (TP_KERNEL_POSITION < (TP_POINT_SIZE % TP_CASC_LEN)) + (TP_POINT_SIZE / TP_CASC_LEN);
    static constexpr int kTotalCoeffSize = coeffColSize * stepSize;

   public:
    TT_TWIDDLE (&inCoeff)[kTotalCoeffSize];
    // Constructor

    dft(TT_TWIDDLE (&coeff)[kTotalCoeffSize]) : inCoeff(coeff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(dft::dftMain);
        REGISTER_PARAMETER(inCoeff);
    }
    // DFT
    void dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                 input_stream<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
                 output_stream<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade);
};

// For multiple kernels, final kernel used input cascade output window
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dft<TT_DATA,
          TT_TWIDDLE,
          TP_POINT_SIZE,
          TP_FFT_NIFFT,
          TP_SHIFT,
          TP_CASC_LEN,
          TP_SSR,
          TP_NUM_FRAMES,
          TP_KERNEL_POSITION,
          CASC_IN_TRUE,
          CASC_OUT_FALSE,
          TP_RND,
          TP_SAT> {
   private:
    kernelDFTClass<TT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_CASC_LEN,
                   TP_SSR,
                   TP_NUM_FRAMES,
                   TP_KERNEL_POSITION,
                   CASC_IN_TRUE,
                   CASC_OUT_FALSE,
                   TP_RND,
                   TP_SAT>
        m_dftKernel;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
    static constexpr int coeffColSize = CEIL(TP_POINT_SIZE, (TP_SSR * kSamplesInVectData)) / TP_SSR;
    static constexpr int stepSize =
        (TP_KERNEL_POSITION < (TP_POINT_SIZE % TP_CASC_LEN)) + (TP_POINT_SIZE / TP_CASC_LEN);
    static constexpr int kTotalCoeffSize = coeffColSize * stepSize;

   public:
    TT_TWIDDLE (&inCoeff)[kTotalCoeffSize];
    // Constructor

    dft(TT_TWIDDLE (&coeff)[kTotalCoeffSize]) : inCoeff(coeff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(dft::dftMain);
        REGISTER_PARAMETER(inCoeff);
    }
    // DFT
    void dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                 input_stream<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
                 output_buffer<T_outDataType>& __restrict outWindow);
};
}
}
}
}
}
#endif // _DSPLIB_DFT_HPP_
