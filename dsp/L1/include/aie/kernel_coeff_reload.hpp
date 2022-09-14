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
#pragma once

#ifndef _DSPLIB_KERNEL_COEFF_RELOAD_HPP_
#define _DSPLIB_KERNEL_COEFF_RELOAD_HPP_

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

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
    T_buff_128b<TT_COEFF>* inTapsPtr = (T_buff_128b<TT_COEFF>*)inTaps;
    T_buff_128b<TT_COEFF>* __restrict cascTapsPtr = (T_buff_128b<TT_COEFF>*)cascTaps;
    T_buff_128b<TT_COEFF>* __restrict outTapsPtr = (T_buff_128b<TT_COEFF>*)outTaps;
    T_buff_128b<TT_COEFF> c_xbuff;
    const int samplesPer256Buff = 128 / 8 / sizeof(TT_COEFF);

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
    T_buff_256b<TT_COEFF>* inTapsPtr = (T_buff_256b<TT_COEFF>*)inTaps;
    T_buff_256b<TT_COEFF>* __restrict outTapsPtr = (T_buff_256b<TT_COEFF>*)outTaps;
    T_buff_256b<TT_COEFF> buff256;
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);

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
    T_buff_256b<int>* inTapsPtr = (T_buff_256b<int>*)inTaps;
    T_buff_256b<int>* __restrict outTapsPtr = (T_buff_256b<int>*)outTaps;
    T_buff_256b<int> buff256;
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPer256Buff); i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = *inTapsPtr++;

        *outTapsPtr++ = buff256;
        put_mcd(buff256.val);
    }
    // make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Double buffer reload. Copies contents read from cascade input into a destination buffer and sends contents through
// cascade output.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps, output_stream_cacc48* outCascade) {
    T_buff_256b<int32>* __restrict outTapsPtr = (T_buff_256b<int32>*)outTaps;
    T_buff_256b<int32> buff256; //
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPer256Buff); i += samplesPer256Buff) {
        // copy 256-bit vector from input cascade to memory location and output cascade
        buff256.val = get_scd_v8int32(); // int32
        *outTapsPtr++ = buff256;
        put_mcd(buff256.val);
    }
    // make sure memory is not accessed too early by adding a fence
    chess_memory_fence();
}

// Buffer reload. Copies contents read from cascade input into a destination buffer.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps) {
    T_buff_256b<int32>* __restrict outTapsPtr = (T_buff_256b<int32>*)outTaps;
    T_buff_256b<int32> buff256; //
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);

    for (int i = 0; i < CEIL(TP_FIR_LEN, samplesPer256Buff); i += samplesPer256Buff) {
        // copy from input cascade to memory location
        buff256.val = get_scd_v8int32(); // int32
        *outTapsPtr++ = buff256;
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
INLINE_DECL void bufferReload(T_inputIF<CASC_OUT_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps, outInterface.outCascade);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void bufferReload(T_inputIF<CASC_OUT_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                              TT_COEFF* outTaps,
                              T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void bufferReload(T_inputIF<CASC_OUT_TRUE, TT_DATA, TP_DUAL_IP> inInterface, TT_COEFF* outTaps) {
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
    T_buff_256b<int32> buff = null_buff_256b<int32>();
    if (updateRtp) {
        chess_memory_fence(); // Make sure buffer update is not pipelined before receiving updateRtp information
        buff.val = upd_elem(buff.val, 0, 1); // set element 0 to 1.
    }

    put_mcd(buff.val);
}

INLINE_DECL bool getRtpTrigger() {
    T_buff_512b<int32> buff = null_buff_512b<int32>();
    T_buff_512b<int32> nullBuff = null_buff_512b<int32>();
    buff.val = upd_w(null_buff_512b<int32>().val, 0, get_scd_v8int32());
    bool ret = ne16(buff.val, nullBuff.val);
    // return true when buffers not equal;
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
template <typename TT_DATA>
INLINE_DECL int nEq(T_buff_512b<TT_DATA> xbuff, T_buff_512b<TT_DATA> ybuff, unsigned int mask) {
    // cast as int16 and comare as int16s
    T_buff_512b<int16> xbuffInt;
    T_buff_512b<int16> ybuffInt;

    xbuffInt.val = as_v32int16(xbuff.val);
    ybuffInt.val = as_v32int16(ybuff.val);

    unsigned int ret = mask & ne32(xbuffInt.val, ybuffInt.val);
    return ret;
}
template <>
INLINE_DECL int nEq(T_buff_512b<int16> xbuff, T_buff_512b<int16> ybuff, unsigned int mask) {
    unsigned int ret = mask & ne32(xbuff.val, ybuff.val);
    return ret;
}
// RTP comparison. Compares in 512-bit chunks
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL bool rtpCompare(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* oldTaps) {
    T_buff_512b<TT_COEFF>* internalTapsRaw = (T_buff_512b<TT_COEFF>*)inTaps;
    T_buff_512b<TT_COEFF>* __restrict comp_ybuff = (T_buff_512b<TT_COEFF>*)oldTaps;

    T_buff_512b<TT_COEFF> c_xbuff = null_buff_512b<TT_COEFF>();
    T_buff_512b<TT_COEFF> c_ybuff = null_buff_512b<TT_COEFF>();

    int coeffnEq = false; //
    static constexpr unsigned int samplesPer256Buff = 512 / 8 / sizeof(TT_COEFF);
    static constexpr unsigned int firLenRemInt16 = (TP_FIR_LEN % samplesPer256Buff) * sizeof(TT_COEFF) / sizeof(int16);
    static constexpr unsigned int fullMask = 0xFFFFFFFF;
    static constexpr unsigned int mask = getCompMask(firLenRemInt16);

    // #pragma unroll (TP_FIR_LEN/samplesPer256Buff)
    for (int i = 0; i < TP_FIR_LEN; i += samplesPer256Buff)
        chess_prepare_for_pipelining chess_loop_range(TP_FIR_LEN / samplesPer256Buff, ) {
            c_xbuff = *internalTapsRaw++;
            c_ybuff = *comp_ybuff++;

            // Offsets are different on final iteration
            if (i == TRUNC(TP_FIR_LEN, samplesPer256Buff)) {
                coeffnEq |= nEq(c_xbuff, c_ybuff, mask);
            } else {
                coeffnEq |= nEq(c_xbuff, c_ybuff, fullMask);
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
    T_buff_256b<TT_DATA> buff;
    T_buff_256b<TT_COEFF> coeffBuff;
    T_buff_256b<TT_COEFF>* __restrict outTapsPtr = (T_buff_256b<TT_COEFF>*)taps;
    const int dataSamplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    const int coeffSamplesPer256Buff = 256 / 8 / sizeof(TT_COEFF);

    for (int i = 0; i < CEIL(TP_FIR_LEN_SSR, coeffSamplesPer256Buff); i += coeffSamplesPer256Buff) {
        // copy 256-bit vector from input stream to memory location and output cascade
        // implement a vector based version of firReload.
        readStream256(buff, 0, inInterface);
        // coeffBuff.val = buff.val.template cast_to<TT_COEFF>();
        // *outTapsPtr++ = coeffBuff;
        outTapsPtr->val = buff.val.template cast_to<TT_COEFF>();
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
    T_buff_256b<TT_DATA> buff = null_buff_256b<TT_DATA>();
    T_buff_256b<TT_DATA> nullBuff = null_buff_256b<TT_DATA>();
    T_buff_256b<int32> headerConfig;
    if
        constexpr(TP_API == USE_STREAM_API && TP_USE_COEFF_RELOAD == 2) {
            readStream256(buff, 0, inInterface);
            // at this point, just check if non-zero.
            // ret = ne16(buff.val, nullBuff.val);
            ret = ::aie::not_equal(buff.val, nullBuff.val);
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
