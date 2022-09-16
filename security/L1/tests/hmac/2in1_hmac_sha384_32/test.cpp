
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

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define XF_SECURITY_DECRYPT_DEBUG 1
#include <fstream>

#include "xf_security/sha512_t.hpp"
#define NUM_TESTS 2 // 200

void test_hmac_sha384(bool isHMAC,
                      hls::stream<ap_uint<32> >& msg_key_strm,
                      hls::stream<ap_uint<4> >& len_strm,
                      ap_uint<384>& res) {
    xf::security::HMAC_SHA384(isHMAC, msg_key_strm, len_strm, res);
}

void injectData(std::string data, hls::stream<ap_uint<32> >& data_strm, hls::stream<ap_uint<4> >& len_strm) {
    size_t total_len = data.length();
    ap_uint<4> tmp_len;
    ap_uint<32> tmp_data;

    if (total_len == 0) {
        tmp_len = 0;
        tmp_len[3] = 1;
        tmp_data = 0;

        data_strm.write(tmp_data);
        len_strm.write(tmp_len);
    } else {
        for (size_t i = 0; i < total_len; i += 4) {
            if (i + 4 >= total_len) { // last pack
                tmp_len = total_len - i;
                tmp_len[3] = 1; // mark last pack
            } else {            // not last pack
                tmp_len = 4;
            }

            tmp_data = 0;
            for (int j = 0; j < tmp_len.range(2, 0); j++) { // fill in pack
                tmp_data.range(j * 8 + 7, j * 8) = data[i + j];
            }
            data_strm.write(tmp_data);
            len_strm.write(tmp_len);
        }
    }
}

std::string ap2string(ap_uint<384> in) {
    std::string a;
    a.resize(384 / 8);
    for (int i = 0; i < 384 / 8; i++) {
        a[i] = in.range(i * 8 + 7, i * 8);
    }
    return a;
}

int main() {
    std::cout << "********************************" << std::endl;
    std::cout << "   Testing hmac+SHA384 on HLS project   " << std::endl;
    std::cout << "********************************" << std::endl;

    std::string key = "012345678901234567890123456789012345678901234567"; // key must be 48 bytes, 384 bits.
    std::string msg = "01234567890123456789012345678901234567890123456789012345678901234567890123456789";
    ap_uint<384> golden =
        "0xE37061C64EE68644CDA9E18EA4DE61D5730FC281F886BFA76FA67FB93DCB347D351F291374AA30D75E9AE015061D9B34";

    hls::stream<ap_uint<32> > msg_key_strm;
    hls::stream<ap_uint<4> > len_strm;
    ap_uint<384> res;

    injectData(key, msg_key_strm, len_strm);
    injectData(msg, msg_key_strm, len_strm);

    std::cout << "test vector injected" << std::endl;

    test_hmac_sha384(true, msg_key_strm, len_strm, res);
    if (res != golden) {
        std::cout << "res = " << std::hex << res << std::endl;
        std::cout << "golden = " << std::hex << golden << std::endl;
        return 1;
    } else {
        return 0;
    }
}
