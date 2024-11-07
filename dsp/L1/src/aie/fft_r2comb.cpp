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
/*
FFT/IFFT DIT single channel R2 combiner stage kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>
#include <stdio.h>

using namespace std;

//#define _DSPLIB_FFT_R2COMB_HPP_DEBUG_

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include "fft_com_inc.h"
#include "fft_r2comb.hpp"
#include "kernel_api_utils.hpp"
#include "fft_ifft_dit_1ch_traits.hpp"
#include "fft_ifft_dit_1ch_utils.hpp"
#include "fft_r2comb_twiddle_lut_all.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {

using namespace xf::dsp::aie::fft::dit_1ch;

template <typename TT_TWIDDLE>
INLINE_DECL constexpr TT_TWIDDLE null_tw(){}; // default error trap?
template <>
INLINE_DECL constexpr cint16 null_tw<cint16>() {
    return {0, 0};
};
template <>
INLINE_DECL constexpr cint32 null_tw<cint32>() {
    return {0, 0};
};
template <>
INLINE_DECL constexpr cfloat null_tw<cfloat>() {
    return {0.0, 0.0};
};

// function to create offsets for multiple twiddle tables stored in one array for the Dynamic Point size case.
template <int TP_POINT_SIZE>
constexpr std::array<int, 12> fnGetR2TwStarts() {
    std::array<int, 12> twStarts = {0};
    int index = 0;
    int offset = 0;
    for (int i = TP_POINT_SIZE; i >= 8; i = (i >> 1)) {
        twStarts[index++] = offset;
        offset += i;
    }
    return twStarts;
}

// function to create R2 twiddle lookup table at compile-time
template <typename TT_TWIDDLE,
          int TP_INDEX,
          int TP_POINT_SIZE,
          int TP_PARALLEL_POWER,
          int TP_DYN_PT_SIZE,
          int TP_TWIDDLE_MODE>
constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / (2 - TP_DYN_PT_SIZE)) >> TP_PARALLEL_POWER)>
fnGetR2CombTwTable() // admittedly ugly way of saying twiddle table is 2x for dynamic case
{
    constexpr TT_TWIDDLE kzero = null_tw<TT_TWIDDLE>();
    constexpr int kTwiddleTableSize = (TP_POINT_SIZE / (2 - TP_DYN_PT_SIZE)) >> TP_PARALLEL_POWER;
    std::array<TT_TWIDDLE, kTwiddleTableSize> twiddles = {kzero};
    constexpr TT_TWIDDLE* twiddle_master = fnGetR2TwiddleMasterBase<TT_TWIDDLE, TP_TWIDDLE_MODE>();

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            int tableIndex = 0;
            int tableFactor = 1; // 1 for max size table, moving up by power of 2 for each
            for (int tableSize = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER; tableSize >= 8;
                 tableSize = (tableSize >> 1)) {
                int idx = (2 * kR2MasterTableSize / TP_POINT_SIZE) * TP_INDEX * tableFactor;
                int stride = ((2 * kR2MasterTableSize / TP_POINT_SIZE) << TP_PARALLEL_POWER) * tableFactor;
                for (int i = 0; i < tableSize; i++) {
                    twiddles[tableIndex++] = twiddle_master[idx];
                    idx += stride;
                }
                tableFactor <<= 1;
            }
        }
    else {
        constexpr int tableSize = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER;
        int idx = (2 * kR2MasterTableSize / TP_POINT_SIZE) * TP_INDEX;
        int stride = (2 * kR2MasterTableSize / TP_POINT_SIZE) << TP_PARALLEL_POWER;
        for (int i = 0; i < tableSize; i++) {
            twiddles[i] = twiddle_master[idx];
            idx += stride;
        }
    }
    return twiddles;
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
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void fft_r2comb_r2stage<TT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    TP_PARALLEL_POWER,
                                    TP_INDEX,
                                    TP_ORIG_PAR_POWER,
                                    TP_TWIDDLE_MODE>::calcR2Comb(TT_DATA* inBuff, TT_DATA* outBuff) {
    static constexpr int kTableFactor = (TP_DYN_PT_SIZE == 1) ? 1 : 2; // for dynamic point size, the max point size
                                                                       // table is not alone. We need 1/2, 1/4, etc,
                                                                       // hence twice the storage
    alignas(32) static constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / kTableFactor) >> TP_PARALLEL_POWER)>
        twiddles = fnGetR2CombTwTable<TT_TWIDDLE, TP_INDEX, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_DYN_PT_SIZE,
                                      TP_TWIDDLE_MODE>();
    static constexpr std::array<int, 12> twiddleStarts = fnGetR2TwStarts<(TP_POINT_SIZE >> (TP_PARALLEL_POWER + 1))>();
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();
    constexpr unsigned int kShift = kTwShift + TP_SHIFT;

    using t256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;

    bool inv;
    TT_DATA* xbuff = (TT_DATA*)&inBuff[0];  // sample-wise pointer
    TT_DATA* ybuff = (TT_DATA*)&outBuff[0]; // sample-wise pointer

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            static constexpr unsigned int kSamplesInHeader = 256 / 8 / sizeof(TT_DATA);
            static constexpr unsigned int kPtSizePwr = fnPointSizePower<TP_POINT_SIZE>();
            static constexpr unsigned int kminPtSizePwr = 4;

            t256VectorType header;
            TT_DATA headerVal;
            t256VectorType blankOp;
            t256VectorType* inPtr = (t256VectorType*)&inBuff[0];   // vector-wise pointer
            t256VectorType* outPtr = (t256VectorType*)&outBuff[0]; // vector-wise pointer
            int ptSizePwr;
            int ptSize;

            blankOp = ::aie::zeros<TT_DATA, kSamplesInHeader>();
            header = *inPtr++;
            // read in direction field
            headerVal = header.get(0);
            inv = headerVal.real == 0 ? true : false;
            // read in number of radix2 ranks field
            headerVal = header.get(1);
            // ptrSizePwr is the point size for this kernel, but the header describes the point size for the graph,
            // hence
            // the kernel needs to know how many levels of divide by two recursion it is down.
            ptSizePwr = (int)headerVal.real - (TP_ORIG_PAR_POWER - TP_PARALLEL_POWER);
            ptSize = (1 << ptSizePwr);
            // read in the incoming status field (1 = error, 0 = ok)
            headerVal = header.get(std::is_same<TT_DATA, cint16>::value ? 7 : 3);

            //    if (headerVal.real == 0 && ptSizePwr >= kminPtSizePwr && ptSizePwr <= kPtSizePwr) { //ie legal request
            if (headerVal.real ==
                0) { // the FFT subframe has already checked pointsize legality, so just use the status field.
                *outPtr++ = header;            // write header to output buffer
                xbuff += 32 / sizeof(TT_DATA); // move past header
                ybuff += 32 / sizeof(TT_DATA); // move past header

                int n = (ptSize >> TP_PARALLEL_POWER);
                int tw_base = kPtSizePwr - ptSizePwr;

                constexpr int kFrameHolder = TP_POINT_SIZE >> TP_PARALLEL_POWER;
                using write_type = ::aie::vector<TT_DATA, kSamplesInHeader>;
                TT_DATA* blankPtr = ybuff;
                write_type* blankVectPtr = (write_type*)(ybuff);
                for (int i = 0; i < TP_WINDOW_VSIZE; i += kFrameHolder) { // loop for multiple frames in window
                    r2comb_dit<TT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE>(xbuff + i,
                                                                     (TT_TWIDDLE*)(&twiddles[twiddleStarts[tw_base]]),
                                                                     n, 0 /* r  */, kShift, ybuff + i, inv);

                    // blank the remainder of the frame holder
                    blankPtr += n;
                    blankVectPtr = (write_type*)(blankPtr); // addition is in TT_DATA currency, then cast to 128b
                    for (int k = n; k < kFrameHolder; k += kSamplesInHeader) {
                        *blankVectPtr++ = ::aie::zeros<TT_DATA, kSamplesInHeader>();
                    }
                    blankPtr = (TT_DATA*)blankVectPtr; // start of next frame within window
                }

            } else { // illegal framesize or invalid incoming
                header.set(unitVector<TT_DATA>(), std::is_same<TT_DATA, cint16>::value
                                                      ? 7
                                                      : 3); // set the invalid flag in the status location.
                *outPtr++ = header;
                // write out blank window
                for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesInHeader; i++) {
                    *outPtr++ = ::aie::zeros<TT_DATA, kSamplesInHeader>();
                }
            } // end of dynamic handling.
        }
    else { // Static point size case

        if
            constexpr(TP_FFT_NIFFT == 1) { inv = false; }
        else {
            inv = true;
        }
        // perform the R2 stage here.
        TT_DATA* xbuff = (TT_DATA*)&inBuff[0];
        TT_DATA* ybuff = (TT_DATA*)&outBuff[0];
        constexpr int n = (TP_POINT_SIZE >> TP_PARALLEL_POWER);

        for (int i = 0; i < TP_WINDOW_VSIZE; i += n) {
            r2comb_dit<TT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE>(xbuff + i, (TT_TWIDDLE*)(&twiddles[0]), n, 0 /* r  */,
                                                             kShift, ybuff + i, inv);
        }
    }
}

//-----------------------------------------------------------------------------------------------------
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
                              TT_TWIDDLE,
                              TP_POINT_SIZE,
                              TP_FFT_NIFFT,
                              TP_SHIFT,
                              TP_DYN_PT_SIZE,
                              TP_WINDOW_VSIZE,
                              TP_PARALLEL_POWER,
                              TP_INDEX,
                              TP_ORIG_PAR_POWER,
                              TP_API_IN,
                              TP_API_OUT,
                              TP_RND,
                              TP_SAT,
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_stream<TT_DATA>* __restrict outStream0,
                                                                output_stream<TT_DATA>* __restrict outStream1) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeStreamOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outBuff);
};

// Specialization for Cascade/Stream in, single Stream out
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream_cacc64* __restrict inStream0, // cascade
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_stream<TT_DATA>* __restrict outStream0) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readCascStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeStreamOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outBuff);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream_cacc64* __restrict inStream0, // cascade
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_stream_cacc64* __restrict outStream0, // cascade
                                                                output_stream<TT_DATA>* __restrict outStream1) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readCascStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeCascStreamOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outBuff);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream_cacc64* __restrict inStream0, // cascade
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_stream<TT_DATA>* __restrict outStream0,
                                                                output_stream_cacc64* __restrict outStream1 // cascade
                                                                ) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readCascStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeStreamCascOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outBuff);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream_cacc64* __restrict inStream1, // cascade
                                                                output_stream<TT_DATA>* __restrict outStream0) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamCascIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeStreamOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outBuff);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream_cacc64* __restrict inStream1,   // cascade
                                                                output_stream_cacc64* __restrict outStream0, // cascade
                                                                output_stream<TT_DATA>* __restrict outStream1) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamCascIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeCascStreamOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outBuff);
};

// Specialization for Stream/Cascade in, Stream/Cascade out
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream_cacc64* __restrict inStream1, // cascade
                                                                output_stream<TT_DATA>* __restrict outStream0,
                                                                output_stream_cacc64* __restrict outStream1 // cascade
                                                                ) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamCascIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outBuff);
    chess_memory_fence();
    writeStreamCascOut<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outBuff);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_buffer<TT_DATA>& __restrict outWindow0) {
    TT_DATA* outPtr = outWindow0.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outPtr);
};

// Specialization for Casc/Stream in, single Window out
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream_cacc64* __restrict inStream0,
                                                                input_stream<TT_DATA>* __restrict inStream1,
                                                                output_buffer<TT_DATA>& __restrict outWindow0) {
    TT_DATA* outPtr = outWindow0.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readCascStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outPtr);
};

// Specialization for Stream/Casc in, single Window out
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_stream<TT_DATA>* __restrict inStream0,
                                                                input_stream_cacc64* __restrict inStream1,
                                                                output_buffer<TT_DATA>& __restrict outWindow0) {
    TT_DATA* outPtr = outWindow0.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // TP_POINT_SIZE and PARALLEL_POWER go down by a power of 2 at each level of recursive hierarchy
    readStreamCascIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inBuff);
    chess_memory_fence();
    this->calcR2Comb(inBuff, outPtr);
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
NOINLINE_DECL void fft_r2comb<TT_DATA,
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
                              TP_TWIDDLE_MODE>::fft_r2comb_main(input_buffer<TT_DATA>& __restrict inWindow0,
                                                                output_buffer<TT_DATA>& __restrict outWindow0) {
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA* outPtr = outWindow0.data();
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    this->calcR2Comb(inPtr, outPtr);
};
}
}
}
}
}
