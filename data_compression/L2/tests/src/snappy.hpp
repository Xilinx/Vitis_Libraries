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
/**
 * @file snappy.hpp
 * @brief Header for snappy host functionality
 *
 * This file is part of Vitis Data Compression Library host code for snappy compression.
 */

#ifndef _XFCOMPRESSION_SNAPPY_HPP_
#define _XFCOMPRESSION_SNAPPY_HPP_

#include "defns.hpp"

/**
 * Maximum compute units supported
 */
#if (C_COMPUTE_UNIT > D_COMPUTE_UNIT)
#define MAX_COMPUTE_UNITS C_COMPUTE_UNIT
#else
#define MAX_COMPUTE_UNITS D_COMPUTE_UNIT
#endif

/**
 * Number of parallel compression/decompression blocks
 */
#ifndef PARALLEL_BLOCK
#define PARALLEL_BLOCK 8
#endif

/**
 * Maximum host buffer used to operate per kernel invocation
 */
#define HOST_BUFFER_SIZE (64 * 1024 * 1024)

/**
 * Default block size
 */
#define BLOCK_SIZE_IN_KB 64

// snappy default max cr
#define MAX_CR 10

/**
 * Maximum number of blocks based on host buffer size
 */
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

/**
 * @brief Validate the compressed file.
 *
 * @param inFile_name input file name
 * @param outFile_name output file name
 */
int validate(std::string& inFile_name, std::string& outFile_name);

/**
 *  xilSnappy class. Class containing methods for snappy
 * compression and decompression to be executed on host side.
 */
class xilSnappy {
   public:
    /**
     * @brief Compress sequential.
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     */
    uint64_t compressSequential(uint8_t* in, uint8_t* out, uint64_t actual_size);

    /**
     * @brief Compress the input file.
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */
    uint64_t compressFile(std::string& inFile_name, std::string& outFile_name, uint64_t actual_size, bool m_flow);

    /**
     * @brief Decompress the input file.
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */
    uint64_t decompressFile(std::string& inFile_name, std::string& outFile_name, uint64_t actual_size, bool m_flow);

    /**
     * @brief Decompress sequential.
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     */
    uint64_t decompressSequential(uint8_t* in, uint8_t* out, uint64_t actual_size);

    /**
     * @brief Get the duration of input event
     *
     * @param event event to get duration for
     */
    uint64_t getEventDurationNs(const cl::Event& event);

    /**
     * @brief Class constructor
     *
     */
    xilSnappy(const std::string& binaryFileName, uint8_t flow, uint32_t m_block_kb, uint8_t max_cr = MAX_CR);

    /**
     * @brief Class destructor.
     */
    ~xilSnappy();

   private:
    // Max CR
    uint8_t m_max_cr;

    /**
      * Binary flow compress/decompress
      */
    uint8_t m_BinFlow;

    /**
     * Block Size
     */
    uint32_t m_BlockSizeInKb;
    /**
     * Switch between FPGA/Standard flows
     */
    bool m_SwitchFlow;

    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q;
    cl::Kernel* compress_kernel_snappy;
    cl::Kernel* decompress_kernel_snappy;

    // Compression related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_in;
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_out;
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_blksize;
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_compressSize;

    // Device buffers
    cl::Buffer* buffer_input;
    cl::Buffer* buffer_output;
    cl::Buffer* buffer_compressed_size;
    cl::Buffer* buffer_block_size;

    // Decompression related
    std::vector<uint32_t> m_blkSize;
    std::vector<uint32_t> m_compressSize;

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilSnappyCompress"};
    std::vector<std::string> decompress_kernel_names = {"xilSnappyDecompress"};
};

#endif // _XFCOMPRESSION_SNAPPY_HPP_
