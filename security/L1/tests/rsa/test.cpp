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

#include "test.hpp"
void rsa_test(hls::stream<ap_uint<32> >& messageStrm,
              hls::stream<ap_uint<32> >& NStrm,
              hls::stream<ap_uint<32> >& keyStrm,
              hls::stream<ap_uint<32> >& resultStrm) {
#pragma HLS stream variable = messageStrm depth = 64
#pragma HLS stream variable = NStrm depth = 64
#pragma HLS stream variable = keyStrm depth = 64
#pragma HLS stream variable = resultStrm depth = 64
    ap_uint<2048> message, N, key, result;

    for (int i = 0; i < 64; i++) {
        message >>= 32;
        N >>= 32;
        key >>= 32;

        message.range(2047, 2016) = messageStrm.read();
        key.range(2047, 2016) = keyStrm.read();
        N.range(2047, 2016) = NStrm.read();
    }

    xf::security::rsa<16, 128> inst;
    inst.updateKey(N, key);
    inst.process(message, result);

    for (int i = 0; i < 64; i++) {
        resultStrm.write(result.range(31, 0));
        result >>= 32;
    }
}
