/*
 * Copyright 2022 Xilinx, Inc.
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
Widget API cast kernal code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "widget_api_cast.hpp"
#include "widget_api_cast_utils.hpp"

#include "kernel_api_utils.hpp"

// Default class is window to window - possibly useful for routing
namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             TP_IN_API,
                             TP_OUT_API,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, TP_IN_API> inInterface,
                                                               T_outputIF<TT_DATA, TP_OUT_API> outInterface) {
    T_buff_256b<TT_DATA> readVal;
    constexpr int kLsize = (TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_HEADER_BYTES) / 32;
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            readVal = window_readincr_256b(inInterface.inWindow0);
            window_writeincr(outInterface.outWindow0, readVal.val);
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, readVal.val);
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, readVal.val);
        }
};

// Stream to window generalized
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kStreamAPI,
                             kWindowAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kStreamAPI> inInterface,
                                                               T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    T_buff_128b<TT_DATA> readVal1;
    T_buff_128b<TT_DATA> readVal2;
    T_buff_256b<TT_DATA> writeVal;
    using in128VectorType = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
    ::std::pair<in128VectorType, in128VectorType> inIntlv;
    constexpr unsigned int kRepeat = 4;
    constexpr unsigned int kDataReadSize = TP_NUM_INPUTS == 1 ? 16 : 32;
    constexpr int kLsize = (TP_WINDOW_VSIZE * sizeof(TT_DATA)) / (kDataReadSize * kRepeat);
    constexpr int kLsizeRem = ((TP_WINDOW_VSIZE * sizeof(TT_DATA)) % (kDataReadSize * kRepeat)) / kDataReadSize;

    // handle header bytes
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++) // 16? 8ytes in 128bits.
            //      chess_prepare_for_pipelining
            // chess_loop_range(TP_HEADER_BYTES/16,)
            {
                readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                if
                    constexpr(TP_NUM_INPUTS == 2) {
                        readVal2 = stream_readincr_128b(inInterface.inStream1, 1); // read, but ignored
                    }
                window_writeincr(outInterface.outWindow0, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4) window_writeincr(outInterface.outWindow3, readVal1.val);
            }
        }

    if
        constexpr(TP_NUM_INPUTS == 1) {
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                    for (int j = 0; j < kRepeat; j++) {
                        readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                        window_writeincr(outInterface.outWindow0, readVal1.val);
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 2)
                                window_writeincr(outInterface.outWindow1, readVal1.val);
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 3)
                                window_writeincr(outInterface.outWindow2, readVal1.val);
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 4)
                                window_writeincr(outInterface.outWindow3, readVal1.val);
                    }
                }
#pragma unroll(GUARD_ZERO(kLsizeRem))
            for (int j = 0; j < kLsizeRem; j++) {
                readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                window_writeincr(outInterface.outWindow0, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, readVal1.val);
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4) window_writeincr(outInterface.outWindow3, readVal1.val);
            }
        }
    else { // TP_NUM_INPUTS==2
        if
            constexpr(TP_PATTERN == 0) {
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                            readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                            writeVal.val.insert(0, readVal1.val);
                            writeVal.val.insert(1, readVal2.val);
                            window_writeincr(outInterface.outWindow0, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 2)
                                    window_writeincr(outInterface.outWindow1, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 3)
                                    window_writeincr(outInterface.outWindow2, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 4)
                                    window_writeincr(outInterface.outWindow3, writeVal.val);
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                    readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                    writeVal.val.insert(0, readVal1.val);
                    writeVal.val.insert(1, readVal2.val);
                    window_writeincr(outInterface.outWindow0, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 4) window_writeincr(outInterface.outWindow3, writeVal.val);
                }
            }
        else if
            constexpr(TP_PATTERN == kSampleIntlv) {
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                            readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                            inIntlv =
                                ::aie::interleave_zip(readVal1.val, readVal2.val,
                                                      1); // convert to complex by interleaving zeros for imag parts
                            writeVal.val =
                                ::aie::concat<in128VectorType, in128VectorType>(inIntlv.first, inIntlv.second);
                            window_writeincr(outInterface.outWindow0, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 2)
                                    window_writeincr(outInterface.outWindow1, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 3)
                                    window_writeincr(outInterface.outWindow2, writeVal.val);
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 4)
                                    window_writeincr(outInterface.outWindow3, writeVal.val);
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                    readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                    inIntlv = ::aie::interleave_zip(readVal1.val, readVal2.val,
                                                    1); // convert to complex by interleaving zeros for imag parts
                    writeVal.val = ::aie::concat<in128VectorType, in128VectorType>(inIntlv.first, inIntlv.second);
                    window_writeincr(outInterface.outWindow0, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, writeVal.val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 4) window_writeincr(outInterface.outWindow3, writeVal.val);
                }
            }
        else if
            constexpr(TP_PATTERN == kSplit) { // merge, in this case
                constexpr int kSamplesIn128b = 128 / (8 * sizeof(TT_DATA));
                in128VectorType* lowerPtr[TP_NUM_OUTPUT_CLONES];
                in128VectorType* upperPtr[TP_NUM_OUTPUT_CLONES];
                lowerPtr[0] = (in128VectorType*)outInterface.outWindow0->ptr;
                upperPtr[0] = (in128VectorType*)(lowerPtr[0] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2) {
                        lowerPtr[1] = (in128VectorType*)outInterface.outWindow1->ptr;
                        upperPtr[1] = (in128VectorType*)(lowerPtr[1] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3) {
                        lowerPtr[2] = (in128VectorType*)outInterface.outWindow2->ptr;
                        upperPtr[2] = (in128VectorType*)(lowerPtr[2] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4) {
                        lowerPtr[3] = (in128VectorType*)outInterface.outWindow3->ptr;
                        upperPtr[3] = (in128VectorType*)(lowerPtr[3] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                            readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                            for (int k = 0; k < TP_NUM_OUTPUT_CLONES; k++) {
                                *lowerPtr[k]++ = readVal1.val;
                                *upperPtr[k]++ = readVal2.val;
                            }
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = stream_readincr_128b(inInterface.inStream0, 0);
                    readVal2 = stream_readincr_128b(inInterface.inStream1, 1);
                    for (int k = 0; k < TP_NUM_OUTPUT_CLONES; k++) {
                        *lowerPtr[k]++ = readVal1.val;
                        *upperPtr[k]++ = readVal2.val;
                    }
                }
            }
    }
};

// window to stream, generalized.
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void
kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES, TP_PATTERN, TP_HEADER_BYTES>::
    kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamAPI> outInterface) {
    T_buff_128b<TT_DATA> read128Val;
    T_buff_256b<TT_DATA> read256Val;
    T_buff_128b<TT_DATA> writeVal;
    using out256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    using out128VectorType = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
    T_buff_128b<TT_DATA> out128a, out128b;
    out128VectorType* rdptr0;
    out128VectorType* rdptr1;
    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);

    // handle header bytes
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    read128Val = window_readincr_128b(inInterface.inWindow0);
                    stream_writeincr_128b(outInterface.outStream0, read128Val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES == 2) stream_writeincr_128b(outInterface.outStream1, read128Val);
                }
        }

    if
        constexpr(TP_NUM_OUTPUT_CLONES == 1) {
            constexpr int kLsize = TP_WINDOW_VSIZE / kSamplesIn128b;
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    read128Val = window_readincr_128b(inInterface.inWindow0);
                    stream_writeincr_128b(outInterface.outStream0, read128Val);
                }
        }
    else { // TP_NUM_OUTPUT_CLONES==2
        if
            constexpr(TP_PATTERN == 0) {
                constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2);
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read256Val = window_readincr_256b(inInterface.inWindow0);
                        writeVal.val = read256Val.val.template extract<kSamplesIn128b>(0);
                        stream_writeincr_128b(outInterface.outStream0, writeVal, 0);
                        writeVal.val = read256Val.val.template extract<kSamplesIn128b>(1);
                        stream_writeincr_128b(outInterface.outStream1, writeVal, 1);
                    }
                if (TP_WINDOW_VSIZE / kSamplesIn128b % 2 == 1) { // odd number of 128b chunks
                    read256Val = window_readincr_256b(inInterface.inWindow0);
                    writeVal.val = read256Val.val.template extract<kSamplesIn128b>(0);
                    stream_writeincr_128b(outInterface.outStream0, writeVal, 0);
                }
            }
        else if
            constexpr(TP_PATTERN == kSampleIntlv) {
                constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / (256 / 8);
                for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read256Val = window_readincr_256b(inInterface.inWindow0);
                        out128a.val = ::aie::filter_even<out256VectorType>(read256Val.val);
                        out128b.val = ::aie::filter_odd<out256VectorType>(read256Val.val);
                        stream_writeincr_128b(outInterface.outStream0, out128a, 0);
                        stream_writeincr_128b(outInterface.outStream1, out128b, 1);
                    }
            }
        else if
            constexpr(TP_PATTERN == kSplit) {
                constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / (128 / 8) / 2;
                rdptr0 = (out128VectorType*)inInterface.inWindow0->ptr;
                rdptr1 = rdptr0 + kLsize; // advance by half the window.
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read128Val.val = *rdptr0++; // = window_readincr_128b(inInterface.inWindow0);
                        stream_writeincr_128b(outInterface.outStream0, read128Val, 0);
                        read128Val.val = *rdptr1++; // = window_readincr_128b(inInterface.inWindow0);
                        stream_writeincr_128b(outInterface.outStream1, read128Val, 1);
                    }
            }
    }
};

//-------------------------------------------------------------------------------------------------------
// This is the base specialization of the main class for when there is only one window in and out
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void widget_api_cast<TT_DATA,
                                   TP_IN_API,
                                   TP_OUT_API,
                                   TP_NUM_INPUTS,
                                   TP_WINDOW_VSIZE,
                                   TP_NUM_OUTPUT_CLONES,
                                   TP_PATTERN,
                                   TP_HEADER_BYTES>::transferData // transferData is QoR hook
    (input_window<TT_DATA>* __restrict inWindow0, output_window<TT_DATA>* __restrict outWindow0) {
    T_inputIF<TT_DATA, TP_IN_API> inInterface;
    T_outputIF<TT_DATA, TP_OUT_API> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outWindow0 = outWindow0;
    this->kernelClassMain(inInterface, outInterface);
};

// window API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_window<TT_DATA>* __restrict inWindow0,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    this->kernelClassMain(inInterface, outInterface);
};

// window API in and out, 3 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_window<TT_DATA>* __restrict inWindow0,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1,
    output_window<TT_DATA>* __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    outInterface.outWindow2 = outWindow2;
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 1 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0, output_window<TT_DATA>* __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = outWindow0;
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 2;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 3 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1,
    output_window<TT_DATA>* __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    outInterface.outWindow2 = outWindow2;
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 4 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1,
    output_window<TT_DATA>* __restrict outWindow2,
    output_window<TT_DATA>* __restrict outWindow3) {
    constexpr unsigned int TP_NUM_OUTPUTS = 4;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    outInterface.outWindow2 = outWindow2;
    outInterface.outWindow3 = outWindow3;
    this->kernelClassMain(inInterface, outInterface);
};

// dual stream in
// stream to window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_window<TT_DATA>* __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = outWindow0;
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 2;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1,
    output_window<TT_DATA>* __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    outInterface.outWindow2 = outWindow2;
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_window<TT_DATA>* __restrict outWindow0,
    output_window<TT_DATA>* __restrict outWindow1,
    output_window<TT_DATA>* __restrict outWindow2,
    output_window<TT_DATA>* __restrict outWindow3) {
    constexpr unsigned int TP_NUM_OUTPUTS = 4;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = outWindow0;
    outInterface.outWindow1 = outWindow1;
    outInterface.outWindow2 = outWindow2;
    outInterface.outWindow3 = outWindow3;
    this->kernelClassMain(inInterface, outInterface);
};

// Window to Stream
// window to stream, 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_window<TT_DATA>* __restrict inWindow0, output_stream<TT_DATA>* __restrict outStream0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outStream0 = outStream0;
    this->kernelClassMain(inInterface, outInterface);
};

// window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_window<TT_DATA>* __restrict inWindow0,
    output_stream<TT_DATA>* __restrict outStream0,
    output_stream<TT_DATA>* __restrict outStream1) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outStream0 = outStream0;
    outInterface.outStream1 = outStream1;
    this->kernelClassMain(inInterface, outInterface);
};
}
}
}
}
}
