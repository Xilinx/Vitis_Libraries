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

#include "zlib_specs.hpp"
#include "lz_optional.hpp"
#include "lz_compress.hpp"
#include "huffman_treegen.hpp"
#include "huffman_encoder.hpp"
#include "inflate.hpp"

#define HUFFMAN_TYPE xf::compression::DYNAMIC

#define D_LOW_OFFSET 1
#define D_MAX_OFFSET (32 * 1024)
#define D_HISTORY_SIZE D_MAX_OFFSET

#define IN_BITWIDTH 8
#define OUT_BITWIDTH 16

#define LTREE_SIZE 286
#define DTREE_SIZE 30
#define BLTREE_SIZE 19
#define EXTRA_BLCODES 32

#define MAX_BITS 15
#define LEAST_VAL 1

#define MIN_BLOCK_SIZE 128
#define LZ_MAX_OFFSET_LIMIT 32768
#define MAX_MATCH_LEN 255
#define MATCH_LEN 6
#define MATCH_LEVEL 6
#define DICT_ELE_WIDTH (MATCH_LEN * 8 + 24)
#define OUT_BYTES (4)
#define MIN_MATCH 3

const uint32_t sizeof_in = (IN_BITWIDTH / 8);
const uint32_t sizeof_out = (OUT_BITWIDTH / 8);

typedef xf::compression::Frequency Frequency;
typedef xf::compression::Codeword Codeword;

void lz77Compress(hls::stream<ap_uint<8> >& inStream,
                  hls::stream<ap_uint<32> >& lz77Out,
                  hls::stream<bool>& lz77Out_eos,
                  hls::stream<uint32_t>& outStreamTree,
                  hls::stream<uint32_t>& compressedSize,
                  uint32_t input_size) {
    hls::stream<ap_uint<32> > compressdStream("compressdStream");
    hls::stream<ap_uint<32> > boosterStream("boosterStream");

#pragma HLS STREAM variable = compressdStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16

#pragma HLS BIND_STORAGE variable = compressdStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = boosterStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::lzCompress<MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(inStream, compressdStream, input_size);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressdStream, boosterStream, input_size);
    xf::compression::lz77Divide(boosterStream, lz77Out, lz77Out_eos, outStreamTree, compressedSize, input_size);
}

void treegenMM(hls::stream<uint32_t>& lz77OutStreamTree,
               hls::stream<uint16_t>& CodeStream,
               hls::stream<uint8_t>& CodeSize) {
    // Internal buffers
    uint32_t ltree_freq[LTREE_SIZE];
    uint32_t dtree_freq[DTREE_SIZE];
    // prepare treegen input
    for (uint32_t i = 0; i < LTREE_SIZE; ++i) ltree_freq[i] = lz77OutStreamTree.read();
    for (uint32_t i = 0; i < DTREE_SIZE; ++i) dtree_freq[i] = lz77OutStreamTree.read();

    xf::compression::zlibTreegenInMMOutStream(ltree_freq, dtree_freq, CodeStream, CodeSize);
}

void huffEncPreProcess(hls::stream<ap_uint<32> >& lz77OutStream,
                       hls::stream<bool>& lz77OutStreamEos,
                       hls::stream<uint32_t>& lz77CompressedSize,
                       hls::stream<ap_uint<32> >& encodedStream,
                       uint32_t& cmp_size) {
    // prepare input for huffman encoder
    uint16_t cnt = 0;
    for (bool eos = lz77OutStreamEos.read(); eos != true; eos = lz77OutStreamEos.read()) {
        encodedStream << lz77OutStream.read();
        cnt += 4;
    }
    ap_uint<32> tmp = lz77OutStream.read();
    cmp_size = lz77CompressedSize.read(); // lz77 compressed size
}

void zlibHuffmanEncoder(hls::stream<ap_uint<32> >& inStream,
                        hls::stream<ap_uint<16> >& huffOut,
                        hls::stream<uint16_t>& huffOutSize,
                        const uint32_t input_size,
                        hls::stream<uint16_t>& StreamCode,
                        hls::stream<uint8_t>& StreamSize) {
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = bitVals depth = 32
#pragma HLS STREAM variable = bitLen depth = 32
#pragma HLS BIND_STORAGE variable = bitVals type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = bitLen type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::huffmanEncoder(inStream, bitVals, bitLen, input_size, StreamCode, StreamSize);

    xf::compression::details::bitPackingSize(bitVals, bitLen, huffOut, huffOutSize);
}

void getHuffOutSize(hls::stream<uint16_t>& huffOutSize, hls::stream<uint32_t>& outSizeStream) {
    // get the compressed output data
    uint32_t outsize = 0;
    for (auto size = huffOutSize.read(); size != 0; size = huffOutSize.read()) {
        outsize += size;
    }
    outSizeStream << outsize;
}

void zlibCompress(hls::stream<ap_uint<8> >& inStream,
                  hls::stream<ap_uint<16> >& outStream,
                  hls::stream<uint32_t>& outSizeStream,
                  uint32_t input_size) {
    // internal streams
    hls::stream<ap_uint<32> > lz77OutStream("lz77OutStream");
    hls::stream<bool> lz77OutStreamEos("lz77OutStreamEos");
    hls::stream<uint32_t> lz77OutStreamTree("lz77OutStreamTree");
    hls::stream<uint32_t> lz77CompressedSize("lz77CompressedSize");

    hls::stream<uint16_t> StreamCode("maxCodeStream");
    hls::stream<uint8_t> StreamSize("maxCodeSize");
    hls::stream<ap_uint<32> > encodedStream("encodedStream");

    hls::stream<uint16_t> huffOutSize("huffOutSize");
    uint32_t lz77CmpSize;
    // call lz77 compression (top function)
    lz77Compress(inStream, lz77OutStream, lz77OutStreamEos, lz77OutStreamTree, lz77CompressedSize, input_size);

    // call memory mapped treegen wrapper
    treegenMM(lz77OutStreamTree, StreamCode, StreamSize);

    // prepare input for huffman encoder
    huffEncPreProcess(lz77OutStream, lz77OutStreamEos, lz77CompressedSize, encodedStream, lz77CmpSize);
    // call huffman encoder
    zlibHuffmanEncoder(encodedStream, outStream, huffOutSize, lz77CmpSize, StreamCode, StreamSize);
    // get compressed size from huffman encoder output
    getHuffOutSize(huffOutSize, outSizeStream);
}

void zlibDecompress(hls::stream<ap_uint<16> >& inStream,
                    hls::stream<ap_uint<8> >& outStream,
                    hls::stream<bool>& outStreamEoS,
                    hls::stream<uint64_t>& outSizeStream,
                    uint32_t input_size) {
    const int c_decoderType = (int)HUFFMAN_TYPE;

    xf::compression::details::inflateCore<c_decoderType, D_HISTORY_SIZE, D_LOW_OFFSET>(
        inStream, outStream, outStreamEoS, outSizeStream, input_size);
    // read extra data
    while (!inStream.empty()) {
        inStream.read();
    }
}

void validateFile(std::string& fileName) {
    hls::stream<ap_uint<8> > inStream("inStream");
    hls::stream<ap_uint<16> > outStream("compressedOutStream");
    hls::stream<uint32_t> cmpSizeStream("compressedSizeStream");
    hls::stream<ap_uint<8> > dcmpOutStream("decompressedOutStream");
    hls::stream<bool> dcmpOutStreamEoS("decompressOutEosStream");
    hls::stream<uint64_t> dcmpOutStreamSize("decompressOutSizeStream");

    uint64_t inFileSize = 0;
    uint64_t dcmpFileSize = 0;
    uint32_t cmpSize = 0;

    // open input file and get file size
    std::ifstream inFile(fileName.c_str(), std::ifstream::binary);
    if (!inFile.is_open()) {
        std::cout << "Cannot open the input file!!" << fileName << std::endl;
        return;
    }
    inFile.seekg(0, std::ios::end); // reaching to end of file
    inFileSize = (uint64_t)inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    for (uint64_t i = 0; i < inFileSize; i += sizeof_in) {
        ap_uint<8> v;
        inFile.read((char*)&v, sizeof_in);
        inStream << v;
    }
    // generate header
    outStream << (1 << 16) + 120;
    // Compression Call
    zlibCompress(inStream, outStream, cmpSizeStream, inFileSize);
    cmpSize = cmpSizeStream.read(); // get compressed output size

    // zlib special block based on Z_SYNC_FLUSH
    outStream << (0x00 << 16) + 0x01;
    outStream << (0xff << 16) + 0x00;
    outStream << (0x00 << 16) + 0xff;
    // write end characters, may be required
    // outStream << 0;
    // outStream << 0;
    cmpSize += 8;

    std::cout << "Compressed Size: " << cmpSize << std::endl;

    // decompress the data in compressed output stream
    zlibDecompress(outStream, dcmpOutStream, dcmpOutStreamEoS, dcmpOutStreamSize, cmpSize);
    dcmpFileSize = dcmpOutStreamSize.read();
    std::cout << "Decompressed size: " << dcmpFileSize << std::endl;

    // verify the decompressed output with the original file
    bool pass = true;
    uint64_t cnt = 1;
    inFile.seekg(0, std::ios::beg);
    for (bool outEoS = dcmpOutStreamEoS.read(); outEoS == 0; outEoS = dcmpOutStreamEoS.read()) {
        ap_uint<8> o_val;
        // read value from original file
        inFile.read((char*)&o_val, sizeof_in);

        // reading value from output stream
        ap_uint<8> d_val = dcmpOutStream.read();

        if (o_val != d_val) {
            pass = false;
            std::cout << "Mismatch at char #" << cnt << std::endl;
            std::cout << "Expected: " << (char)o_val << " Got: " << (char)d_val << std::endl;
        }
        if (!pass) break;
        ++cnt;
    }
    ap_uint<8> d_val = dcmpOutStream.read();
    // check the decompressed output file size
    if (inFileSize != dcmpFileSize) {
        std::cout << "Decompressed file size mismatch!!" << std::endl;
        std::cout << "Original file Size: " << inFileSize << std::endl;
        pass = false;
    }
    if (pass) {
        std::cout << "\nTEST PASSED\n" << std::endl;
    } else {
        std::cout << "TEST FAILED\n" << std::endl;
    }

    inFile.close();
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;

    parser.addSwitch("--file_list", "-l", "List of files", "");
    parser.addSwitch("--original_file", "-o", "Original file path", "");
    parser.addSwitch("--current_path", "-p", "Current data path", "");
    parser.parse(argc, argv);

    std::string listFileName = parser.value("file_list");
    std::string singleInputFileName = parser.value("original_file");
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
            auto origninalFileName = currentPath + "/" + curFileName; // get original file path

            std::cout << "File: " << curFileName << std::endl;
            validateFile(origninalFileName);
        }
    } else {
        std::cout << "File: " << singleInputFileName << std::endl;
        // decompress and validate
        validateFile(singleInputFileName);
    }

    return 0;
}
