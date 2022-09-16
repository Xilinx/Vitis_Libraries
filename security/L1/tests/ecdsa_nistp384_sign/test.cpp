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
          ap_uint<384> lamda,
          bool& kValid) {
    // xf::security::nistp384Sign(hash, k, privateKey, r, s);
    xf::security::nist384p_array<384, 64, 8, 384> inst;
    // inst.sign(hash, k, privateKey, r, s);
    inst.process(opType, hash, k, privateKey, r, s, lamda);
}

#ifndef __SYNTHESIS__
int main() {
    ap_uint<384> m = ap_uint<384>(
        "0x31a452d6164d904bb5724c878280231eae705c29ce9d4bc7d58e020e1085f17eebcc1a38f0ed0bf2b344d81fbd896825");
    ap_uint<384> privateKey = ap_uint<384>(
        "0x201b432d8df14324182d6261db3e4b3f46a8284482d52e370da41e6cbdf45ec2952f5db7ccbce3bc29449f4fb080ac97");
    ap_uint<384> k = ap_uint<384>(
        "0xdcedabf85978e090f733c6e16646fa34df9ded6e5ce28c6676a00f58a25283db8885e16ce5bf97f917c81e1f25c9c771");
    ap_uint<384> gold_r = ap_uint<384>(
        "0x50835a9251bad008106177ef004b091a1e4235cd0da84fff54542b0ed755c1d6f251609d14ecf18f9e1ddfe69b946e32");
    ap_uint<384> gold_s = ap_uint<384>(
        "0x0475f3d30c6463b646e8d3bf2455830314611cbde404be518b14464fdb195fdcc92eb222e61f426a4a592c00a6a89721");
    ap_uint<384> lamda = ap_uint<384>("0x12");

    ap_uint<384> r, s;
    bool kValid;

    int i = 0;
    for (i; i < 1; i++) {
        test(0, m, k, privateKey, r, s, lamda, kValid);
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
