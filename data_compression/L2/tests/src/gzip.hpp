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
#include <sys/stat.h>
#include "xcl2.hpp"
#include "zlib_config.hpp"

#define PARALLEL_ENGINES 8
#define C_COMPUTE_UNIT 1
#define D_COMPUTE_UNIT 1
#define H_COMPUTE_UNIT 1
#define T_COMPUTE_UNIT 1
#define MAX_CCOMP_UNITS C_COMPUTE_UNIT
#define MAX_DDCOMP_UNITS D_COMPUTE_UNIT

// Default block size
#define BLOCK_SIZE_IN_KB 1024

// Maximum host buffer used to operate
// per kernel invocation
#define HOST_BUFFER_SIZE (PARALLEL_ENGINES * BLOCK_SIZE_IN_KB * 1024)

// Value below is used to associate with
// Overlapped buffers, ideally overlapped
// execution requires 2 resources per invocation
#define OVERLAP_BUF_COUNT 2

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

int validate(std::string& inFile_name, std::string& outFile_name);

uint32_t get_file_size(std::ifstream& file);

class xil_gzip {
   public:
    int init(const std::string& binaryFile);
    int release();
    uint32_t compress(uint8_t* in, uint8_t* out, uint32_t actual_size, uint32_t host_buffer_size);
    uint32_t decompress(uint8_t* in, uint8_t* out, uint32_t actual_size, int cu_run);
    uint32_t compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);
    uint32_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu_run);
    uint64_t get_event_duration_ns(const cl::Event& event);

    xil_gzip(const std::string& binaryFile);
    ~xil_gzip();

   private:
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q[C_COMPUTE_UNIT * OVERLAP_BUF_COUNT];
    cl::CommandQueue* m_q_dec[D_COMPUTE_UNIT];

    // Kernel declaration
    cl::Kernel* compress_kernel[C_COMPUTE_UNIT];
    cl::Kernel* huffman_kernel[H_COMPUTE_UNIT];
    cl::Kernel* treegen_kernel[T_COMPUTE_UNIT];
    cl::Kernel* decompress_kernel[D_COMPUTE_UNIT];

    // Compression related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_in[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_out[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_gzipout[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_blksize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_compressSize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Decompression Related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_in[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_gzipout[MAX_DDCOMP_UNITS];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressSize[MAX_DDCOMP_UNITS];

    // Buffers related to Dynamic Huffman

    // Literal & length frequency tree
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Distance frequency tree
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Bit Length frequency
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Literal Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Distance Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Bit Length Codes
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Literal Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_ltree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Distance Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_dtree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    // Bit Length Bitlength
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dyn_bltree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    std::vector<uint32_t, aligned_allocator<uint32_t> > h_buff_max_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Device buffers
    cl::Buffer* buffer_input[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_lz77_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_gzip_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_compress_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_inblk_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_dyn_ltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_dtree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_bltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_dyn_ltree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_dtree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_bltree_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_dyn_ltree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_dtree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_bltree_blen[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_max_codes[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Decompress Device Buffers
    cl::Buffer* buffer_dec_input[MAX_DDCOMP_UNITS];
    cl::Buffer* buffer_dec_gzip_output[MAX_DDCOMP_UNITS];
    cl::Buffer* buffer_dec_compress_size[MAX_DDCOMP_UNITS];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz77Compress"};
    std::vector<std::string> huffman_kernel_names = {"xilHuffmanKernel"};
    std::vector<std::string> treegen_kernel_names = {"xilTreegenKernel"};
    std::vector<std::string> decompress_kernel_names = {"xilDecompressZlib"};
};
