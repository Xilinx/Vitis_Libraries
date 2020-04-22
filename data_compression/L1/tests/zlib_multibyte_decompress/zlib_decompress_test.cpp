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

#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>

#include "inflate.hpp"

#define LOW_OFFSET 1
#define MAX_OFFSET (32 * 1024)
#define HISTORY_SIZE MAX_OFFSET

#define HUFFMAN_TYPE xf::compression::DYNAMIC

#define IN_BITWIDTH 16
#define OUT_BITWIDTH (MULTIPLE_BYTES * 8)
const uint32_t sizeof_in = (IN_BITWIDTH / 8);
const uint32_t sizeof_out = (OUT_BITWIDTH / 8);

typedef ap_uint<IN_BITWIDTH> in_t;
typedef ap_uint<OUT_BITWIDTH> out_t;

void zlibMultiByteDecompressEngineRun(hls::stream<in_t>& inStream,
                                      hls::stream<out_t>& outStream,
                                      hls::stream<bool>& outStreamEoS,
                                      hls::stream<uint64_t>& outSizeStream,
                                      const uint32_t input_size)

{
    const int c_decoderType = (int)HUFFMAN_TYPE;

    xf::compression::details::inflateMultiByteCore<c_decoderType, MULTIPLE_BYTES>(inStream, outStream, outStreamEoS,
                                                                                  outSizeStream, input_size);
}

int main(int argc, char* argv[]) {
    hls::stream<in_t> inStream("inStream");
    hls::stream<out_t> outStream("decompressOut");
    hls::stream<bool> outStreamEoS("decompressOut");
    hls::stream<uint64_t> outStreamSize("decompressOut");
    uint32_t input_size;
    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];
    std::string goldenFileName = argv[3];

    std::fstream inFile;
    inFile.open(inputFileName.c_str(), std::fstream::binary | std::fstream::in);
    if (!inFile.is_open()) {
        std::cout << "Cannot open the compressed file!!" << inputFileName << std::endl;
        exit(0);
    }
    inFile.seekg(0, std::ios::end); // reaching to end of file
    uint32_t comp_length = (uint32_t)inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    for (int i = 0; i < comp_length; i += sizeof_in) {
        in_t x;
        inFile.read((char*)&x, sizeof_in);
        inStream << x;
    }

    // DECOMPRESSION CALL
    zlibMultiByteDecompressEngineRun(inStream, outStream, outStreamEoS, outStreamSize, comp_length);

    std::ofstream outFile;
    outFile.open(outputFileName.c_str(), std::fstream::binary | std::fstream::out);
    std::ifstream originalFile;
    originalFile.open(goldenFileName.c_str(), std::ofstream::binary | std::ofstream::in);
    if (!originalFile.is_open()) {
        std::cout << "Cannot open the original file " << goldenFileName << std::endl;
        exit(0);
    }
    bool pass = true;
    uint64_t outSize = outStreamSize.read();
    std::cout << "Uncompressed size =" << outSize << std::endl;
    uint64_t outCnt = 0;
    out_t g;
    for (bool outEoS = outStreamEoS.read(); outEoS == 0; outEoS = outStreamEoS.read()) {
        // reading value from output stream
        out_t o = outStream.read();

        // writing output file
        if (outCnt + sizeof_out < outSize) {
            outFile.write((char*)&o, sizeof_out);
            outCnt += sizeof_out;
        } else {
            outFile.write((char*)&o, outSize - outCnt);
            outCnt = outSize;
        }

        // Comparing with input file
        g = 0;
        originalFile.read((char*)&g, sizeof_out);
        if (o != g) {
            uint8_t range = ((outSize - outCnt) > sizeof_out) ? sizeof_out : (outSize - outCnt);
            for (uint8_t v = 0; v < range; v++) {
                uint8_t e = g.range((v + 1) * 8 - 1, v * 8);
                uint8_t r = o.range((v + 1) * 8 - 1, v * 8);
                if (e != r) {
                    pass = false;
                    std::cout << "Expected=" << std::hex << e << " got=" << r << std::endl;
                    std::cout << "-----TEST FAILED: The input file and the file after "
                              << "decompression are not similar!-----" << std::endl;
                }
            }
        }
    }
    out_t o = outStream.read();
    outFile.close();
    if (pass) {
        std::cout << "TEST PASSED" << std::endl;
    } else {
        std::cout << "TEST FAILED" << std::endl;
    }
    originalFile.close();
    inFile.close();
}
