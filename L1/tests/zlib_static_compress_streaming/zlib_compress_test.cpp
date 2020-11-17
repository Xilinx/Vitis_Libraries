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

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include "cmdlineparser.h"
#include "axi_stream_utils.hpp"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "zlib_compress.hpp"

#define IN_DATAWIDTH 8
#define OUT_DATAWIDTH 16
#define BLOCK_LENGTH 32
#define BLOCK_SIZE (BLOCK_LENGTH * 1024)
#define STRATEGY 1

typedef ap_axiu<IN_DATAWIDTH, 0, 0, 0> in_dT;
typedef ap_axiu<OUT_DATAWIDTH, 0, 0, 0> out_dT;
typedef ap_axiu<32, 0, 0, 0> size_dT;

const uint32_t c_size = (OUT_DATAWIDTH / 8);

void hls_zlibCompressStreaming(hls::stream<in_dT>& inStream,
                               hls::stream<out_dT>& outStream,
                               hls::stream<size_dT>& inSizeStream,
                               hls::stream<size_dT>& outSizeStream) {
#pragma HLS INTERFACE AXIS port = inStream
#pragma HLS INTERFACE AXIS port = inSizeStream
#pragma HLS INTERFACE AXIS port = outStream
#pragma HLS INTERFACE AXIS port = outSizeStream
#pragma HLS INTERFACE ap_ctrl_none port = return

#pragma HLS DATAFLOW
    xf::compression::zlibCompressStaticStreaming<IN_DATAWIDTH, OUT_DATAWIDTH, BLOCK_SIZE, STRATEGY>(
        inStream, outStream, inSizeStream, outSizeStream);
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

    hls::stream<in_dT> inStream("inStream");
    hls::stream<out_dT> outStream("outStream");
    hls::stream<size_dT> inSizeStream("inSizeStream");
    hls::stream<size_dT> outSizeStream("outSizeStream");

    size_dT inSize;
    inFile.seekg(0, std::ios::end); // reaching to end of file
    const uint32_t inFileSize = (uint32_t)inFile.tellg();
    const uint32_t no_blocks = (inFileSize - 1) / BLOCK_SIZE + 1;
    inSize.data = inFileSize;
    inSizeStream << inSize;
    inSize.data = 0;
    inSizeStream << inSize;
    inFile.seekg(0, std::ios::beg);

    // Input 1st File
    for (uint32_t i = 0; i < inFileSize; i++) {
        ap_uint<IN_DATAWIDTH> v;
        inFile.read((char*)&v, 1);
        in_dT inData;
        inData.data = v;
        inStream << inData;
    }

    // Compression Call
    hls_zlibCompressStreaming(inStream, outStream, inSizeStream, outSizeStream);

    uint32_t byteCounter = 0;
    // 1st file
    for (out_dT val = outStream.read(); val.last != true; val = outStream.read()) {
        ap_uint<OUT_DATAWIDTH> o = val.data;
        byteCounter += c_size;
        outFile.write((char*)&o, c_size);
    }
    size_dT axiSizeVBytes = outSizeStream.read();
    ap_uint<32> sizeVBytes = axiSizeVBytes.data;

    inFile.close();
    outFile.close();
}
