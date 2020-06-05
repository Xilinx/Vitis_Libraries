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

#include <hls_stream.h>
#define AP_INT_MAX_W 4097
#include <ap_int.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;
#include <sstream>
#include <string>
#include <vector>

#include "xf_security/modular.hpp"

void test(ap_uint<2048> a, ap_uint<2048> b, ap_uint<2048> m, ap_uint<2048>& ab) {
    ab = xf::security::internal::monProduct<2048>(a, b, m);
}

/*
void test(ap_uint<32> a, ap_uint<32> b, ap_uint<32> m, ap_uint<32>& ab) {
    ab = xf::security::internal::monProduct<32>(a, b, m);
}
*/
#ifndef __SYNTHESIS__
int main() {
    /*
    ap_uint<32> m = ap_uint<32>("0xffffffff");
    ap_uint<32> a = ap_uint<32>("0xbbbbbbbb");

    ap_uint<64> tmp;
    tmp = a;
    tmp <<= 32;
    tmp %= m;
    ap_uint<32> mont_a = tmp;

    ap_uint<32> mont_asquare;
    test(mont_a, mont_a, m, mont_asquare);

    tmp = a * a;
    tmp %= m;
    tmp <<= 32;
    tmp %= m;

    std::cout << "a:" << std::hex << a << std::endl;
    std::cout << "m:" << std::hex << m << std::endl;
    std::cout << "mont_a:" << std::hex << mont_a << std::endl;
    std::cout << "a*a*r mod m" << std::hex << tmp << std::endl;
    std::cout << "calculated:" << mont_asquare << std::endl;

    if (tmp != mont_asquare) {
        return 1;
    } else {
        return 0;
    }
    */

    ap_uint<2048> m = ap_uint<2048>(
        "0x9d41cd0d38339220ebd110e8c31feb279c5fae3c23090045a0886301588d4c8114fa5cdde708ea77ba0f527e6f6ea8f5634acf517f04"
        "ca6399e188d5c2d7f03cc90e04dbf7d5d0056ee1b14b8baaf90ef78f5142ddce9ba2eff84c0295f656c29aecaae80ddd5c7127ddc60215"
        "9458f272316100f726a71362516223f26ddeafa425d3eb2c7f61de7e8586e77d475037563425d931885f03693618bb885ab9b58de74f60"
        "4a86f28e494dcd819bd8c0bb42f699596969b84f680819e4c9fc0ba687558775f770a302d5b266905defe47bc53c98ce261523b49db624"
        "1567f4b48c661482ef9c453750c6d420a0b1a3bd4d3d05b060c026ce8efd9bb9456dfe2f5d");

    ap_uint<2048> a = 0;
    unsigned char rawm[256] =
        "RSA TEST FILE : "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuv"
        "wxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefgh"
        "ijklmnopqrstuvwxyz.";
    for (int i = 0; i < 256; i++) {
        a.range(i * 8 + 7, i * 8) = (unsigned int)rawm[255 - i];
    }

    ap_uint<2048> pp = ap_uint<2048>(
        "0x6B78068D76CE44418A85B1FBF1FEE6D1A9C9A73928967C37A9DA26AB0AD6B42F0B81877573A27718B27C7D0452C678A06CDFE27CE9EA"
        "6F77E450E3A22DA7FEFD819E60B4D6033FFAC5304C89944D2FA4DF2174028999F8620A59D4A4651D822E2218D196A765BACE269D049324"
        "09A365182AFC856D364206B469F7A9E3478C78D240046B371E98888622CB950489D67FC3CE47AC4CE883A1AE67329103B9A9BA395481D1"
        "D308573EEB7861B19FC8E29E73412B45C79FECDD7603FC5445EC7E2EBAF02D7E19F1800DC2F3EC078A45936AE63C5E45B5D33906B628AA"
        "888B9AA4B6499C5734AB30FA769F92A09ACF6FA9F94FB369406C624E090BFCD4F423BF8BEB");
    ap_uint<2048> rr = ap_uint<2048>(
        "0x62BE32F2C7CC6DDF142EEF173CE014D863A051C3DCF6FFBA5F779CFEA772B37EEB05A32218F7158845F0AD819091570A9CB530AE80FB"
        "359C661E772A3D280FC336F1FB24082A2FFA911E4EB4745506F10870AEBD2231645D1007B3FD6A09A93D65135517F222A38ED82239FDEA"
        "6BA70D8DCE9EFF08D958EC9DAE9DDC0D9221505BDA2C14D3809E21817A791882B8AFC8A9CBDA26CE77A0FC96C9E74477A5464A7218B09F"
        "B5790D71B6B2327E64273F44BD0966A6969647B097F7E61B3603F45978AA788A088F5CFD2A4D996FA2101B843AC36731D9EADC4B6249DB"
        "EA980B4B7399EB7D1063BAC8AF392BDF5F4E5C42B2C2FA4F9F3FD93171026446BA9201D0A3");

    ap_uint<4096> tmp;
    tmp = pp;
    tmp <<= 2048;
    tmp %= m;
    ap_uint<2048> mont_a = tmp;
    tmp = rr;
    tmp <<= 2048;
    tmp %= m;
    ap_uint<2048> mont_b = tmp;

    ap_uint<2048> mont_ab;
    test(mont_a, mont_b, m, mont_ab);

    tmp = pp * rr;
    tmp %= m;
    tmp <<= 2048;
    tmp %= m;

    if (tmp != mont_ab) {
        return 1;
    } else {
        return 0;
    }
}

#endif
