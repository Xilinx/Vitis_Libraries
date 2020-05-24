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
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <thread>
#include "xcl2.hpp"
#include <sys/stat.h>
#include <random>
#include <new>
const int gz_max_literal_count = 4096;

#define PARALLEL_ENGINES 8
#define MAX_CCOMP_UNITS C_COMPUTE_UNIT
#define MAX_DDCOMP_UNITS D_COMPUTE_UNIT

// Default block size
#define BLOCK_SIZE_IN_KB 1024

// Input and output buffer size
#define INPUT_BUFFER_SIZE (8 * MEGA_BYTE)
#define OUTPUT_BUFFER_SIZE (16 * MEGA_BYTE)

// zlib max cr limit
#define MAX_CR 10

// buffer count for data in in decompression data movers
#define DIN_BUFFERCOUNT 2
#define DOUT_BUFFERCOUNT 4

// Maximum host buffer used to operate
// per kernel invocation
#define HOST_BUFFER_SIZE (PARALLEL_ENGINES * BLOCK_SIZE_IN_KB * 1024)

// Value below is used to associate with
// Overlapped buffers, ideally overlapped
// execution requires 2 resources per invocation
#define OVERLAP_BUF_COUNT 2

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

#define DECOMP_OUT_SIZE 170
constexpr auto page_aligned_mem = (1 << 21);

int validate(std::string& inFile_name, std::string& outFile_name);

uint64_t get_file_size(std::ifstream& file);
enum comp_decom_flows { BOTH, COMP_ONLY, DECOMP_ONLY };
enum list_mode { ONLY_COMPRESS, ONLY_DECOMPRESS, COMP_DECOMP };
enum d_type { DYNAMIC = 0, FIXED = 1, FULL = 2 };

constexpr auto c_clOutOfResource = -5;
constexpr auto c_clinvalidbin = -42;
constexpr auto c_clinvalidvalue = -30;

#ifdef VERBOSE
#define ZOCL_CHECK(error, call, eflag, expected)                                    \
    call;                                                                           \
    if (error != CL_SUCCESS) {                                                      \
        if (error == expected) {                                                    \
            std::cout << __FILE__ << ":" << __LINE__ << " OPENCL API --> ";         \
            std::cout << #call;                                                     \
            std::cout << ", RESULT: -->  ";                                         \
            std::cout << error_string(error);                                       \
            std::cout << ", EXPECTED: --> ";                                        \
            std::cout << error_string(expected) << std::endl;                       \
            eflag = error;                                                          \
        } else {                                                                    \
            std::cout << "Unexpected Error \n" << error_string(error) << std::endl; \
            exit(EXIT_FAILURE);                                                     \
        }                                                                           \
    }
#else
#define ZOCL_CHECK(error, call, eflag, expected)                                    \
    call;                                                                           \
    if (error != CL_SUCCESS) {                                                      \
        if (error == expected)                                                      \
            eflag = error;                                                          \
        else {                                                                      \
            std::cout << "Unexpected Error \n" << error_string(error) << std::endl; \
            exit(EXIT_FAILURE);                                                     \
        }                                                                           \
    }
#endif

#ifdef VERBOSE
#define ZOCL_CHECK_2(error, call, eflag, expected_1, expected_2)                    \
    call;                                                                           \
    if (error != CL_SUCCESS) {                                                      \
        if ((error == expected_1)) {                                                \
            std::cout << __FILE__ << ":" << __LINE__ << " OPENCL API --> ";         \
            std::cout << #call;                                                     \
            std::cout << ", RESULT: -->  ";                                         \
            std::cout << error_string(error);                                       \
            std::cout << ", EXPECTED: --> ";                                        \
            std::cout << error_string(expected_1) << std::endl;                     \
            eflag = error;                                                          \
        } else if (error == expected_2) {                                           \
            std::cout << __FILE__ << ":" << __LINE__ << " OPENCL API --> ";         \
            std::cout << #call;                                                     \
            std::cout << ", RESULT: -->  ";                                         \
            std::cout << error_string(error);                                       \
            std::cout << ", EXPECTED: --> ";                                        \
            std::cout << error_string(expected_2) << std::endl;                     \
            eflag = error;                                                          \
        } else {                                                                    \
            std::cout << "Unexpected Error \n" << error_string(error) << std::endl; \
            exit(EXIT_FAILURE);                                                     \
        }                                                                           \
    }
#else
#define ZOCL_CHECK_2(error, call, eflag, expected_1, expected_2)                    \
    call;                                                                           \
    if (error != CL_SUCCESS) {                                                      \
        if ((error == expected_1) || (error == expected_2))                         \
            eflag = error;                                                          \
        else {                                                                      \
            std::cout << "Unexpected Error \n" << error_string(error) << std::endl; \
            exit(EXIT_FAILURE);                                                     \
        }                                                                           \
    }
#endif

#define ERROR_STATUS(call)                                      \
    if (call) {                                                 \
        std::cerr << "\nFailed to create object " << std::endl; \
        exit(EXIT_FAILURE);                                     \
    }

#define DELETE_OBJ(buffer)     \
    if (buffer) delete buffer; \
    buffer = nullptr;

// Aligned allocator for ZLIB
template <typename T>
struct zlib_aligned_allocator {
    using value_type = T;
    T* allocate(std::size_t num) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, page_aligned_mem, num * sizeof(T))) throw std::bad_alloc();

        // madvise is a system call to allocate
        // huge pages. By allocating huge pages
        // for memory allocation improves overall
        // time by reduction in page initialization
        // in next step.
        madvise(ptr, num, MADV_HUGEPAGE);

        T* array = reinterpret_cast<T*>(ptr);

        // Write a value in each virtual memory page to force the
        // materialization in physical memory to avoid paying this price later
        // during the first use of the memory
        for (std::size_t i = 0; i < num; i += (page_aligned_mem)) array[i] = 0;
        return array;
    }

    // Dummy construct which doesnt initialize
    template <typename U>
    void construct(U* ptr) noexcept {
        // Skip the default construction
    }

    void deallocate(T* p, std::size_t num) { free(p); }
};

namespace xf {
namespace compression {

/**
 *  xfZlib class. Class containing methods for Zlib
 * compression and decompression to be executed on host side.
 */
class xfZlib {
   public:
    /**
     * @brief This method does the overlapped execution of compression
     * where data transfers and kernel computation are overlapped
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     */

    uint64_t compress(uint8_t* in, uint8_t* out, uint64_t actual_size, uint32_t host_buffer_size);

    /**
     * @brief This method does serial execution of decompression
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

    uint64_t compress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size);

    /**
     * @brief In shared library flow this call can be used for decompress buffer
     * in serial manner. This is used in libz.so created.
     *
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param input_size input size
     */

    int decompress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size, uint8_t cu_id);

    /**
     * @brief This method does file operations and invokes compress API which
     * internally does zlib compression on FPGA in overlapped manner
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */

    uint64_t compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);

    /**
     * @brief This method  does file operations and invokes decompress API which
     * internally does zlib decompression on FPGA in overlapped manner
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param input_size input size
     * @param cu_run compute unit number
     */

    uint32_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu_run);

    uint64_t get_event_duration_ns(const cl::Event& event);

    /**
     * @brief Constructor responsible for creating various host/device buffers.
     *
     */
    xfZlib(const std::string& binaryFile,
           uint8_t c_max_cr = MAX_CR,
           uint8_t cd_flow = BOTH,
           uint8_t device_id = 0,
           uint8_t profile = 0,
           uint8_t d_type = DYNAMIC);

    /**
     * @brief OpenCL setup initialization
     * @param binaryFile
     *
     */
    int init(const std::string& binaryFile, uint8_t dtype);

    /**
     * @brief OpenCL setup release
     *
     */
    void release();

    /**
     * @brief Release host/device memory
     */
    ~xfZlib();

    /**
     * @brief error_code of a OpenCL call
     */
    int error_code(void);

   private:
    void _enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize, int cu);
    void _enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize, int cu, uint32_t max_outbuf);

    uint8_t m_cdflow;
    bool m_isProfile;
    uint8_t m_deviceid = 0;
    uint8_t m_max_cr = MAX_CR;
    int m_err_code = 0;

    cl::Device m_device;
    cl::Context* m_context = nullptr;
    cl::Program* m_program = nullptr;
    cl::CommandQueue* m_q[C_COMPUTE_UNIT * OVERLAP_BUF_COUNT] = {nullptr};
    cl::CommandQueue* m_q_dec[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_rd[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_rdd[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_wr[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_wrd[D_COMPUTE_UNIT] = {nullptr};

    // Kernel declaration
    cl::Kernel* compress_kernel[C_COMPUTE_UNIT] = {nullptr};
    cl::Kernel* huffman_kernel[H_COMPUTE_UNIT] = {nullptr};
    cl::Kernel* decompress_kernel[D_COMPUTE_UNIT] = {nullptr};
    cl::Kernel* data_writer_kernel[D_COMPUTE_UNIT] = {nullptr};
    cl::Kernel* data_reader_kernel[D_COMPUTE_UNIT] = {nullptr};

    // Compression related
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_buf_in[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_buf_out[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_buf_zlibout[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_blksize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_compressSize[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Decompression Related
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbuf_in[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbuf_zlibout[MAX_DDCOMP_UNITS];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_dcompressSize[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbufstream_in[DIN_BUFFERCOUNT];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbufstream_zlibout[DOUT_BUFFERCOUNT];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_dcompressSize_stream[DOUT_BUFFERCOUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressStatus;

    // Device buffers
    cl::Buffer* buffer_input[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_lz77_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_zlib_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_compress_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_inblk_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_dyn_ltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_dtree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    // Decompress Device Buffers
    cl::Buffer* buffer_dec_input[DIN_BUFFERCOUNT] = {nullptr};
    cl::Buffer* buffer_dec_zlib_output[DOUT_BUFFERCOUNT] = {nullptr};
    cl::Buffer* buffer_dec_compress_size[MAX_DDCOMP_UNITS];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz77Compress"};
    std::vector<std::string> huffman_kernel_names = {"xilHuffmanKernel"};
    std::vector<std::string> stream_decompress_kernel_name = {"xilDecompressStream", "xilDecompressFixed",
                                                              "xilDecompressFull"};
    std::string data_writer_kernel_name = "xilZlibDmWriter";
    std::string data_reader_kernel_name = "xilZlibDmReader";
};
}
}
#endif // _XFCOMPRESSION_ZLIB_HPP_
