/*
Widget API cast kernal code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIEARCH__ 1
#define __AIENGINE__ 1
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "widget_api_cast.hpp"
#include "widget_api_cast_utils.hpp"

#include "kernel_api_utils.hpp"

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
          unsigned int TP_NUM_OUTPUT_CLONES>
INLINE_DECL void
kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>::kernelClassMain(
    T_inputIF<TT_DATA, TP_IN_API> inInterface, T_outputIF<TT_DATA, TP_OUT_API> outInterface) {
    T_buff_256b<TT_DATA> readVal;
    constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / 32;
    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            readVal = window_readincr_256b(inInterface.inWindow0);
            window_writeincr(outInterface.outWindow0, readVal.val);
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 2) window_writeincr(outInterface.outWindow1, readVal.val);
            if
                constexpr(TP_NUM_OUTPUT_CLONES >= 3) window_writeincr(outInterface.outWindow2, readVal.val);
        }
};

template <typename TT_DATA, unsigned int TP_NUM_INPUTS, unsigned int TP_WINDOW_VSIZE, unsigned int TP_NUM_OUTPUT_CLONES>
INLINE_DECL void
kernelClass<TT_DATA, kStreamAPI, kWindowAPI, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>::kernelClassMain(
    T_inputIF<TT_DATA, kStreamAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface) {
    T_buff_128b<TT_DATA> readVal1;
    T_buff_128b<TT_DATA> readVal2;
    T_buff_256b<TT_DATA> writeVal;
    if
        constexpr(TP_NUM_INPUTS == 1) {
            constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / 16;
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    readVal1 = stream_readincr_128b(inInterface.inStream0);
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
        constexpr int kLsize = TP_WINDOW_VSIZE * sizeof(TT_DATA) / 32;
        for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                readVal1 = stream_readincr_128b(inInterface.inStream0);
                readVal2 = stream_readincr_128b(inInterface.inStream1);
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
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_NUM_OUTPUT_CLONES>
INLINE_DECL void
kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>::kernelClassMain(
    T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamAPI> outInterface) {
    T_buff_128b<TT_DATA> readVal;
    T_buff_128b<TT_DATA> writeVal;
    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    if
        constexpr(TP_NUM_OUTPUT_CLONES == 1) {
            constexpr int kLsize = TP_WINDOW_VSIZE / kSamplesIn128b;
            for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                    readVal = window_readincr_128b(inInterface.inWindow0);
                    writeincr(outInterface.outStream0, readVal.val);
                }
        }
    else { // TP_NUM_OUTPUT_CLONES==2
        T_buff_256b<TT_DATA> readVal;
        T_buff_128b<TT_DATA> writeVal;
        constexpr int kLsize = TP_WINDOW_VSIZE / (kSamplesIn128b * 2);
        for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
                readVal = window_readincr_256b(inInterface.inWindow0);
                writeVal.val = readVal.val.template extract<kSamplesIn128b>(0);
                writeincr(outInterface.outStream0, writeVal.val);
                writeVal.val = readVal.val.template extract<kSamplesIn128b>(1);
                writeincr(outInterface.outStream1, writeVal.val);
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
          unsigned int TP_NUM_OUTPUT_CLONES>
__attribute__((noinline)) void
    widget_api_cast<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>::
        transferData // transferData is QoR hook
    (input_window<TT_DATA>* __restrict inWindow0, output_window<TT_DATA>* __restrict outWindow0) {
    T_inputIF<TT_DATA, TP_IN_API> inInterface;
    T_outputIF<TT_DATA, TP_OUT_API> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outWindow0 = outWindow0;
    this->kernelClassMain(inInterface, outInterface);
};

// window API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1>::transferData(
    input_stream<TT_DATA>* __restrict inStream0, output_window<TT_DATA>* __restrict outWindow0) {
    constexpr unsigned int TP_NUM_OUTPUTS = 1;
    T_inputIF<TT_DATA, kStreamAPI> inInterface;
    T_outputIF<TT_DATA, kWindowAPI> outInterface;
    inInterface.inStream0 = inStream0;
    outInterface.outWindow0 = outWindow0;
    this->kernelClassMain(inInterface, outInterface);
};

// stream API in and out, 2 outputs
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4>::transferData(
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
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1>::transferData(
    input_window<TT_DATA>* __restrict inWindow0, output_stream<TT_DATA>* __restrict outStream0) {
    T_inputIF<TT_DATA, kWindowAPI> inInterface;
    T_outputIF<TT_DATA, kStreamAPI> outInterface;
    inInterface.inWindow0 = inWindow0;
    outInterface.outStream0 = outStream0;
    this->kernelClassMain(inInterface, outInterface);
};

// window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline)) void widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
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
/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
