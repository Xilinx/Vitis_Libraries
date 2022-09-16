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

#include <hls_stream.h>
#include <ap_int.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;
#include <sstream>
#include <string>
#include <vector>

#include <fstream>

// number of times to perform the test in different message and length
#define NUM_TESTS 300
// the size of each message word in byte
#define MSG_SIZE 8
// the size of the digest in byte
#define DIG_SIZE 48

// table to save each message and its hash value
struct Test {
    string msg;
    unsigned char hash[DIG_SIZE];
    Test(const char* m, const void* h) : msg(m) { memcpy(hash, h, DIG_SIZE); }
};

// print hash value
std::string hash2str(unsigned char* h, int len) {
    ostringstream oss;
    string retstr;

    // check output
    oss.str("");
    oss << hex;
    for (int i = 0; i < len; i++) {
        oss << setw(2) << setfill('0') << (unsigned)h[i];
    }
    retstr = oss.str();
    return retstr;
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

int main() {
    std::cout << "************************************" << std::endl;
    std::cout << "   Testing SHA-384 on HLS project   " << std::endl;
    std::cout << "************************************" << std::endl;

    // the original message to be digested
    const char message[] =
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz";
    vector<Test> tests;
    std::ifstream ifile;
    ifile.open("gld.dat");

    // generate golden
    for (unsigned int i = 0; i < NUM_TESTS; i++) {
        unsigned int len = i % 256;
        char m[256];
        if (len != 0) {
            memcpy(m, message, len);
        }
        m[len] = 0;
        unsigned char h[DIG_SIZE];
        // call OpenSSL API to get the MD5 hash value of each message
        // SHA384((const unsigned char*)message, len, (unsigned char*)h);
        ifile.read((char*)h, DIG_SIZE);
        tests.push_back(Test(m, h));
    }
    ifile.close();

    unsigned int nerror = 0;
    unsigned int ncorrect = 0;

    hls::stream<ap_uint<32> > msg_strm("msg_strm");
    hls::stream<ap_uint<4> > len_strm("len_strm");
    ap_uint<8 * DIG_SIZE> digest;

    // generate input message words
    for (vector<Test>::const_iterator test = tests.begin(); test != tests.end(); test++) {
        injectData((*test).msg, msg_strm, len_strm);
    }

    // check result
    for (vector<Test>::const_iterator iter = tests.begin(); iter != tests.end(); iter++) {
        test(false, msg_strm, len_strm, digest);

        unsigned char hash[DIG_SIZE];
        for (unsigned int i = 0; i < DIG_SIZE; i++) {
            hash[i] = (unsigned char)(digest.range(7 + 8 * i, 8 * i).to_int() & 0xff);
        }

        if (memcmp((*iter).hash, hash, DIG_SIZE)) {
            ++nerror;
            cout << "fpga   : " << hash2str((unsigned char*)hash, DIG_SIZE) << endl;
            cout << "golden : " << hash2str((unsigned char*)(*iter).hash, DIG_SIZE) << endl;
        } else {
            ++ncorrect;
        }
    }

    if (nerror) {
        cout << "FAIL: " << dec << nerror << " errors found." << endl;
    } else {
        cout << "PASS: " << dec << ncorrect << " inputs verified, no error found." << endl;
    }

    return nerror;
}
