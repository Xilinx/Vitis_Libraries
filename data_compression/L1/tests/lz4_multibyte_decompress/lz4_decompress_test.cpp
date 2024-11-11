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

#include "lz4_decompress.hpp"
#define PARALLEL_BYTES (8)
#define PARALLEL_OUT_BYTES (8)
#define MAX_OFFSET (64 * 1024)
#define HISTORY_SIZE MAX_OFFSET
#define INPUT_BYTES (8)
#define OUTPUT_BYTES PARALLEL_OUT_BYTES
typedef ap_uint<INPUT_BYTES * 8> uintV_t;
typedef ap_uint<(OUTPUT_BYTES * 8) + OUTPUT_BYTES> uintS_t;

void lz4DecompressEngineRun(hls::stream<ap_uint<INPUT_BYTES * 8> >& inStream,
                            hls::stream<ap_uint<(OUTPUT_BYTES * 8) + OUTPUT_BYTES> >& outStream,
                            const uint32_t input_size)

{
    bool error;
    bool modeBlk = false;
    // xf::compression::lz4DecompressEngine_NinMout<INPUT_BYTES, OUTPUT_BYTES, HISTORY_SIZE>(inStream, outStream,
    // input_size, &error, modeBlk);
    xf::compression::lz4DecompressEngine<INPUT_BYTES, HISTORY_SIZE>(inStream, outStream, input_size);
}

int main(int argc, char* argv[]) {
    hls::stream<uintV_t> inStream("inStream");
    hls::stream<uintS_t> outStream("decompressOut");
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
    for (int i = 0; i < comp_length; i += INPUT_BYTES) {
        uintV_t x;
        outputFile.read((char*)&x, INPUT_BYTES);
        inStream << x;
    }

    // DECOMPRESSION CALL
    lz4DecompressEngineRun(inStream, outStream, comp_length);

    std::ofstream outFile;
    outFile.open(argv[2], std::fstream::binary | std::fstream::out);
    std::ifstream originalFile;
    originalFile.open(argv[3], std::ofstream::binary | std::ofstream::in);
    if (!originalFile.is_open()) {
        std::cout << "Cannot open the original file!!" << std::endl;
        exit(0);
    }
    bool pass = true;
    uintS_t g;
    for (ap_uint<(OUTPUT_BYTES * 8) + OUTPUT_BYTES> val = outStream.read(); val != 0; val = outStream.read()) {
        // reading value from output stream
        ap_uint<OUTPUT_BYTES* 8> o = val.range((OUTPUT_BYTES * 8) + OUTPUT_BYTES - 1, OUTPUT_BYTES);
        ap_uint<OUTPUT_BYTES> strb = val.range(OUTPUT_BYTES - 1, 0);
        size_t size = __builtin_popcount(strb.to_uint());
        // writing output file
        outFile.write((char*)&o, size);

        // Comparing with input file
        g = 0;
        originalFile.read((char*)&g, OUTPUT_BYTES);
        if (o != g) {
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
