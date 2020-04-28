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
 * @file xil_lz4.hpp
 * @brief Header for LZ4 host functionality
 *
 * This file is part of Vitis Data Compression Library host code for lz4 compression.
 */

#ifndef _XFCOMPRESSION_LZ4_HPP_
#define _XFCOMPRESSION_LZ4_HPP_

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
 *  xfLz4 class. Class containing methods for LZ4
 * compression and decompression to be executed on host side.
 */
class xfLz4 {
   public:
    /**
     * @brief Initialize the class object.
     *
     * @param binaryFile file to be read
     */
    int init(const std::string& binaryFile, uint8_t flow, uint32_t block_size_kb);

    /**
     * @brief release
     *
     */
    int release();

    /**
     * @brief This module does the sequential execution of compression
     * where all the I/O operations and kernel execution are done one
     * after another in sequential order.
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     */
    uint64_t compress(uint8_t* in, uint8_t* out, uint64_t actual_size, uint32_t host_buffer_size, bool file_list_flag);

    /**
    * @brief Decompress.
    *
    * @param in input byte sequence
    * @param out output byte sequence
    * @param actual_size input size
    * @param original_size original size
    * @param host_buffer_size host buffer size
    */

    uint64_t decompress(uint8_t* in,
                        uint8_t* out,
                        uint64_t actual_size,
                        uint64_t original_size,
                        uint32_t host_buffer_size,
                        bool file_list_flag);

    /**
     * @brief This module does the memory mapped execution of decompression
     * where the I/O operations and kernel execution is done in sequential order
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param original_size original size
     * @param host_buffer_size host buffer size
     */

    uint64_t decompressFile(
        std::string& inFile_name, std::string& outFile_name, uint64_t actual_size, bool file_list_flag, bool m_flow);

    /**
     * @brief This module is provided to support compress API and
     * it's not recommended to use for high throughput.
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */

    uint64_t compressFile(
        std::string& inFile_name, std::string& outFile_name, uint64_t actual_size, bool file_list_flag, bool m_flow);

    /**
     * @brief Class constructor
     *
     */
    xfLz4();

    /**
     * @brief Class destructor.
     */
    ~xfLz4();

   private:
    /**
     * Block Size
     */
    uint32_t m_BlockSizeInKb;

    /**
     * Binary flow compress/decompress
     */
    bool m_BinFlow;

    /**
     * Switch between FPGA/Standard flows
     */
    bool m_SwitchFlow;

    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q;
    cl::Kernel* compress_kernel_lz4[C_COMPUTE_UNIT];
    cl::Kernel* decompress_kernel_lz4[D_COMPUTE_UNIT];

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
    std::vector<std::string> compress_kernel_names = {"xilLz4Compress"};
    std::vector<std::string> decompress_kernel_names = {"xilLz4Decompress"};
};

} // end namespace compression
} // end namespace xf
#endif // _XFCOMPRESSION_LZ4_HPP_
