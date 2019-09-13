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

#ifndef __SYNTHESIS__
#include <iostream>
#include <string>
#endif
#include "xf_security/gmac.hpp"

void test_gmac(
    // stream in
    hls::stream<ap_uint<128> >& dataStrm,
    hls::stream<bool>& eDataStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<256> >& keyStrm,
    hls::stream<ap_uint<96> >& ivStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    xf::security::aes256Gmac(dataStrm, eDataStrm, keyStrm, ivStrm, tagStrm);
}

template <int n>
ap_uint<n * 8> array2apuint(unsigned char data[n], const char* cap) {
    ap_uint<n* 8> d = 0;
#ifndef __SYNTHESIS__
    std::string cp(cap);
    std::cout << cp << std::endl;
#endif
    // here map data[0] to MSB of d
    for (int i = 0; i < n; ++i) {
        int j = n - i - 1;
        unsigned char c = data[i];
        d.range((j + 1) * 8 - 1, j * 8) = c;
#ifndef __SYNTHESIS__
        std::cout << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)(c & 0xff));
#endif
    }
#ifndef __SYNTHESIS__
    std::cout << std::endl;
#endif
    return d;
}
template <int n>
void print(ap_uint<n * 8> d) {
#ifndef __SYNTHESIS__
    for (int i = n - 1; i >= 0; --i) {
        unsigned int c = d.range((i + 1) * 8 - 1, i * 8);
        std::cout << std::hex << std::setw(2) << std::setfill('0') << c;
    }
    std::cout << std::endl;
#endif
}

int main() {
    hls::stream<ap_uint<128> > textStrm;
    hls::stream<bool> eTextStrm;
    hls::stream<ap_uint<256> > hashKeyStrm;
    hls::stream<ap_uint<128> > eky0Strm;
    hls::stream<ap_uint<96> > ivStrm;
    hls::stream<ap_uint<128> > tagStrm;
    // The test vector is test case 13 from appendix B of Galois/Counter Mode of Operation (GCM)
    unsigned char txt[16] = {0};
    unsigned char key[32] = {0};
    unsigned char iv[12] = {0};
    unsigned char aad[16] = {0};
    unsigned char gld[16] = {0x53, 0x0f, 0x8a, 0xfb, 0xc7, 0x45, 0x36, 0xb9,
                             0xa9, 0x63, 0xb4, 0xf1, 0xc4, 0xcb, 0x73, 0x8b};

    unsigned char eky0[16] = {0x58, 0xe2, 0xfc, 0xce, 0xfa, 0x7e, 0x30, 0x61,
                              0x36, 0x7f, 0x1d, 0x57, 0xa4, 0xe7, 0x45, 0x5a};

    ap_uint<128> t = array2apuint<16>(txt, "text:");
    eTextStrm.write(true);

    ap_uint<256> k = array2apuint<32>(key, "key: ");
    hashKeyStrm.write(k);

    ap_uint<96> v = array2apuint<12>(iv, "IV:  ");
    ivStrm.write(v);

    ap_uint<128> glden = array2apuint<16>(gld, "gold:");

    test_gmac(textStrm, eTextStrm, hashKeyStrm, ivStrm, tagStrm);
    ap_uint<128> testTag = tagStrm.read();
    ap_uint<128> tag;

    for (int i = 0; i < 16; ++i) {
        tag.range(i * 8 + 7, i * 8) = testTag.range((16 - i) * 8 - 1, (16 - i - 1) * 8);
    }
#ifndef __SYNTHESIS__
    std::cout << std::endl << "test tag:";
    for (int i = 0; i < 16; ++i) {
        int c = testTag.range((i + 1) * 8 - 1, i * 8);
        std::cout << std::hex << std::setw(2) << std::setfill('0') << c;
    }
    std::cout << std::endl;
#endif
    int nerr = 0;
    if (tag != glden) {
        nerr = 1;
#ifndef __SYNTHESIS__
        std::cout << "==============    test failed ==========" << std::endl;
#endif
    } else {
#ifndef __SYNTHESIS__
        std::cout << "==============    test pass ============" << std::endl;
#endif
    }
    return nerr;
}
