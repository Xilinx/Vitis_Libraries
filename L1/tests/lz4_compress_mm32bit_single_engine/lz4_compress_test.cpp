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
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "xxhash.h"

typedef ap_uint<32> data_t;

#include "lz4_compress.hpp"

#define GMEM_DWIDTH 32
#define GMEM_BURST_SIZE 16
/*#define MIN_BLOCK_SIZE 128
#define LZ_MAX_OFFSET_LIMIT 65536
#define MIN_MATCH 4
#define MAX_MATCH_LEN 255
#define OFFSET_WINDOW (64 * 1024)
#define MATCH_LEN 6
#define MAX_LIT_COUNT 4096*/
#define BLOCK_SIZE 64
#define BLOCK_LENGTH (BLOCK_SIZE * 1024)
#define PARALLEL_BLOCK 1
#define CONST_SIZE 2 * 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

const uint32_t c_size = (GMEM_DWIDTH / 8);
const uint32_t c_csize = CONST_SIZE / c_size;

void hls_lz4CompressMM32bitSingleEngine(const data_t* in, data_t* out, uint32_t* compressedSize, uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0 depth = c_csize
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0 depth = c_csize
#pragma HLS INTERFACE m_axi port = compressedSize offset = slave bundle = gmem1 depth = c_csize
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressedSize bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::compression::lz4CompressMM<data_t, GMEM_DWIDTH, GMEM_BURST_SIZE, PARALLEL_BLOCK>(in, out, compressedSize,
                                                                                         input_size);
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
    uint32_t inSizeV = (input_size - 1) / c_size + 1;
    std::cout << "DATA_SIZE: " << input_size << " PARALLEL_BLOCK: " << PARALLEL_BLOCK << std::endl;

    // LZ4 Header
    outFile.put(MAGIC_BYTE_1);
    outFile.put(MAGIC_BYTE_2);
    outFile.put(MAGIC_BYTE_3);
    outFile.put(MAGIC_BYTE_4);
    // FLG & BD bytes
    // --no-frame-crc flow
    // --content-size
    outFile.put(FLG_BYTE);
    outFile.put(BLOCK_SIZE);

    uint8_t temp_buff[10] = {FLG_BYTE,         BLOCK_SIZE,       input_size,       input_size >> 8,  input_size >> 16,
                             input_size >> 24, input_size >> 32, input_size >> 40, input_size >> 48, input_size >> 56};

    // xxhash is used to calculate hash value
    uint32_t xxh = XXH32(temp_buff, 10, 0);
    outFile.write((char*)&temp_buff[2], 8);

    // Header CRC
    outFile.put((uint8_t)(xxh >> 8));

    data_t* source_in = new data_t[CONST_SIZE];
    data_t* source_out = new data_t[CONST_SIZE];
    uint32_t* compressedSize = new uint32_t[CONST_SIZE];
    uint32_t bIdx;
    for (int i = 0; i < CONST_SIZE; i++) {
        source_in[i] = 0;
    }
    for (int i = 0; i < CONST_SIZE; i++) {
        source_out[i] = 0;
    }
    inFile.seekg(0, std::ios::beg);
    int index = 0;

    for (uint64_t i = 0; i < input_size; i += c_size) {
        data_t x = 0;
        inFile.read((char*)&x, c_size);
        source_in[index++] = x;
    }

    hls_lz4CompressMM32bitSingleEngine(source_in, source_out, compressedSize, (uint32_t)input_size);

    uint32_t block_length = BLOCK_LENGTH;
    uint32_t no_blocks = (input_size - 1) / block_length + 1;
    uint32_t oIdx = 0;

    for (uint32_t i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        uint32_t nblocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) {
            nblocks = no_blocks - i;
        }

        for (uint32_t j = 0; j < nblocks; j++) {
            uint32_t outCnt = 0;
            uint32_t sizeVBytes = compressedSize[bIdx++];
            outFile.write((char*)&sizeVBytes, 4);
            uint32_t sizeV = (sizeVBytes - 1) / c_size + 1;
            uint32_t kIdx = oIdx / c_size;
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
            oIdx += BLOCK_LENGTH;
        }
    }

    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.close();
    inFile.close();
}
