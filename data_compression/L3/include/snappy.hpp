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
 * @file xil_snappy.hpp
 * @brief Header for snappy host functionality
 *
 * This file is part of Vitis Data Compression Library host code for snappy compression.
 */

#ifndef _XFCOMPRESSION_XIL_SNAPPY_HPP_
#define _XFCOMPRESSION_XIL_SNAPPY_HPP_

#include <iomanip>
#include "xcl2.hpp"

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
#define HOST_BUFFER_SIZE (2 * 1024 * 1024)

/**
 * Default block size
 */
#ifndef BLOCK_SIZE_IN_KB
#define BLOCK_SIZE_IN_KB 64
#endif

/**
 * Value below is used to associate with
 * Overlapped buffers, ideally overlapped
 * execution requires 2 resources per invocation
 */
#define OVERLAP_BUF_COUNT 2

namespace xf {
namespace compression {

/**
 *  xilSnappy class. Class containing methods for snappy
 * compression and decompression to be executed on host side.
 */
class xilSnappy {
   public:
    /**
     * @brief Initialize the class object.
     *
     * @param binaryFile file to be read
     */
    int init(const std::string& binaryFile);

    /**
     * @brief release
     *
     */
    int release();

    /**
     * @brief This module does the overlapped execution of compression
     * between Kernel and Host. I/O operations between Host and
     * Device are overlapped with kernel execution between multiple
     * compute units.
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     */
    uint64_t compress(uint8_t* in, uint8_t* out, uint64_t actual_size, uint32_t host_buffer_size, bool file_list_flag);

    /**
    * @brief Decompress sequential.
    *
    * @param in input byte sequence
    * @param out output byte sequence
    * @param actual_size input size
    */
    uint64_t decompressSequential(uint8_t* in, uint8_t* out, uint64_t actual_size, bool file_list_flag);

    /**
    * @brief This module is provided to support compress API and
    * it's not recommended to use for high throughput.
    *
    * @param inFile_name input file name
    * @param outFile_name output file name
    * @param actual_size input size
    */
    uint64_t compressFile(std::string& inFile_name,
                          std::string& outFile_name,
                          uint64_t actual_size,
                          bool file_list_flag);

    /**
     * @brief This module is provided to support decompress API and
     * not recommended to use for higher throughput.
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */

    uint64_t decompressFile(std::string& inFile_name,
                            std::string& outFile_name,
                            uint64_t actual_size,
                            bool file_list_flag);

    /**
     * Binary flow compress/decompress
     */
    bool m_bin_flow;

    /**
     * Block Size
     */
    uint32_t m_block_size_in_kb;

    /**
     * Switch between FPGA/Standard flows
     */
    bool m_switch_flow;

    /**
     * @brief Class constructor
     *
     */
    xilSnappy();

    /**
     * @brief Class destructor.
     */
    ~xilSnappy();

   private:
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q;
    cl::Kernel* compress_kernel_snappy[C_COMPUTE_UNIT];
    cl::Kernel* decompress_kernel_snappy[D_COMPUTE_UNIT];

    // Compression related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_in[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_out[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_blksize[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_compressSize[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    // Device buffers
    cl::Buffer* buffer_input[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_output[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_compressed_size[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_block_size[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    // Decompression related
    std::vector<uint32_t> m_blkSize[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t> m_compressSize[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    std::vector<bool> m_is_compressed[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilSnappyCompress"};
    std::vector<std::string> decompress_kernel_names = {"xilSnappyDecompress"};
};

} // end namespace compression
} // end namespace xf

#endif // _XFCOMPRESSION_XIL_SNAPPY_HPP_
