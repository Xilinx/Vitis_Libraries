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

// Window to window - possibly useful for routing
namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kWindowAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                               T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    constexpr int kVectSize = 256 / 8 / sizeof(TT_DATA);
    using t256v = ::aie::vector<TT_DATA, kVectSize>;
    t256v readVal;
    constexpr int kLsize = (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA)) / kVectSize;
    t256v* inPtr = (t256v*)inInterface.inWindow0;
    t256v* outPtr0 = (t256v*)outInterface.outWindow0;
    t256v* outPtr1 = (TP_NUM_OUTPUT_CLONES >= 2) ? (t256v*)outInterface.outWindow1 : 0;
    t256v* outPtr2 = (TP_NUM_OUTPUT_CLONES >= 2) ? (t256v*)outInterface.outWindow2 : 0;

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
    using t128v = ::aie::vector<TT_DATA, kVectSize>;
    using t256v = ::aie::vector<TT_DATA, kVectSize * 2>;
    t128v readVal1;
    t128v readVal2;
    t256v writeVal;
    t128v* out128Ptr0 = (t128v*)outInterface.outWindow0;
    t128v* out128Ptr1 = TP_NUM_OUTPUT_CLONES >= 2 ? (t128v*)outInterface.outWindow1 : 0;
    t128v* out128Ptr2 = TP_NUM_OUTPUT_CLONES >= 3 ? (t128v*)outInterface.outWindow2 : 0;
    t128v* out128Ptr3 = TP_NUM_OUTPUT_CLONES >= 4 ? (t128v*)outInterface.outWindow3 : 0;
    t256v* outPtr0 = (t256v*)outInterface.outWindow0;
    t256v* outPtr1 = TP_NUM_OUTPUT_CLONES >= 2 ? (t256v*)outInterface.outWindow1 : 0;
    t256v* outPtr2 = TP_NUM_OUTPUT_CLONES >= 3 ? (t256v*)outInterface.outWindow2 : 0;
    t256v* outPtr3 = TP_NUM_OUTPUT_CLONES >= 4 ? (t256v*)outInterface.outWindow3 : 0;
    ::std::pair<t128v, t128v> inIntlv;
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
            // since the header has been output, all output 256b pointers must move on.
            outPtr0++;
            outPtr1++;
            outPtr2++;
            outPtr3++;
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
                            writeVal = ::aie::concat<t128v, t128v>(inIntlv.first, inIntlv.second);
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
                    writeVal = ::aie::concat<t128v, t128v>(inIntlv.first, inIntlv.second);
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
                t128v* lowerPtr[TP_NUM_OUTPUT_CLONES];
                t128v* upperPtr[TP_NUM_OUTPUT_CLONES];
                lowerPtr[0] = (t128v*)outPtr0;
                upperPtr[0] = (t128v*)(lowerPtr[0] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 2) {
                        lowerPtr[1] = (t128v*)outPtr1;
                        upperPtr[1] = (t128v*)(lowerPtr[1] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 3) {
                        lowerPtr[2] = (t128v*)outPtr2;
                        upperPtr[2] = (t128v*)(lowerPtr[2] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
                    }
                if
                    constexpr(TP_NUM_OUTPUT_CLONES >= 4) {
                        lowerPtr[3] = (t128v*)outPtr3;
                        upperPtr[3] = (t128v*)(lowerPtr[3] + TP_WINDOW_VSIZE / (2 * kSamplesIn128b));
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
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kStreamAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                               T_outputIF<TT_DATA, kStreamAPI> outInterface) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    using t256v = ::aie::vector<TT_DATA, kSamplesIn128b * 2>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t128v read128Val;
    t256v read256Val;
    t128v writeVal;
    t128v out128a, out128b;
    t128v* rdptr0 = (t128v*)inInterface.inWindow0;
    t128v* rdptr1;
    t256v* rd256ptr;

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
    rd256ptr = (t256v*)rdptr0;

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
                        out128a = ::aie::filter_even<t256v>(read256Val);
                        out128b = ::aie::filter_odd<t256v>(read256Val);
                        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outInterface.outStream0,
                                                                                       out128a);
                        writeincr<aie_stream_resource_out::b, TT_DATA, kSamplesIn128b>(outInterface.outStream1,
                                                                                       out128b);
                    }
            }
        else if
            constexpr(TP_PATTERN == kSplit) {
                constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / (128 / 8) / 2;
                // rdptr0 = (out128v*)inInterface.inWindow0->ptr; set at head of function
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

#ifdef __SUPPORTS_ACC64__
// CascStream to window
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kCascStreamAPI,
                             kWindowAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kCascStreamAPI> inInterface,
                                                               T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 256 / 8 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t512v = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    input_cascade<accTag>* __restrict inCasc0 = (input_cascade<accTag>*)inInterface.inStream0;
    accVect_t acc;
    t256v readVal1, readVal1a, readVal1b;
    t256v readVal2;
    t256v read256Val;
    t512v writeVal;
    t256v writeVal2a, writeVal2b;
    t256v* outPtr0 = (t256v*)outInterface.outWindow0; //&inBuff[0];
    ::std::pair<t256v, t256v> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    acc = readincr_v<kCascVWidth, accTag>(inCasc0); // cascade read;
                    readVal1 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
                    chess_separator_scheduler(0);                   // avoid deadlock in larger parallel powers
                    readVal2 = readincr_v<kSamplesIn256b>(inInterface.inStream1); // read, but ignored
                    *outPtr0++ = readVal1;
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            acc = readincr_v<kCascVWidth, accTag>(inCasc0); // cascade read;
            readVal1 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
            chess_separator_scheduler(0);                   // avoid deadlock in larger parallel powers
            readVal2 = readincr_v<kSamplesIn256b>(inInterface.inStream1);
            inIntlv = ::aie::interleave_zip(readVal1, readVal2, 1);
            writeVal = ::aie::concat<t256v, t256v>(inIntlv.first, inIntlv.second);
            writeVal2a = writeVal.template extract<kSamplesIn256b>(0);
            *outPtr0++ = writeVal2a;
            writeVal2b = writeVal.template extract<kSamplesIn256b>(1);
            *outPtr0++ = writeVal2b;
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// StreamCasc to window
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kStreamCascAPI,
                             kWindowAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kStreamCascAPI> inInterface,
                                                               T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 256 / 8 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t512v = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    input_cascade<accTag>* __restrict inCasc1 = (input_cascade<accTag>*)inInterface.inStream1;
    accVect_t acc;
    t256v readVal1, readVal1a, readVal1b;
    t256v readVal2;
    t256v read256Val;
    t512v writeVal;
    t256v writeVal2a, writeVal2b;
    t256v* outPtr0 = (t256v*)outInterface.outWindow0; //&inBuff[0];
    ::std::pair<t256v, t256v> inIntlv;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++) // 16? bytes/ 128bits. 8/128 = 1/16
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    acc = readincr_v<kCascVWidth, accTag>(inCasc1); // cascade read;
                    readVal1 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
                    chess_separator_scheduler(0);                   // avoid deadlock in larger parallel powers
                    readVal2 = readincr_v<kSamplesIn256b>(inInterface.inStream0); // read, but ignored
                    *outPtr0++ = readVal1;
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            acc = readincr_v<kCascVWidth, accTag>(inCasc1); // cascade read;
            readVal2 = acc.template to_vector<TT_DATA>(0);  // convert acc type to standard type.
            chess_separator_scheduler(0);                   // avoid deadlock in larger parallel powers
            readVal1 = readincr_v<kSamplesIn256b>(inInterface.inStream0);
            inIntlv = ::aie::interleave_zip(readVal1, readVal2, 1);
            writeVal = ::aie::concat<t256v, t256v>(inIntlv.first, inIntlv.second);
            writeVal2a = writeVal.template extract<kSamplesIn256b>(0);
            *outPtr0++ = writeVal2a;
            writeVal2b = writeVal.template extract<kSamplesIn256b>(1);
            *outPtr0++ = writeVal2b;
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// window to CascStream
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kCascStreamAPI,
                             1,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                               T_outputIF<TT_DATA, kCascStreamAPI> outInterface) {
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 32 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t512v = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256v read256Val, read256Val0, read256Val1;
    t512v read512Val;
    t256v writeVal;
    t256v out256a, out256b, out256a0, out256a1;
    t512v out512a;
    t256v* rdptr0 = (t256v*)inInterface.inWindow0; //&outBuff[0];
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    accVect_t acc;
    output_cascade<accTag>* __restrict outCasc0 = (output_cascade<accTag>*)outInterface.outStream0;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    read256Val = *rdptr0++;
                    acc = ::aie::from_vector<accTag>(read256Val);
                    writeincr<accTag, kCascVWidth>(outCasc0, acc);
                    chess_separator_scheduler(0); // avoid deadlock in larger parallel powers
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream1, read256Val);
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val0 = *rdptr0++;
            read256Val1 = *rdptr0++;
            read512Val = ::aie::concat<t256v, t256v>(read256Val0, read256Val1);
            out256a = ::aie::filter_even<t512v>(read512Val);
            out256b = ::aie::filter_odd<t512v>(read512Val);
            acc = ::aie::from_vector<accTag>(out256a);
            writeincr<accTag, kCascVWidth>(outCasc0, acc);
            chess_separator_scheduler(0); // avoid deadlock in larger parallel powers
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream1, out256b);
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// window to StreamCasc
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kStreamCascAPI,
                             1,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                               T_outputIF<TT_DATA, kStreamCascAPI> outInterface) {
    constexpr unsigned int kDataReadSize = 32;
    constexpr int kSamplesIn256b = 32 / sizeof(TT_DATA);
    constexpr int kCascVWidth = fnFFTCascVWidth<TT_DATA>();
    using t512v = ::aie::vector<TT_DATA, kSamplesIn256b * 2>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    t256v read256Val, read256Val0, read256Val1;
    t512v read512Val;
    t256v writeVal;
    t256v out256a, out256b, out256b0, out256b1;
    t512v out512b;
    t256v* rdptr0 = (t256v*)inInterface.inWindow0; //&outBuff[0];
    using accTag = typename tFFTAccBaseType<TT_DATA>::type;
    using accVect_t = ::aie::accum<accTag, kCascVWidth>;
    accVect_t acc;
    output_cascade<accTag>* __restrict outCasc1 = (output_cascade<accTag>*)outInterface.outStream1;

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / kDataReadSize; i++)
                chess_prepare_for_pipelining chess_loop_range(TP_HEADER_BYTES / kDataReadSize, ) {
                    read256Val = *rdptr0++;
                    acc = ::aie::from_vector<accTag>(read256Val);
                    writeincr<accTag, kCascVWidth>(outCasc1, acc);
                    chess_separator_scheduler(0); // avoid deadlock in larger parallel powers
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream0, read256Val);
                }
            chess_memory_fence(); // necessary, as figure of 8 lockup can occur in the trellis of kernels
        }

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b * 2); // number of 256 bit ops in Window
    for (int k = 0; k < kLsize; k++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val0 = *rdptr0++;
            read256Val1 = *rdptr0++;
            read512Val = ::aie::concat<t256v, t256v>(read256Val0, read256Val1);
            out256a = ::aie::filter_even<t512v>(read512Val);
            out256b = ::aie::filter_odd<t512v>(read512Val);
            acc = ::aie::from_vector<accTag>(out256b);
            writeincr<accTag, kCascVWidth>(outCasc1, acc);
            chess_separator_scheduler(0); // avoid deadlock in larger parallel powers
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream0, out256a);
        }
};
#endif //__SUPPORTS_ACC64__

// Window to stream TP_NUM_INPUTS to 1 interleave
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kStreamAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMW2SS(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                                T_outputIF<TT_DATA, kStreamAPI> outInterface) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    t256v read256Val0;
    t256v read256Val1;
    t256v writeVal;
    t256v* rdptr0 = (t256v*)inInterface.inWindow0;
    t256v* rdptr1 = (t256v*)inInterface.inWindow1;
    t128v* rdptr128_0;
    t128v* rdptr128_1;
    ::std::pair<t256v, t256v> inIntlv;
    t128v read128Val0;
    t128v read128Val1;
    ::std::pair<t128v, t128v> inIntlv128;

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b);
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            read256Val0 = *rdptr0++;
            read256Val1 = *rdptr1++;
            inIntlv = ::aie::interleave_zip(read256Val0, read256Val1, 1); // Interleave 2 to 1
            writeVal = inIntlv.first;
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream0, writeVal);
            writeVal = inIntlv.second;
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream0, writeVal);
        }
    rdptr128_0 = (t128v*)rdptr0;
    rdptr128_1 = (t128v*)rdptr1;
    if ((TP_WINDOW_VSIZE / kSamplesIn256b) % 2 == 1) { // odd number of 256b chunks
        // read 128-bits from each and interleave to a 256-bit vector
        read128Val0 = *rdptr128_0;
        read128Val1 = *rdptr128_1;
        inIntlv128 = ::aie::interleave_zip(read128Val0, read128Val1, 1); // Interleave 2 to 1
        writeVal = ::aie::concat<t128v, t128v>(inIntlv128.first, inIntlv128.second);
        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn256b>(outInterface.outStream0, writeVal);
    }
};

// Window to window TP_NUM_INPUTS to 1 interleave
template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
INLINE_DECL void kernelClass<TT_DATA,
                             kWindowAPI,
                             kWindowAPI,
                             TP_NUM_INPUTS,
                             TP_WINDOW_VSIZE,
                             TP_NUM_OUTPUT_CLONES,
                             TP_PATTERN,
                             TP_HEADER_BYTES>::kernelClassMW2SW(T_inputIF<TT_DATA, kWindowAPI> inInterface,
                                                                T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    constexpr int kSamplesIn512b = kSamplesIn128b * 4;
    static_assert((TP_WINDOW_VSIZE / kSamplesIn256b) % 2 == 0, "ERROR: Window size must be divisible by 256-bits");
    using t512v = ::aie::vector<TT_DATA, kSamplesIn512b>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    std::array<t256v, TP_NUM_INPUTS> read256Val;
    t256v read256Val0;
    t256v read256Val1;
    t256v read256Val2;
    t256v read256Val3;
    t256v writeVal;
    t256v* rdptr0 = (t256v*)inInterface.inWindow0;
    t256v* rdptr1 = (t256v*)inInterface.inWindow1;
    t256v* rdptr2 = (t256v*)inInterface.inWindow2;
    t256v* rdptr3 = (t256v*)inInterface.inWindow3;
    t128v* rdptr128_0;
    t128v* rdptr128_1;
    t128v* rdptr128_2;
    t128v* rdptr128_3;
    ::std::pair<t256v, t256v> inIntlv;
    t128v read128Val0;
    t128v read128Val1;
    t128v read128Val2;
    t128v read128Val3;
    ::std::pair<t128v, t128v> inIntlv128;

    t256v* outPtr0 = (t256v*)outInterface.outWindow0;

    constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn256b);

    if
        constexpr(TP_NUM_INPUTS == 2) {
            // Max efficiency with 256-bit read vectors. Loop consists of 2 VLDs, 2VST and 1 VSHUFFLE in 2 cycle loop
            // iteration.
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    read256Val0 = *rdptr0++;
                    read256Val1 = *rdptr1++;
                    inIntlv = ::aie::interleave_zip(read256Val0, read256Val1, 1); // Interleave 2 to 1
                    writeVal = inIntlv.first;
                    *outPtr0++ = writeVal;
                    writeVal = inIntlv.second;
                    *outPtr0++ = writeVal;
                }
            rdptr128_0 = (t128v*)rdptr0;
            rdptr128_1 = (t128v*)rdptr1;
            if ((TP_WINDOW_VSIZE / kSamplesIn256b) % 2 == 1) { // odd number of 256b chunks
                // read 128-bits from each and interleave to a 256-bit vector
                read128Val0 = *rdptr128_0;
                read128Val1 = *rdptr128_1;
                inIntlv128 = ::aie::interleave_zip(read128Val0, read128Val1, 1); // Interleave 2 to 1
                writeVal = ::aie::concat<t128v, t128v>(inIntlv128.first, inIntlv128.second);
                *outPtr0++ = writeVal;
            }
        }
    else if
        constexpr(TP_NUM_INPUTS == 3 || TP_NUM_INPUTS == 5 || TP_NUM_INPUTS == 6 || TP_NUM_INPUTS == 7) {
            // Loop: vector read, extract and interleave
            //   for (int i = 0; i<kLsize; i++)
            //     chess_prepare_for_pipelining
            //     chess_loop_range(kLsize,)
            //     {
            //       read256Val[0] = *rdptr0++;
            //       read256Val[1] = *rdptr1++;
            //       read256Val[2] = *rdptr2++;
            //       #pragma unroll (TP_NUM_INPUTS * kSamplesIn256b)
            //       for (int j = 0; j<(TP_NUM_INPUTS * kSamplesIn256b); j++) {
            //         writeVal[j % kSamplesIn256b] = read256Val[j % TP_NUM_INPUTS][j / TP_NUM_INPUTS];
            //           if ((j - kSamplesIn256b + 1) % kSamplesIn256b == 0){
            //             *outPtr0++ = writeVal;

            //           }
            //       }
            //     }

            // Loop unrolled: vector read, extract and interleave
            // for (int i = 0; i<kLsize; i++)
            //   chess_prepare_for_pipelining
            //   chess_loop_range(kLsize,)
            //   {
            //     read256Val[0] = *rdptr0++;
            //     read256Val[1] = *rdptr1++;
            //     read256Val[2] = *rdptr2++;
            //       writeVal[0] = read256Val[0][0];
            //       writeVal[1] = read256Val[1][0];
            //       writeVal[2] = read256Val[2][0];
            //       writeVal[3] = read256Val[0][1];
            //       writeVal[4] = read256Val[1][1];
            //       writeVal[5] = read256Val[2][1];
            //       writeVal[6] = read256Val[0][2];
            //       writeVal[7] = read256Val[1][2];
            //       *outPtr0++ = writeVal;
            //       writeVal[0] = read256Val[2][2];
            //       writeVal[1] = read256Val[0][3];
            //       writeVal[2] = read256Val[1][3];
            //       writeVal[3] = read256Val[2][3];
            //       writeVal[4] = read256Val[0][4];
            //       writeVal[5] = read256Val[1][4];
            //       writeVal[6] = read256Val[2][4];
            //       writeVal[7] = read256Val[0][5];
            //       *outPtr0++ = writeVal;
            //       writeVal[0] = read256Val[1][5];
            //       writeVal[1] = read256Val[2][5];
            //       writeVal[2] = read256Val[0][6];
            //       writeVal[3] = read256Val[1][6];
            //       writeVal[4] = read256Val[2][6];
            //       writeVal[5] = read256Val[0][7];
            //       writeVal[6] = read256Val[1][7];
            //       writeVal[7] = read256Val[2][7];
            //       *outPtr0++ = writeVal;
            //   }

            // Loop unrolled: sample read and interleave. Optimal performance (point solution)
            // TT_DATA* rdptrSample0 = (TT_DATA*)inInterface.inWindow0;
            // TT_DATA* rdptrSample1 = (TT_DATA*)inInterface.inWindow1;
            // TT_DATA* rdptrSample2 = (TT_DATA*)inInterface.inWindow2;

            // for (int i = 0; i<kLsize; i++)
            //   chess_prepare_for_pipelining
            //   chess_loop_range(kLsize,)
            //   {
            //       t256v writeValLoc;

            //       writeValLoc[0] = *rdptrSample0++;
            //       writeValLoc[1] = *rdptrSample1++;
            //       writeValLoc[2] = *rdptrSample2++;
            //       writeValLoc[3] = *rdptrSample0++;
            //       writeValLoc[4] = *rdptrSample1++;
            //       writeValLoc[5] = *rdptrSample2++;
            //       writeValLoc[6] = *rdptrSample0++;
            //       writeValLoc[7] = *rdptrSample1++;
            //       *outPtr0++ = writeValLoc;
            //       writeValLoc[0] = *rdptrSample2++;
            //       writeValLoc[1] = *rdptrSample0++;
            //       writeValLoc[2] = *rdptrSample1++;
            //       writeValLoc[3] = *rdptrSample2++;
            //       writeValLoc[4] = *rdptrSample0++;
            //       writeValLoc[5] = *rdptrSample1++;
            //       writeValLoc[6] = *rdptrSample2++;
            //       writeValLoc[7] = *rdptrSample0++;
            //       *outPtr0++ = writeValLoc;
            //       writeValLoc[0] = *rdptrSample1++;
            //       writeValLoc[1] = *rdptrSample2++;
            //       writeValLoc[2] = *rdptrSample0++;
            //       writeValLoc[3] = *rdptrSample1++;
            //       writeValLoc[4] = *rdptrSample2++;
            //       writeValLoc[5] = *rdptrSample0++;
            //       writeValLoc[6] = *rdptrSample1++;
            //       writeValLoc[7] = *rdptrSample2++;
            //       *outPtr0++ = writeValLoc;
            //   }

            // Loop: sample read and interleave
            std::array<TT_DATA*, 8> rdptrSample = {(TT_DATA*)inInterface.inWindow0, (TT_DATA*)inInterface.inWindow1,
                                                   (TT_DATA*)inInterface.inWindow2, (TT_DATA*)inInterface.inWindow3,
                                                   (TT_DATA*)inInterface.inWindow4, (TT_DATA*)inInterface.inWindow5,
                                                   (TT_DATA*)inInterface.inWindow6, (TT_DATA*)inInterface.inWindow7};

            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    t256v writeValLoc;
#pragma unroll(TP_NUM_INPUTS* kSamplesIn256b)
                    for (int j = 0; j < (TP_NUM_INPUTS * kSamplesIn256b); j++) {
                        writeValLoc[j % kSamplesIn256b] = *rdptrSample[j % TP_NUM_INPUTS]++;

                        if ((j - kSamplesIn256b + 1) % kSamplesIn256b == 0) {
                            *outPtr0++ = writeValLoc;
                        }
                    }
                }

            // Interleave 4 to 1
        }
    else if
        constexpr(TP_NUM_INPUTS == 4) {
            // All good here, but VSHUFFLE operates on 256-bits, but it could operate on 512=-bits.
            // Efficiency 4 VST every 6 clock cycles
            // for (int i = 0; i<kLsize; i++)
            //   chess_prepare_for_pipelining
            //   chess_loop_range(kLsize,)
            //   {
            //     ::std::pair<t256v,t256v>      inIntlv0;
            //     ::std::pair<t256v,t256v>      inIntlv1;
            //     read256Val0 = *rdptr0++;
            //     read256Val1 = *rdptr1++;
            //     read256Val2 = *rdptr2++;
            //     read256Val3 = *rdptr3++;
            //     inIntlv0 = ::aie::interleave_zip(read256Val0,read256Val2, 1);//Interleave 4 to 2
            //     inIntlv1 = ::aie::interleave_zip(read256Val1,read256Val3, 1);//Interleave 4 to 2
            //     inIntlv  = ::aie::interleave_zip(inIntlv0.first,inIntlv1.first, 1);//Interleave 2 to 1
            //     writeVal = inIntlv.first;
            //     *outPtr0++ = writeVal;
            //     writeVal = inIntlv.second;
            //     *outPtr0++ = writeVal;
            //     inIntlv  = ::aie::interleave_zip(inIntlv0.second,inIntlv1.second, 1);//Interleave 2 to 1
            //     writeVal = inIntlv.first;
            //     *outPtr0++ = writeVal;
            //     writeVal = inIntlv.second;
            //     *outPtr0++ = writeVal;
            //   }
            //   rdptr128_0 = (t128v*)rdptr0;
            //   rdptr128_1 = (t128v*)rdptr1;
            //   rdptr128_2 = (t128v*)rdptr2;
            //   rdptr128_3 = (t128v*)rdptr3;
            //   ::std::pair<t128v,t128v>      inIntlv128_0;
            //   ::std::pair<t128v,t128v>      inIntlv128_1;
            //   if ((TP_WINDOW_VSIZE/kSamplesIn256b) % 2 == 1) {//odd number of 256b chunks
            //     // read 128-bits from each and interleave to a 256-bit vector
            //       read128Val0 = *rdptr128_0;
            //       read128Val1 = *rdptr128_1;
            //       read128Val2 = *rdptr128_2;
            //       read128Val3 = *rdptr128_3;
            //       inIntlv128_0 = ::aie::interleave_zip(read128Val0,read128Val2, 1);//Interleave 4 to 2
            //       inIntlv128_1 = ::aie::interleave_zip(read128Val1,read128Val3, 1);//Interleave 4 to 2
            //       inIntlv128   = ::aie::interleave_zip(inIntlv128_0.first, inIntlv128_0.first, 1);//Interleave 4 to 2
            //       writeVal = ::aie::concat<t128v,t128v>(inIntlv128.first, inIntlv128.second);
            //       *outPtr0++ = writeVal;
            //       inIntlv128   = ::aie::interleave_zip(inIntlv128_0.second, inIntlv128_0.second, 1);//Interleave 4 to
            //       2
            //       writeVal = ::aie::concat<t128v,t128v>(inIntlv128.first, inIntlv128.second);
            //       *outPtr0++ = writeVal;
            //   }

            // OK, that's much better. 100% efficiency. VST on every cycle
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    ::std::pair<t512v, t512v> inIntlv0;
                    ::std::pair<t512v, t512v> inIntlv1;
                    read256Val0 = *rdptr0++;
                    read256Val1 = *rdptr1++;
                    read256Val2 = *rdptr2++;
                    read256Val3 = *rdptr3++;
                    t256v writeValLoc;
                    t512v read512_0 = ::aie::concat<t256v, t256v>(read256Val0, read256Val1);
                    t512v read512_1 = ::aie::concat<t256v, t256v>(read256Val2, read256Val3);
                    inIntlv0 = ::aie::interleave_zip(read512_0, read512_1, 1);            // Interleave 4 to 2
                    inIntlv1 = ::aie::interleave_zip(inIntlv0.first, inIntlv0.second, 1); // Interleave 2 to 1
                    writeValLoc = inIntlv1.first.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv1.first.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv1.second.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv1.second.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
                }

            //   } else if constexpr (TP_NUM_INPUTS == 5) {
            // // Loop: sample read and interleave
            //   std::array<TT_DATA*,TP_NUM_INPUTS> rdptrSample =   {(TT_DATA*)inInterface.inWindow0,
            //                                                     (TT_DATA*)inInterface.inWindow1,
            //                                                     (TT_DATA*)inInterface.inWindow2,
            //                                                     (TT_DATA*)inInterface.inWindow3,
            //                                                     (TT_DATA*)inInterface.inWindow4
            //                                                     };

            //   for (int i = 0; i<kLsize; i++)
            //     chess_prepare_for_pipelining
            //     chess_loop_range(kLsize,)
            //     {
            //       t256v writeValLoc;
            //       #pragma unroll (TP_NUM_INPUTS * kSamplesIn256b)
            //       for (int j = 0; j<(TP_NUM_INPUTS * kSamplesIn256b); j++) {
            //         writeValLoc[j % kSamplesIn256b] = *rdptrSample[j % TP_NUM_INPUTS]++;

            //           if ((j - kSamplesIn256b + 1) % kSamplesIn256b == 0){
            //             *outPtr0++ = writeValLoc;
            //           }
            //       }
            //     }

            // Interleave 6 to 1
        }
    else if
        constexpr(TP_NUM_INPUTS == 8) {
            std::array<t256v*, 8> rdptrSampleVec = {(t256v*)inInterface.inWindow0, (t256v*)inInterface.inWindow1,
                                                    (t256v*)inInterface.inWindow2, (t256v*)inInterface.inWindow3,
                                                    (t256v*)inInterface.inWindow4, (t256v*)inInterface.inWindow5,
                                                    (t256v*)inInterface.inWindow6, (t256v*)inInterface.inWindow7};

            //
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    ::std::pair<t512v, t512v> inIntlv0;
                    ::std::pair<t512v, t512v> inIntlv1;
                    ::std::pair<t512v, t512v> inIntlv2;
                    ::std::pair<t512v, t512v> inIntlv3;
                    ::std::pair<t512v, t512v> inIntlv4;
                    read256Val0 = *rdptrSampleVec[0]++;
                    read256Val1 = *rdptrSampleVec[1]++;
                    read256Val2 = *rdptrSampleVec[4]++;
                    read256Val3 = *rdptrSampleVec[5]++;
                    t256v writeValLoc;
                    t512v read512_0 = ::aie::concat<t256v, t256v>(read256Val0, read256Val1); // 01
                    t512v read512_1 = ::aie::concat<t256v, t256v>(read256Val2, read256Val3); // 45
                    inIntlv0 = ::aie::interleave_zip(read512_0, read512_1, 1); // Interleave 8 to 4 - 04.15
                    read256Val0 = *rdptrSampleVec[2]++;
                    read256Val1 = *rdptrSampleVec[3]++;
                    read256Val2 = *rdptrSampleVec[6]++;
                    read256Val3 = *rdptrSampleVec[7]++;
                    read512_0 = ::aie::concat<t256v, t256v>(read256Val0, read256Val1);     // 23
                    read512_1 = ::aie::concat<t256v, t256v>(read256Val2, read256Val3);     // 67
                    inIntlv1 = ::aie::interleave_zip(read512_0, read512_1, 1);             // Interleave 8 to 4 - 26.37
                    inIntlv2 = ::aie::interleave_zip(inIntlv0.first, inIntlv1.first, 1);   // Interleave 4 to 2 - 02.46
                    inIntlv3 = ::aie::interleave_zip(inIntlv0.second, inIntlv1.second, 1); // Interleave 4 to 2 - 13.57
                    inIntlv4 = ::aie::interleave_zip(inIntlv2.first, inIntlv3.first, 1);   // Interleave 4 to 2 - 01.23
                    writeValLoc = inIntlv4.first.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.first.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.second.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.second.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
                    inIntlv4 = ::aie::interleave_zip(inIntlv2.second, inIntlv3.second, 1); // Interleave 4 to 2 - 45.67
                    writeValLoc = inIntlv4.first.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.first.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.second.template extract<kSamplesIn256b>(0);
                    *outPtr0++ = writeValLoc;
                    writeValLoc = inIntlv4.second.template extract<kSamplesIn256b>(1);
                    *outPtr0++ = writeValLoc;
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

#ifdef __SUPPORTS_ACC64__
// CascStream to window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kCascStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_cascade<cacc64>* __restrict inStream0,
    input_stream<TT_DATA>* __restrict inStream1,
    output_circular_buffer<TT_DATA>& __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kCascStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMain(inInterface, outInterface);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// StreamCasc to window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kStreamCascAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_stream<TT_DATA>* __restrict inStream0,
    input_cascade<cacc64>* __restrict inStream1,
    output_circular_buffer<TT_DATA>& __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamCascAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    inInterface.inStream1 = inStream1;
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMain(inInterface, outInterface);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// window to CascStream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    output_cascade<cacc64>* __restrict outStream0,
    output_stream<TT_DATA>* __restrict outStream1) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kCascStreamAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outStream0 = outStream0;
    outInterface.outStream1 = outStream1;
    this->kernelClassMain(inInterface, outInterface);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// window to StreamCasc, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    output_stream<TT_DATA>* __restrict outStream0,
    output_cascade<cacc64>* __restrict outStream1) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamCascAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    outInterface.outStream0 = outStream0;
    outInterface.outStream1 = outStream1;
    this->kernelClassMain(inInterface, outInterface);
};
#endif //__SUPPORTS_ACC64__

// Window to stream 2 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    output_stream<TT_DATA>* __restrict outStream0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    outInterface.outStream0 = outStream0;
    this->kernelClassMW2SS(inInterface, outInterface);
};

// Window to stream 3 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    output_stream<TT_DATA>* __restrict outStream0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    outInterface.outStream0 = outStream0;
    this->kernelClassMW2SS(inInterface, outInterface);
};

// Window to window 2 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};

// Window to window 3 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};

// Window to window 4 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 4, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    input_buffer<TT_DATA>& __restrict inWindow3,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    inInterface.inWindow3 = (void*)inWindow3.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};

// Window to window 5 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 5, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    input_buffer<TT_DATA>& __restrict inWindow3,
    input_buffer<TT_DATA>& __restrict inWindow4,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    inInterface.inWindow3 = (void*)inWindow3.data();
    inInterface.inWindow4 = (void*)inWindow4.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};
// Window to window 6 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 6, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    input_buffer<TT_DATA>& __restrict inWindow3,
    input_buffer<TT_DATA>& __restrict inWindow4,
    input_buffer<TT_DATA>& __restrict inWindow5,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    inInterface.inWindow3 = (void*)inWindow3.data();
    inInterface.inWindow4 = (void*)inWindow4.data();
    inInterface.inWindow5 = (void*)inWindow5.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};

// Window to window 7 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 7, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    input_buffer<TT_DATA>& __restrict inWindow3,
    input_buffer<TT_DATA>& __restrict inWindow4,
    input_buffer<TT_DATA>& __restrict inWindow5,
    input_buffer<TT_DATA>& __restrict inWindow6,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    inInterface.inWindow3 = (void*)inWindow3.data();
    inInterface.inWindow4 = (void*)inWindow4.data();
    inInterface.inWindow5 = (void*)inWindow5.data();
    inInterface.inWindow6 = (void*)inWindow6.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};

// Window to window 8 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
NOINLINE_DECL void
widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 8, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::transferData(
    input_buffer<TT_DATA>& __restrict inWindow0,
    input_buffer<TT_DATA>& __restrict inWindow1,
    input_buffer<TT_DATA>& __restrict inWindow2,
    input_buffer<TT_DATA>& __restrict inWindow3,
    input_buffer<TT_DATA>& __restrict inWindow4,
    input_buffer<TT_DATA>& __restrict inWindow5,
    input_buffer<TT_DATA>& __restrict inWindow6,
    input_buffer<TT_DATA>& __restrict inWindow7,
    output_buffer<TT_DATA>& __restrict outWindow0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inWindow0 = (void*)inWindow0.data();
    inInterface.inWindow1 = (void*)inWindow1.data();
    inInterface.inWindow2 = (void*)inWindow2.data();
    inInterface.inWindow3 = (void*)inWindow3.data();
    inInterface.inWindow4 = (void*)inWindow4.data();
    inInterface.inWindow5 = (void*)inWindow5.data();
    inInterface.inWindow6 = (void*)inWindow6.data();
    inInterface.inWindow7 = (void*)inWindow7.data();
    outInterface.outWindow0 = (void*)outWindow0.data();
    this->kernelClassMW2SW(inInterface, outInterface);
};
}
}
}
}
}
