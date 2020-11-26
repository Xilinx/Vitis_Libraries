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
#include "cmdlineparser.h"

#include "zlib_compress.hpp"

typedef ap_uint<512> data_t;

#define BLOCK_LENGTH 32
#define BLOCK_SIZE (BLOCK_LENGTH * 1024)
#define GMEM_DWIDTH 512
#define PARALLEL_BLOCK 4
#define GMEM_BURST_SIZE 16
#define CONST_SIZE (2 * 1024 * 1024)
#define HOST_CHUNK_SIZE (2 * 1024 * 1024)

constexpr uint32_t c_size = (GMEM_DWIDTH / 8);
constexpr uint32_t c_csize = CONST_SIZE / c_size;

void hls_zlibCompressMultipleEngine(const data_t* in, data_t* out, uint32_t* compressd_size, uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem depth = c_csize
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem depth = c_csize
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem depth = c_csize
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressedSize bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::compression::zlibCompressMultiEngineMM<GMEM_DWIDTH, PARALLEL_BLOCK, GMEM_BURST_SIZE, BLOCK_SIZE>(
        in, out, compressd_size, input_size);
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
    uint32_t input_size = (uint32_t)inFile.tellg();
    std::cout << "DATA_SIZE: " << input_size << std::endl;

    outFile.put(120);
    outFile.put(1);

    data_t* source_in = new data_t[CONST_SIZE];
    data_t* source_out = new data_t[CONST_SIZE];
    uint32_t* compressedSize = new uint32_t[CONST_SIZE];
    for (int i = 0; i < CONST_SIZE; i++) {
        source_in[i] = 0;
        source_out[i] = 0;
        compressedSize[i] = 0;
    }
    inFile.seekg(0, std::ios::beg);
    int index = 0;

    for (uint64_t i = 0; i < input_size; i += c_size) {
        data_t x = 0;
        inFile.read((char*)&x, c_size);
        source_in[index++] = x;
    }

    uint32_t blkCalls = (input_size - 1) / HOST_CHUNK_SIZE + 1;
    uint32_t blkSourceStride = HOST_CHUNK_SIZE / c_size;
    uint32_t blkStride = HOST_CHUNK_SIZE / PARALLEL_BLOCK / c_size;
    uint32_t readSize = 0, idxCntr = 0;

    for (uint32_t i = 0; i < blkCalls; i++) {
        uint32_t pSize = HOST_CHUNK_SIZE;
        if (readSize + pSize > input_size) pSize = input_size - readSize;
        readSize += pSize;

        hls_zlibCompressMultipleEngine(&source_in[blkSourceStride * i], source_out, compressedSize, pSize);

        uint32_t no_blocks = (pSize - 1) / BLOCK_SIZE + 1;
        uint32_t bIdx = 0;

        for (uint32_t j = 0; j < no_blocks; j++) {
            uint32_t outCnt = 0;
            uint32_t sizeVBytes = compressedSize[j];
            if (sizeVBytes <= BLOCK_SIZE) {
                uint32_t sizeV = 0;
                if (sizeVBytes > 0) sizeV = (sizeVBytes - 1) / c_size + 1;
                uint32_t kIdx = j * BLOCK_SIZE / c_size;
                for (uint32_t k = 0; k < sizeV; k++) {
                    data_t o = source_out[kIdx + k];
                    if (outCnt + c_size < sizeVBytes) {
                        outFile.write((char*)&o, c_size);
                        outCnt += c_size;
                    } else {
                        outFile.write((char*)&o, sizeVBytes - outCnt);
                        outCnt += (sizeVBytes - outCnt);
                    }
                }
            } else { // Stored Block Support
                uint32_t bSize = BLOCK_SIZE;
                uint32_t sizeV = BLOCK_SIZE / c_size;
                uint32_t kIdx = idxCntr * sizeV;
                uint8_t len_low = (uint8_t)bSize;
                uint8_t len_high = (uint8_t)(bSize >> 8);
                uint8_t len_low_n = ~len_low;
                uint8_t len_high_n = ~len_high;
                outFile.put(0);
                outFile.put(len_low);
                outFile.put(len_high);
                outFile.put(len_low_n);
                outFile.put(len_high_n);
                for (uint32_t k = 0; k < sizeV; k++) {
                    data_t o = source_in[kIdx + k];
                    outFile.write((char*)&o, c_size);
                }
            }
            idxCntr++;
        }
    }

    // Last Block
    outFile.put(1);
    outFile.put(0);
    outFile.put(0);
    outFile.put(255);
    outFile.put(255);

    outFile.put(1);
    outFile.put(0);
    outFile.put(0);
    outFile.put(255);
    outFile.put(255);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.close();
    inFile.close();
    return 0;
}
