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
#include "lz4OCLHost.hpp"
#include "xxhash.h"
#include <vector>

// Constructor: Kernel creation
lz4OCLHost::lz4OCLHost(enum State flow,
                       const std::string& binaryFileName,
                       uint8_t device_id,
                       uint32_t block_size_kb,
                       uint8_t mcr,
                       bool enable_profile)
    : lz4Base(true),
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
        compress_kernel_lz4 = new cl::Kernel(*m_program, compress_kernel_names[0].c_str());
    }
    if (m_flow == DECOMPRESS || m_flow == BOTH) {
        // Create Decompress kernels
        decompress_kernel_lz4 = new cl::Kernel(*m_program, decompress_kernel_names[0].c_str());
    }
}

// Destructor
lz4OCLHost::~lz4OCLHost() {
    if (compress_kernel_lz4 != nullptr) {
        delete compress_kernel_lz4;
        compress_kernel_lz4 = nullptr;
    }

    if (decompress_kernel_lz4 != nullptr) {
        delete decompress_kernel_lz4;
        decompress_kernel_lz4 = nullptr;
    }

    delete (m_program);
    delete (m_q);
    delete (m_context);
}

// Driving compress API, includes header processing and calling core compress engine
uint64_t lz4OCLHost::xilCompress(uint8_t* in, uint8_t* out, size_t input_size) {
    m_InputSize = input_size;

    // LZ4 header
    out += writeHeader(out);
    uint64_t enbytes;

    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }
    // LZ4 multiple/single cu sequential version
    enbytes = compressEngine(in, out, m_InputSize);

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

// Driving decompress API, includes header processing and calling core decompress engine
uint64_t lz4OCLHost::xilDecompress(uint8_t* in, uint8_t* out, size_t input_size) {
    in += readHeader(in);
    m_inputSize = input_size - 15;
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

    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }

    // Decompression Engine multiple cus.
    debytes = decompressEngine(in, out, m_inputSize);

    if (m_enableProfile) {
        total_end = std::chrono::high_resolution_clock::now();
        auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
        float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    return debytes;
}

// Core compress engine API
uint64_t lz4OCLHost::compressEngine(uint8_t* in, uint8_t* out, size_t input_size) {
    uint32_t host_buffer_size = m_HostBufferSize;
    uint32_t max_num_blks = (host_buffer_size) / (m_BlockSizeInKb * 1024);

    h_buf_in.resize(host_buffer_size);
    h_buf_out.resize(host_buffer_size);
    h_blksize.resize(max_num_blks);
    h_compressSize.resize(max_num_blks);

    m_compressSize.reserve(max_num_blks);
    m_blkSize.reserve(max_num_blks);

    uint32_t block_size_in_kb = m_BlockSizeInKb;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;

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

    // This buffer contains total number of m_BlockSizeInKb blocks per CU
    // For Example: HOST_BUFFER_SIZE = 2MB/m_BlockSizeInKb = 32block (Block
    // size 64 by default)
    uint32_t total_blocks_cu;

    // This buffer holds exact size of the chunk in bytes for all the CUs
    uint32_t bufSize_in_bytes_cu;

    // Holds value of total compute units to be
    // used per iteration
    uint32_t compute_cu = 0;

    for (uint64_t inIdx = 0; inIdx < input_size; inIdx += host_buffer_size) {
        // Needs to reset this variable
        // As this drives compute unit launch per iteration
        compute_cu = 0;

        // Pick buffer size as predefined one
        // If yet to be consumed input is lesser
        // the reset to required size
        uint32_t buf_size = host_buffer_size;

        // This loop traverses through each compute based current inIdx
        // It tries to calculate chunk size and total compute units need to be
        // launched (based on the input_size)
        hostChunk_cu = 0;
        // If amount of data to be consumed is less than HOST_BUFFER_SIZE
        // Then choose to send is what is needed instead of full buffer size
        // based on host buffer macro
        if (inIdx + (buf_size) > input_size) {
            hostChunk_cu = input_size - (inIdx);
            compute_cu++;
        } else {
            hostChunk_cu = buf_size;
            compute_cu++;
        }
        // Figure out total number of blocks need per each chunk
        // Copy input data from in to host buffer based on the inIdx and cu
        uint32_t nblocks = (hostChunk_cu - 1) / block_size_in_bytes + 1;
        total_blocks_cu = nblocks;
        std::memcpy(h_buf_in.data(), &in[inIdx], hostChunk_cu);

        // Fill the host block size buffer with various block sizes per chunk/cu
        uint32_t bIdx = 0;
        uint32_t chunkSize_curr_cu = hostChunk_cu;

        for (uint32_t bs = 0; bs < chunkSize_curr_cu; bs += block_size_in_bytes) {
            uint32_t block_size = block_size_in_bytes;
            if (bs + block_size > chunkSize_curr_cu) {
                block_size = chunkSize_curr_cu - bs;
            }
            h_blksize.data()[bIdx++] = block_size;
        }

        // Calculate chunks size in bytes for device buffer creation
        bufSize_in_bytes_cu = ((hostChunk_cu - 1) / m_BlockSizeInKb + 1) * m_BlockSizeInKb;

        // Device buffer allocation
        buffer_input =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, bufSize_in_bytes_cu, h_buf_in.data());

        buffer_output =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, bufSize_in_bytes_cu, h_buf_out.data());

        buffer_compressed_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                sizeof(uint32_t) * total_blocks_cu, h_compressSize.data());

        buffer_block_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           sizeof(uint32_t) * total_blocks_cu, h_blksize.data());

        // Set kernel arguments
        uint32_t narg = 0;
        compress_kernel_lz4->setArg(narg++, *(buffer_input));
        compress_kernel_lz4->setArg(narg++, *(buffer_output));
        compress_kernel_lz4->setArg(narg++, *(buffer_compressed_size));
        compress_kernel_lz4->setArg(narg++, *(buffer_block_size));
        compress_kernel_lz4->setArg(narg++, block_size_in_kb);
        compress_kernel_lz4->setArg(narg++, hostChunk_cu);
        std::vector<cl::Memory> inBufVec;

        inBufVec.push_back(*(buffer_input));
        inBufVec.push_back(*(buffer_block_size));

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_q->finish();

        if (!m_enableProfile) {
            // Measure kernel execution time
            kernel_start = std::chrono::high_resolution_clock::now();
        }
        // Fire kernel execution
        m_q->enqueueTask(*compress_kernel_lz4);
        // Wait till kernels complete
        m_q->finish();
        if (!m_enableProfile) {
            kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;
        }
        // Setup output buffer vectors
        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*(buffer_output));
        outBufVec.push_back(*(buffer_compressed_size));

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        // Copy data into out buffer
        // Include compress and block size data
        // Copy data block by block within a chunk example 2MB (64block size) - 32 blocks data
        // Do the same for all the compute units
        writeCompressedBlock<uint8_t>(hostChunk_cu, block_size_in_bytes, h_compressSize.data(), out, in,
                                      h_buf_out.data(), outIdx, inIdx);

        // Buffer deleted
        delete (buffer_input);
        delete (buffer_output);
        delete (buffer_compressed_size);
        delete (buffer_block_size);
    }
    if (!m_enableProfile) {
        float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    return outIdx;
} // End of compress

// Core decompress engine API
uint64_t lz4OCLHost::decompressEngine(uint8_t* in, uint8_t* out, size_t input_size) {
    size_t original_size = m_ActualSize;

    size_t host_buffer_size = m_HostBufferSize;
    uint32_t max_num_blks = (host_buffer_size) / (m_BlockSizeInKb * 1024);
    h_buf_in.resize(host_buffer_size);
    h_buf_out.resize(host_buffer_size);
    h_blksize.resize(max_num_blks);
    h_compressSize.resize(max_num_blks);

    m_compressSize.reserve(max_num_blks);
    m_blkSize.reserve(max_num_blks);

    uint32_t block_size_in_bytes = m_BlockSizeInKb * 1024;

    // Total number of blocks exists for this file
    uint32_t total_block_cnt = (original_size - 1) / block_size_in_bytes + 1;
    uint32_t block_cntr = 0;
    uint32_t done_block_cntr = 0;

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    uint64_t inIdx = 0;
    uint64_t total_decomression_size = 0;

    uint64_t hostChunk_cu;
    uint32_t compute_cu;
    uint64_t output_idx = 0;

    // To handle uncompressed blocks
    bool compressBlk = false;

    for (uint64_t outIdx = 0; outIdx < original_size; outIdx += host_buffer_size) {
        compute_cu = 0;
        uint64_t chunk_size = host_buffer_size;

        // Figure out the chunk size for each compute unit
        hostChunk_cu = 0;
        if (outIdx + (chunk_size) > original_size) {
            hostChunk_cu = original_size - (outIdx);
            compute_cu++;
        } else {
            hostChunk_cu = chunk_size;
            compute_cu++;
        }

        uint32_t nblocks;
        uint32_t bufblocks;
        uint64_t total_size;
        uint64_t buf_size;
        uint32_t block_size = 0;
        uint32_t compressed_size = 0;

        nblocks = 0;
        buf_size = 0;
        bufblocks = 0;
        total_size = 0;
        for (uint64_t cIdx = 0; cIdx < hostChunk_cu; cIdx += block_size_in_bytes, nblocks++, total_size += block_size) {
            if (block_cntr == (total_block_cnt - 1)) {
                block_size = original_size - done_block_cntr * block_size_in_bytes;
            } else {
                block_size = block_size_in_bytes;
            }
            std::memcpy(&compressed_size, &in[inIdx], 4);
            inIdx += 4;

            uint32_t tmp = compressed_size;
            tmp >>= 24;

            if (tmp == 128) {
                uint8_t b1 = compressed_size;
                uint8_t b2 = compressed_size >> 8;
                uint8_t b3 = compressed_size >> 16;
                // uint8_t b4 = compressed_size >> 24;

                if (b3 == 1) {
                    compressed_size = block_size_in_bytes;
                } else {
                    uint16_t size = 0;
                    size = b2;
                    size <<= 8;
                    uint16_t temp = b1;
                    size |= temp;
                    compressed_size = size;
                }
            }

            // Fill original block size and compressed size
            m_blkSize.data()[nblocks] = block_size;
            m_compressSize.data()[nblocks] = compressed_size;
            // If compressed size is less than original block size
            if (compressed_size < block_size) {
                h_compressSize.data()[bufblocks] = compressed_size;
                h_blksize.data()[bufblocks] = block_size;
                std::memcpy(&(h_buf_in.data()[buf_size]), &in[inIdx], compressed_size);
                inIdx += compressed_size;
                buf_size += block_size;
                bufblocks++;
                compressBlk = true;
            } else if (compressed_size == block_size) {
                // No compression block
                std::memcpy(&(out[outIdx + cIdx]), &in[inIdx], block_size);
                inIdx += block_size;
            } else {
                assert(0);
            }
            block_cntr++;
            done_block_cntr++;
        }
        assert(total_size <= original_size);

        if (nblocks == 1 && compressed_size == block_size) break;

        if (compressBlk) {
            // Device buffer allocation
            buffer_input =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, buf_size, h_buf_in.data());

            buffer_output =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, buf_size, h_buf_out.data());

            buffer_block_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                               sizeof(uint32_t) * bufblocks, h_blksize.data());

            buffer_compressed_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    sizeof(uint32_t) * bufblocks, h_compressSize.data());

            // Set kernel arguments
            uint32_t narg = 0;
            decompress_kernel_lz4->setArg(narg++, *(buffer_input));
            decompress_kernel_lz4->setArg(narg++, *(buffer_output));
            decompress_kernel_lz4->setArg(narg++, *(buffer_block_size));
            decompress_kernel_lz4->setArg(narg++, *(buffer_compressed_size));
            decompress_kernel_lz4->setArg(narg++, m_BlockSizeInKb);
            decompress_kernel_lz4->setArg(narg++, bufblocks);

            std::vector<cl::Memory> inBufVec;
            inBufVec.push_back(*(buffer_input));
            inBufVec.push_back(*(buffer_block_size));
            inBufVec.push_back(*(buffer_compressed_size));

            // Migrate memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
            m_q->finish();

            if (!m_enableProfile) {
                // Measure kernel execution time
                kernel_start = std::chrono::high_resolution_clock::now();
            }

            // Kernel invocation
            m_q->enqueueTask(*decompress_kernel_lz4);
            m_q->finish();

            if (!m_enableProfile) {
                kernel_end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
                kernel_time_ns_1 += duration;
            }

            std::vector<cl::Memory> outBufVec;
            outBufVec.push_back(*(buffer_output));

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();
        }
        uint32_t bufIdx = 0;
        for (uint32_t bIdx = 0; bIdx < nblocks; bIdx++) {
            uint32_t block_size = m_blkSize.data()[bIdx];
            uint32_t compressed_size = m_compressSize.data()[bIdx];
            if (compressed_size < block_size) {
                std::memcpy(&out[output_idx], &h_buf_out.data()[bufIdx], block_size);
                output_idx += block_size;
                bufIdx += block_size;
                total_decomression_size += block_size;
            } else if (compressed_size == block_size) {
                output_idx += block_size;
            }
        }

        if (compressBlk) {
            // Delete device buffers
            delete (buffer_input);
            delete (buffer_output);
            delete (buffer_block_size);
            delete (buffer_compressed_size);
        }
    } // Top - Main loop ends here
    if (!m_enableProfile) {
        float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    return original_size;

} // End of decompress
