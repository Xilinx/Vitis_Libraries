#pragma once

#ifndef _DSPLIB_KERNEL_RTP_RELOAD_HPP_
#define _DSPLIB_KERNEL_RTP_RELOAD_HPP_

// This file holds sets of templated types and overloaded (or template specialized) functions
// for use by multiple kernels.
// Functions in this file as a rule use intrinsics from a single set. For instance, a set
// may contain a MAC with pre-add which uses a single 1024 bit buffer for both forward
// and reverse data. In cases where a library element has to use an intrinsic which differs
// by more than the types used for some combinations of library element parameter types
// then the set of templatized functions will be particular to that library element and should
// therefore be in <library_element>_utils.hpp

#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {

// Double buffer reload. Copies contents of one buffer into 2 destination buffers.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
inline void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                         TT_COEFF (&cascTaps)[TP_FIR_LEN],
                         TT_COEFF (&outTaps)[TP_FIR_LEN]) {
    T_buff_128b<TT_COEFF>* inTapsPtr = (T_buff_128b<TT_COEFF>*)inTaps;
    T_buff_128b<TT_COEFF>* restrict cascTapsPtr = (T_buff_128b<TT_COEFF>*)cascTaps;
    T_buff_128b<TT_COEFF>* restrict outTapsPtr = (T_buff_128b<TT_COEFF>*)outTaps;
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
    TT_COEFF* restrict cascRemPtr = (TT_COEFF*)cascTapsPtr;
    TT_COEFF* restrict outRemPtr = (TT_COEFF*)outTapsPtr;
    for (int i = 0; i < TP_FIR_LEN % samplesPer256Buff; i++) {
        // copy remainder sample by sample
        *cascRemPtr++ = *inRemPtr;
        *outRemPtr++ = *inRemPtr++;
    }
}

// Buffer reload. Copies contents of one buffer into a destination buffer.
// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
inline void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* outTaps) {
    T_buff_256b<TT_COEFF>* inTapsPtr = (T_buff_256b<TT_COEFF>*)inTaps;
    T_buff_256b<TT_COEFF>* restrict outTapsPtr = (T_buff_256b<TT_COEFF>*)outTaps;
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
inline void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* outTaps, output_stream_cacc48* outCascade) {
    T_buff_256b<int>* inTapsPtr = (T_buff_256b<int>*)inTaps;
    T_buff_256b<int>* restrict outTapsPtr = (T_buff_256b<int>*)outTaps;
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
inline void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps, output_stream_cacc48* outCascade) {
    T_buff_256b<int32>* restrict outTapsPtr = (T_buff_256b<int32>*)outTaps;
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
inline void bufferReload(input_stream_cacc48* inCascade, TT_COEFF* outTaps) {
    T_buff_256b<int32>* restrict outTapsPtr = (T_buff_256b<int32>*)outTaps;
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
inline void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                         TT_COEFF* outTaps,
                         T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    bufferReload(inTaps, outTaps, outInterface.outCascade);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
inline void bufferReload(const TT_COEFF (&inTaps)[TP_FIR_LEN],
                         TT_COEFF* outTaps,
                         T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    bufferReload(inTaps, outTaps);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
inline void bufferReload(T_inputIF<CASC_OUT_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                         TT_COEFF* outTaps,
                         T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps, outInterface.outCascade);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DUAL_IP = 0>
inline void bufferReload(T_inputIF<CASC_OUT_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                         TT_COEFF* outTaps,
                         T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    bufferReload<TT_COEFF, TP_FIR_LEN>(inInterface.inCascade, outTaps);
}

// sendRtpTrigger
template <typename TT_DATA>
inline void sendRtpTrigger(bool updateRtp, T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    // Nothing to do here.
}

// sendRtpTrigger - send a vector over cascade. Non-zero when argument it set to true.
template <typename TT_DATA>
inline void sendRtpTrigger(bool updateRtp, T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    T_buff_256b<int32> buff = null_buff_256b<int32>();
    if (updateRtp) {
        buff.val = upd_elem(buff.val, 0, 1); // set element 0 to 1.
    }

    put_mcd(buff.val);
}

inline bool getRtpTrigger() {
    T_buff_512b<int32> buff = null_buff_512b<int32>();
    T_buff_512b<int32> nullBuff = null_buff_512b<int32>();
    buff.val = upd_w(buff.val, 0, get_scd_v8int32());
    bool ret = ne16(buff.val, nullBuff.val);
    // return true when buffers not equal;
    return ret;
}

// getCompMask
inline constexpr unsigned int getCompMask(const unsigned int size) {
    unsigned int mask = 0;

    for (int i = 0; i < size; i++) {
        mask |= 1 << (i);
    }
    return mask;
}

// Not equal
template <typename TT_DATA>
inline int nEq(T_buff_512b<TT_DATA> xbuff, T_buff_512b<TT_DATA> ybuff, unsigned int mask) {
    // cast as int16 and comare as int16s
    T_buff_512b<int16> xbuffInt;
    T_buff_512b<int16> ybuffInt;

    xbuffInt.val = as_v32int16(xbuff.val);
    ybuffInt.val = as_v32int16(ybuff.val);

    unsigned int ret = mask & ne32(xbuffInt.val, ybuffInt.val);
    return ret;
}
template <>
inline int nEq(T_buff_512b<int16> xbuff, T_buff_512b<int16> ybuff, unsigned int mask) {
    unsigned int ret = mask & ne32(xbuff.val, ybuff.val);
    return ret;
}
// RTP comparison. Compares in 512-bit chunks
template <typename TT_COEFF, unsigned int TP_FIR_LEN>
inline bool rtpCompare(const TT_COEFF (&inTaps)[TP_FIR_LEN], TT_COEFF* oldTaps) {
    T_buff_512b<TT_COEFF>* internalTapsRaw = (T_buff_512b<TT_COEFF>*)inTaps;
    T_buff_512b<TT_COEFF>* restrict comp_ybuff = (T_buff_512b<TT_COEFF>*)oldTaps;

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
}
}
}
} // namespaces
#endif // _DSPLIB_KERNEL_RTP_RELOAD_HPP_

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
