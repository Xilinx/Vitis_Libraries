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

#include "xf_security/aes.hpp"

void test(hls::stream<ap_uint<128> >& plaintext_strm,
          hls::stream<bool>& i_e_strm,
          hls::stream<ap_uint<256> >& cipherkey_strm,
          hls::stream<ap_uint<128> >& ciphertext_strm,
          hls::stream<bool>& o_e_strm) {
    bool is_end = i_e_strm.read();
    xf::security::aesEnc<256> cipher;
    ap_uint<256> cipherkey = cipherkey_strm.read();
    cipher.updateKey(cipherkey);
LOOP_A:
    while (!is_end) {
#pragma HLS PIPELINE II = 1
        i_e_strm >> is_end;
        ap_uint<128> plaintext = plaintext_strm.read();
        ap_uint<128> ciphertext;
        cipher.process(plaintext, cipherkey, ciphertext);
        ciphertext_strm.write(ciphertext);
        o_e_strm << false;
    }
    o_e_strm << true;
}
