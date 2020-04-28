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
#ifndef _XFCOMPRESSION_LZ4_P2P_COMP_HPP_
#define _XFCOMPRESSION_LZ4_P2P_COMP_HPP_

#pragma once
#include "defns.h"

// Maximum compute units supported
#define MAX_COMPUTE_UNITS 2

// Maximum host buffer used to operate
// per kernel invocation
#define HOST_BUFFER_SIZE (100 * 1024 * 1024)

// Default block size
#define BLOCK_SIZE_IN_KB 64

// Value below is used to associate with
// Overlapped buffers, ideally overlapped
// execution requires 2 resources per invocation
#define OVERLAP_BUF_COUNT 1

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

// Below are the codes as per LZ4 standard for
// various maximum block sizes supported.
#define BSIZE_STD_64KB 64
#define BSIZE_STD_256KB 80
#define BSIZE_STD_1024KB 96
#define BSIZE_STD_4096KB 112

// Maximum block sizes supported by LZ4
#define MAX_BSIZE_64KB 65536
#define MAX_BSIZE_256KB 262144
#define MAX_BSIZE_1024KB 1048576
#define MAX_BSIZE_4096KB 4194304

// This value is used to set
// uncompressed block size value
// 4th byte is always set to below
// and placed as uncompressed byte
#define NO_COMPRESS_BIT 128

// In case of uncompressed block
// Values below are used to set
// 3rd byte to following values
// w.r.t various maximum block sizes
// supported by standard
#define BSIZE_NCOMP_64 1
#define BSIZE_NCOMP_256 4
#define BSIZE_NCOMP_1024 16
#define BSIZE_NCOMP_4096 64

int validate(std::string& inFile_name, std::string& outFile_name);

class xflz4 {
   public:
    void compress_in_line_multiple_files(std::vector<char*>& inVec,
                                         const std::vector<std::string>& outFileVec,
                                         std::vector<uint32_t>& inSizeVec,
                                         bool enable_p2p);
    static uint32_t get_file_size(std::ifstream& file) {
        file.seekg(0, file.end);
        uint32_t file_size = file.tellg();
        file.seekg(0, file.beg);
        return file_size;
    }

    xflz4(const std::string& binaryFile, uint8_t device_id, uint32_t m_block_kb);
    ~xflz4();

   private:
    // Block Size
    uint32_t m_BlockSizeInKb;
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q;
    uint64_t get_event_duration_ns(const cl::Event& event);
    size_t create_header(uint8_t* h_header, uint32_t inSize);

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz4Compress"};

    std::vector<std::string> packer_kernel_names = {"xilLz4Packer"};
};
#endif // _XFCOMPRESSION_LZ4_P2P_COMP_HPP_
