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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_HPP_

/*
FFT/IFFT, DIT, single channel.
This file exists to capture the definition of the single channel FFT/iFFT filter kernel class.
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
using namespace adf;
#include <vector>

#include "device_defs.h"
#include "fft_ifft_dit_1ch_traits.hpp"
#include "widget_api_cast.hpp" //for API definitions
#include "fir_utils.hpp"       //for ROUND_MIN/MAX and SAT_MODE_MIN/MAX

#ifndef __SUPPORTS_ACC64__
using output_stream_cacc64 = output_stream_cacc80;
using input_stream_cacc64 = input_stream_cacc80;
#endif //__SUPPORTS_ACC64__

#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_HPP_DEBUG_
//#define _DSPLIB_FFT_IFFT_DIT_1CH_HPP_DEBUG_
#endif //_DSPLIB_FFT_IFFT_DIT_1CH_HPP_DEBUG_

using namespace xf::dsp::aie::widget::api_cast;
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA = cint16,
          typename TT_OUT_DATA = cint16,
          typename TT_TWIDDLE = cint16,
          typename TT_INTERNAL_DATA = cint16,
          unsigned int TP_POINT_SIZE = 4096,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_START_RANK = 0,
          unsigned int TP_END_RANK = 8,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_TWIDDLE_MODE = 0>
class stockhamStages {
   private:
   public:
    stockhamStages() {} // Constructor

    static constexpr unsigned int kPointSizePower = fnPointSizePower<TP_POINT_SIZE>();
    static constexpr unsigned int kPointSizePowerCeiled =
        fnCeil<kPointSizePower, 2>(); // round up to 2 to allow loop of r4 stages
    static constexpr unsigned int kPointSizeCeiled = 1 << kPointSizePowerCeiled; // loop is for R4 or R2

    static constexpr unsigned int kOddPower = fnOddPower<TP_POINT_SIZE>();
    static constexpr unsigned int kIntR4Stages =
        (kPointSizePower - 3) / 2; // number of internal radix 4 stags, i.e. not including input or output stages.
    static constexpr unsigned int kIntR2Stages =
        (kPointSizePower -
         2); // The 2 comes from the last 2 ranks being handled differently, being Stockham stage 1 and 2.

    void calc(TT_DATA* __restrict xbuff,
              TT_TWIDDLE** tw_table,
              TT_INTERNAL_DATA* tmp1_buf,
              TT_INTERNAL_DATA* tmp2_buf,
              TT_OUT_DATA* __restrict obuff);
    void calc(TT_DATA* __restrict xbuff,
              TT_TWIDDLE** tw_table,
              TT_INTERNAL_DATA* tmp1_buf,
              TT_INTERNAL_DATA* tmp2_buf,
              TT_OUT_DATA* __restrict obuff,
              int ptSizePwr,
              bool inv);
    void stagePreamble(TT_TWIDDLE** tw_table,
                       TT_INTERNAL_DATA* tmp1_buf,
                       TT_INTERNAL_DATA* tmp2_buf,
                       TT_DATA* __restrict inptr,
                       TT_OUT_DATA* __restrict outptr);
};

template <typename TT_DATA = cint16,
          typename TT_OUT_DATA = cint16,
          typename TT_TWIDDLE = cint16,
          unsigned int TP_POINT_SIZE = 4096,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_START_RANK = 0,
          unsigned int TP_END_RANK = 8,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_TWIDDLE_MODE = 0>
class kernelFFTClass {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    stockhamStages<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   T_internalDataType,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        stages;
    void kernelFFT(TT_DATA* __restrict inptr, TT_OUT_DATA* __restrict outptr);
};

template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
class kernelFFTClass<TT_DATA,
                     TT_OUT_DATA,
                     TT_TWIDDLE,
                     16,
                     TP_FFT_NIFFT,
                     TP_SHIFT,
                     TP_START_RANK,
                     TP_END_RANK,
                     TP_DYN_PT_SIZE,
                     TP_WINDOW_VSIZE,
                     TP_ORIG_PAR_POWER,
                     TP_TWIDDLE_MODE> {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    stockhamStages<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   T_internalDataType,
                   16,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        stages;
    void kernelFFT(TT_DATA* __restrict inptr, TT_OUT_DATA* __restrict outptr);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
class kernelFFTClass<cint16,
                     TT_OUT_DATA,
                     cint16,
                     16,
                     TP_FFT_NIFFT,
                     TP_SHIFT,
                     TP_START_RANK,
                     TP_END_RANK,
                     TP_DYN_PT_SIZE,
                     TP_WINDOW_VSIZE,
                     TP_ORIG_PAR_POWER,
                     TP_TWIDDLE_MODE> {
   public:
    typedef cint32_t T_internalDataType;
    stockhamStages<cint16,
                   TT_OUT_DATA,
                   cint16,
                   T_internalDataType,
                   16,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        stages;

    void kernelFFT(cint16* inptr, TT_OUT_DATA* __restrict outptr);
};

//-----------------------------------------------------------------------------------------------------
// base class to hold legality checking for top FFT kernel class.
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_ifft_dit_1ch_legality {
   public:
    // Parameter value defensive and legality checks
    static_assert(fnCheckDataType<TT_DATA>(), "ERROR: TT_IN_DATA is not a supported type");
    static_assert(fnCheckDataType<TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
    static_assert(fnCheckDataIOType<TT_DATA, TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
    static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
    static_assert(fnCheckDataTwiddleType<TT_DATA, TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
    static_assert(fnCheckPointSize<TP_POINT_SIZE>(),
                  "ERROR: TP_POINT_SIZE is not a supported value {16, 32, 64, ..., 4096}");
    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
    static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
    static_assert(fnCheckShiftFloat<TT_DATA, TP_SHIFT>(),
                  "ERROR: TP_SHIFT is ignored for data type cfloat so must be set to 0");
    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0, "ERROR: TP_WINDOW_SIZE must be a multiple of TP_POINT_SIZE");
    static_assert(TP_WINDOW_VSIZE / TP_POINT_SIZE >= 1, "ERROR: TP_WINDOW_SIZE must be a multiple of TP_POINT_SIZE");
    static_assert((TP_DYN_PT_SIZE == 0) || (TP_POINT_SIZE != 16),
                  "ERROR: Dynamic point size is not supported for only one legal size (16)");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX && TP_SAT != 2,
                  "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert(TP_TWIDDLE_MODE >= 0 && TP_TWIDDLE_MODE <= 1,
                  "ERROR: TP_TWIDDLE_MODE is out of the supported range.");
};

//-----------------------------------------------------------------------------------------------------
// Top level single kernel specialization. Also specialization for window in, window out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_IN_API = 0,
          unsigned int TP_OUT_API = 0,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_ifft_dit_1ch : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                          TT_OUT_DATA,
                                                          TT_TWIDDLE,
                                                          TP_POINT_SIZE,
                                                          TP_FFT_NIFFT,
                                                          TP_SHIFT,
                                                          TP_START_RANK,
                                                          TP_END_RANK,
                                                          TP_DYN_PT_SIZE,
                                                          TP_WINDOW_VSIZE,
                                                          TP_ORIG_PAR_POWER,
                                                          TP_RND,
                                                          TP_SAT,
                                                          TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    fft_ifft_dit_1ch() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain); }
    void fftMain(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

// specialization of top level for streams in, window out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kStreamAPI,
                       kWindowAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kInBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);

   public:
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kInBufferSize];

    fft_ifft_dit_1ch(TT_DATA (&myInBuff)[kInBufferSize]) : inBuff(myInBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(inBuff);
    }
#if __STREAMS_PER_TILE__ == 2
    void fftMain(input_stream<TT_DATA>* __restrict inStream0,
                 input_stream<TT_DATA>* __restrict inStream1,
                 output_buffer<TT_OUT_DATA>& __restrict outWindow);
#else
    void fftMain(input_stream<TT_DATA>* __restrict inStream0, output_buffer<TT_OUT_DATA>& __restrict outWindow);
#endif
};

// specialization of top level for window in, streams out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kWindowAPI,
                       kStreamAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kInBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);

   public:
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kInBufferSize];

    fft_ifft_dit_1ch(TT_OUT_DATA (&myOutBuff)[kInBufferSize]) : outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(outBuff);
    }
#if __STREAMS_PER_TILE__ == 2
    void fftMain(input_buffer<TT_DATA>& __restrict inWindow,
                 output_stream<TT_OUT_DATA>* __restrict inStream0,
                 output_stream<TT_OUT_DATA>* __restrict inStream1);
#else
    void fftMain(input_buffer<TT_DATA>& __restrict inWindow, output_stream<TT_OUT_DATA>* __restrict inStream0);
#endif
};

// specialization of top level for streams in, streams out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kStreamAPI,
                       kStreamAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kInBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    static constexpr int kOutBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kInBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kOutBufferSize];

    fft_ifft_dit_1ch(TT_DATA (&myInBuff)[kInBufferSize], TT_OUT_DATA (&myOutBuff)[kOutBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }
#if __STREAMS_PER_TILE__ == 2
    void fftMain(input_stream<TT_DATA>* __restrict inStream0,
                 input_stream<TT_DATA>* __restrict inStream1,
                 output_stream<TT_OUT_DATA>* __restrict outStream0,
                 output_stream<TT_OUT_DATA>* __restrict outStream1);
#else
    void fftMain(input_stream<TT_DATA>* __restrict inStream0, output_stream<TT_OUT_DATA>* __restrict outStream0);
#endif
};

// specialization of top level for streams in, casc/streams out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kStreamAPI,
                       kCascStreamAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kInBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    static constexpr int kOutBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kInBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kOutBufferSize];

    fft_ifft_dit_1ch(TT_DATA (&myInBuff)[kInBufferSize], TT_OUT_DATA (&myOutBuff)[kOutBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }
    void fftMain(input_stream<TT_DATA>* __restrict inStream0,
                 output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                 output_stream<TT_OUT_DATA>* __restrict outStream1);
};

// specialization of top level for iobuffer, casc/streams out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kWindowAPI,
                       kCascStreamAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kOutBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kOutBufferSize];

    fft_ifft_dit_1ch(TT_OUT_DATA (&myOutBuff)[kOutBufferSize]) : outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(outBuff);
    }
    void fftMain(input_buffer<TT_DATA>& __restrict inWindow0,
                 output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                 output_stream<TT_OUT_DATA>* __restrict outStream1);
};

// specialization of top level for single stream in, stream/cascade out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kStreamAPI,
                       kStreamCascAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kInBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    static constexpr int kOutBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kInBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kOutBufferSize];

    fft_ifft_dit_1ch(TT_DATA (&myInBuff)[kInBufferSize], TT_OUT_DATA (&myOutBuff)[kOutBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }
    void fftMain(input_stream<TT_DATA>* __restrict inStream0,
                 output_stream<TT_OUT_DATA>* __restrict outStream0,
                 output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream1); // Cascade
};

// specialization of top level for iobuffer in, stream/cascade out
template <typename TT_DATA, // input
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_ifft_dit_1ch<TT_DATA,
                       TT_OUT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_START_RANK,
                       TP_END_RANK,
                       TP_DYN_PT_SIZE,
                       TP_WINDOW_VSIZE,
                       kWindowAPI,
                       kStreamCascAPI,
                       TP_ORIG_PAR_POWER,
                       TP_RND,
                       TP_SAT,
                       TP_TWIDDLE_MODE> : public fft_ifft_dit_1ch_legality<TT_DATA,
                                                                           TT_OUT_DATA,
                                                                           TT_TWIDDLE,
                                                                           TP_POINT_SIZE,
                                                                           TP_FFT_NIFFT,
                                                                           TP_SHIFT,
                                                                           TP_START_RANK,
                                                                           TP_END_RANK,
                                                                           TP_DYN_PT_SIZE,
                                                                           TP_WINDOW_VSIZE,
                                                                           TP_ORIG_PAR_POWER,
                                                                           TP_RND,
                                                                           TP_SAT,
                                                                           TP_TWIDDLE_MODE> {
   private:
    kernelFFTClass<TT_DATA,
                   TT_OUT_DATA,
                   TT_TWIDDLE,
                   TP_POINT_SIZE,
                   TP_FFT_NIFFT,
                   TP_SHIFT,
                   TP_START_RANK,
                   TP_END_RANK,
                   TP_DYN_PT_SIZE,
                   TP_WINDOW_VSIZE,
                   TP_ORIG_PAR_POWER,
                   TP_TWIDDLE_MODE>
        m_fftKernel; // Kernel for FFT

   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kOutBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_OUT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_OUT_DATA (&outBuff)[kOutBufferSize];

    fft_ifft_dit_1ch(TT_OUT_DATA (&myOutBuff)[kOutBufferSize]) : outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_ifft_dit_1ch::fftMain);
        REGISTER_PARAMETER(outBuff);
    }
    void fftMain(input_buffer<TT_DATA>& __restrict inWindow0,
                 output_stream<TT_OUT_DATA>* __restrict outStream0,
                 output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream1); // Cascade
};
}
}
}
}
}
#endif // _DSPLIB_FFT_IFFT_DIT_1CH_HPP_
