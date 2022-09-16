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

#include "xf_security/ecdsa_nistp384p.hpp"
#ifndef __SYNTHESIS__
#include <iostream>
#endif

void test(ap_uint<2> opType,
          ap_uint<384> hash,
          ap_uint<384> k,
          ap_uint<384> privateKey,
          ap_uint<384>& r,
          ap_uint<384>& s,
          ap_uint<384>& lamda,
          bool& kValid) {
    // xf::security::nistp384Sign(hash, k, privateKey, r, s);
    xf::security::nist384p_array<384, 64, 8, 384> inst;
    // inst.sign(hash, k, privateKey, r, s);
    inst.process(opType, hash, privateKey, k, r, s, lamda);
}

#ifndef __SYNTHESIS__
int main() {
    ap_uint<384> privateKey = ap_uint<384>(
        "0x41FFC1FFFFFE01FFFC0003FFFE0007C001FFF00003FFF07FFE0007C000000003FFFFFF807FFF8007FFFFF800FFFE0000");
    ap_uint<384> gold_r = ap_uint<384>(
        "0xF2A066BD332DC59BBC3D01DA1B124C687D8BB44611186422DE94C1DA4ECF150E664D353CCDB5CB2652685F8EB4D2CD49");
    ap_uint<384> gold_s = ap_uint<384>(
        "0xD6ED0BF75FDD8E53D87765FA746835B673881D6D1907163A2C43990D75B454294F942EC571AD5AAE1806CAF2BB8E9A4A");
    ap_uint<384> lamda = ap_uint<384>("0x12");

    ap_uint<384> r, s;
    bool kValid;

    int i = 0;
    for (i; i < 1; i++) {
        test(2, 0, 0, privateKey, r, s, lamda, kValid);
    }

    if (gold_r != r || gold_s != s) {
        std::cout << std::hex << "r:      " << r << std::endl;
        std::cout << std::hex << "gold_r: " << gold_r << std::endl;
        std::cout << std::hex << "s:      " << s << std::endl;
        std::cout << std::hex << "gold_s: " << gold_s << std::endl;
        return 1;
    } else {
        return 0;
    }
}
#endif
