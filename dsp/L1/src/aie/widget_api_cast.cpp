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
    constexpr int kVectSize = 256 / 8 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kVectSize>;
    t256VectorType readVal;
    constexpr int kLsize = (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA)) / kVectSize;
    t256VectorType* inPtr = (t256VectorType*)inInterface.inWindow0;
    t256VectorType* outPtr0 = (t256VectorType*)outInterface.outWindow0;
    t256VectorType* outPtr1 = (TP_NUM_OUTPUT_CLONES >= 2) ? (t256VectorType*)outInterface.outWindow1 : 0;
    t256VectorType* outPtr2 = (TP_NUM_OUTPUT_CLONES >= 2) ? (t256VectorType*)outInterface.outWindow2 : 0;

    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            readVal = *inPtr++;
            *outPtr0++ = readVal;
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 2)* outPtr1++ = readVal;
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 3)* outPtr2++ = readVal;
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
    constexpr int kVectSize = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kVectSize>;
    using t256VectorType = ::aie::vector<TT_DATA, kVectSize * 2>;
    t128VectorType readVal1;
    t128VectorType readVal2;
    t256VectorType writeVal;
    t128VectorType* out128Ptr0 = (t128VectorType*)outInterface.outWindow0;
    t128VectorType* out128Ptr1 = TP_NUM_OUTPUT_CLONES >= 2 ? (t128VectorType*)outInterface.outWindow1 : 0;
    t128VectorType* out128Ptr2 = TP_NUM_OUTPUT_CLONES >= 3 ? (t128VectorType*)outInterface.outWindow2 : 0;
    t128VectorType* out128Ptr3 = TP_NUM_OUTPUT_CLONES >= 4 ? (t128VectorType*)outInterface.outWindow3 : 0;
    t256VectorType* outPtr0 = (t256VectorType*)outInterface.outWindow0;
    t256VectorType* outPtr1 = TP_NUM_OUTPUT_CLONES >= 2 ? (t256VectorType*)outInterface.outWindow1 : 0;
    t256VectorType* outPtr2 = TP_NUM_OUTPUT_CLONES >= 3 ? (t256VectorType*)outInterface.outWindow2 : 0;
    t256VectorType* outPtr3 = TP_NUM_OUTPUT_CLONES >= 4 ? (t256VectorType*)outInterface.outWindow3 : 0;
    ::std::pair<t128VectorType, t128VectorType> inIntlv;
    constexpr unsigned int kRepeat = 4;
    constexpr unsigned int kDataReadSize = TP_NUM_INPUTS == 1 ? 16 : 32;
    constexpr int kLsize = (TP_WINDOW_VSIZE * sizeof(TT_DATA)) / (kDataReadSize * kRepeat);
    constexpr int kLsizeRem = ((TP_WINDOW_VSIZE * sizeof(TT_DATA)) % (kDataReadSize * kRepeat)) / kDataReadSize;

    // handle header bytes
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++) // 16? 8ytes in 128bits.
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                    if
                        constexpr(TP_NUM_INPUTS == 2) {
                            readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(
                                inInterface.inStream1); // read, but ignored
                        }
                    *out128Ptr0++ = readVal1;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 2)* out128Ptr1++ = readVal1;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 3)* out128Ptr2++ = readVal1;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 4)* out128Ptr3++ = readVal1;
                }
        }

    if
        constexpr(TP_NUM_INPUTS == 1) {
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                    for (int j = 0; j < kRepeat; j++) {
                        readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                        *out128Ptr0++ = readVal1;
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 2)* out128Ptr1++ = readVal1;
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 3)* out128Ptr2++ = readVal1;
                        if
                            constexpr(TP_NUM_OUTPUT_CLONES >= 4)* out128Ptr3++ = readVal1;
                    }
                }
#pragma unroll(GUARD_ZERO(kLsizeRem))
            for (int j = 0; j < kLsizeRem; j++) {
                readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                *out128Ptr0++ = readVal1;
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2)* out128Ptr1++ = readVal1;
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3)* out128Ptr2++ = readVal1;
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4)* out128Ptr3++ = readVal1;
            }
        }
    else { // TP_NUM_INPUTS==2
        if
            constexpr(TP_PATTERN == 0) {
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                            readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                            writeVal.insert(0, readVal1);
                            writeVal.insert(1, readVal2);
                            *outPtr0++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 2)* outPtr1++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 3)* outPtr2++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 4)* outPtr3++ = writeVal;
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                    readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                    writeVal.insert(0, readVal1);
                    writeVal.insert(1, readVal2);
                    *outPtr0++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 2)* outPtr1++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 3)* outPtr2++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 4)* outPtr3++ = writeVal;
                }
            }
        else if
            constexpr(TP_PATTERN == kSampleIntlv) {
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                            readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                            inIntlv = ::aie::interleave_zip(
                                readVal1, readVal2, 1); // convert to complex by interleaving zeros for imag parts
                            writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                            *outPtr0++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 2)* outPtr1++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 3)* outPtr2++ = writeVal;
                            if
                                constexpr(TP_NUM_OUTPUT_CLONES >= 4)* outPtr3++ = writeVal;
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                    readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                    inIntlv = ::aie::interleave_zip(readVal1, readVal2,
                                                    1); // convert to complex by interleaving zeros for imag parts
                    writeVal = ::aie::concat<t128VectorType, t128VectorType>(inIntlv.first, inIntlv.second);
                    *outPtr0++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 2)* outPtr1++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 3)* outPtr2++ = writeVal;
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES >= 4)* outPtr3++ = writeVal;
                }
            }
        else if
            constexpr(TP_PATTERN == kSplit) { // merge, in this case
                constexpr int kSamplesIn128b = 128 / (8 * sizeof(TT_DATA));
                t128VectorType* lowerPtr[TP_NUM_OUTPUT_CLONES];
                t128VectorType* upperPtr[TP_NUM_OUTPUT_CLONES];
                lowerPtr[0] = (t128VectorType*)outPtr0;
                upperPtr[0] = (t128VectorType*)(lowerPtr[0] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2) {
                        lowerPtr[1] = (t128VectorType*)outPtr1;
                        upperPtr[1] = (t128VectorType*)(lowerPtr[1] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3) {
                        lowerPtr[2] = (t128VectorType*)outPtr2;
                        upperPtr[2] = (t128VectorType*)(lowerPtr[2] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4) {
                        lowerPtr[3] = (t128VectorType*)outPtr3;
                        upperPtr[3] = (t128VectorType*)(lowerPtr[3] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
#pragma unroll(kRepeat)
                        for (int j = 0; j < kRepeat; j++) {
                            readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                            readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                            for (int k = 0; k < TP_NUM_OUTPUT_CLONES; k++) {
                                *lowerPtr[k]++ = readVal1;
                                *upperPtr[k]++ = readVal2;
                            }
                        }
                    }
#pragma unroll(GUARD_ZERO(kLsizeRem))
                for (int j = 0; j < kLsizeRem; j++) {
                    readVal1 = readincr_v<kVectSize, aie_stream_resource_in::a>(inInterface.inStream0);
                    readVal2 = readincr_v<kVectSize, aie_stream_resource_in::b>(inInterface.inStream1);
                    for (int k = 0; k < TP_NUM_OUTPUT_CLONES; k++) {
                        *lowerPtr[k]++ = readVal1;
                        *upperPtr[k]++ = readVal2;
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
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256VectorType = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128VectorType read128Val;
    t256VectorType read256Val;
    t128VectorType writeVal;
    t128VectorType out128a, out128b;
    t128VectorType* rdptr0 = (t128VectorType*)inInterface.inWindow0;
    t128VectorType* rdptr1;
    t256VectorType* rd256ptr;

    // handle header bytes
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / 16; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / 16, ) {
                    read128Val = *rdptr0++;
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0, read128Val);
                    if
                        constexpr(TP_NUM_OUTPUT_CLONES == 2)
                            writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outInterface.outStream1,
                                                                                           read128Val);
                }
        }
    rd256ptr = (t256VectorType*)rdptr0;

    if
        constexpr(TP_NUM_OUTPUT_CLONES == 1) {
            constexpr int kLsize = TP_WINDOW_VSIZE / kSamplesIn128b;
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    read128Val = *rdptr0++;
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0, read128Val);
                }
        }
    else { // TP_NUM_OUTPUT_CLONES==2
        if
            constexpr(TP_PATTERN == 0) {
                constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2);
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read256Val = *rd256ptr++;
                        writeVal = read256Val.template extract<kSamplesIn128b>(0);
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0,
                                                                                       writeVal);
                        writeVal = read256Val.template extract<kSamplesIn128b>(1);
                        writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outInterface.outStream1,
                                                                                       writeVal);
                    }
                if (TP_WINDOW_VSIZE / kSamplesIn128b % 2 == 1) { // odd number of 128b chunks
                    read256Val = *rd256ptr++;
                    writeVal = read256Val.template extract<kSamplesIn128b>(0);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0, writeVal);
                }
            }
        else if
            constexpr(TP_PATTERN == kSampleIntlv) {
                constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / (256 / 8);
                for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read256Val = *rd256ptr++;
                        out128a = ::aie::filter_even<t256VectorType>(read256Val);
                        out128b = ::aie::filter_odd<t256VectorType>(read256Val);
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0,
                                                                                       out128a);
                        writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outInterface.outStream1,
                                                                                       out128b);
                    }
            }
        else if
            constexpr(TP_PATTERN == kSplit) {
                constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / (128 / 8) / 2;
                // rdptr0 = (out128VectorType*)inInterface.inWindow0->ptr; set at head of function
                rdptr1 = rdptr0 + kLsize; // advance by half the window.
                for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                        read128Val = *rdptr0++; // = window_readincr_128b(inInterface.inWindow0);
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0,
                                                                                       read128Val);
                        read128Val = *rdptr1++; // = window_readincr_128b(inInterface.inWindow0);
                        writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outInterface.outStream1,
                                                                                       read128Val);
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
    (input_buffer<TT_DATA>& __restrict inWindow0, output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, TP_IN_API> inInterface;
    T_outputIF<TT_DATA, TP_OUT_API> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMain(inInterface, outInterface);
};

// window API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    this->kernelClassMain(inInterface, outInterface);
};

// window API in and out, 3 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1,
    output_buffer<TT_DATA>& __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    outInterface.outWindow2 = (void*)outWindow2.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 1 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0, output_buffer<TT_DATA>& __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 2;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 3 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1,
    output_buffer<TT_DATA>& __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    outInterface.outWindow2 = (void*)outWindow2.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 4 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1,
    output_buffer<TT_DATA>& __restrict outWindow2,
    output_buffer<TT_DATA>& __restrict outWindow3) {
    constexpr unsigned int TP_NUM_OUTPUTS = 4;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    outInterface.outWindow2 = (void*)outWindow2.data();
    outInterface.outWindow3 = (void*)outWindow3.data();
    this->kernelClassMain(inInterface, outInterface);
};

// dual stream in
// stream to window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_circular_buffer<TT_DATA>& __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1) {
    constexpr unsigned int TP_NUM_OUTPUTS = 2;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1,
    output_buffer<TT_DATA>& __restrict outWindow2) {
    constexpr unsigned int TP_NUM_OUTPUTS = 3;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    outInterface.outWindow2 = (void*)outWindow2.data();
    this->kernelClassMain(inInterface, outInterface);
};

// stream to window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_buffer<TT_DATA>& __restrict outWindow0,
    output_buffer<TT_DATA>& __restrict outWindow1,
    output_buffer<TT_DATA>& __restrict outWindow2,
    output_buffer<TT_DATA>& __restrict outWindow3) {
    constexpr unsigned int TP_NUM_OUTPUTS = 4;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    outInterface.outWindow1 = (void*)outWindow1.data();
    outInterface.outWindow2 = (void*)outWindow2.data();
    outInterface.outWindow3 = (void*)outWindow3.data();
    this->kernelClassMain(inInterface, outInterface);
};

// Window to Stream
// window to stream, 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0, output_stream<TT_DATA>* __restrict outStream0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outStream0 = outStream0;
    this->kernelClassMain(inInterface, outInterface);
};

// window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    output_stream<TT_DATA>* __restrict outStream0,
    output_stream<TT_DATA>* __restrict outStream1) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outStream0 = outStream0;
    outInterface.outStream1 = outStream1;
    this->kernelClassMain(inInterface, outInterface);
};
}
}
}
}
}
