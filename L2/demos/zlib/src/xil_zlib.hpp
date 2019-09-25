/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
#pragma once

#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <fstream>
#include "xcl2.hpp"
#include "zlib_config.h"

#define PARALLEL_ENGINES 8
// Default block size
#define BLOCK_SIZE_IN_KB 1024

// Maximum host buffer used to operate
// per kernel invocation
#define HOST_BUFFER_SIZE (PARALLEL_ENGINES * BLOCK_SIZE_IN_KB * 1024)

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

int validate(std::string& inFile_name, std::string& outFile_name);

uint32_t get_file_size(std::ifstream& file);

class xil_zlib {
   public:
    int init(const std::string& binaryFile);
    int release();
    uint32_t compress(uint8_t* in, uint8_t* out, uint32_t actual_size, uint32_t host_buffer_size);
    uint32_t decompress(uint8_t* in, uint8_t* out, uint32_t actual_size, int cu_run);
    uint32_t compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);
    uint32_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu_run);
    uint64_t get_event_duration_ns(const cl::Event& event);

    xil_zlib(const std::string& binaryFile);
    ~xil_zlib();

   private:
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q;
    cl::CommandQueue* m_q_dec;

    // Kernel declaration
    cl::Kernel* compress_kernel;
    cl::Kernel* huffman_kernel;
    cl::Kernel* treegen_kernel;
    cl::Kernel* decompress_kernel;

    // Compression related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_in;
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_out;
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_zlibout;
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_blksize;
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_compressSize;

    // Decompression Related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_in;
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_zlibout;
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressSize;

    // Buffers related to Dynamic Huffman

    // Literal & length frequency tree
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_freq;
    // Distance frequency tree
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_freq;
    // Bit Length frequency
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_freq;

    // Literal Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_codes;
    // Distance Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_codes;
    // Bit Length Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_codes;

    // Literal Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_blen;
    // Distance Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_blen;
    // Bit Length Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_blen;

    std::vector<uint32_t, aligned_allocator<uint32_t> > h_buff_max_codes;

    // Device buffers
    cl::Buffer* buffer_input;
    cl::Buffer* buffer_lz77_output;
    cl::Buffer* buffer_zlib_output;
    cl::Buffer* buffer_compress_size;
    cl::Buffer* buffer_inblk_size;

    cl::Buffer* buffer_dyn_ltree_freq;
    cl::Buffer* buffer_dyn_dtree_freq;
    cl::Buffer* buffer_dyn_bltree_freq;

    cl::Buffer* buffer_dyn_ltree_codes;
    cl::Buffer* buffer_dyn_dtree_codes;
    cl::Buffer* buffer_dyn_bltree_codes;

    cl::Buffer* buffer_dyn_ltree_blen;
    cl::Buffer* buffer_dyn_dtree_blen;
    cl::Buffer* buffer_dyn_bltree_blen;

    cl::Buffer* buffer_max_codes;

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz77Compress"};
    std::vector<std::string> huffman_kernel_names = {"xilHuffmanKernel"};
    std::vector<std::string> treegen_kernel_names = {"xilTreegenKernel"};
    std::vector<std::string> decompress_kernel_names = {"xilDecompressZlib"};
};
