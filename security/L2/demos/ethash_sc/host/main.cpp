/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_security/ethash_krl.hpp"

#include "sha3_ethash.hpp"

#include <algorithm>
#include <iterator>

#include <sys/time.h>
#include <new>
#include <cstdlib>
#include <ap_int.h>
#include <iostream>

#include <vector>
#include <string>
#include <fstream>

static ap_uint<32> fnv_hash_host(ap_uint<32> x, ap_uint<32> y) {
    // result = (x * FNV_PRIME) ^ y;
    // the second operator is XOR, not POWR...
    const ap_uint<25> FNV_PRIME = 0x01000193;
    ap_uint<32> tmp = x * FNV_PRIME;
    return tmp ^ y;
}

inline void ethash_naive(ap_uint<64> cnt,
                         ap_uint<256>& result,
                         ap_uint<256>& mix_hash,
                         ap_uint<512>* full_node,
                         ap_uint<64> full_size,
                         ap_uint<256> header_hash,
                         ap_uint<64> nonce) {
    // 1. pack hash and nonce together into first 40 bytes of s_mix
    ap_uint<512> s_mix[3];
    //#pragma HLS array_partition variable = s_mix dim = 1
    for (int i = 0; i < 3; i++) {
#pragma HLS unroll
        s_mix[3] = 0;
    }
    s_mix[0].range(255, 0) = header_hash;
    s_mix[0].range(319, 256) = nonce;

    // 2. compute sha3-512 hash and replicate across mix
    s_mix[0] = sha3_512_40(s_mix[0]);
    s_mix[1] = s_mix[0];
    s_mix[2] = s_mix[0];

    const unsigned int page_size = 128;
    const unsigned int num_full_pages = full_size / page_size;

    for (ap_uint<32> i = 0; i < 64; i++) {
#pragma HLS pipeline
        const ap_uint<32> fnv_x = s_mix[0].range(31, 0) ^ i;
        const ap_uint<32> fnv_t1 = (i % 32) / 16;
        const ap_uint<32> fnv_t2 = (i % 32) % 16;
        const ap_uint<32> fnv_y =
            fnv_t1 == 0 ? s_mix[1].range(fnv_t2 * 32 + 31, fnv_t2 * 32) : s_mix[2].range(fnv_t2 * 32 + 31, fnv_t2 * 32);
        const ap_uint<32> index = fnv_hash_host(fnv_x, fnv_y) % num_full_pages;

        for (int j = 0; j < 2; j++) {
#pragma HLS unroll
            ap_uint<512> dag_node = full_node[index * 2 + j];

            for (int k = 0; k < 16; k++) {
#pragma HLS unroll
                s_mix[j + 1].range(k * 32 + 31, k * 32) =
                    fnv_hash_host(s_mix[j + 1].range(k * 32 + 31, k * 32), dag_node.range(k * 32 + 31, k * 32));
            }
        }
    }

    // 3. compress mix
    ap_uint<256> res_hash;
    for (int w = 0; w < 32; w += 4) {
#pragma HLS unroll
        const ap_uint<32> t1 = w / 16;
        const ap_uint<32> t2 = w % 16;
        const ap_uint<512> local_mix = t1 == 0 ? s_mix[1] : s_mix[2];

        ap_uint<32> reduction = local_mix.range(t2 * 32 + 31, t2 * 32);
        reduction = fnv_hash_host(reduction, local_mix.range((t2 + 1) * 32 + 31, (t2 + 1) * 32));
        reduction = fnv_hash_host(reduction, local_mix.range((t2 + 2) * 32 + 31, (t2 + 2) * 32));
        reduction = fnv_hash_host(reduction, local_mix.range((t2 + 3) * 32 + 31, (t2 + 3) * 32));
        res_hash.range((w / 4) * 32 + 31, (w / 4) * 32) = reduction;
    }
    if (cnt == 0) {
        //        std::cout << "compress: " << res_hash;
    }

    s_mix[1].range(255, 0) = res_hash;
    mix_hash = res_hash;

    // 4. final Keccak hash
    result = sha3_256_96(s_mix[0], s_mix[1]);
    if (cnt == 0) {
        //        std::cout << "sha256: " << result;
    }
}

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

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

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

int main(int argc, char* argv[]) {
    ArgParser parser(argc, (const char**)argv);

    std::string cnt_str;
    if (!parser.getCmdOption("-n", cnt_str)) {
        std::cout << "ERROR:batch number is not set!\n";
        return 1;
    }
    unsigned int num = std::stoi(cnt_str);

    // prepare host buffer
    // dag size, header hash, nonce_start, nonce_count
    // long full_size = 8UL * 1024UL * 1024UL * 1024UL - 4096UL - 4096UL;
    long full_size = 1UL * 1024UL * 1024UL * 1024UL;
    char header_hash_string[] = "~~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    ap_uint<256> header_hash = 0;
    for (int i = 0; i < 32; i++) {
        header_hash.range(i * 8 + 7, i * 8) = header_hash_string[i];
    }
    ap_uint<64> nonce_start = 0x7c7c597c;
    ap_uint<64> dag_magic_number = 0xfee1deadbaddcafe;
    std::cout << "dag magic number:" << std::hex << dag_magic_number << std::dec << std::endl;

    ap_uint<512>* full_node = aligned_alloc<ap_uint<512> >((full_size + 63) / 64);
    char* full_node_char = reinterpret_cast<char*>(full_node);
    srand(5);
    for (int i = 0; i < ((full_size + 63) / 64); i++) {
        full_node_char[i] = rand() % 256;
    }
    std::cout << "Finish feeding all nodes.\n";

    std::vector<char*> dram(4);
    for (unsigned long int i = 0; i < 3; i++) {
        dram[i] = aligned_alloc<char>(256UL * 1024UL * 1024UL);
    }
    dram[3] = aligned_alloc<char>(256UL * 1024UL * 1024UL - 4096UL - 4096UL);
    for (int i = 0; i < 8192; i++) {
        dram[3][256UL * 1024UL * 1024UL - 8192UL] = 0;
    }

    ap_uint<1024>* fullnode_1K = reinterpret_cast<ap_uint<1024>*>(full_node);
    std::vector<ap_uint<1024>*> dram1K(4);
    for (int i = 0; i < 4; i++) {
        dram1K[i] = reinterpret_cast<ap_uint<1024>*>(dram[i]);
    }

    for (ap_uint<26> i = 0; i < (full_size + 127) / 128; i++) {
        ap_uint<12> index_remapL = i.range(11, 0);
        ap_uint<2> index_remapM = i.range(13, 12);
        ap_uint<12> index_remapH = i.range(25, 14);
        ap_uint<12> index_remapHR = index_remapH.reverse();

        ap_uint<2> addrH = index_remapM;
        ap_uint<24> addrL = index_remapHR.concat(index_remapL);
        addrL.range(23, 21) = 0;

        dram1K[addrH][addrL] = fullnode_1K[i];
    }
    std::cout << "DRAM updated.\n";

    auto dram_buf0 = ethash_acc::create_bufpool(vpp::input);
    auto dram_buf1 = ethash_acc::create_bufpool(vpp::input);
    auto dram_buf2 = ethash_acc::create_bufpool(vpp::input);
    auto dram_buf3 = ethash_acc::create_bufpool(vpp::bidirectional);

    unsigned int index = 0;
    ethash_acc::send_while([&]() -> bool {
        std::cout << "send_while index=" << index << "\n";
        ap_uint<512>* acc_dram0 = (ap_uint<512>*)ethash_acc::alloc_buf(dram_buf0, 256UL * 1024UL * 1024UL);
        ap_uint<512>* acc_dram1 = (ap_uint<512>*)ethash_acc::alloc_buf(dram_buf1, 256UL * 1024UL * 1024UL);
        ap_uint<512>* acc_dram2 = (ap_uint<512>*)ethash_acc::alloc_buf(dram_buf2, 256UL * 1024UL * 1024UL);
        ap_uint<512>* acc_dram3 = (ap_uint<512>*)ethash_acc::alloc_buf(dram_buf3, 256UL * 1024UL * 1024UL);
        memcpy(acc_dram0, dram[0], 256UL * 1024UL * 1024UL);
        memcpy(acc_dram1, dram[1], 256UL * 1024UL * 1024UL);
        memcpy(acc_dram2, dram[2], 256UL * 1024UL * 1024UL);
        memcpy(acc_dram3, dram[3], 256UL * 1024UL * 1024UL);

        ethash_acc::compute(index, (full_size / 128), header_hash, nonce_start, num, acc_dram0, acc_dram1, acc_dram2,
                            acc_dram3);

        index++;
        nonce_start = nonce_start + 4096 * num * index;

        return (index < 8);
    });
    std::cout << "All task requests sent.\n";

    char* ret = aligned_alloc<char>(4096UL);
    for (int i = 0; i < 4096; i++) {
        ret[i] = 0;
    }
    ap_uint<32>* ret32 = reinterpret_cast<ap_uint<32>*>(ret);
    unsigned int order = 0;
    ethash_acc::receive_all_in_order([&]() {
        char* ret_acc = (char*)ethash_acc::get_buf(dram_buf3);
        ap_uint<32>* ret32_acc = reinterpret_cast<ap_uint<32>*>(&ret_acc[256UL * 1024UL * 1024UL - 4096UL]);
        std::cout << "Round " << order << std::endl;
        for (int i = 0; i < 8; i++) {
            std::cout << "ret32[" << i << "] = " << std::hex << ret32_acc[i] << std::dec << std::endl;
        }
        memcpy(&ret[order * 4], &ret_acc[256UL * 1024UL * 1024UL - 4096UL + order * 4], 4);
        order++;
    });
    std::cout << "All result receiving requests sent.\n";

    std::cout << "Starting ethash_acc::join()\n";
    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    ethash_acc::join();
    gettimeofday(&end_time, 0);
    std::cout << "Finishing ethash_acc::join()\n";
    std::cout << "Total execution time " << tvdiff(&start_time, &end_time) / 1000 << "ms" << std::endl;

    std::cout << "============================================================" << std::endl;
    ap_uint<32> sum = 0;
    for (int i = 0; i < 8; i++) {
        std::cout << "ret32[" << i << "] = " << std::hex << ret32[i] << std::dec << std::endl;
        sum += ret32[i];
    }
    std::cout << "Checksum = " << std::hex << sum << std::dec << std::endl;

    return 0;
}
