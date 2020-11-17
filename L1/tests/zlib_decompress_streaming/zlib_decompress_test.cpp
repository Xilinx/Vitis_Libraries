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
#include "inflate.hpp"

#define IN_DATAWIDTH 8
#define OUT_DATAWIDTH 16
#define BLOCK_LENGTH 32
#define BLOCK_SIZE (BLOCK_LENGTH * 1024)
#define STRATEGY 1
#define D_LOW_OFFSET 1
#define D_MAX_OFFSET (32 * 1024)
#define D_HISTORY_SIZE D_MAX_OFFSET
#define HUFFMAN_TYPE xf::compression::DYNAMIC

typedef ap_axiu<IN_DATAWIDTH, 0, 0, 0> in_dT;
typedef ap_axiu<OUT_DATAWIDTH, 0, 0, 0> out_dT;
typedef ap_axiu<32, 0, 0, 0> size_dT;

const uint32_t c_size = (OUT_DATAWIDTH / 8);

void zlibDecompressParser(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                          hls::stream<bool>& inStreamEos,
                          hls::stream<ap_uint<IN_DATAWIDTH> >& outStream,
                          hls::stream<bool>& outStreamEos,
                          hls::stream<uint32_t>& zlibBlockSizeStream,
                          hls::stream<uint32_t>& totalSizeStream,
                          hls::stream<ap_uint<32> >& numBlockStream,
                          hls::stream<ap_uint<32> >& outNumBlockStream) {
    for (uint32_t no_blocks = numBlockStream.read(); no_blocks != 0; no_blocks = numBlockStream.read()) {
        outNumBlockStream << no_blocks;
        uint32_t size = 0;
        for (uint32_t i = 0; i < no_blocks; i++) {
            for (bool eos = inStreamEos.read(); eos == false; eos = inStreamEos.read()) {
#pragma HLS PIPELINE II = 1
                outStream << inStream.read();
                outStreamEos << eos;
            }
            uint32_t tempVal = inStream.read();
            size += zlibBlockSizeStream.read();
        }
        outStream << 0;
        outStreamEos << 1;
        totalSizeStream << size;
    }
    outNumBlockStream << 0;
}

// DUT
void hls_zlibDecompressStreaming(hls::stream<out_dT>& inStream,
                                 hls::stream<in_dT>& outStream,
                                 hls::stream<size_dT>& inSizeStream,
                                 hls::stream<size_dT>& outSizeStream) {
#pragma HLS INTERFACE AXIS port = inStream
#pragma HLS INTERFACE AXIS port = inSizeStream
#pragma HLS INTERFACE AXIS port = outStream
#pragma HLS INTERFACE AXIS port = outSizeStream

#pragma HLS DATAFLOW
    constexpr int depthBlockSizeInBytes = BLOCK_SIZE;
    hls::stream<ap_uint<OUT_DATAWIDTH> > inHlsStream("inHlsStream");
    hls::stream<ap_uint<IN_DATAWIDTH> > outCompressedStream("outCompressedStream");
    hls::stream<uint32_t> inZlibSizeStream("inZlibSizeStream");
    hls::stream<uint32_t> outZlibSizeStream("outZlibSizeStream");
    hls::stream<bool> outCompressedStreamEos("outCompressedStreamEos");
#pragma HLS STREAM variable = outCompressedStream depth = depthBlockSizeInBytes
#pragma HLS STREAM variable = outCompressedStreamEos depth = depthBlockSizeInBytes

    // AXI 2 HLS Stream
    xf::compression::details::axiu2hlsStreamSize<OUT_DATAWIDTH>(inStream, inHlsStream, inZlibSizeStream, inSizeStream);

    // Zlib Decompress Stream IO Engine
    xf::compression::details::inflateCoreStream<HUFFMAN_TYPE, D_HISTORY_SIZE, D_LOW_OFFSET>(
        inHlsStream, outCompressedStream, outCompressedStreamEos, outZlibSizeStream, inZlibSizeStream);

    // HLS 2 AXI Stream
    xf::compression::details::hlsStream2axiu<IN_DATAWIDTH>(outCompressedStream, outCompressedStreamEos, outStream,
                                                           outSizeStream, outZlibSizeStream);
}

// Compression Module for Testbench
void hls_zlibCompressStreaming(hls::stream<in_dT>& inStream,
                               hls::stream<out_dT>& outStream,
                               hls::stream<size_dT>& inSizeStream,
                               hls::stream<size_dT>& outSizeStream) {
    xf::compression::zlibCompressStreaming<IN_DATAWIDTH, OUT_DATAWIDTH, BLOCK_SIZE, STRATEGY>(
        inStream, outStream, inSizeStream, outSizeStream);
}

// Testbench
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
    hls::stream<in_dT> outDecStream("outDecStream");
    hls::stream<out_dT> outStream("outStream");
    hls::stream<size_dT> inSizeStream("inSizeStream");
    hls::stream<size_dT> outSizeStream("outSizeStream");
    hls::stream<size_dT> outDecSizeStream("outDecSizeStream");

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

    // Compression Call for creating test data
    hls_zlibCompressStreaming(inStream, outStream, inSizeStream, outSizeStream);

    size_dT sizeEnd;
    sizeEnd.data = 0;
    outSizeStream << sizeEnd;

    // Decompression Call
    hls_zlibDecompressStreaming(outStream, outDecStream, outSizeStream, outDecSizeStream);

    uint32_t byteCounter = 0;
    // 1st file
    for (in_dT val = outDecStream.read(); val.last != true; val = outDecStream.read()) {
        ap_uint<IN_DATAWIDTH> o = val.data;
        byteCounter++;
        outFile.write((char*)&o, 1);
    }
    size_dT axiSizeVBytes = outDecSizeStream.read();
    ap_uint<32> sizeVBytes = axiSizeVBytes.data;

    inFile.close();
    outFile.close();
}
