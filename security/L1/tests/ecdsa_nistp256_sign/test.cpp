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

#include "xf_security/ecdsa_nistp256.hpp"
#ifndef __SYNTHESIS__
#include <iostream>
#endif

void test(ap_uint<256> hash, ap_uint<256> k, ap_uint<256> privateKey, ap_uint<256>& r, ap_uint<256>& s, bool& kValid) {
    xf::security::nistp256Sign(hash, k, privateKey, r, s);
}

#ifndef __SYNTHESIS__
int main() {
    ap_uint<256> m = ap_uint<256>("0x44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56");
    ap_uint<256> privateKey = ap_uint<256>("0x519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464");
    ap_uint<256> k = ap_uint<256>("0x94a1bbb14b906a61a280f245f9e93c7f3b4a6247824f5d33b9670787642a68de");
    ap_uint<256> gold_r = ap_uint<256>("0xf3ac8061b514795b8843e3d6629527ed2afd6b1f6a555a7acabb5e6f79c8c2ac");
    ap_uint<256> gold_s = ap_uint<256>("0x8bf77819ca05a6b2786c76262bf7371cef97b218e96f175a3ccdda2acc058903");

    ap_uint<256> r, s;
    bool kValid;

    int i = 0;
    for (i; i < 5; i++) {
        test(m, k, privateKey, r, s, kValid);
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