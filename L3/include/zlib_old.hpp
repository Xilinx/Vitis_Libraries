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
#include <map>
#include <future>
#include <queue>
#include <assert.h>
#ifdef ENABLE_XRM
#include <xrm.h>
#endif
#include <syslog.h>

const int gz_max_literal_count = 4096;

#define PARALLEL_ENGINES 8

#ifndef C_COMPUTE_UNIT
#define C_COMPUTE_UNIT 1
#endif

#ifndef H_COMPUTE_UNIT
#define H_COMPUTE_UNIT 1
#endif

#ifndef D_COMPUTE_UNIT
#define D_COMPUTE_UNIT 1
#endif

#define MAX_CCOMP_UNITS C_COMPUTE_UNIT
#define MAX_DDCOMP_UNITS D_COMPUTE_UNIT

// Default block size
#ifdef USE_SINGLE_KERNEL_ZLIBC
#define BLOCK_SIZE_IN_KB 32
#else
#define BLOCK_SIZE_IN_KB 1024
#endif

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
#ifdef USE_SINGLE_KERNEL_ZLIBC
#define MAX_HOST_BUFFER_SIZE (2 * 1024 * 1024)
#define HOST_BUFFER_SIZE (1024 * 1024)
#else
#define HOST_BUFFER_SIZE (PARALLEL_ENGINES * BLOCK_SIZE_IN_KB * 1024)
#endif

// Value below is used to associate with
// Overlapped buffers, ideally overlapped
// execution requires 2 resources per invocation
#ifdef USE_SINGLE_KERNEL_ZLIBC
#ifndef OVERLAP_BUF_COUNT
#define OVERLAP_BUF_COUNT (2 * (MAX_HOST_BUFFER_SIZE / HOST_BUFFER_SIZE))
#endif
#else
#define OVERLAP_BUF_COUNT 2
#endif

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

#define DECOMP_OUT_SIZE 170

// Zlib method information
constexpr auto DEFLATE_METHOD = 8;

constexpr auto MIN_BLOCK_SIZE = 1024;
constexpr auto page_aligned_mem = (1 << 21);

int validate(std::string& inFile_name, std::string& outFile_name);

uint64_t get_file_size(std::ifstream& file);
enum comp_decom_flows { BOTH, COMP_ONLY, DECOMP_ONLY };
enum list_mode { ONLY_COMPRESS, ONLY_DECOMPRESS, COMP_DECOMP };
enum d_type { DYNAMIC = 0, FIXED = 1, FULL = 2 };
enum design_flow { XILINX_GZIP, XILINX_ZLIB };

constexpr auto c_clOutOfResource = -5;
constexpr auto c_clinvalidbin = -42;
constexpr auto c_clinvalidvalue = -30;
constexpr auto c_clOutOfHostMemory = -6;
constexpr auto c_headermismatch = 404;
constexpr auto c_hardXclbinPath = "/opt/xilinx/apps/zlib/xclbin/u50_gen3x16_xdma_201920_3.xclbin";

// Structure to hold
// libzso - deflate/inflate info
typedef struct {
    uint8_t* in_buf;
    uint8_t* out_buf;
    uint64_t input_size;
    uint64_t output_size;
    int level;
    int strategy;
    int window_bits;
    uint32_t adler32;
    int flush;
    std::string u50_xclbin;
} xlibData;

#if (VERBOSE_LEVEL >= 3)
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
            std::cout << __FILE__ << ":" << __LINE__ << " OPENCL API --> ";         \
            std::cout << #call;                                                     \
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

#if (VERBOSE_LEVEL >= 3)
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
            std::cout << __FILE__ << ":" << __LINE__ << " OPENCL API --> ";         \
            std::cout << #call;                                                     \
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

    void deallocate(T* p) { free(p); }
};

namespace xf {
namespace compression {

void event_cb(cl_event event, cl_int cmd_status, void* data);
class memoryManager;
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
     * @param last_buffer
     * @param host_buffer_size host buffer size
     */

    size_t compress_buffer(uint8_t* in,
                           uint8_t* out,
                           size_t actual_size,
                           bool last_buffer = true,
                           uint32_t host_buffer_size = HOST_BUFFER_SIZE,
                           int cu = 0);

    size_t deflate_buffer(
        uint8_t* in, uint8_t* out, size_t& input_size, bool& last_data, bool last_buffer, std::string cu);

    /**
     * @brief This method does serial execution of decompression
     * where data transfers and kernel execution in serial manner
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param cu_run compute unit number
     */

    size_t decompress(uint8_t* in, uint8_t* out, size_t actual_size, size_t max_outbuf_size, int cu_run = 0);

    /**
     * @brief This API does end to end compression in a single call
     *
     *
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param input_size input size
     */
    // Default values
    // Level    = Fast method
    // Strategy = Default
    // Window   = 32K (Max supported)
    size_t compress(
        uint8_t* in, uint8_t* out, size_t input_size, int cu, int level = 1, int strategy = 0, int window_bits = 15);

    /**
     * @brief This method does file operations and invokes compress API which
     * internally does zlib compression on FPGA in overlapped manner
     *
     * @param inFile_name input file name
     * @param outFile_name output file name
     * @param actual_size input size
     */

    uint64_t compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu_run = 0);

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
           uint8_t d_type = DYNAMIC,
           enum design_flow dflow = XILINX_GZIP);

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

    /**
     * @brief returns check sum value (CRC32/Adler32)
     */
    uint32_t get_checksum(void);

    /**
     * @brief set check sum value (CRC32/Adler32)
     */
    void set_checksum(uint32_t cval);

    /**
     * @brief get compress kernel name
     */
    std::string getCompressKernel(int index);

    /**
     * @brief get decompress kernel name
     */
    std::string getDeCompressKernel(int index);

   private:
    friend void xf::compression::event_cb(cl_event event, cl_int cmd_status, void* data);
    void _enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize, int cu);
    void _enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize, int cu, uint32_t max_outbuf);
    size_t add_header(uint8_t* out, int level, int strategy, int window_bits);
    size_t add_footer(uint8_t* out, size_t compressSize);
    void release_dec_buffers(void);

    enum m_threadStatus { IDLE, RUNNING };
    bool m_isGzip = 0; // Zlib=0, Gzip=1
    uint32_t m_checksum = 0;
    m_threadStatus m_checksumStatus = IDLE;
    std::thread* m_thread_checksum;
    bool m_lastData = false;

    void gzip_headers(std::string& inFile, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes);
    void zlib_headers(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes);

    void generate_checksum(uint8_t* in, size_t input_size);
    uint32_t calculate_crc32(uint8_t* in, size_t input_size);
    uint32_t calculate_adler32(uint8_t* in, size_t input_size);
    std::future<uint32_t> m_future_checksum;
    enum design_flow m_zlibFlow;

    uint8_t m_cdflow;
    bool m_isProfile;
    uint8_t m_deviceid = 0;
    uint8_t m_max_cr = MAX_CR;
    int m_err_code = 0;
    int m_derr_code = false;
    std::string m_infile;
    uint32_t m_kidx;
    xf::compression::memoryManager* m_memoryManager;
    uint8_t m_pending = 0;

    cl::Device m_device;
    cl::Context* m_context = nullptr;
    cl::Program* m_program = nullptr;
    cl::CommandQueue* m_def_q = nullptr;
#ifdef USE_SINGLE_KERNEL_ZLIBC
    cl::CommandQueue* m_q[C_COMPUTE_UNIT] = {nullptr};
#else
    cl::CommandQueue* m_q[C_COMPUTE_UNIT * OVERLAP_BUF_COUNT] = {nullptr};
#endif
    cl::CommandQueue* m_q_dec[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_rd[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_rdd[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_wr[D_COMPUTE_UNIT] = {nullptr};
    cl::CommandQueue* m_q_wrd[D_COMPUTE_UNIT] = {nullptr};

    // Kernel declaration
    cl::Kernel* checksum_kernel = {nullptr};
    cl::Kernel* compress_kernel[C_COMPUTE_UNIT] = {nullptr};
    cl::Kernel* compress_stream_kernel = {nullptr};
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

#ifdef ENABLE_HW_CHECKSUM
    // Checksum related
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_buf_checksum_data;
#endif

    // Decompression Related
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbuf_in[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbuf_zlibout[MAX_DDCOMP_UNITS];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_dcompressSize[MAX_DDCOMP_UNITS];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbufstream_in[DIN_BUFFERCOUNT];
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > h_dbufstream_zlibout[DOUT_BUFFERCOUNT + 1];
    std::vector<uint32_t, zlib_aligned_allocator<uint32_t> > h_dcompressSize_stream[DOUT_BUFFERCOUNT];
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressStatus;

    // Compress Device buffers
    cl::Buffer* buffer_input[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_lz77_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_zlib_output[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_compress_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_inblk_size[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

    cl::Buffer* buffer_dyn_ltree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_dyn_dtree_freq[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

#ifdef ENABLE_HW_CHECKSUM
    // Checksum Device buffers
    cl::Buffer* buffer_checksum_data;
#endif

    // Decompress Device Buffers
    cl::Buffer* buffer_dec_input[DIN_BUFFERCOUNT] = {nullptr};
    cl::Buffer* buffer_dec_zlib_output[DOUT_BUFFERCOUNT + 1] = {nullptr};
    cl::Buffer* buffer_dec_compress_size[MAX_DDCOMP_UNITS];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {"xilLz77Compress", "xilZlibCompressFull"};
    std::vector<std::string> huffman_kernel_names = {"xilHuffmanKernel"};
#ifdef ENABLE_HW_CHECKSUM
    std::vector<std::string> checksum_kernel_names = {"xilChecksum32"};
#endif
    std::vector<std::string> stream_decompress_kernel_name = {"xilDecompressStream", "xilDecompressFixed",
                                                              "xilDecompressFull"};
    std::string data_writer_kernel_name = "xilZlibDmWriter";
    std::string data_reader_kernel_name = "xilZlibDmReader";
    std::map<std::string, int> compressKernelMap;
    std::map<std::string, int> decKernelMap;
    std::map<std::string, int> lz77KernelMap;
    std::map<std::string, int> huffKernelMap;
};

struct buffers {
    uint8_t* h_buf_in;
    uint8_t* h_buf_zlibout;
    uint32_t* h_compressSize;
    cl::Buffer* buffer_input;
    cl::Buffer* buffer_zlib_output;
    cl::Buffer* buffer_compress_size;
    uint32_t input_size;
    uint32_t store_size;
    uint32_t allocated_size;
    cl::Event wr_event;
    cl::Event rd_event;
    cl::Event cmp_event;
    cl::Event chk_wr_event;
    cl::Event chk_event;
    bool finish;
};

class memoryManager {
   public:
    memoryManager(uint8_t max_buffers, cl::Context* context);
    ~memoryManager();
    buffers* createBuffer(size_t size);
    buffers* getBuffer();
    buffers* peekBuffer();
    buffers* getLastBuffer();
    bool isPending();

   private:
    void release();
    std::queue<buffers*> freeBuffers;
    std::queue<buffers*> busyBuffers;
    uint8_t bufCount;
    uint8_t maxBufCount;
    cl::Context* mContext;
    buffers* lastBuffer = NULL;
};
}
}
#endif // _XFCOMPRESSION_ZLIB_HPP_
