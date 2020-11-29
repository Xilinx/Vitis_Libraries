/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
#include "zlib.h"
#include "zlib_compress.hpp"
#include <fcntl.h>  /* For O_RDWR */
#include <unistd.h> /* For open(), creat() */
#include <sys/stat.h>
auto crc = 0; // CRC32 value
#ifdef GZIP_MODE
extern unsigned long crc32(unsigned long crc, const unsigned char* buf, uint32_t len);
#endif

void gzip_headers(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
    const uint16_t c_format_0 = 31;
    const uint16_t c_format_1 = 139;
    const uint16_t c_variant = 8;
    const uint16_t c_real_code = 8;
    const uint16_t c_opcode = 3;

    // 2 bytes of magic header
    outFile.put(c_format_0);
    outFile.put(c_format_1);

    // 1 byte Compression method
    outFile.put(c_variant);

    // 1 byte flags
    uint8_t flags = 0;
    flags |= c_real_code;
    outFile.put(flags);

    // 4 bytes file modification time in unit format
    unsigned long time_stamp = 0;
    struct stat istat;
    stat(inFile_name.c_str(), &istat);
    time_stamp = istat.st_mtime;
    // put_long(time_stamp, outFile);
    uint8_t time_byte = 0;
    time_byte = time_stamp;
    outFile.put(time_byte);
    time_byte = time_stamp >> 8;
    outFile.put(time_byte);
    time_byte = time_stamp >> 16;
    outFile.put(time_byte);
    time_byte = time_stamp >> 24;
    outFile.put(time_byte);

    // 1 byte extra flag (depend on compression method)
    uint8_t deflate_flags = 0;
    outFile.put(deflate_flags);

    // 1 byte OPCODE - 0x03 for Unix
    outFile.put(c_opcode);

    // Dump file name
    for (int i = 0; inFile_name[i] != '\0'; i++) {
        outFile.put(inFile_name[i]);
    }

    outFile.put(0);
    outFile.write((char*)zip_out, enbytes);

    // Last Block
    outFile.put(1);
    outFile.put(0);
    outFile.put(0);
    outFile.put(255);
    outFile.put(255);

    unsigned long ifile_size = istat.st_size;
    uint8_t crc_byte = 0;
    long crc_val = crc;
    crc_byte = crc_val;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 8;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 16;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 24;
    outFile.put(crc_byte);

    uint8_t len_byte = 0;
    len_byte = ifile_size;
    outFile.put(len_byte);
    len_byte = ifile_size >> 8;
    outFile.put(len_byte);
    len_byte = ifile_size >> 16;
    outFile.put(len_byte);
    len_byte = ifile_size >> 24;
    outFile.put(len_byte);

    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

#ifdef GZIP_MODE
void gzip_crc32(uint8_t* in, uint32_t input_size) {
    crc = crc32(crc, in, input_size);
}
#endif

void zlib_headers(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
    outFile.put(120);
    outFile.put(1);
    outFile.write((char*)zip_out, enbytes);
    // Last Block
    outFile.put(1);
    outFile.put(0);
    outFile.put(0);
    outFile.put(255);
    outFile.put(255);

    outFile.put(1);
    outFile.put(0);
    outFile.put(0);
    outFile.put(255);
    outFile.put(255);

    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

uint64_t xfZlib::compressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
    std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size);

    inFile.read((char*)in.data(), input_size);

#ifdef GZIP_MODE
    std::thread gzipCrc(gzip_crc32, in.data(), input_size);
#endif

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    uint64_t enbytes;

#ifdef GZIP_MULTICORE_COMPRESS
    // Full file compress
    enbytes = compressFull(in.data(), out.data(), input_size);
#else
    // Zlib multiple/single cu sequential version
    enbytes = compressSequential(in.data(), out.data(), input_size, host_buffer_size);
#endif
    if (enbytes > 0) {
#ifdef GZIP_MODE
        gzipCrc.join();
        // Pack gzip encoded stream .gz file
        gzip_headers(inFile_name, outFile, out.data(), enbytes);
#else
        // Pack zlib encoded stream .zlib file
        zlib_headers(inFile_name, outFile, out.data(), enbytes);
#endif
    }

    // Close file
    inFile.close();
    outFile.close();
    return enbytes;
}

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

// Constructor
xfZlib::xfZlib(const std::string& binaryFile, uint32_t block_size_kb) {
    h_buf_in.resize(HOST_BUFFER_SIZE);
    h_buf_out.resize(HOST_BUFFER_SIZE);
    h_compressSize.resize(HOST_BUFFER_SIZE);

    // unsigned fileBufSize;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // v++ compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);

    m_program = new cl::Program(*m_context, {device}, bins);
    m_BlockSizeInKb = block_size_kb;
// Create Compress kernels
#ifdef GZIP_MULTICORE_COMPRESS
    compress_kernel_zlib = new cl::Kernel(*m_program, compress_kernel_names[1].c_str());
#else
    compress_kernel_zlib = new cl::Kernel(*m_program, compress_kernel_names[0].c_str());
#endif
}

// Destructor
xfZlib::~xfZlib() {
    delete (compress_kernel_zlib);
    delete (m_program);
    delete (m_q);
    delete (m_context);
}

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xfZlib::compressSequential(uint8_t* in, uint8_t* out, uint64_t input_size, uint32_t host_buffer_size) {
    uint32_t block_size_in_bytes = m_BlockSizeInKb * 1024;
    uint32_t max_num_blks = host_buffer_size / block_size_in_bytes;

    h_buf_in.resize(host_buffer_size);
    h_buf_out.resize(host_buffer_size);
    h_compressSize.resize(max_num_blks);

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // Keeps track of output buffer index
    uint64_t outIdx = 0;

    // Given a input file, we process it as multiple chunks
    // Each compute unit is assigned with a chunk of data
    // In this example HOST_BUFFER_SIZE is the chunk size.
    // For example: Input file = 12 MB
    //              HOST_BUFFER_SIZE = 2MB
    // Each compute unit processes 2MB data per kernel invocation
    uint32_t hostChunk_cu;

    for (uint64_t inIdx = 0; inIdx < input_size; inIdx += host_buffer_size) {
        // Pick buffer size as predefined one
        // If yet to be consumed input is lesser
        // the reset to required size
        uint32_t buf_size = host_buffer_size;

        // This loop traverses through each compute based current inIdx
        // It tries to calculate chunk size and total compute units need to be
        // launched (based on the input_size)
        hostChunk_cu = host_buffer_size;
        // If amount of data to be consumed is less than HOST_BUFFER_SIZE
        // Then choose to send is what is needed instead of full buffer size
        // based on host buffer macro
        if (inIdx + buf_size > input_size) hostChunk_cu = input_size - inIdx;

        max_num_blks = (hostChunk_cu - 1) / block_size_in_bytes + 1;
        // Copy input data from in to host buffer based on the inIdx and cu
        std::memcpy(h_buf_in.data(), &in[inIdx], hostChunk_cu);

        // Device buffer allocation
        buffer_input =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, hostChunk_cu, h_buf_in.data());

        buffer_output =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, hostChunk_cu, h_buf_out.data());

        buffer_compressed_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                sizeof(uint32_t) * max_num_blks, h_compressSize.data());

        // Set kernel arguments
        uint32_t narg = 0;
        compress_kernel_zlib->setArg(narg++, *buffer_input);
        compress_kernel_zlib->setArg(narg++, *buffer_output);
        compress_kernel_zlib->setArg(narg++, *buffer_compressed_size);
        compress_kernel_zlib->setArg(narg++, hostChunk_cu);
        std::vector<cl::Memory> inBufVec;

        inBufVec.push_back(*(buffer_input));

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_q->finish();

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // Fire kernel execution
        m_q->enqueueTask(*compress_kernel_zlib);
        // Wait till kernels complete
        m_q->finish();

        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;

        // Setup output buffer vectors
        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*(buffer_output));
        outBufVec.push_back(*(buffer_compressed_size));

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        for (uint32_t bIdx = 0; bIdx < max_num_blks; bIdx++) {
            uint32_t compressed_size = h_compressSize.data()[bIdx];
            if (compressed_size <= block_size_in_bytes) {
                std::memcpy(&out[outIdx], &h_buf_out[bIdx * block_size_in_bytes], compressed_size);
                outIdx += compressed_size;
            } else {
                uint8_t zeroData = 0x00;
                uint8_t len_low = (uint8_t)block_size_in_bytes;
                uint8_t len_high = (uint8_t)(block_size_in_bytes >> 8);
                uint8_t len_low_n = ~len_low;
                uint8_t len_high_n = ~len_high;
                out[outIdx++] = zeroData;
                out[outIdx++] = len_low;
                out[outIdx++] = len_high;
                out[outIdx++] = len_low_n;
                out[outIdx++] = len_high_n;
                std::memcpy(&out[outIdx], &h_buf_in[bIdx * block_size_in_bytes], block_size_in_bytes);
                outIdx += block_size_in_bytes;
            }
        }

        // Buffer deleted
        delete buffer_input;
        delete buffer_output;
        delete buffer_compressed_size;
    }

    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    return outIdx;
}

// This compress function transfer full input file to DDR before kernel call
// For CR >10 please increase output size else will fail.
uint64_t xfZlib::compressFull(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t outputSize = input_size * 10;

    h_buf_in.resize(input_size);
    h_buf_out.resize(outputSize);
    h_compressSize.resize(sizeof(uint32_t));

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // Copy input data from in to host buffer based on the input_size
    std::memcpy(h_buf_in.data(), in, input_size);

    // Device buffer allocation
    buffer_input = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_size, h_buf_in.data());

    buffer_output = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, outputSize, h_buf_out.data());

    buffer_compressed_size =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(uint32_t), h_compressSize.data());

    // Set kernel arguments
    uint32_t narg = 0;
    compress_kernel_zlib->setArg(narg++, *buffer_input);
    compress_kernel_zlib->setArg(narg++, *buffer_output);
    compress_kernel_zlib->setArg(narg++, *buffer_compressed_size);
    compress_kernel_zlib->setArg(narg++, input_size);

    // Migrate memory - Map host to device buffers
    m_q->enqueueMigrateMemObjects({*(buffer_input), *(buffer_output), *(buffer_compressed_size)},
                                  0 /* 0 means from host*/);
    m_q->finish();

    // Measure kernel execution time
    auto kernel_start = std::chrono::high_resolution_clock::now();

    // Fire kernel execution
    m_q->enqueueTask(*compress_kernel_zlib);
    // Wait till kernels complete
    m_q->finish();

    auto kernel_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
    kernel_time_ns_1 += duration;

    // Migrate memory - Map device to host buffers
    m_q->enqueueMigrateMemObjects({*(buffer_output), *(buffer_compressed_size)}, CL_MIGRATE_MEM_OBJECT_HOST);
    m_q->finish();

    uint32_t compressedSize = h_compressSize[0];
    std::memcpy(out, h_buf_out.data(), compressedSize);

    // Buffer deleted
    delete buffer_input;
    delete buffer_output;
    delete buffer_compressed_size;

    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    return compressedSize;
}
