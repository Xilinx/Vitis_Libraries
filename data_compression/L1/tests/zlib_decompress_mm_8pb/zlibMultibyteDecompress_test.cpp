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
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "inflate.hpp"

#define HUFFMAN_TYPE xf::compression::DYNAMIC

#define MULTIPLE_BYTES 8
#define GMEM_DATAWIDTH 512
#define KRNL_IN_DATAWIDTH 16
#define KRNL_OUT_DATAWIDTH (MULTIPLE_BYTES * 8)
#define BURST_SIZE 16
#define CONST_SIZE 1024

const uint32_t c_size = (GMEM_DATAWIDTH / 8);
const uint32_t c_constDepth = CONST_SIZE;
typedef ap_uint<GMEM_DATAWIDTH> data_t;

void hls_zlibMultibyteDecompress(const data_t* in, data_t* out, data_t* outSize, const uint32_t inputSize) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem depth = c_constDepth
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem depth = c_constDepth
#pragma HLS INTERFACE m_axi port = outSize offset = slave bundle = gmem depth = c_constDepth
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = inputSize bundle = control
#pragma HLS INTERFACE s_axilite port = outSize bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    const int c_decoderType = (int)HUFFMAN_TYPE;

    xf::compression::inflateMultiByteMM<c_decoderType, MULTIPLE_BYTES>(in, out, outSize, inputSize);
}

int main(int argc, char* argv[]) {
    uint32_t input_size;
    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];
    std::string goldenFileName = argv[3];

    // File Handling
    std::fstream inFile;
    inFile.open(inputFileName.c_str(), std::fstream::binary | std::fstream::in);
    if (!inFile.is_open()) {
        std::cout << "Cannot open the compressed file!!" << inputFileName << std::endl;
        exit(0);
    }
    std::ofstream outFile;
    outFile.open(outputFileName.c_str(), std::fstream::binary | std::fstream::out);
    std::ifstream originalFile;
    originalFile.open(goldenFileName.c_str(), std::ofstream::binary | std::ofstream::in);
    if (!originalFile.is_open()) {
        std::cout << "Cannot open the original file " << goldenFileName << std::endl;
        exit(0);
    }

    inFile.seekg(0, std::ios::end);       // reaching to end of file
    originalFile.seekg(0, std::ios::end); // reaching to end of file
    uint32_t comp_length = (uint32_t)inFile.tellg();
    uint32_t out_comp_length = (uint32_t)originalFile.tellg();
    uint32_t inSizeV = (comp_length - 1) / c_size + 1;
    std::cout << "INPUT_SIZE: " << inSizeV << std::endl;
    uint32_t outSize = (out_comp_length - 1) / c_size + 1;
    std::cout << "OUTPUT_SIZE: " << outSize << std::endl;
    data_t* source_in = new data_t[CONST_SIZE];
    data_t* source_out = new data_t[CONST_SIZE];
    for (int i = 0; i < CONST_SIZE; i++) {
        source_in[i] = 0;
    }
    for (int i = 0; i < CONST_SIZE; i++) {
        source_out[i] = 0;
    }
    inFile.seekg(0, std::ios::beg);
    originalFile.seekg(0, std::ios::beg);
    int index = 0;

    for (int i = 0; i < comp_length; i += c_size) {
        data_t x = 0;
        inFile.read((char*)&x, c_size);
        source_in[index++] = x;
    }

    data_t outSizeV[1024];

    // DECOMPRESSION CALL
    hls_zlibMultibyteDecompress(source_in, source_out, outSizeV, comp_length);

    uint32_t value = outSizeV[0];
    assert(value == out_comp_length);

    bool pass = true;
    uint32_t outCnt = 0;
    data_t g;
    for (uint32_t i = 0; i < outSize; i++) {
        // reading value from output stream
        data_t o = source_out[i];

        // writing output file
        if (outCnt + c_size < outSize) {
            outFile.write((char*)&o, c_size);
            outCnt += c_size;
        } else {
            outFile.write((char*)&o, outSize - outCnt);
            outCnt = outSize;
        }

        // Comparing with input file
        g = 0;
        originalFile.read((char*)&g, c_size);
        if (o != g) {
            uint8_t range = ((outSize - outCnt) > c_size) ? c_size : (outSize - outCnt);
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
    outFile.close();
    if (pass) {
        std::cout << "TEST PASSED" << std::endl;
    } else {
        std::cout << "TEST FAILED" << std::endl;
    }
    originalFile.close();
    inFile.close();
}
