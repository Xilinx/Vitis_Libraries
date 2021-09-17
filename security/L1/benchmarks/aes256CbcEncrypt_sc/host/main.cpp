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
#include <ap_int.h>
#include <iostream>

#include "aes256CbcEncrypt_acc.hpp"
#include <sys/time.h>
#include <new>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include "xf_security/msgpack.hpp"

// text length for each task in 128-bit
#define N_ROW 64
// number of tasks for a single PCIe block
#define N_TASK 2 // 8192
// number of PUs
#define CH_NM 4
// cipher key size in bytes
#define KEY_SIZE 32

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

void genMsg(unsigned char* ptr, uint64_t msg_len) {
    // just repeat pattern to genereate msg_len data for test
    // Any other msg will be fine too.

    const char datapattern[] = {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                                0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f};
    for (uint64_t i = 0; i < msg_len; i += 16) {
        memcpy(ptr + i, datapattern, 16);
    }
}

uint64_t check(unsigned char* res, unsigned char* gld, uint64_t len) {
    int num = 0;
    for (uint64_t i = 0; i < len; i++) {
        if (res[i] != gld[i]) {
            num++;
            std::cout << i << "th char not match" << std::endl;
        }
    }
    return num;
}

bool aes256CbcEncrypt_execute_fpag(
    const int num_rep, int hb_in_size, int hb_out_size, ap_uint<128>* hb_in, unsigned char* golden) {
    std::vector<ap_uint<128>*> hb_outs(num_rep);
    for (int i = 0; i < num_rep; i++) {
        hb_outs[i] = aligned_alloc<ap_uint<128> >(hb_out_size);
    }

    int si = 0;
    int ri = 0;
    auto hbInBP = aes256CbcEncrypt::create_bufpool(vpp::input);
    auto hbOutBP = aes256CbcEncrypt::create_bufpool(vpp::output);

    aes256CbcEncrypt::send_while([&]() -> bool {
        // std::cout << "---->> sending input" << std::endl;
        aes256CbcEncrypt::set_handle(si);

        ap_uint<128>* hbIn = (ap_uint<128>*)aes256CbcEncrypt::alloc_buf(hbInBP, sizeof(ap_uint<128>) * hb_in_size);
        ap_uint<128>* hbOut = (ap_uint<128>*)aes256CbcEncrypt::alloc_buf(hbOutBP, sizeof(ap_uint<128>) * hb_out_size);

        memcpy(hbIn, hb_in, sizeof(ap_uint<128>) * hb_in_size);

        aes256CbcEncrypt::compute(hb_in_size, hb_out_size, hbIn, hbOut);
        return (++si < num_rep);
    });

    aes256CbcEncrypt::receive_all_in_order([&]() {
        ap_uint<128>* hbOut = (ap_uint<128>*)aes256CbcEncrypt::get_buf(hbOutBP);
        memcpy(hb_outs[ri++], hbOut, sizeof(ap_uint<128>) * hb_out_size);

    });
    aes256CbcEncrypt::join();

    bool verify = true;
    for (int i = 0; i < num_rep; i++) {
        uint64_t res_num = hb_outs[i][0];
        uint64_t ptr_cnt = 1;
        for (uint64_t k = 0; k < res_num; k++) {
            unsigned res_len = hb_outs[i][ptr_cnt++];
            std::cout << "res_num=" << res_num << ", res_len=" << res_len << std::endl;
            verify = !check((unsigned char*)(hb_outs[i] + ptr_cnt), golden, res_len);
            ptr_cnt += 64;
        }
    }
    return verify;
}

int main(int argc, char* argv[]) {
    // cmd parser
    ArgParser parser(argc, (const char**)argv);

    int num_rep = 1;
    std::string msg_len_str;
    uint64_t msg_len;
    if (!parser.getCmdOption("-len", msg_len_str)) {
        std::cout << "ERROR:msg length is not set!\n";
        return 1;
    } else {
        msg_len = std::stoi(msg_len_str);
        if (msg_len % 16 != 0) {
            std::cout << "ERROR: msg length is nott multiple of 16!\n";
            return 1;
        }
        std::cout << "Length of single message is " << msg_len << " Bytes " << std::endl;
    }

    std::string msg_num_str;
    uint64_t msg_num;
    if (!parser.getCmdOption("-num", msg_num_str)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    } else {
        msg_num = std::stoi(msg_num_str);
        std::cout << "Message num is " << msg_num << std::endl;
    }

    std::string gld_path;
    if (!parser.getCmdOption("-gld", gld_path)) {
        std::cout << "ERROR:golden path is not set!\n";
        return 1;
    }
    // cipher key for test, other keys are fine.
    unsigned char key[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                           0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                           0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};

    // initialization vector for test, other IVs are fine.
    unsigned char ivec[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                            0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};

    // generate msg and corresponding its encrypted txt
    unsigned char* msg = (unsigned char*)malloc(msg_len + 16);
    genMsg(msg, msg_len);

    std::ifstream ifile;
    ifile.open(gld_path);
    unsigned char* gld = (unsigned char*)malloc(msg_len + 16);
    //{
    //    int outlen1 = 0;
    //    int outlen2 = 0;
    //    EVP_CIPHER_CTX ctx;
    //    EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), key, ivec);
    //    EVP_EncryptUpdate(&ctx, gld, &outlen1, msg, msg_len);
    //    EVP_EncryptFinal(&ctx, gld + outlen1, &outlen2);
    //}
    ifile.read((char*)gld, msg_len + 16);
    ifile.close();

    // Use packer to prepare msg package.
    //
    // Package is aligned to 16Bytes.
    // Row[0] contains msg_num, and messages are stored in sequential block.
    // Each block's first row will contains its message's length, followed by IV, Key and message itself.
    // IV will take one row, Key will take two row.
    // After all messages are added, call "finishPack()" to write package header to Row[0]
    // Then no message should be added.
    //
    // Result will also be packed which is aligned to 16Bytes.
    // Row[0] contains msg_num. Messages are stored in sequential block.
    // Each block's first row will contains its message length.
    uint64_t in_pack_size = ((msg_len + 15) / 16 * 16 + 16 + 16 + 32) * msg_num + 16;
    uint64_t out_pack_size = ((msg_len + 15) / 16 * 16 + 16) * msg_num + 16;
    uint64_t pure_msg_size = (msg_len + 15) / 16 * 16 * msg_num;
    unsigned char* inputData = aligned_alloc<unsigned char>(in_pack_size);
    // unsigned char* inputData = (unsigned char*)malloc(in_pack_size);
    unsigned char* outputData = aligned_alloc<unsigned char>(out_pack_size);
    // unsigned char* outputData = (unsigned char*)malloc(out_pack_size);

    xf::security::internal::aesCbcPack<256> packer;
    packer.reset();
    packer.setPtr(inputData, in_pack_size);
    for (uint64_t i = 0; i < msg_num; i++) {
        packer.addOneMsg(msg, msg_len, ivec, key);
    }
    packer.finishPack();

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    std::cout << "Host map buffer has been allocated and set.\n";
    bool checked =
        aes256CbcEncrypt_execute_fpag(num_rep, in_pack_size / 16, out_pack_size / 16, (ap_uint<128>*)inputData, gld);
    gettimeofday(&end_time, 0);
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

    // release resource
    free(msg);
    free(gld);
    free(inputData);

    // if passed
    std::cout << (checked ? "PASSED" : "ERROR: FAILED") << ": kernel end------" << std::endl;
    if (checked) {
        return 0;
    } else {
        return -1;
    }
}
