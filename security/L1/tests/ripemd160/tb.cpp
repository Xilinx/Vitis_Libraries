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

#include <vector>
#include <string>
#include <ap_int.h>
#include <iostream>
#include <fstream>

#include "xf_security/ripemd.hpp"
void dut(hls::stream<ap_uint<32> >& inStrm,
         hls::stream<ap_uint<64> >& inLenStrm,
         hls::stream<bool>& endInLenStrm,
         hls::stream<ap_uint<160> >& outStrm,
         hls::stream<bool>& endOutStrm) {
    xf::security::ripemd160(inStrm, inLenStrm, endInLenStrm, outStrm, endOutStrm);
}

#ifndef __SYNTHESIS__
int main() {
    const int caseNum = 8;
    std::vector<std::string> testVectors;
    testVectors.resize(caseNum);
    testVectors[0] = "";
    testVectors[1] = "a";
    testVectors[2] = "abc";
    testVectors[3] = "message digest";
    testVectors[4] = "abcdefghijklmnopqrstuvwxyz";
    testVectors[5] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    testVectors[6] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    testVectors[7] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";

    hls::stream<ap_uint<32> > inStrm;
    hls::stream<ap_uint<64> > inLenStrm;
    hls::stream<bool> endLenStrm;
    hls::stream<ap_uint<160> > outStrm;
    hls::stream<bool> endOutStrm;

    for (int i = 0; i < caseNum; i++) {
        ap_uint<64> len = testVectors[i].size();
        inLenStrm.write(len);
        endLenStrm.write(false);
        for (int j = 0; j < len; j += 4) {
            int left;
            if ((j + 4) < len) {
                left = 4;
            } else {
                left = len - j;
            }
            ap_uint<32> data = 0;
            for (int k = 0; k < left; k++) {
                data.range(k * 8 + 7, k * 8) = testVectors[i][j + k];
            }
            inStrm.write(data);
        }
    }
    endLenStrm.write(true);

    dut(inStrm, inLenStrm, endLenStrm, outStrm, endOutStrm);

    int nerr = 0;

    while (!endOutStrm.read()) {
        ap_uint<160> res = outStrm.read();
        std::cout << std::hex << res << std::endl;
    }

    return nerr;
}
#endif
