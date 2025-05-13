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
#ifndef _DSPLIB_FFT_R2COMB_HPP_
#define _DSPLIB_FFT_R2COMB_HPP_

/*
FFT/IFFT, DIT, single channel, R2 combiner stage.
The R2 combiner stage is used for cases where the FFT operation as a whole is split into FFT
subframes whose outputs need to be combined to perform the overall FFT.
This file exists to capture the definition of the single channel FFT/iFFT R2 combiner kernel class.
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

using namespace adf;

#include "device_defs.h"
#include "fft_ifft_dit_1ch_traits.hpp"
#include "widget_api_cast_traits.hpp"

using namespace xf::dsp::aie::widget::api_cast; // for API definitions like kStreamAPI

#ifndef _DSPLIB_FFT_R2COMB_HPP_DEBUG_
//#define _DSPLIB_FFT_R2COMB_HPP_DEBUG_
#endif //_DSPLIB_FFT_R2COMB_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {

// helper (base) class for fft_r2comb. This holds the computation which is performed oblivious to the form of interface,
// so need not be specialized
// according to interface
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb_r2stage {
   public:
    fft_r2comb_r2stage() {}

    void calcR2Comb(TT_DATA* inBuff, TT_DATA* outBuff);
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API_IN,
          unsigned int TP_API_OUT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb : public fft_r2comb_r2stage<TT_DATA,
                                             TT_TWIDDLE,
                                             TP_POINT_SIZE,
                                             TP_FFT_NIFFT,
                                             TP_SHIFT,
                                             TP_DYN_PT_SIZE,
                                             TP_WINDOW_VSIZE,
                                             TP_PARALLEL_POWER,
                                             TP_INDEX,
                                             TP_ORIG_PAR_POWER,
                                             TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_stream<TT_DATA>* __restrict outStream0,
                         output_stream<TT_DATA>* __restrict outStream1);
};

// Specialization for Cascade/Stream in, single Stream
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kCascStreamAPI,
                 kStreamAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream0, // Cascade
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_stream<TT_DATA>* __restrict outStream0);
};

// Specialization for Cascade/Stream in, Cascade/Stream out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kCascStreamAPI,
                 kCascStreamAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream0, // Cascade
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                         output_stream<TT_DATA>* __restrict outStream1);
};

// Specialization for Cascade/Stream in, Stream/Cascade out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kCascStreamAPI,
                 kStreamCascAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream0, // Cascade
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_stream<TT_DATA>* __restrict outStream0,
                         output_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict outStream1); // Cascade
};

// Specialization for Stream/Cascade in, single Stream out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kStreamCascAPI,
                 kStreamAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream1, // Cascade
                         output_stream<TT_DATA>* __restrict outStream0);
};

// Specialization for Stream/Cascade in, Cascade/Stream out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kStreamCascAPI,
                 kCascStreamAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream1,   // Cascade
                         output_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                         output_stream<TT_DATA>* __restrict outStream1);
};

// Specialization for Stream/Cascade in, StreamCascade out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kStreamCascAPI,
                 kStreamCascAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&outBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize], TT_DATA (&myOutBuff)[kBufferSize])
        : inBuff(myInBuff), outBuff(myOutBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
        REGISTER_PARAMETER(outBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream1, // Cascade
                         output_stream<TT_DATA>* __restrict outStream0,
                         output_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict outStream1); // Cascade
};

// Specialization for Streams in, single Window out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kStreamAPI,
                 kWindowAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize]) : inBuff(myInBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_buffer<TT_DATA>& __restrict outWindow0);
};

// Specialization for Cascade/Stream in, single Window out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kCascStreamAPI,
                 kWindowAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize]) : inBuff(myInBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream0, // Cascade
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_buffer<TT_DATA>& __restrict outWindow0);
};

// Specialization for Stream/Cascade in, single Window out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kStreamCascAPI,
                 kWindowAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&inBuff)[kBufferSize];

    // Constructor
    fft_r2comb(TT_DATA (&myInBuff)[kBufferSize]) : inBuff(myInBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main);
        REGISTER_PARAMETER(inBuff);
    }

    // r2comb main
    void fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_cascade<typename dit_1ch::t_accType<TT_DATA>::type>* __restrict inStream1, // Cascade
                         output_buffer<TT_DATA>& __restrict outWindow0);
};

// Specialization for Window in, single Window out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class fft_r2comb<TT_DATA,
                 TT_TWIDDLE,
                 TP_POINT_SIZE,
                 TP_FFT_NIFFT,
                 TP_SHIFT,
                 TP_DYN_PT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_PARALLEL_POWER,
                 TP_INDEX,
                 TP_ORIG_PAR_POWER,
                 kWindowAPI,
                 kWindowAPI,
                 TP_RND,
                 TP_SAT,
                 TP_TWIDDLE_MODE> : public fft_r2comb_r2stage<TT_DATA,
                                                              TT_TWIDDLE,
                                                              TP_POINT_SIZE,
                                                              TP_FFT_NIFFT,
                                                              TP_SHIFT,
                                                              TP_DYN_PT_SIZE,
                                                              TP_WINDOW_VSIZE,
                                                              TP_PARALLEL_POWER,
                                                              TP_INDEX,
                                                              TP_ORIG_PAR_POWER,
                                                              TP_TWIDDLE_MODE> {
   public:
    static constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__;
    static constexpr int kBufferSize = TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kHeaderSize / sizeof(TT_DATA);

    // Constructor
    fft_r2comb() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main); }

    // r2comb main
    void fft_r2comb_main(input_buffer<TT_DATA>& __restrict inWindow0, output_buffer<TT_DATA>& __restrict outWindow0);
};
}
}
}
}
}
#endif // _DSPLIB_FFT_R2COMB_HPP_
