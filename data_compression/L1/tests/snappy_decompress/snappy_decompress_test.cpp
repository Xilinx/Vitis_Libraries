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
#include "snappy_decompress_core.hpp"
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>

#define READ_STATE 0
#define MATCH_STATE 1
#define LOW_OFFSET_STATE 2
#define MAX_OFFSET 65536
#define HISTORY_SIZE MAX_OFFSET
#define LOW_OFFSET 8
#define SOP READ_STATE, MATCH_STATE, LOW_OFFSET_STATE, LOW_OFFSET, HISTORY_SIZE

void snappyDecompressEngineRun(hls::stream<uintV_t>& inStream,
                               hls::stream<uintV_t>& snappyOut,
                               const uint32_t input_size,
                               const uint32_t output_size) {
    snappy_decompress_engine<SOP>(inStream, snappyOut, input_size, output_size);
}

int main(int argc, char* argv[]) {
    hls::stream<uintV_t> dec_bytestr_in("decompressIn");
    hls::stream<uintV_t> dec_bytestr_out("decompressOut");

    std::ifstream originalFile;
    std::fstream outputFile;

    outputFile.open(argv[1], std::fstream::binary | std::fstream::in);
    if (!outputFile.is_open()) {
        printf("Cannot open the compressed file!!\n");
        exit(0);
    }
    uint32_t output_size;
    outputFile >> output_size;
    outputFile.seekg(0, std::ios::end);
    uint32_t comp_length = (uint32_t)outputFile.tellg() - 4;
    outputFile.seekg(4, std::ios::beg);
    uint32_t p = comp_length;
    for (int i = 0; i < p; i++) {
        uint8_t x;
        outputFile.read((char*)&x, 1);
        dec_bytestr_in << x;
    }

    // DECOMPRESSION CALL
    snappyDecompressEngineRun(dec_bytestr_in, dec_bytestr_out, comp_length, output_size);

    uint32_t outputsize;
    outputsize = output_size;

    originalFile.open(argv[2], std::ofstream::binary | std::ofstream::in);
    if (!originalFile.is_open()) {
        printf("Cannot open the original file!!\n");
        exit(0);
    }
    uint8_t s, t;
    bool pass = true;
    for (uint32_t i = 0; i < outputsize; i++) {
        s = dec_bytestr_out.read();
        originalFile.read((char*)&t, 1);
        if (s == t)
            continue;
        else {
            pass = false;
            printf("\n-----TEST FAILED: The input file and the file after decompression are not similar!-----\n");
            exit(0);
        }
    }
    if (pass) {
        printf(
            "\n-----TEST PASSED: Original file and the file after decompression "
            "are same.-------\n");
    }
    printf("\n");
    originalFile.close();
    outputFile.close();
}
