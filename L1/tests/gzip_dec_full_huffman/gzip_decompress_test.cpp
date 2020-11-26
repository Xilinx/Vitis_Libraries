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
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include "cmdlineparser.h"

#include "inflate.hpp"

#define LOW_OFFSET 1
#define MAX_OFFSET (32 * 1024)
#define HISTORY_SIZE MAX_OFFSET

#define HUFFMAN_TYPE xf::compression::FULL

#define IN_BITWIDTH 16
#define OUT_BITWIDTH 8
const uint32_t sizeof_in = (IN_BITWIDTH / 8);
const uint32_t sizeof_out = (OUT_BITWIDTH / 8);

typedef ap_uint<IN_BITWIDTH> in_t;
typedef ap_uint<OUT_BITWIDTH> out_t;

void gzipDecompressEngineRun(hls::stream<in_t>& inStream,
                             hls::stream<out_t>& outStream,
                             hls::stream<bool>& outStreamEoS,
                             hls::stream<uint64_t>& outSizeStream,
                             const uint32_t input_size)

{
    const int c_decoderType = (int)HUFFMAN_TYPE;

    xf::compression::details::inflateCore<c_decoderType, HISTORY_SIZE, LOW_OFFSET>(inStream, outStream, outStreamEoS,
                                                                                   outSizeStream, input_size);
}

void validateFile(std::string& fileName, std::string& originalFileName) {
    hls::stream<in_t> inStream("inStream");
    hls::stream<out_t> outStream("decompressOut");
    hls::stream<bool> outStreamEoS("decompressOutEos");
    hls::stream<uint64_t> outStreamSize("outStreamSize");

    std::string outputFileName = fileName + ".out";

    std::ifstream inFile(fileName.c_str(), std::ifstream::binary);
    if (!inFile.is_open()) {
        std::cout << "Cannot open the compressed file!!" << fileName << std::endl;
        return;
    }
    inFile.seekg(0, std::ios::end); // reaching to end of file
    uint32_t comp_length = (uint32_t)inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    for (uint32_t i = 0; i < comp_length; i += sizeof_in) {
        in_t x;
        inFile.read((char*)&x, sizeof_in);
        inStream << x;
    }
    inFile.close();

    // DECOMPRESSION CALL
    gzipDecompressEngineRun(inStream, outStream, outStreamEoS, outStreamSize, comp_length);

    std::ofstream outFile;
    outFile.open(outputFileName.c_str(), std::ofstream::binary);

    std::ifstream originalFile;
    originalFile.open(originalFileName.c_str(), std::ifstream::binary);
    if (!originalFile.is_open()) {
        std::cout << "Cannot open the original file " << originalFileName << std::endl;
        return;
    }

    bool pass = true;
    uint64_t outSize = outStreamSize.read();

    std::cout << "Decompressed size: " << outSize << " ";
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
                std::cout << std::hex << e << " : " << r << std::endl;
                if (e != r) {
                    pass = false;
                    std::cout << "Expected=" << std::hex << e << " got=" << r << std::endl;
                    std::cout << "-----TEST FAILED: The input file and the file after "
                              << "decompression are not similar!-----" << std::endl;
                }
                if (!pass) break;
            }
        }
    }
    out_t o = outStream.read();
    outFile.close();
    if (pass) {
        std::cout << "\nTEST PASSED\n" << std::endl;
    } else {
        std::cout << "TEST FAILED\n" << std::endl;
    }
    originalFile.close();
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;

    parser.addSwitch("--file_list", "-l", "List of files", "");
    parser.addSwitch("--compressed_file", "-f", "Compressed gzip file path", "");
    parser.addSwitch("--original_file", "-o", "Original file path", "");
    parser.addSwitch("--current_path", "-p", "Current data path", "");
    parser.parse(argc, argv);

    std::string listFileName = parser.value("file_list");
    std::string singleInputFileName = parser.value("compressed_file");
    std::string singleGoldenFileName = parser.value("original_file");
    std::string currentPath = parser.value("current_path");

    // parse the arguments
    if (!listFileName.empty()) {
        // validate multiple files
        if (currentPath.empty()) {
            std::cout << "Path for data not specified.." << std::endl;
            std::cout << "Expecting absolute paths for files in list file." << std::endl;
        }

        std::ifstream infilelist(listFileName.c_str());
        std::string curFileName;
        // decompress and validate
        while (std::getline(infilelist, curFileName)) {
            auto origninalFileName = currentPath + "/" + curFileName;         // get original file path
            auto dcmpFileName = currentPath + "/" + curFileName + ".full.gz"; // compressed file path

            std::cout << "File: " << curFileName << std::endl;
            validateFile(dcmpFileName, origninalFileName);
        }
    } else {
        std::cout << "File: " << singleInputFileName << std::endl;
        // decompress and validate
        validateFile(singleInputFileName, singleGoldenFileName);
    }

    return 0;
}
