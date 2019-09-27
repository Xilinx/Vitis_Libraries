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

#include "lz4_compress_core.hpp"
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>

int const c_minOffset = 1;
int const c_minMatch = 4;

#define LZ_MAX_OFFSET_LIMIT 65536
#define OFFSET_WINDOW (64 * 1024)
#define BOOSTER_OFFSET_WINDOW (16 * 1024)
#define LZ_HASH_BIT 12
#define LZ_DICT_SIZE (1 << LZ_HASH_BIT)
#define MAX_MATCH_LEN 255
#define MATCH_LEN 6
#define MATCH_LEVEL 2
#define LOP                                                                                                          \
    c_minOffset, c_minMatch, LZ_MAX_OFFSET_LIMIT, OFFSET_WINDOW, BOOSTER_OFFSET_WINDOW, LZ_DICT_SIZE, MAX_MATCH_LEN, \
        MATCH_LEN, MATCH_LEVEL

void lz4CompressEngineRun(hls::stream<uintV_t>& inStream,
                          hls::stream<uintV_t>& lz4Out,
                          hls::stream<bool>& lz4Out_eos,
                          hls::stream<uint32_t>& lz4OutSize,
                          uint32_t max_lit_limit[PARALLEL_BLOCK],
                          uint32_t input_size,
                          uint32_t core_idx) {
    lz4_compress_engine<LOP>(inStream, lz4Out, lz4Out_eos, lz4OutSize, max_lit_limit, input_size, 0);
}

int main(int argc, char* argv[]) {
    hls::stream<uintV_t> bytestr_in("compressIn");
    hls::stream<uintV_t> bytestr_out("compressOut");

    hls::stream<bool> lz4Out_eos;
    hls::stream<uint32_t> lz4OutSize;
    uint32_t max_lit_limit[PARALLEL_BLOCK];
    uint32_t input_size;
    uint32_t core_idx;

    std::ifstream inputFile;
    std::fstream outputFile;

    // Input file open for input_size
    inputFile.open(argv[1], std::ofstream::binary | std::ofstream::in);
    if (!inputFile.is_open()) {
        printf("Cannot open the input file!!\n");
        exit(0);
    }
    inputFile.seekg(0, std::ios::end);
    uint32_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    input_size = fileSize;
    uint32_t p = fileSize;

    // Pushing input file into input stream for compression
    while (p--) {
        uint8_t x;
        inputFile.read((char*)&x, 1);
        bytestr_in << x;
    }
    inputFile.close();

    // COMPRESSION CALL
    lz4CompressEngineRun(bytestr_in, bytestr_out, lz4Out_eos, lz4OutSize, max_lit_limit, input_size, 0);

    uint32_t outsize;
    outsize = lz4OutSize.read();
    printf("\n------- Compression Ratio: %f-------\n\n", (float)fileSize / outsize);

    outputFile.open(argv[2], std::fstream::binary | std::fstream::out);
    if (!outputFile.is_open()) {
        printf("Cannot open the output file!!\n");
        exit(0);
    }

    outputFile << input_size;

    bool eos_flag = lz4Out_eos.read();
    while (outsize > 0) {
        while (!eos_flag) {
            uint8_t w = bytestr_out.read();
            eos_flag = lz4Out_eos.read();
            outputFile.write((char*)&w, 1);
            outsize--;
        }
        if (!eos_flag) outsize = lz4OutSize.read();
    }
    uint8_t w = bytestr_out.read();
    outputFile.close();
}
