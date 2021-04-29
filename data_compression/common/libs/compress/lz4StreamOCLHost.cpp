/*
 * (c) Copyright 2019-2021 Xilinx, Inc. All rights reserved.
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
#include "lz4StreamOCLHost.hpp"
#include "xxhash.h"

using namespace xf::compression;

// Constructor: Kernel creation
lz4StreamOCLHost::lz4StreamOCLHost(enum State flow,
                                   const std::string& binaryFileName,
                                   uint8_t device_id,
                                   uint32_t block_size_kb,
                                   uint8_t mcr,
                                   bool enable_profile)
    : lz4Base(false),
      m_xclbin(binaryFileName),
      m_enableProfile(enable_profile),
      m_deviceId(device_id),
      m_mcr(mcr),
      m_flow(flow) {
    m_BlockSizeInKb = block_size_kb;

    // unsigned fileBufSize;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[m_deviceId];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // v++ compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(m_xclbin);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

    m_program = new cl::Program(*m_context, {device}, bins);
    if (m_flow == COMPRESS || m_flow == BOTH) {
        // Create Compress kernels
        compress_kernel_lz4 = new cl::Kernel(*m_program, compress_kernel_name.c_str());
        // Create Compress datamover kernels
        compress_data_mover_kernel = new cl::Kernel(*m_program, compress_dm_kernel_name.c_str());
    }
    if (m_flow == DECOMPRESS || m_flow == BOTH) {
        // Create Decompress kernels
        decompress_kernel_lz4 = new cl::Kernel(*m_program, decompress_kernel_name.c_str());
        // Create Decompress datamover kernels
        decompress_data_mover_kernel = new cl::Kernel(*m_program, decompress_dm_kernel_name.c_str());
    }
}

// Destructor
lz4StreamOCLHost::~lz4StreamOCLHost() {
    if (compress_kernel_lz4 != nullptr) {
        delete compress_kernel_lz4;
        compress_kernel_lz4 = nullptr;
    }
    if (compress_data_mover_kernel != nullptr) {
        delete compress_data_mover_kernel;
        compress_data_mover_kernel = nullptr;
    }
    if (decompress_kernel_lz4 != nullptr) {
        delete decompress_kernel_lz4;
        decompress_kernel_lz4 = nullptr;
    }
    if (decompress_data_mover_kernel != nullptr) {
        delete decompress_data_mover_kernel;
        decompress_data_mover_kernel = nullptr;
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);
}

// Compress driving API includes header processing and calling core compress engine
uint64_t lz4StreamOCLHost::xilCompress(uint8_t* in, uint8_t* out, size_t input_size) {
    m_InputSize = input_size;

    // LZ4 header
    out += writeHeader(out);

    uint64_t enbytes;

    // For end to end compression throughput
    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }
    // LZ4 multiple/single cu sequential version
    enbytes = compressEngine(in, out, m_InputSize);

    // E2E compression throughput
    if (m_enableProfile) {
        total_end = std::chrono::high_resolution_clock::now();
        auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
        float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    // lz4 frame formatting
    out = out + enbytes;
    writeFooter(out);
    enbytes += m_frameByteCount;
    return enbytes;
}

// Decompress driving API for calling core compress engine
uint64_t lz4StreamOCLHost::xilDecompress(uint8_t* in, uint8_t* out, size_t input_size) {
    m_InputSize = input_size;
    if (m_ActualSize > m_mcr * input_size) {
        std::cout << "\n" << std::endl;
        std::cout << "\x1B[35mExceeded output buffer size during "
                     "decompression \033[0m \n"
                  << std::endl;
        std::cout << "\x1B[35mUse -mcr option to increase the maximum "
                     "compression ratio (Default: 10) \033[0m \n"
                  << std::endl;
        std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
        delete this;
        exit(EXIT_FAILURE);
    }

    uint64_t debytes;

    // For end to end decompression throughput
    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }

    // Decompression Engine multiple cus.
    debytes = decompressEngine(in, out, m_InputSize);

    // For E2E decompression throughput
    if (m_enableProfile) {
        total_end = std::chrono::high_resolution_clock::now();
        auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
        float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    return debytes;
}

// Core Compress Engine API
uint64_t lz4StreamOCLHost::compressEngine(uint8_t* in, uint8_t* out, size_t input_size) {
    h_buf_in.resize(m_BlockSizeInKb * 1024);
    h_buf_out.resize(m_BlockSizeInKb * 1024);

    h_compressSize.resize(1);

    uint32_t host_buffer_size = m_BlockSizeInKb * 1024;
    uint32_t total_block_count = (input_size - 1) / host_buffer_size + 1;

    uint64_t outIdx = 0;
    buffer_compressed_size =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), h_compressSize.data());
    // device buffer allocation
    buffer_input =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, host_buffer_size, h_buf_in.data());

    buffer_output =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, host_buffer_size, h_buf_out.data());

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // sequentially copy block sized buffers to kernel and wait for them to finish before enqueueing
    for (uint32_t blkIndx = 0, bufIndx = 0; blkIndx < total_block_count; blkIndx++, bufIndx += host_buffer_size) {
        // current block input size
        uint32_t c_input_size = host_buffer_size;
        if (blkIndx == total_block_count - 1) c_input_size = input_size - bufIndx;

        // copy input to input buffer
        std::memcpy(h_buf_in.data(), in + bufIndx, c_input_size);

        // set kernel args
        uint32_t narg = 0;
        compress_data_mover_kernel->setArg(narg++, *buffer_input);
        compress_data_mover_kernel->setArg(narg++, *buffer_output);
        compress_data_mover_kernel->setArg(narg++, *buffer_compressed_size);
        compress_data_mover_kernel->setArg(narg, c_input_size);

        compress_kernel_lz4->setArg(2, c_input_size);
        // Migrate Memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects({*(buffer_input)}, 0);
        m_q->finish();

        // For Kernel compression throughput
        if (!m_enableProfile) {
            kernel_start = std::chrono::high_resolution_clock::now();
        }

        // enqueue the kernels and wait for them to finish
        m_q->enqueueTask(*compress_data_mover_kernel);
        m_q->enqueueTask(*compress_kernel_lz4);
        m_q->finish();

        // For K2K compression throughput
        if (!m_enableProfile) {
            kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;
        }

        // Setup output buffer vectors
        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*buffer_output);
        outBufVec.push_back(*buffer_compressed_size);

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        // read the data to output buffer
        // copy the compressed data to out pointer
        uint32_t compressedSize = h_compressSize.data()[0];

        if (c_input_size > compressedSize) {
            // copy the compressed data
            std::memcpy(out + outIdx, &compressedSize, 4);
            outIdx += 4;
            std::memcpy(out + outIdx, h_buf_out.data(), compressedSize);
            outIdx += compressedSize;
        } else {
            // copy the original data, since no compression
            if (c_input_size == host_buffer_size) {
                out[outIdx++] = 0;
                out[outIdx++] = 0;
                out[outIdx++] = get_bsize(c_input_size);
                out[outIdx++] = NO_COMPRESS_BIT;
            } else {
                uint8_t tmp = c_input_size;
                out[outIdx++] = tmp;
                tmp = c_input_size >> 8;
                out[outIdx++] = tmp;
                tmp = c_input_size >> 16;
                out[outIdx++] = tmp;
                out[outIdx++] = NO_COMPRESS_BIT;
            }
            std::memcpy(out + outIdx, in + (host_buffer_size * blkIndx), c_input_size);
            outIdx += c_input_size;
        }
    }
    // For compression kernel throughput
    if (!m_enableProfile) {
        float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    // Free CL buffers
    delete (buffer_input);
    delete (buffer_output);
    delete (buffer_compressed_size);

    return outIdx;

} // End of compress

// Core Decompress Engine API including kernel header processing
uint64_t lz4StreamOCLHost::decompressEngine(uint8_t* in, uint8_t* out, size_t input_size) {
    std::vector<uint32_t, aligned_allocator<uint32_t> > decompressSize;
    uint32_t outputSize = (input_size * 6) + 16;
    cl::Buffer* bufferOutputSize;
    // Index calculation
    h_buf_in.resize(input_size);
    h_buf_out.resize(outputSize);
    h_buf_decompressSize.resize(sizeof(uint32_t));

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    std::memcpy(h_buf_in.data(), in, input_size);

    // Device buffer allocation
    buffer_input = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_size, h_buf_in.data());
    buffer_output = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, outputSize, h_buf_out.data());
    bufferOutputSize = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(uint32_t),
                                      h_buf_decompressSize.data());

    uint32_t inputSize_32t = uint32_t(input_size);

    // set kernel arguments
    int narg = 0;
    decompress_data_mover_kernel->setArg(narg++, *(buffer_input));
    decompress_data_mover_kernel->setArg(narg++, *(buffer_output));
    decompress_data_mover_kernel->setArg(narg++, inputSize_32t);
    decompress_data_mover_kernel->setArg(narg, *(bufferOutputSize));

    decompress_kernel_lz4->setArg(3, inputSize_32t);

    // Migrate Memory - Map host to device buffers
    m_q->enqueueMigrateMemObjects({*(buffer_input)}, 0);
    m_q->finish();
    if (!m_enableProfile) {
        // Measure kernel execution time
        kernel_start = std::chrono::high_resolution_clock::now();
    }
    // enqueue the kernels and wait for them to finish
    m_q->enqueueTask(*decompress_data_mover_kernel);
    m_q->enqueueTask(*decompress_kernel_lz4);
    m_q->finish();

    if (!m_enableProfile) {
        kernel_end = std::chrono::high_resolution_clock::now();
    }

    // Migrate memory - Map device to host buffers
    m_q->enqueueMigrateMemObjects({*(buffer_output), *(bufferOutputSize)}, CL_MIGRATE_MEM_OBJECT_HOST);
    m_q->finish();

    uint32_t uncompressedSize = h_buf_decompressSize[0];
    std::memcpy(out, h_buf_out.data(), uncompressedSize);

    if (!m_enableProfile) {
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;
        float throughput_in_mbps_1 = (float)uncompressedSize * 1000 / kernel_time_ns_1.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }

    delete buffer_input;
    buffer_input = nullptr;
    delete buffer_output;
    buffer_output = nullptr;
    h_buf_in.clear();
    h_buf_out.clear();

    return uncompressedSize;

} // End of decompress
