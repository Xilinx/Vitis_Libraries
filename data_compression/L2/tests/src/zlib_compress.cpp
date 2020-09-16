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
#include "zlib_compress.hpp"
#include "xxhash.h"

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

    // Zlib header
    outFile.put(120);
    outFile.put(1);

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    uint64_t enbytes;

    // Zlib multiple/single cu sequential version
    enbytes = compressSequential(in.data(), out.data(), input_size, host_buffer_size);
    // Writing compressed data
    outFile.write((char*)out.data(), enbytes);

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
    compress_kernel_zlib = new cl::Kernel(*m_program, compress_kernel_names[0].c_str());
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
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, hostChunk_cu, h_buf_out.data());

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
