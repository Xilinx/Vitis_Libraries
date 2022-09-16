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

void test(ap_uint<2> opType,
          ap_uint<384> hash,
          ap_uint<384> Qx,
          ap_uint<384> Qy,
          ap_uint<384> r,
          ap_uint<384> s,
          ap_uint<384> lamda,
          bool& ifValid) {
    // ifValid = xf::security::nistp384Verify(r, s, hash, Qx, Qy);
    xf::security::nist384p_array<384, 64, 8, 384> inst;
    // ifValid = inst.verify(r, s, hash, Qx, Qy);
    ifValid = inst.process(opType, hash, Qx, Qy, r, s, lamda);
}

int main() {
    ap_uint<384> m = ap_uint<384>(
        "0x31a452d6164d904bb5724c878280231eae705c29ce9d4bc7d58e020e1085f17eebcc1a38f0ed0bf2b344d81fbd896825");
    ap_uint<384> r = ap_uint<384>(
        "0x50835a9251bad008106177ef004b091a1e4235cd0da84fff54542b0ed755c1d6f251609d14ecf18f9e1ddfe69b946e32");
    ap_uint<384> s = ap_uint<384>(
        "0x0475f3d30c6463b646e8d3bf2455830314611cbde404be518b14464fdb195fdcc92eb222e61f426a4a592c00a6a89721");
    ap_uint<384> Qx = ap_uint<384>(
        "0xc2b47944fb5de342d03285880177ca5f7d0f2fcad7678cce4229d6e1932fcac11bfc3c3e97d942a3c56bf34123013dbf");
    ap_uint<384> Qy = ap_uint<384>(
        "0x37257906a8223866eda0743c519616a76a758ae58aee81c5fd35fbf3a855b7754a36d4a0672df95d6c44a81cf7620c2d");
    ap_uint<384> lamda = ap_uint<384>("0x12");

    bool ifValid;

    test(1, m, Qx, Qy, r, s, lamda, ifValid);

    if (ifValid != true) {
        return 1;
    } else {
        return 0;
    }
}
