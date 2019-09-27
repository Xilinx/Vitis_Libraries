/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 * @file poly1305.hpp
 * @brief header file for poly1305.
 * This file part of Vitis Security Library.
 *
 */

#ifndef _XF_SECURITY_POLY1305_HPP_
#define _XF_SECURITY_POLY1305_HPP_

#include <ap_int.h>
#include <hls_stream.h>

#if !defined(__SYNTHESIS__)
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

/**
 *
 * @brief The implementation of large bit-width multiplication, the result is A * B.
 * The optimization goal of this function to improve timing.
 *
 * @param A The input multiplicand.
 * @param B The input multiplier.
 * @return The output product.
 */

ap_uint<264> multOperator(ap_uint<136> A, ap_uint<128> B) {
#pragma HLS inline off
    //#pragma HLS ALLOCATION instances=mul limit=1 operation
    int segA = 5; //_W1 / 27;
    int segB = 7; //_W2 / 18;
    ap_uint<27> arrayA[6];
    ap_uint<18> arrayB[8];
#pragma HLS resource variable = arrayA core = RAM_2P_LUTRAM
#pragma HLS resource variable = arrayB core = RAM_2P_LUTRAM
LOOP_INIT_A:
    for (unsigned int i = 0; i < segA; i++) {
#pragma HLS unroll
        arrayA[i] = A.range(27 * i + 26, 27 * i);
    }

    // the upper bits will automatically reset to zeros by default
    arrayA[segA] = A.range(136 - 1, segA * 27);

LOOP_INIT_B:
    for (unsigned int i = 0; i < segB; i++) {
#pragma HLS unroll
        arrayB[i] = B.range(18 * i + 17, 18 * i);
    }
    // the upper bits will automatically reset to zeros by default
    arrayB[segB] = B.range(128 - 1, segB * 18);

    ap_uint<264> result = 0;
    ap_uint<264> tmp = 0;
LOOP_MULT:
    for (unsigned int i = 0; i <= segA; i++) {
        for (unsigned int j = 0; j <= segB; j++) {
            tmp = arrayA[i] * arrayB[j];
            result += (tmp << (i * 27 + j * 18));
        }
    }
    return result; // A * B;
}

/**
 *
 * @brief The implementation of large bit-width Module Operation, the result is A % 2^130-5.
 * The optimization goal of this function to improve timing.
 *
 * @param A The input parameter.
 * @return The output result.
 */

ap_uint<132> resOperator(ap_uint<264> A) {
#pragma HLS inline off
    ap_uint<132> P; // 2^130-5
    P.range(131, 128) = 0x3;
    P.range(127, 64) = 0xffffffffffffffff;
    P.range(63, 0) = 0xfffffffffffffffb;
    ap_uint<264> aTmp = A;
loop_mod: // mod(a,2^130-5)
    while (aTmp > P) {
#pragma HLS loop_tripcount max = 1 min = 1
        ap_uint<133> aHigh = aTmp.range(263, 130);
        ap_uint<136> aHigh2 = 0;
        aHigh2.range(134, 2) = aTmp.range(263, 130);
        aHigh2 += aHigh; // aHigh*5
        aTmp = aTmp.range(129, 0) + aHigh2;
    }
    return aTmp.range(131, 0);
    // return a % P;
}

/**
 *
 * @brief The implementation of poly1305
 *
 * @param keyStrm initail key
 * @param payloadStrm For a massage, input block stream text, 128 bits per block, less than 128 bits, high padding 0
 * @param lenPldStrm Length of payload in byte.
 * @param endLenStrm Flag to signal the end of the length streams.
 * @param tagStrm Return a 16-byte tag to to authenticate the message.
 */

void poly1305Imp(
    // stream in
    hls::stream<ap_uint<256> >& keyStrm,
    hls::stream<ap_uint<128> >& payloadStrm,
    hls::stream<ap_uint<64> >& lenPldStrm,
    hls::stream<bool>& endLenStrm,
    // stream out
    hls::stream<ap_uint<128> >& tagStrm) {
    hls::stream<ap_uint<128> > rStrm;
#pragma HLS stream variable = rStrm depth = 32
    hls::stream<ap_uint<128> > sStrm;
#pragma HLS stream variable = sStrm depth = 32

    int cnt = 0;

loop_Len:
    while (endLenStrm.read()) {
#pragma HLS loop_tripcount max = 10 min = 10
        // initKeyConvert(keyStrm, rStrm, sStrm);
        ap_uint<128> pValue;
        pValue.range(127, 64) = 0x0ffffffc0ffffffc;
        pValue.range(63, 0) = 0x0ffffffc0fffffff;
        ap_uint<256> keyValue = keyStrm.read();
        ap_uint<128> rValue, sValue;
        rValue = keyValue.range(127, 0) & pValue;

        sValue = keyValue.range(255, 128);

        ap_int<64> len = lenPldStrm.read();
#if !defined(__SYNTHESIS__)
        std::cout << "cnt=" << cnt << ", len=" << len << std::endl;
#endif
        ap_uint<132> Acc = 0;
    // ap_uint<128> r = rStrm.read();
    loop_Block:
        while (len > 0) {
#pragma HLS loop_tripcount max = 10 min = 10
#pragma HLS pipeline
            ap_uint<136> payload = 0;
            payload.range(127, 0) = payloadStrm.read();
            if (len >= 16)
                payload.range(135, 128) = 0x01;
            else
                payload.range(len * 8 + 7, len * 8) = 0x01;

            ap_uint<136> tmp1 = Acc + payload;
            // ap_uint<264> tmp2 = tmp1 * rValue;
            ap_uint<264> tmp2 = multOperator(tmp1, rValue);
            // Acc = tmp2 % P;
            Acc = resOperator(tmp2);

#if !defined(__SYNTHESIS__)
            std::cout << std::hex << "  len=" << len << ", payload=" << payload << ", r=" << rValue << ", tmp1=" << tmp1
                      << ", tmp2=" << tmp2 << ", Acc=" << Acc << std::endl;
#endif
            len -= 16;
        }
        Acc += sValue;
        tagStrm.write(Acc.range(127, 0));
    }
}

} // end of namespace internal

/**
 *
 * @brief The poly1305 takes a 32-byte one-time key and a message and produces a 16-byte tag. This tag is used to
 * authenticate the message.
 *
 * @param keyStrm initail key
 * @param payloadStrm For a massage, input block stream text, 128 bits per block, less than 128 bits, high padding 0
 * @param lenPldStrm Length of payload in byte.
 * @param endLenStrm Flag to signal the end of the length streams.
 * @param tagStrm Return a 16-byte tag to to authenticate the message.
 */
void poly1305(
    // stream in
    hls::stream<ap_uint<256> >& keyStrm,
    hls::stream<ap_uint<128> >& payloadStrm,
    hls::stream<ap_uint<64> >& lenPldStrm,
    hls::stream<bool>& endLenStrm,
    // stream out
    hls::stream<ap_uint<128> >& tagStrm) {
    internal::poly1305Imp(keyStrm, payloadStrm, lenPldStrm, endLenStrm, tagStrm);
}

} // end of namespace security
} // end of namespace xf
#endif // _XF_SECURITY_POLY1305_HPP_
