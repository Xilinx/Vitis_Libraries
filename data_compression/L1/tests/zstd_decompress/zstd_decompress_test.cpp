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

#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "cmdlineparser.h"

#define ZSTD

#define PARALLEL_BYTES_READ 4
#define ZSTD_BLOCK_SIZE_KB 32
#define WINDOW_SIZE (32 * 1024)

// Window size or LZ History size can be modified as required above 128KB
// Below 128KB it must be equal to block size

#include "zstd_decompress.hpp"

constexpr int getDataPortWidth(int maxVal) {
    return (31 - __builtin_clz(maxVal));
}

uint64_t getFileSize(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void decompressFrame(hls::stream<ap_uint<(8 * PARALLEL_BYTES_READ)> >& inStream,
                     hls::stream<ap_uint<4> >& inStrobe,
                     hls::stream<ap_uint<(8 * PARALLEL_BYTES_READ)> >& outStream,
                     hls::stream<bool>& endOfStream,
                     hls::stream<uint64_t>& outSizeStream) {
    const int c_lmoDWidth = 1 + getDataPortWidth(WINDOW_SIZE);
    xf::compression::zstdDecompressStream<PARALLEL_BYTES_READ, ZSTD_BLOCK_SIZE_KB, WINDOW_SIZE, c_lmoDWidth>(
        inStream, inStrobe, outStream, endOfStream, outSizeStream);
}

void validateFile(std::string& fileName, std::string& cmpFileName, uint8_t max_cr) {
    uint64_t inputSize;
    uint64_t originalSize;
    uint64_t outputSize;
    // original file
    std::ifstream origFile(fileName.c_str(), std::ifstream::binary);
    if (!origFile) {
        std::cout << "Unable to open file " << fileName << std::endl;
        return;
    }
    // compressed file
    std::ifstream inCmpFile(cmpFileName.c_str(), std::ifstream::binary);
    if (!inCmpFile) {
        std::cout << "Unable to open file " << fileName << std::endl;
        return;
    }

    inputSize = getFileSize(inCmpFile);
    originalSize = getFileSize(origFile);
    // decompress the file in a loop frame by frame
    std::string out_file_name = fileName + ".raw";

    hls::stream<ap_uint<(8 * PARALLEL_BYTES_READ)> > inputStream("inputStream");
    hls::stream<ap_uint<4> > inStrobe("inStrobe");
    hls::stream<ap_uint<(8 * PARALLEL_BYTES_READ)> > outputStream("outputStream");
    hls::stream<uint64_t> outSizeStream("outSizeStream");
    hls::stream<bool> endOfStream("endOfStream");

    uint8_t lbWidth = inputSize % PARALLEL_BYTES_READ;
    ap_uint<4> strb = PARALLEL_BYTES_READ;
    // Fill Header to Stream
    for (int i = 0; i < inputSize; i += PARALLEL_BYTES_READ) {
        ap_uint<(8 * PARALLEL_BYTES_READ)> val;
        inCmpFile.read((char*)&val, PARALLEL_BYTES_READ);
        if (inputSize < (i + PARALLEL_BYTES_READ)) strb = lbWidth;
        inStrobe << strb;
        inputStream << val;
    }
    inStrobe << 0;
    inputStream << 0;
    inCmpFile.close();
    decompressFrame(inputStream, inStrobe, outputStream, endOfStream, outSizeStream);

    uint64_t k = 0;
    bool pass = true;
    for (bool last = endOfStream.read(); !last; last = endOfStream.read()) {
        ap_uint<(8 * PARALLEL_BYTES_READ)> tbuf = outputStream.read();
        for (uint8_t i = 0; i < PARALLEL_BYTES_READ && k < originalSize; ++i) {
            uint8_t od = tbuf.range(((i + 1) * 8) - 1, i * 8);
            uint8_t ov;
            origFile.read((char*)&ov, 1);
            if (od != ov) {
                std::cout << "Error at char index: " << k << " !!" << std::endl;
                pass = false;
            }
            k++;
        }
        if (!pass) break;
    }
    origFile.close();

    outputStream.read();
    outputSize = outSizeStream.read();
    printf("\nOriginal Size: %lld, OutputSize: %lld\n", originalSize, outputSize);

    if (pass)
        std::cout << "Test PASSED" << std::endl;
    else
        std::cout << "Test FAILED" << std::endl;
}

int main(int argc, char* argv[]) {
    // parse the decompression
    uint8_t max_cr = 20;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--file", "-f", "File to decompress", "");
    parser.addSwitch("--original_file", "-o", "Original file path", "");
    parser.addSwitch("--max_cr", "-mcr", "Max compression ratio", "");

    parser.parse(argc, argv);

    std::string inCmpFileName = parser.value("file");
    std::string originalFileName = parser.value("original_file");
    std::string max_cr_s = parser.value("max_cr");

    if (inCmpFileName.empty()) {
        std::cerr << "Error: Input file name not specified !!" << std::endl;
        return 0;
    }
    if (!max_cr_s.empty()) {
        max_cr = atoi(max_cr_s.c_str());
    }
    validateFile(originalFileName, inCmpFileName, max_cr);

    return 0;
}
