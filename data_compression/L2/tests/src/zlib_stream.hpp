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
#ifndef _XFCOMPRESSION_ZLIB_STREAM_HPP_
#define _XFCOMPRESSION_ZLIB_STREAM_HPP_

#pragma once

#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <thread>
#include "xcl2.hpp"

// This extension file is required for stream APIs
#include <CL/cl_ext_xilinx.h>

#define PARALLEL_ENGINES 8

// Default block size
#define BLOCK_SIZE_IN_KB 1024

// Input and output buffer size
#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE (2 * 1024 * 1024)
#endif

#ifndef OUTPUT_BUFFER_SIZE
#define OUTPUT_BUFFER_SIZE (32 * 1024 * 1024)
#endif

// buffer count for data in
#define DIN_BUFFERCOUNT 8
#define DOUT_BUFFERCOUNT 2 // mandatorily 2

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

class xfZlibStream {
   public:
    int init(const std::string& binaryFile);
    int release();
    uint32_t decompress(uint8_t* in, uint8_t* out, uint32_t actual_size);
    uint32_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);
    uint64_t get_event_duration_ns(const cl::Event& event);

    xfZlibStream(const std::string& binaryFile);
    ~xfZlibStream();

   private:
    void _enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize);
    void _enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize);

    cl::Device m_device;
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q_dec;
    cl::CommandQueue* m_q_rd;
    cl::CommandQueue* m_q_rdd;
    cl::CommandQueue* m_q_wr;
    cl::CommandQueue* m_q_wrd;

    // Kernel declaration
    cl::Kernel* decompress_kernel;
    cl::Kernel* data_writer_kernel;
    cl::Kernel* data_reader_kernel;

    // Decompression Related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_in[DIN_BUFFERCOUNT];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_zlibout[DOUT_BUFFERCOUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressSize[DOUT_BUFFERCOUNT];

    // Kernel names
    std::string decompress_kernel_name = "xilDecompressStream";
    std::string data_writer_kernel_name = "xilZlibDmWriter";
    std::string data_reader_kernel_name = "xilZlibDmReader";
};
#endif // _XFCOMPRESSION_ZLIB_STREAM_HPP_
