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

#include <openssl/aes.h>
#include <openssl/evp.h>

// number of times to perform the test in different text and length
// XXX notice that the datain char array should be long enough
#define NUM_TESTS 300
// cipherkey size in byte
#define KEY_SIZE 24
// cipher block size in byte
#define BLK_SIZE 16

// print result
std::string printr(unsigned char* result, unsigned int len) {
    ostringstream oss;
    string retstr;

    // check output
    oss.str("");
    oss << hex;
    for (unsigned int i = 0; i < len; i++) {
        oss << setw(2) << setfill('0') << (unsigned)result[i];
    }
    retstr = oss.str();
    return retstr;
}

// table to save each input data and its result
struct Test {
    unsigned char* data;
    unsigned char* result;
    unsigned int length;
    unsigned char* tag;
    Test(const char* d, const char* r, unsigned int len, const char* t) : length(len) {
        data = (unsigned char*)malloc(len);
        memcpy(data, d, len);
        result = (unsigned char*)malloc(len);
        memcpy(result, r, len);
        tag = (unsigned char*)malloc(BLK_SIZE);
        memcpy(tag, t, BLK_SIZE);
    }
};

int main() {
    std::cout << "**************************************************" << std::endl;
    std::cout << "   Testing GCM mode with AES-192 on HLS project   " << std::endl;
    std::cout << "**************************************************" << std::endl;

    // plaintext
    const unsigned char plaintext[] =
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
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz";

    // cipher key
    const unsigned char key[] =
        "abcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz";

    // initialization vector
    const unsigned char iv[] = "0000000000000000";
    int iv_len = 12;

    // additional authenticated data
    const unsigned char aad[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int aad_len = 0;

    vector<Test> tests;

    // generate golden
    for (unsigned int i = 1; i < NUM_TESTS + 1; i++) {
        // ouput length of the result
        int len = 0;
        int ciphertext_len = 0;
        // input data length must be a multiple of 16
        unsigned int plaintext_len = (i % 16) * BLK_SIZE + BLK_SIZE;
        // output result buffer
        unsigned char ciphertext[2 * plaintext_len];
        unsigned char tag[BLK_SIZE];

        char din[300];
        if (plaintext_len != 0) {
            memcpy(din, plaintext + i, plaintext_len);
        }
        din[plaintext_len] = 0;
        // call OpenSSL API to get the golden
        EVP_CIPHER_CTX* ctx;
        ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_192_gcm(), NULL, NULL, NULL);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL);
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
        EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len);
        EVP_EncryptUpdate(ctx, ciphertext, &len, (const unsigned char*)din, plaintext_len);
        ciphertext_len = len;
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        ciphertext_len += len;
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, BLK_SIZE, tag);
        EVP_CIPHER_CTX_free(ctx);
        for (unsigned int i = 0; i < plaintext_len / BLK_SIZE; i++) {
            cout << "EVP_golden[" << dec << i << "] : " << printr((unsigned char*)ciphertext + i * BLK_SIZE, BLK_SIZE)
                 << endl;
        }
        cout << "tag_golden : " << printr((unsigned char*)tag, BLK_SIZE) << endl;
        tests.push_back(Test((const char*)plaintext + i, (const char*)ciphertext, plaintext_len, (const char*)tag));
    }

    unsigned int nerror = 0;
    unsigned int ncorrect = 0;

    hls::stream<ap_uint<128> > plaintextStrm("plaintextStrm");
    hls::stream<bool> endPlaintextStrm("endPlaintextStrm");
    hls::stream<ap_uint<8 * KEY_SIZE> > cipherkeyStrm("cipherkeyStrm");
    hls::stream<ap_uint<96> > IVStrm("IVStrm");
    hls::stream<ap_uint<128> > AADStrm("AADStrm");
    hls::stream<ap_uint<128> > ciphertextStrm("ciphertextStrm");
    hls::stream<bool> endCiphertextStrm("endCiphertextStrm");
    hls::stream<ap_uint<128> > tagStrm("tagStrm");

    for (vector<Test>::const_iterator singletest = tests.begin(); singletest != tests.end(); singletest++) {
        // generate cipherkey
        ap_uint<8 * KEY_SIZE> keyReg;
        for (unsigned int i = 0; i < KEY_SIZE; i++) {
            keyReg.range(i * 8 + 7, i * 8) = key[i];
        }
        cipherkeyStrm.write(keyReg);

        // generate initialization vector
        ap_uint<96> IVReg;
        for (unsigned int i = 0; i < 12; i++) {
            IVReg.range(i * 8 + 7, i * 8) = iv[i];
        }
        IVStrm.write(IVReg);

        // generate additional authenticated data
        ap_uint<128> AADReg;
        for (unsigned int i = 0; i < 16; i++) {
            AADReg.range(i * 8 + 7, i * 8) = aad[i];
        }
        AADStrm.write(AADReg);

        ap_uint<8 * BLK_SIZE> plaintextReg;
        unsigned int n = 0;
        // generate input data stream
        for (unsigned int i = 0; i < (*singletest).length; i++) {
            if (n == 0) {
                plaintextReg = 0;
            }
            plaintextReg.range(7 + 8 * n, 8 * n) = (unsigned char)((*singletest).data[i]);
            n++;
            if (n == BLK_SIZE) {
                plaintextStrm.write(plaintextReg);
                endPlaintextStrm.write(false);
                n = 0;
            }
        }
        // deal with the condition that we didn't hit a boundary of the last block
        if (n != 0) {
            plaintextStrm.write(plaintextReg);
            endPlaintextStrm.write(false);
        }
        endPlaintextStrm.write(true);

        // call fpga module
        test(plaintextStrm, endPlaintextStrm, cipherkeyStrm, IVStrm, AADStrm, ciphertextStrm, endCiphertextStrm,
             tagStrm);

        // check result
        ap_uint<8 * BLK_SIZE> ciphertext;
        ap_uint<8 * BLK_SIZE> ciphertext_golden;
        ap_uint<8 * BLK_SIZE> tag;
        ap_uint<8 * BLK_SIZE> tag_golden;
        bool end = endCiphertextStrm.read();
        bool checked = true;
        int index = 0;
        while (!end) {
            ciphertext = ciphertextStrm.read();
            for (unsigned int i = 0; i < BLK_SIZE; i++) {
                ciphertext_golden.range(7 + 8 * i, 8 * i) = (unsigned char)((*singletest).result[i + index]);
            }
            if (ciphertext_golden != ciphertext) {
                checked = false;
            }
            end = endCiphertextStrm.read();
            index += BLK_SIZE;
        }

        tag = tagStrm.read();
        for (unsigned int i = 0; i < BLK_SIZE; i++) {
            tag_golden.range(7 + 8 * i, 8 * i) = (unsigned char)((*singletest).tag[i]);
        }
        if (tag_golden != tag) {
            checked = false;
        }

        if (!checked) {
            ++nerror;
            cout << "fpga_ciphertext   : " << hex << ciphertext << endl;
            cout << "golden_ciphertext : " << hex << ciphertext_golden << endl;
            cout << "fpga_tag   : " << hex << tag << endl;
            cout << "golden_tag : " << hex << tag_golden << endl;
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
