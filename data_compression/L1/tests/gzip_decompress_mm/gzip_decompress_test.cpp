/*
 * Copyright 2019-2021 Xilinx, Inc.
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
#include "inflate.hpp"

#define LOW_OFFSET 1
#define MAX_OFFSET (32 * 1024)
#define HISTORY_SIZE MAX_OFFSET

#define HUFFMAN_TYPE xf::compression::FULL

#define OUT_BITWIDTH (MULTIPLE_BYTES * 8)
#define GMEM_DWIDTH (MULTIPLE_BYTES * 8)
#define GMEM_BURST_SIZE 64
#define CONST_SIZE (4 * 1024)
#define LL_MODEL false
constexpr uint32_t strbSize = (OUT_BITWIDTH / 8);
constexpr uint32_t c_size = (GMEM_DWIDTH / 8);
constexpr uint32_t cc_size = CONST_SIZE;

void gzipDecompressMM(const ap_uint<GMEM_DWIDTH>* in,
                      ap_uint<GMEM_DWIDTH>* out,
                      uint32_t* encodedSize,
                      uint32_t inputSize) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem depth = cc_size
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem depth = cc_size
#pragma HLS INTERFACE m_axi port = encodedSize offset = slave bundle = gmem depth = 2
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = encodedSize bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS dataflow
    constexpr int c_decoderType = (int)HUFFMAN_TYPE;

    // Inflate MM Call
    xf::compression::inflateMultiByteMM<GMEM_DWIDTH, GMEM_BURST_SIZE, c_decoderType, MULTIPLE_BYTES,
                                        xf::compression::FileFormat::BOTH, LL_MODEL, HISTORY_SIZE>(in, out, encodedSize,
                                                                                                   inputSize);
}

int main(int argc, char* argv[]) {
    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];

    // File Handling
    std::fstream inFile;
    inFile.open(inputFileName.c_str(), std::fstream::binary | std::fstream::in);
    if (!inFile.is_open()) {
        std::cout << "Cannot open the input file!!" << inputFileName << std::endl;
        exit(0);
    }
    std::ofstream outFile;
    outFile.open(outputFileName.c_str(), std::fstream::binary | std::fstream::out);

    inFile.seekg(0, std::ios::end); // reaching to end of file
    uint64_t input_size = (uint64_t)inFile.tellg();
    ap_uint<GMEM_DWIDTH> source_in[CONST_SIZE];
    ap_uint<GMEM_DWIDTH> source_out[10 * CONST_SIZE];
    uint32_t encodedSize[1];
    inFile.seekg(0, std::ios::beg);
    int index = 0;
    inFile.read(reinterpret_cast<char*>(source_in), input_size);

    // Decompression MM Call
    gzipDecompressMM(source_in, source_out, encodedSize, (uint32_t)input_size);

    uint32_t sizeV = (encodedSize[0] > 0) ? encodedSize[0] : 0;
    uint32_t oIdx = 0, outCnt = 0;

    for (uint32_t k = 0; k < sizeV; k += c_size) {
        ap_uint<GMEM_DWIDTH> o = source_out[oIdx++];
        if (outCnt + c_size < sizeV) {
            outFile.write((char*)&o, c_size);
            outCnt += c_size;
        } else {
            outFile.write((char*)&o, sizeV - outCnt);
            outCnt += (sizeV - outCnt);
        }
    }

    outFile.close();
    inFile.close();
    return 0;
}
