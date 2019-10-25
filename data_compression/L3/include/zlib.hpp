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
#ifndef _XFCOMPRESSION_ZLIB_HPP_
#define _XFCOMPRESSION_ZLIB_HPP_

#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <fstream>
#include "xcl2.hpp"

const int gz_max_literal_count = 4096;

// Dynamic Huffman Related Content

// Literals
#define LITERALS 256

// Length codes
#define LENGTH_CODES 29

// Literal Codes
#define LITERAL_CODES (LITERALS + 1 + LENGTH_CODES)

// Distance Codes
#define DISTANCE_CODES 30

// bit length codes
#define BL_CODES 19

// Literal Tree size - 573
#define HEAP_SIZE (2 * LITERAL_CODES + 1)

// Bit length codes must not exceed MAX_BL_BITS bits
#define MAX_BL_BITS 7

#define REUSE_PREV_BLEN 16

#define REUSE_ZERO_BLEN 17

#define REUSE_ZERO_BLEN_7 18

// LTREE, DTREE and BLTREE sizes
#define LTREE_SIZE 1024
#define DTREE_SIZE 64
#define BLTREE_SIZE 64
#define EXTRA_LCODES 32
#define EXTRA_DCODES 32
#define EXTRA_BLCODES 32
#define MAXCODE_SIZE 16

#define PARALLEL_ENGINES 8
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

namespace xf {
namespace compression {

/**
 *  xfZlib class. Class containing methods for Zlib
 * compression and decompression to be executed on host side.
 */
class xfZlib {
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
     * where data transfers and kernel computation are overlapped
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     * @param file_list_flag flag for list of files
     */
    uint64_t compress(uint8_t* in, uint8_t* out, uint64_t actual_size, uint32_t host_buffer_size, bool file_list_flag);

    /**
     * @brief This module does the overlapped execution of compression
     * where data transfers and kernel computation are overlapped
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     */

    uint32_t compress(uint8_t* in, uint8_t* out, uint32_t actual_size, uint32_t host_buffer_size);

    /**
     * @brief This module does serial execution of decompression
     * where data transfers and kernel execution in serial manner
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param cu_run compute unit number
     */

    uint32_t decompress(uint8_t* in, uint8_t* out, uint32_t actual_size, int cu_run);

    /**
     * @brief In shared library flow this call can be used for compress buffer
     * in overlapped manner. This is used in libz.so created.
     *
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param input_size input size
     */

    int compress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size);

    /**
     * @brief In shared library flow this call can be used for decompress buffer
     * in serial manner. This is used in libz.so created.
     *
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param input_size input size
     */

    int decompress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size);

    /**
     * @brief This module does file operations and invokes compress API which
     * internally does zlib compression on FPGA in overlapped manner
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */

    uint32_t compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);

    /**
     * @brief This module does file operations and invokes decompress API which
     * internally does zlib decompression on FPGA in overlapped manner
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param input_size input size
     * @param cu_run compute unit number
     */

    uint32_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu_run);

    /**
     * @brief This module is used for profiling by using kernel events
     *
     * @param inFile_name kernel events
     */

    uint64_t get_event_duration_ns(const cl::Event& event);

    /**
     * @brief Class constructor
     *
     */
    xfZlib(const std::string& binaryFile);

    /**
     * @brief Class destructor.
     */
    ~xfZlib();

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
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_zlibout[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_blksize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_compressSize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Decompression Related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_in[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_dbuf_zlibout[MAX_DDCOMP_UNITS];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressSize[MAX_DDCOMP_UNITS];


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
    cl::Buffer* buffer_zlib_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
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
    cl::Buffer* buffer_dec_zlib_output[MAX_DDCOMP_UNITS];
    cl::Buffer* buffer_dec_compress_size[MAX_DDCOMP_UNITS];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz77Compress"};
    std::vector<std::string> huffman_kernel_names = {"xilHuffmanKernel"};
    std::vector<std::string> treegen_kernel_names = {"xilTreegenKernel"};
    std::vector<std::string> decompress_kernel_names = {"xilDecompressZlib"};
};
}
}
#endif
