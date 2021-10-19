/*
 * Copyright 2020 Xilinx, Inc.
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

#include "xf_database/gqe_bloomfilter.hpp"

// left rotates x with k bits
#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))));

// #define BLOOM_FILTER_DEBUG

namespace xf {
namespace database {
namespace gqe {

// calculates bloom-filter size under the equation provided in:
// https://en.wikipedia.org/wiki/Bloom_filter
// returns the number of bits needed.
uint64_t BloomFilter::calcSize(uint64_t num_keys) {
    // p ~= (1 - e^(-kn/m))^k
    uint64_t ideal_size_in_bits = -4.0f * num_keys / log(1.0f - sqrt(sqrt(FPP))) + 1;
    if (dbg) std::cout << "Debug: Ideal BF size in bits = " << ideal_size_in_bits << std::endl;
    // need 4096-bit aligned, as hardware utilizes 8 HBMs with 8 * 64-bit width cache line size
    uint64_t aligned_bits_num = 8 * 8 * 64;
    uint64_t aligned_size_in_bits = ideal_size_in_bits / aligned_bits_num * aligned_bits_num +
                                    ((ideal_size_in_bits % aligned_bits_num) > 0) * aligned_bits_num;
    // 16Gbits is currently hardware limit.
    if (aligned_size_in_bits >= 16UL * 1024 * 1024 * 1024) {
        std::cout << "Requires " << (float)aligned_size_in_bits / (1 << 30) << " Gbits for " << num_keys
                  << " unique keys, please reduce the number of unique keys\n";
        exit(1);
    }
    std::cout << "Hardware bloom-filter size in bits = " << aligned_size_in_bits << std::endl << std::endl;
    return aligned_size_in_bits;
}

// calculates bloom-filter size as well as allocates buffer for internal hash-table
BloomFilter::BloomFilter(uint64_t num_keys, float fpp) : FPP(fpp) {
    // calculates bloom-filter size based on number of unique keys
    bloom_filter_bits = calcSize(num_keys);

    // allocates buffer for internal hash-table
    // buffuers will be released automatically with the dead of the mm object
    hash_table = mm.aligned_alloc<uint64_t*>(8);
    for (int i = 0; i < 8; i++) {
        uint64_t nv = bloom_filter_bits / 8 / (sizeof(uint64_t) * 8);
        hash_table[i] = mm.aligned_alloc<uint64_t>(nv);
        // clean up the hash-table
        memset(hash_table[i], 0, sizeof(uint64_t) * nv);
    }
}

// calculates hash value with the given key(s)
uint64_t BloomFilter::hashLookup3(uint64_t key0_val, uint64_t key1_val) {
    const int key96blen = 128 / 96;

    // key8blen is the BYTE len of the key.
    const int key8blen = 128 / 8;
    const uint32_t c1 = 0xdeadbeef;

    // use magic word(seed) to initial the output
    uint64_t hash1 = 1032032634;
    uint64_t hash2 = 2818135537;

    uint32_t a = c1 + ((uint32_t)key8blen) + ((uint32_t)hash1);
    uint32_t b = c1 + ((uint32_t)key8blen) + ((uint32_t)hash1);
    uint32_t c = c1 + ((uint32_t)key8blen) + ((uint32_t)hash1);
    c += (uint32_t)hash2;

    a += (uint32_t)(key0_val & 0x00000000ffffffffUL);
    b += (uint32_t)(key0_val >> 32);
    c += (uint32_t)(key1_val & 0x00000000ffffffffUL);

    a -= c;
    a ^= rot(c, 4);
    c += b;

    b -= a;
    b ^= rot(a, 6);
    a += c;

    c -= b;
    c ^= rot(b, 8);
    b += a;

    a -= c;
    a ^= rot(c, 16);
    c += b;

    b -= a;
    b ^= rot(a, 19);
    a += c;

    c -= b;
    c ^= rot(b, 4);
    b += a;

    a += (uint32_t)(key1_val >> 32);

    // finalization
    c ^= b;
    c -= rot(b, 14);

    a ^= c;
    a -= rot(c, 11);

    b ^= a;
    b -= rot(a, 25);

    c ^= b;
    c -= rot(b, 16);

    a ^= c;
    a -= rot(c, 4);

    b ^= a;
    b -= rot(a, 14);

    c ^= b;
    c -= rot(b, 24);

    hash1 = (uint64_t)c;
    hash2 = (uint64_t)b;

    uint64_t hash_val = hash1 << 32 | hash2;

    return hash_val;
}

// gets index of the single string in group of string (if any)
int BloomFilter::findStrInd(std::vector<std::string> group_str, std::string single_str) {
    auto pos_iter = std::find(group_str.begin(), group_str.end(), single_str);
    if (pos_iter != group_str.end()) return (pos_iter - group_str.begin());
    return -1;
}

// extracts key name(s) from input_str, the vector of key names is returned
// for single key, key_names.size() = 1; for dual key, key_names.size() = 2
std::vector<std::string> BloomFilter::extractKeyNames(std::string input_str) {
    // single key or dual keys
    std::vector<std::string> key_names;

    if (input_str == "") {
        std::cout << "At least 1 key column needed to build bloom-filter\n";
        exit(1);
    }
    // gets key name one by one
    std::istringstream f(input_str);
    std::string s;
    while (getline(f, s, ',')) {
        key_names.push_back(s);
    }
    if (key_names.size() > 2) {
        std::cout << "At most 2 key columns provided to build bloom-filter\n";
        exit(1);
    }
    return key_names;
}

// builds hash-table with the given keys
void BloomFilter::build(Table tab_in, std::string key_names_str) {
    // gets number of rows of input table
    size_t n_row = tab_in.getRowNum();
    // remove the space key_names str
    xf::database::internals::filter_config::trim(key_names_str);
    // from comma separated key_names_str to std::vector<std::string> key_names
    std::vector<std::string> key_names = extractKeyNames(key_names_str);
    // gets column names in table
    std::vector<std::string> tab_col_names = tab_in.getColNames();
    // column pointer(s)
    std::vector<uint64_t*> key_ptr;
    key_ptr.resize(key_names.size());
    for (size_t i = 0; i < key_names.size(); i++) {
        int found = findStrInd(tab_col_names, key_names[i]);
        if (found != -1) {
            key_ptr[i] = (uint64_t*)tab_in.getColPointer(found);
        } else {
            std::cout << "No column named " << key_names[i] << " in the input table\n";
            exit(1);
        }
    }

    for (size_t i = 0; i < n_row; i++) {
        // calculates hash value with the input keys
        uint64_t hash64;
        if (key_names.size() == 1) {
#ifdef BLOOM_FILTER_DEBUG
            std::cout << "No." << i << ", build key " << key_ptr[0][i] << " into bloom-filter" << std::endl;
#endif
            hash64 = hashLookup3(key_ptr[0][i], 0);
        } else {
#ifdef BLOOM_FILTER_DEBUG
            std::cout << "No." << i << ", build key0 " << key_ptr[0][i] << " , key1 " << key_ptr[1][i]
                      << " into bloom-filter" << std::endl;
#endif
            hash64 = hashLookup3(key_ptr[0][i], key_ptr[1][i]);
        }
        // index of the HBM
        uint64_t idx = hash64 >> 61; // 3 bits (63, 62, 61) for selecting HBM buffer.
        hash64 = hash64 & 0x1FFFFFFFFFFFFFFFUL;
        // first hash is used to locate start of the block
        uint32_t hash1 = (uint32_t)hash64;
        uint32_t hash2 = (uint32_t)(hash64 >> 32);
        uint32_t firstHash = hash1 + hash2;
        uint32_t totalBlockCountEach = bloom_filter_bits / 64 / 8;
        uint32_t blockIdx = firstHash % totalBlockCountEach;
        uint32_t blockBaseOffset = blockIdx & 0xFFFFFFF8;
        // subsequent K hashes are used to generate K bits within a cache line (4 blocks)
        for (int i = 1; i <= NUM_HASH_FUNCS; i++) {
            uint32_t combinedHash = hash1 + ((i + 1) * hash2);
            uint32_t absOffset = blockBaseOffset + (combinedHash & 0x00000007);
            int bitPos = (combinedHash >> 3) & 63;
            hash_table[idx][absOffset] |= ((uint64_t)1 << bitPos);
        }
    }
}

// merges the given bloom-filter into current one
void BloomFilter::merge(BloomFilter& bf_in) {
    // get the hash-table of the input bloom-filter
    uint64_t** in_hash = bf_in.getHashTable();
    uint64_t in_bf_size = bf_in.getBloomFilterSize();

    // same size?
    if (in_bf_size != bloom_filter_bits) {
        std::cout << "Only bloom-filters in the same size can be merged!\n";
        exit(1);
    }

    // indexing each individual HBM storage
    for (int i = 0; i < 8; i++) {
        // merge BF2 into BF1
        for (uint32_t j = 0; j < (bloom_filter_bits / 8 / (sizeof(uint64_t) * 8)); j++) {
            hash_table[i][j] |= in_hash[i][j];
        }
    }
}

// gets hash-table
uint64_t** BloomFilter::getHashTable() const {
    return hash_table;
}

// gets bloom-filter size
uint64_t BloomFilter::getBloomFilterSize() const {
    return bloom_filter_bits;
}

} // namespace gqe
} // namespace database
} // namespace xf
