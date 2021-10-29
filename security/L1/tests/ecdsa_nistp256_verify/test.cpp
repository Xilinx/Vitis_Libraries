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

void test(ap_uint<256> hash, ap_uint<256> Qx, ap_uint<256> Qy, ap_uint<256> r, ap_uint<256> s, bool& ifValid) {
    ifValid = xf::security::nistp256Verify(r, s, hash, Qx, Qy);
}

int main() {
    ap_uint<256> m = ap_uint<256>("0x44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56");
    ap_uint<256> r = ap_uint<256>("0xf3ac8061b514795b8843e3d6629527ed2afd6b1f6a555a7acabb5e6f79c8c2ac");
    ap_uint<256> s = ap_uint<256>("0x8bf77819ca05a6b2786c76262bf7371cef97b218e96f175a3ccdda2acc058903");
    ap_uint<256> Qx = ap_uint<256>("0x1ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83");
    ap_uint<256> Qy = ap_uint<256>("0xce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9");

    bool ifValid;

    test(m, Qx, Qy, r, s, ifValid);

    if (ifValid != true) {
        return 1;
    } else {
        return 0;
    }
}