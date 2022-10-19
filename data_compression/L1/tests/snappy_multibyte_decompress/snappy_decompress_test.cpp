/*
 * Copyright 2019-2022 Xilinx, Inc.
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

#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>

#include "snappy_decompress_details.hpp"
#define MULTIPLE_BYTES 8

#define MAX_OFFSET (64 * 1024)
#define HISTORY_SIZE MAX_OFFSET

typedef ap_uint<MULTIPLE_BYTES * 8> uintS_t;
typedef ap_uint<(MULTIPLE_BYTES * 8) + 8> uintV_t;

void snappyDecompressEngineRun(hls::stream<uintS_t>& inStream, hls::stream<uintV_t>& outStream, uint32_t input_size) {
    xf::compression::snappyDecompressEngine<MULTIPLE_BYTES, HISTORY_SIZE>(inStream, outStream, input_size);
}

int main(int argc, char* argv[]) {
    hls::stream<uintS_t> inStream("inStream");
    hls::stream<bool> inStreamEos("inStreamEos");
    hls::stream<uintV_t> outStream("decompressOut");
    uint32_t input_size;

    std::fstream outputFile;
    outputFile.open(argv[1], std::fstream::binary | std::fstream::in);
    if (!outputFile.is_open()) {
        std::cout << "Cannot open the compressed file!!" << std::endl;
        exit(0);
    }
    outputFile.seekg(0, std::ios::end); // reaching to end of file
    uint32_t comp_length = (uint32_t)outputFile.tellg();
    outputFile.seekg(0, std::ios::beg);
    for (int i = 0; i < comp_length; i += MULTIPLE_BYTES) {
        uintS_t x;
        outputFile.read((char*)&x, MULTIPLE_BYTES);
        inStream << x;
    }

    // DECOMPRESSION CALL
    snappyDecompressEngineRun(inStream, outStream, comp_length);

    std::ofstream outFile;
    outFile.open(argv[2], std::fstream::binary | std::fstream::out);
    std::ifstream originalFile;
    originalFile.open(argv[3], std::ofstream::binary | std::ofstream::in);
    if (!originalFile.is_open()) {
        std::cout << "Cannot open the original file!!" << std::endl;
        exit(0);
    }

    bool pass = true;
    uintV_t g;
    uintV_t o = outStream.read();
    ap_uint<MULTIPLE_BYTES> strb = o.range(MULTIPLE_BYTES - 1, 0);
    size_t size = __builtin_popcount(strb.to_uint());

    while (size != 0) {
        ap_uint<MULTIPLE_BYTES* 8> w = o.range((MULTIPLE_BYTES + 1) * 8 - 1, 8);
        // writing output file
        outFile.write((char*)&w, size);

        // Comparing with input file
        g = 0;
        originalFile.read((char*)&g, MULTIPLE_BYTES);
        if (w != g) {
            for (uint8_t v = 0; v < size; v++) {
                uint8_t e = g.range((v + 1) * 8 - 1, v * 8);
                uint8_t r = o.range((v + 1) * 8 - 1, v * 8);
                if (e != r) {
                    pass = false;
                    std::cout << "Expected=" << std::hex << e << " got=" << r << std::endl;
                    std::cout << "-----TEST FAILED: The input file and the file after "
                              << "decompression are not similar!-----" << std::endl;
                    exit(0);
                }
            }
        }

        // reading value from output stream
        o = outStream.read();
        strb = o.range(MULTIPLE_BYTES - 1, 0);
        size = __builtin_popcount(strb.to_uint());
    }
    outFile.close();
    if (pass) {
        std::cout << "TEST PASSED" << std::endl;
    } else {
        std::cout << "TEST FAILED" << std::endl;
    }
    originalFile.close();
    outputFile.close();
}
