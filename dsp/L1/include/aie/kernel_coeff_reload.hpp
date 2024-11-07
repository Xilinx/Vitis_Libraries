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
#pragma once

#ifndef _DSPLIB_KERNEL_COEFF_RELOAD_HPP_
#define _DSPLIB_KERNEL_COEFF_RELOAD_HPP_

#include "device_defs.h"
#include <stdio.h>
#include <adf.h>

// #define _DSPLIB_KERNEL_COEFF_RELOAD_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {

// Double buffer reload. Copies contents of one buffer into 2 destination buffers.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                              TT_COEFF (&cascTaps)[TP_FIR_LEN],
                              TT_COEFF (&outTaps)[TP_FIR_LEN]) {
    const int samplesPer256Buff = 128 / 8 / sizeof(TT_COEFF);
    ::aie::vector<TT_COEFF, samplesPer256Buff>* inTapsPtr = (::aie::vector<TT_COEFF, samplesPer256Buff>*)inTaps;
    ::aie::vector<TT_COEFF, samplesPer256Buff>* __restrict cascTapsPtr =
        (::aie::vector<TT_COEFF, samplesPer256Buff>*)cascTaps;
    ::aie::vector<TT_COEFF, samplesPer256Buff>* __restrict outTapsPtr =
        (::aie::vector<TT_COEFF, samplesPer256Buff>*)outTaps;
    ::aie::vector<TT_COEFF, samplesPer256Buff> c_xbuff;

    for (int i = 0; i < TRUNC(TP_FIR_LEN, samplesPer256Buff); i += samplesPer256Buff) {
        // copy 256 vector at a time
        c_xbuff = *inTapsPtr;

        *cascTapsPtr = c_xbuff;
        *outTapsPtr = c_xbuff;
        inTapsPtr++;
        cascTapsPtr++;
        outTapsPtr++;
    }
    TT_COEFF* inRemPtr = (TT_COEFF*)inTapsPtr;
    TT_COEFF* __restrict cascRemPtr = (TT_COEFF*)cascTapsPtr;
    TT_COEFF* __restrict outRemPtr = (TT_COEFF*)outTapsPtr;
    for (int i = 0; i < TP_FIR_LEN % samplesPer256Buff; i++) {
        // copy remainder sample by sample
        *cascRemPtr++ = *inRemPtr;
        *outRemPtr++ = *inRemPtr++;
    }
}

// Buffer reload. Copies contents of one buffer into a destination buffer.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* outTaps) {
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);
    ::aie::vector<TT_COEFF, samplesPer256Buff>* inTapsPtr = (::aie::vector<TT_COEFF, samplesPer256Buff>*)inTaps;
    ::aie::vector<TT_COEFF, samplesPer256Buff>* __restrict outTapsPtr =
        (::aie::vector<TT_COEFF, samplesPer256Buff>*)outTaps;
    ::aie::vector<TT_COEFF, samplesPer256Buff> buff256;

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPer256Buff); i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = *inTapsPtr++;
        *outTapsPtr++ = buff256;
    }
    // Make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Double buffer reload. Copies contents of one buffer into a destination buffer and sends contents through cascade
// output.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                              TT_COEFF* outTaps,
                              output_stream_cacc48* outCascade) {
    const int samplesPerBuffWrite = MCD_SIZE / 8 / sizeof(TT_COEFF);
    ::aie::vector<TT_COEFF, samplesPerBuffWrite>* inTapsPtr = (::aie::vector<TT_COEFF, samplesPerBuffWrite>*)inTaps;
    ::aie::vector<TT_COEFF, samplesPerBuffWrite>* __restrict outTapsPtr =
        (::aie::vector<TT_COEFF, samplesPerBuffWrite>*)outTaps;
    ::aie::vector<TT_COEFF, samplesPerBuffWrite> buffWrite;

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPerBuffWrite); i += samplesPerBuffWrite) {
        buffWrite = *inTapsPtr++;
        *outTapsPtr++ = buffWrite;
        put_mcd(buffWrite);
    }
    // make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Double buffer reload. Copies contents read from cascade input into a destination buffer and sends contents through
// cascade output.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps, output_stream_cacc48* outCascade) {
    ::aie::vector<int32, MCD_SIZE / 8 / 4>* __restrict outTapsPtr = (::aie::vector<int32, MCD_SIZE / 8 / 4>*)outTaps;
    ::aie::vector<int32, MCD_SIZE / 8 / 4> readBuff; //
    const int samplesPerReadBuff = MCD_SIZE / 8 / sizeof(TT_COEFF);
    // output_stream<cacc64>*
    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPerReadBuff); i += samplesPerReadBuff) {
// copy 512-bit vector from input cascade to memory location and output cascade
#if MCD_SIZE == 256
        readBuff = get_scd_v8int32(); // int32
#else
        readBuff = get_scd_v16int32(); // int32
#endif
        *outTapsPtr++ = readBuff;
        put_mcd(readBuff);
    }
    // make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Buffer reload. Copies contents read from cascade input into a destination buffer.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps) {
    const int samplesPerBuffRead = SCD_SIZE / 8 / sizeof(TT_COEFF);
    const int intsPerBuffRead = SCD_SIZE / 8 / sizeof(int32);
    ::aie::vector<int32, intsPerBuffRead>* __restrict outTapsPtr = (::aie::vector<int32, intsPerBuffRead>*)outTaps;
    ::aie::vector<int32, intsPerBuffRead> readBuff; //

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPerBuffRead); i += samplesPerBuffRead) {
// copy from input cascade to memory location
#if SCD_SIZE == 256
        readBuff = get_scd_v8int32(); // int32
#else
        readBuff = get_scd_v16int32();
#endif // SCD_SIZE
        *outTapsPtr++ = readBuff;
    }
    // make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    bufferReload(inTaps, outTaps, outInterface.outCascade);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    bufferReload(inTaps, outTaps);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void bufferReload(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps, outInterface.outCascade);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void bufferReload(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void bufferReload(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface, TT_COEFF* outTaps) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.in, outTaps);
}

// sendRtpTrigger
template <typename TT_DATA>
INLINE_DECL void sendRtpTrigger(bool updateRtp, T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    // Nothing to do here.
}

// sendRtpTrigger - send a vector over cascade. Non-zero when argument it set to true.
template <typename TT_DATA>
INLINE_DECL void sendRtpTrigger(bool updateRtp, T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    ::aie::vector<int32, MCD_SIZE / 8 / 4> buff = ::aie::zeros<int32, MCD_SIZE / 8 / 4>();
    if (updateRtp) {
        chess_memory_fence();        // Make sure buffer update is not pipelined before receiving updateRtp information
        buff = upd_elem(buff, 0, 1); // set element 0 to 1.
    }
    put_mcd(buff);
}

INLINE_DECL bool getRtpTrigger() {
    ::aie::vector<int32, 512 / 8 / 4> buff = ::aie::zeros<int32, 512 / 8 / 4>();
    ::aie::vector<int32, 512 / 8 / 4> nullBuff = ::aie::zeros<int32, 512 / 8 / 4>();
#ifdef __SUPPORTS_PUT_MCD__
    buff = upd_w(::aie::zeros<int32, 512 / 8 / 4>(), 0, get_scd_v8int32());
#else
    buff = get_scd_v16int32();
#endif
    bool ret = ::aie::not_equal(buff, nullBuff);
    return ret;
}

// getCompMask
INLINE_DECL constexpr unsigned int getCompMask(const unsigned int size) {
    unsigned int mask = 0;

    for (int i = 0; i < size; i++) {
        mask |= 1 << (i);
    }
    return mask;
}

// Not equal
template <typename TT_DATA, unsigned int buffSize, unsigned int maskElems>
INLINE_DECL int nEq(::aie::vector<TT_DATA, buffSize / 8 / sizeof(TT_DATA)> xbuff,
                    ::aie::vector<TT_DATA, buffSize / 8 / sizeof(TT_DATA)> ybuff,
                    ::aie::mask<maskElems> compMask) {
    ::aie::mask<maskElems> resNeq = ::aie::neq(xbuff, ybuff);
    ::aie::mask<maskElems> retComp = compMask & ::aie::neq(xbuff, ybuff);
    unsigned int ret = retComp.to_uint32();
    return ret;
}

// RTP comparison. Compares in 512-bit chunks
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL bool rtpCompare(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* oldTaps) {
    constexpr int buffSize = 512;
    ::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>* internalTapsRaw =
        (::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>*)inTaps;
    ::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>* __restrict comp_ybuff =
        (::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>*)oldTaps;

    ::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)> c_xbuff =
        ::aie::zeros<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>();
    ::aie::vector<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)> c_ybuff =
        ::aie::zeros<TT_COEFF, buffSize / 8 / sizeof(TT_COEFF)>();

    int coeffnEq = false; //
    static constexpr unsigned int samplesPerBuff = buffSize / 8 / sizeof(TT_COEFF);
    static constexpr unsigned int firLenRemInt16 = (TP_FIR_LEN % samplesPerBuff) * sizeof(TT_COEFF) / sizeof(int16);
    ::aie::mask fullMask = ::aie::mask<samplesPerBuff>(true);
    ::aie::mask compMask = ::aie::mask<samplesPerBuff>::from_uint32(getCompMask(firLenRemInt16));

    // #pragma unroll (TP_FIR_LEN/samplesPer512Buff)
    for (int i = 0; i < TP_FIR_LEN; i += samplesPerBuff)
        chess_prepare_for_pipelining chess_loop_range(TP_FIR_LEN / samplesPerBuff, ) {
            c_xbuff = *internalTapsRaw++;
            c_ybuff = *comp_ybuff++;

            // Offsets are different on final iteration
            if (i == TRUNC(TP_FIR_LEN, samplesPerBuff)) {
                coeffnEq = nEq<TT_COEFF, buffSize, samplesPerBuff>(c_xbuff, c_ybuff, compMask);
            } else {
                coeffnEq = nEq<TT_COEFF, buffSize, samplesPerBuff>(c_xbuff, c_ybuff, fullMask);
            }
            if (coeffnEq) { // Coefficients have changed
                break;
            }
        }
    return coeffnEq == 0 ? 0 : 1;
}

// Read 256-bits of header data from the stream(s).
//
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN_SSR, unsigned int TP_DUAL_IP, bool TP_CASC_IN>
NOINLINE_DECL void bufferReloadSSR(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface, TT_COEFF* taps) {
    const int dataSamplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    const int coeffSamplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);
    ::aie::vector<TT_DATA, dataSamplesPer256Buff> buff;
    ::aie::vector<TT_COEFF, coeffSamplesPer256Buff> coeffBuff;
    ::aie::vector<TT_COEFF, coeffSamplesPer256Buff>* __restrict outTapsPtr =
        (::aie::vector<TT_COEFF, coeffSamplesPer256Buff>*)taps;

    for (int i = 0; i < CEIL(TP_FIR_LEN_SSR, coeffSamplesPer256Buff); i += coeffSamplesPer256Buff) {
        readStream256(buff, 0, inInterface);
        *outTapsPtr = buff.template cast_to<TT_COEFF>();
        outTapsPtr++;
    }
}
// Read out stream Header Config WORD and check if Coefficient set is appended to it
//
template <unsigned int TP_API,
          unsigned int TP_USE_COEFF_RELOAD,
          bool TP_CASC_IN,
          typename TT_DATA,
          unsigned int TP_DUAL_IP = 0>
INLINE_DECL bool checkHeaderForUpdate(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    bool updateNotRequired = false;
    bool ret;
    const int dataSamplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    ::aie::vector<TT_DATA, dataSamplesPer256Buff> buff = ::aie::zeros<TT_DATA, 256 / 8 / sizeof(TT_DATA)>();
    ::aie::vector<TT_DATA, dataSamplesPer256Buff> nullBuff = ::aie::zeros<TT_DATA, 256 / 8 / sizeof(TT_DATA)>();
    ::aie::vector<int32, 256 / 8 / 4> headerConfig;
    if
        constexpr(TP_API == USE_STREAM_API && TP_USE_COEFF_RELOAD == 2) {
            readStream256(buff, 0, inInterface);
            // at this point, just check if non-zero.
            ret = ::aie::not_equal(buff, nullBuff);
        }
    else {
        ret = updateNotRequired;
    }
    return ret;
}
}
}
}
} // namespaces
#endif // _DSPLIB_KERNEL_COEFF_RELOAD_HPP_
