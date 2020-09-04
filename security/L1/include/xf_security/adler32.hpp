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
 * @file adler32.hpp
 * @brief header file for adler32.
 * This file part of Vitis Security Library.
 *
 */

#ifndef _XF_SECURITY_ADLER32_HPP_
#define _XF_SECURITY_ADLER32_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#if !defined(__SYNTHESIS__)
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

const ap_uint<32> BASE[] = {
    65521,  131042, 196563, 262084, 327605, 393126, 458647, 524168,
    589689, 655210, 720731, 786252, 851773, 917294, 982815, 1048336}; /* largest prime smaller than 65536 */

} // end of namespace internal

/**
 * @brief adler32 computes the Adler-32 checksum of an input data.
 * @tparam W byte number of input data, the value of W includes 1, 2, 4, 8, 16.
 * @param adlerStrm initialize adler32 value
 * @param inStrm messages to be checked
 * @param inLenStrm length of messages to be checked.
 * @param endInLenStrm end flag of inLenStrm
 * @param outStrm checksum result
 * @param end flag of outStrm
 */
template <int W>
void adler32(hls::stream<ap_uint<32> >& adlerStrm,
             hls::stream<ap_uint<W * 8> >& inStrm,
             hls::stream<ap_uint<32> >& inLenStrm,
             hls::stream<bool>& endInLenStrm,
             hls::stream<ap_uint<32> >& outStrm,
             hls::stream<bool>& endOutStrm) {
    bool e = endInLenStrm.read();
    while (!e) {
        ap_uint<32> adler = adlerStrm.read();
        ap_uint<32> len = inLenStrm.read();
        e = endInLenStrm.read();

        ap_uint<32> s1 = adler & 0xffff;
        ap_uint<32> s2 = ((adler >> 16) & 0xffff);
        ap_uint<W * 8> inData;
        for (ap_uint<32> i = 0; i < len / W; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = 100 min = 100
            inData = inStrm.read();
            ap_uint<32> sTmp[W];
#pragma HLS array_partition variable = sTmp dim = 1
            for (int i = 0; i < W; i++) {
#pragma HLS unroll
                sTmp[i] = 0;
            }
            for (int j = 0; j < W; j++) {
                for (int k = 0; k < W; k++) {
                    if (k >= j) sTmp[k] += inData(j * 8 + 7, j * 8);
                }
            }
            s2 += s1 * W;
            for (int j = 0; j < W; j++) {
                s2 += sTmp[j];
            }
            s1 += sTmp[W - 1];
            for (int j = 0; j < W; j++) {
                if (s2 > internal::BASE[W - 1 - j]) {
                    s2 -= internal::BASE[W - 1 - j];
                    break;
                }
            }
            if (s1 > internal::BASE[0]) s1 -= internal::BASE[0];
        }
        for (int j = 0; j < len - (len / W) * W; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = W min = W
            if (j == 0) inData = inStrm.read();
            s1 += inData(j * 8 + 7, j * 8);
            if (s1 > internal::BASE[0]) s1 -= internal::BASE[0];
            s2 += s1;
            if (s2 > internal::BASE[0]) s2 -= internal::BASE[0];
        }
        ap_uint<32> res = (s2 << 16) + s1;
        outStrm.write(res);
        endOutStrm.write(false);
    }
    endOutStrm.write(true);
}

} // end of namespace security
} // end of namespace xf
#endif // _XF_SECURITY_ADLER32_HPP_
