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
#include "snappy.hpp"
#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

uint64_t xilSnappy::compressFile(std::string& inFile_name,
                                 std::string& outFile_name,
                                 uint64_t input_size,
                                 bool m_flow) {
    m_SwitchFlow = m_flow;
    if (m_SwitchFlow == 0) { // Xilinx FPGA compression flow
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size + ((input_size / 65536) * 10));

        inFile.read((char*)in.data(), input_size);

        // Snappy Stream Identfier
        outFile.put(0xff);
        outFile.put(0x06);
        outFile.put(0x00);
        outFile.put(0x00);
        outFile.put(0x73);
        outFile.put(0x4e);
        outFile.put(0x61);
        outFile.put(0x50);
        outFile.put(0x70);
        outFile.put(0x59);

        uint32_t host_buffer_size = HOST_BUFFER_SIZE;
        uint32_t acc_buff_size = m_BlockSizeInKb * 1024 * PARALLEL_BLOCK;
        if (acc_buff_size > host_buffer_size) {
            host_buffer_size = acc_buff_size;
        }
        if (host_buffer_size > input_size) {
            host_buffer_size = input_size;
        }
        if (host_buffer_size > HOST_BUFFER_SIZE) {
            host_buffer_size = HOST_BUFFER_SIZE;
        }

        // Snappy Sequential compress
        uint64_t enbytes = compressSequential(in.data(), out.data(), input_size);

        // Writing compressed data
        outFile.write((char*)out.data(), enbytes);

        // Close file
        inFile.close();
        outFile.close();
        return enbytes;
    } else { // Standard Snappy flow
        // Build Java based snappy source code
        std::string command =
            "java -cp \".:snappy-0.5-SNAPSHOT-bin.jar:commons-io-2.6.jar\" MainClass -c " + inFile_name;
        system(command.c_str());
        return 0;
    }
}

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

uint64_t xilSnappy::decompressFile(std::string& inFile_name,
                                   std::string& outFile_name,
                                   uint64_t input_size,
                                   bool m_flow) {
    m_SwitchFlow = m_flow;
    if (m_SwitchFlow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size * m_max_cr);

        char c = 0;

        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);
        inFile.get(c);

        // Read block data from compressed stream .snappy
        inFile.read((char*)in.data(), (input_size - 10));

        // Decompression Sequential multiple cus.
        uint64_t debytes = decompressSequential(in.data(), out.data(), (input_size - 10));
        outFile.write((char*)out.data(), debytes);

        // Close file
        inFile.close();
        outFile.close();
        return debytes;
    } else {
        // Use standard snappy compress/decompress below
        std::string command =
            "java -cp \".:snappy-0.5-SNAPSHOT-bin.jar:commons-io-2.6.jar\" MainClass -d " + inFile_name;
        system(command.c_str());
        return 0;
    }
}

uint64_t xilSnappy::decompressSequential(uint8_t* in, uint8_t* out, uint64_t input_size) {
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    uint32_t buf_size = BLOCK_SIZE_IN_KB * 1024;
    uint32_t blocksPerChunk = HOST_BUFFER_SIZE / buf_size;
    uint32_t host_buffer_size = ((HOST_BUFFER_SIZE - 1) / BLOCK_SIZE_IN_KB + 1) * BLOCK_SIZE_IN_KB;

    // Allocate global buffers
    // Device buffer allocation
    buffer_input =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, host_buffer_size, h_buf_in.data());

    buffer_output =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, host_buffer_size, h_buf_out.data());

    buffer_block_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       sizeof(uint32_t) * blocksPerChunk, h_blksize.data());

    buffer_compressed_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            sizeof(uint32_t) * blocksPerChunk, h_compressSize.data());

    // Set Kernel Arguments
    uint32_t narg = 0;
    decompress_kernel_snappy->setArg(narg++, *(buffer_input));
    decompress_kernel_snappy->setArg(narg++, *(buffer_output));
    decompress_kernel_snappy->setArg(narg++, *(buffer_block_size));
    decompress_kernel_snappy->setArg(narg++, *(buffer_compressed_size));
    decompress_kernel_snappy->setArg(narg++, m_BlockSizeInKb);
    decompress_kernel_snappy->setArg(narg++, blocksPerChunk);

    uint32_t chunk_size = 0;
    uint8_t chunk_idx = 0;
    uint32_t block_cntr = 0;
    uint32_t block_size = 0;
    uint32_t chunk_cntr = 0;
    uint32_t bufblocks = 0;
    uint64_t output_idx = 0;
    uint32_t bufIdx = 0;
    uint32_t over_block_cntr = 0;
    uint32_t brick = 0;
    uint16_t stride_cidsize = 4;
    bool blkDecomExist = false;
    uint32_t blkUnComp = 0;

    // Maximum allowed outbuffer size, if it exceeds then exit
    uint32_t c_max_outbuf = input_size * m_max_cr;

    // Go over overall input size
    for (uint32_t idxSize = 0; idxSize < input_size; idxSize += stride_cidsize, chunk_cntr++) {
        // Chunk identifier
        chunk_idx = in[idxSize];
        chunk_size = 0;

        // Chunk Compressed size
        uint8_t cbyte_1 = in[idxSize + 1];
        uint8_t cbyte_2 = in[idxSize + 2];
        uint8_t cbyte_3 = in[idxSize + 3];

        uint32_t temp = cbyte_3;
        temp <<= 16;
        chunk_size |= temp;
        temp = 0;
        temp = cbyte_2;
        temp <<= 8;
        chunk_size |= temp;
        temp = 0;
        chunk_size |= cbyte_1;

        if (chunk_idx == 0x00) {
            uint8_t bval1 = in[idxSize + 8];
            uint32_t final_size = 0;

            if ((bval1 >> 7) == 1) {
                uint8_t b1 = bval1 & 0x7F;
                bval1 = in[idxSize + 9];
                uint8_t b2 = bval1 & 0x7F;
                if ((bval1 >> 7) == 1) {
                    bval1 = in[idxSize + 10];
                    uint8_t b3 = bval1 & 0x7F;
                    uint32_t temp1 = b3;
                    temp1 <<= 14;
                    uint32_t temp2 = b2;
                    temp2 <<= 7;
                    uint32_t temp3 = b1;
                    final_size |= temp1;
                    final_size |= temp2;
                    final_size |= temp3;
                } else {
                    uint32_t temp1 = b2;
                    temp1 <<= 7;
                    uint32_t temp2 = b1;
                    final_size |= temp1;
                    final_size |= temp2;
                }
                block_size = final_size;
            } else {
                block_size = bval1;
            }
            m_compressSize.data()[over_block_cntr] = chunk_size - 4;
            m_blkSize.data()[over_block_cntr] = block_size;

            h_compressSize.data()[bufblocks] = chunk_size - 4;
            h_blksize.data()[bufblocks] = block_size;
            bufblocks++;

            // Copy data
            std::memcpy(&(h_buf_in.data()[block_cntr * buf_size]), &in[idxSize + 8], chunk_size - 4);
            block_cntr++;
            blkDecomExist = true;
        } else if (chunk_idx == 0x01) {
            m_compressSize.data()[over_block_cntr] = chunk_size - 4;
            m_blkSize.data()[over_block_cntr] = chunk_size - 4;
            std::memcpy(&out[brick * HOST_BUFFER_SIZE + over_block_cntr * buf_size], &in[idxSize + 8], chunk_size - 4);
            blkUnComp += chunk_size - 4;
        }

        over_block_cntr++;

        // Increment the input idx to
        // compressed size length
        idxSize += chunk_size;

        if (over_block_cntr == blocksPerChunk && blkDecomExist) {
            blkDecomExist = false;

            // Track the chunks processed
            brick++;

            // In case of left over set kernel arg to no blocks
            decompress_kernel_snappy->setArg(5, block_cntr);

            // For big files go ahead do it here
            std::vector<cl::Memory> inBufVec;
            inBufVec.push_back(*(buffer_input));
            inBufVec.push_back(*(buffer_block_size));
            inBufVec.push_back(*(buffer_compressed_size));

            // Migrate memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects(inBufVec, 0 /*0 means from host*/);
            m_q->finish();

            // Measure kernel execution time
            auto kernel_start = std::chrono::high_resolution_clock::now();

            // Kernel invocation
            m_q->enqueueTask(*decompress_kernel_snappy);
            m_q->finish();

            auto kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;

            std::vector<cl::Memory> outBufVec;
            outBufVec.push_back(*(buffer_output));

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();

            bufIdx = 0;
            // copy output
            for (uint32_t bIdx = 0; bIdx < over_block_cntr; bIdx++) {
                uint32_t block_size = m_blkSize.data()[bIdx];
                uint32_t compressed_size = m_compressSize.data()[bIdx];

                if ((output_idx + block_size) > c_max_outbuf) {
                    std::cout << "\n" << std::endl;
                    std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during decompression \033[0m \n"
                              << std::endl;
                    std::cout
                        << "\x1B[35mUse -mcr option to increase the maximum compression ratio (Default: 10) \033[0m \n"
                        << std::endl;
                    std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
                    exit(1);
                }

                if (compressed_size < block_size) {
                    std::memcpy(&out[output_idx], &h_buf_out.data()[bufIdx], block_size);
                    output_idx += block_size;
                    bufIdx += block_size;
                } else if (compressed_size == block_size) {
                    output_idx += block_size;
                    blkUnComp -= block_size;
                }
            }

            block_cntr = 0;
            bufblocks = 0;
            over_block_cntr = 0;
        } else if (over_block_cntr == blocksPerChunk) {
            over_block_cntr = 0;
            brick++;
            bufblocks = 0;
            block_cntr = 0;
        }
    }

    if (block_cntr != 0) {
        // In case of left over set kernel arg to no blocks
        decompress_kernel_snappy->setArg(5, block_cntr);

        std::vector<cl::Memory> inBufVec;
        inBufVec.push_back(*(buffer_input));
        inBufVec.push_back(*(buffer_block_size));
        inBufVec.push_back(*(buffer_compressed_size));

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /*0 means from host*/);
        m_q->finish();

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // Kernel invocation
        m_q->enqueueTask(*decompress_kernel_snappy);
        m_q->finish();

        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;

        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*(buffer_output));

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        bufIdx = 0;

        // copy output
        for (uint32_t bIdx = 0; bIdx < over_block_cntr; bIdx++) {
            uint32_t block_size = m_blkSize.data()[bIdx];
            uint32_t compressed_size = m_compressSize.data()[bIdx];

            if ((output_idx + block_size) > c_max_outbuf) {
                std::cout << "\n" << std::endl;
                std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during decompression \033[0m \n"
                          << std::endl;
                std::cout
                    << "\x1B[35mUse -mcr option to increase the maximum compression ratio (Default: 10) \033[0m \n"
                    << std::endl;
                std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
                exit(1);
            }

            if (compressed_size < block_size) {
                std::memcpy(&out[output_idx], &h_buf_out.data()[bufIdx], block_size);
                output_idx += block_size;
                bufIdx += block_size;
            } else if (compressed_size == block_size) {
                output_idx += block_size;
                blkUnComp -= block_size;
            }
        }

    } // If to see if tehr eare some blocks to be processed

    if (output_idx == 0 && blkUnComp != 0) {
        output_idx = blkUnComp;
    }

    float kernel_throughput_in_mbps_1 = (float)output_idx * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;

    delete (buffer_input);
    delete (buffer_output);
    delete (buffer_block_size);
    delete (buffer_compressed_size);

    return output_idx;
    return 0;
}

// Constructor
xilSnappy::xilSnappy(const std::string& binaryFile, uint8_t flow, uint32_t block_size, uint8_t max_cr) {
    // Index calculation
    h_buf_in.resize(HOST_BUFFER_SIZE);
    h_buf_out.resize(HOST_BUFFER_SIZE);
    h_blksize.resize(MAX_NUMBER_BLOCKS);
    h_compressSize.resize(MAX_NUMBER_BLOCKS);

    m_compressSize.reserve(MAX_NUMBER_BLOCKS);
    m_blkSize.reserve(MAX_NUMBER_BLOCKS);
    m_max_cr = max_cr;
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
    m_program = new cl::Program(*m_context, devices, bins);
    m_BinFlow = flow;
    m_BlockSizeInKb = block_size;
    // Create Compress kernels
    if (flow == 1 || flow == 2) compress_kernel_snappy = new cl::Kernel(*m_program, compress_kernel_names[0].c_str());

    // Create Decompress kernels
    if (flow == 0 || flow == 2)
        decompress_kernel_snappy = new cl::Kernel(*m_program, decompress_kernel_names[0].c_str());
}

// Destructor
xilSnappy::~xilSnappy() {
    if (m_BinFlow) {
        delete (compress_kernel_snappy);
    }
    if (m_BinFlow == 0 || m_BinFlow == 2) {
        delete (decompress_kernel_snappy);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);
}

// Note: Various block sizes supported by Snappy standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xilSnappy::compressSequential(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
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

    // This buffer contains total number of BLOCK_SIZE_IN_KB blocks per CU
    // For Example: HOST_BUFFER_SIZE = 2MB/BLOCK_SIZE_IN_KB = 32block (Block
    // size 64 by default)
    uint32_t total_blocks_cu;

    // This buffer holds exact size of the chunk in bytes for all the CUs
    uint32_t bufSize_in_bytes_cu;

    // Holds value of total compute units to be
    // used per iteration
    int compute_cu = 0;

    for (uint64_t inIdx = 0; inIdx < input_size; inIdx += HOST_BUFFER_SIZE) {
        // Needs to reset this variable
        // As this drives compute unit launch per iteration
        compute_cu = 0;

        // Pick buffer size as predefined one
        // If yet to be consumed input is lesser
        // the reset to required size
        uint32_t buf_size = HOST_BUFFER_SIZE;

        // This loop traverses through each compute based current inIdx
        // It tries to calculate chunk size and total compute units need to be
        // launched (based on the input_size)
        for (int bufCalc = 0; bufCalc < 1; bufCalc++) {
            hostChunk_cu = 0;
            // If amount of data to be consumed is less than HOST_BUFFER_SIZE
            // Then choose to send is what is needed instead of full buffer size
            // based on host buffer macro
            if (inIdx + (buf_size * (bufCalc + 1)) > input_size) {
                hostChunk_cu = input_size - (inIdx + HOST_BUFFER_SIZE * bufCalc);
                compute_cu++;
                break;
            } else {
                hostChunk_cu = buf_size;
                compute_cu++;
            }
        }
        // Figure out total number of blocks need per each chunk
        // Copy input data from in to host buffer based on the inIdx and cu
        for (int blkCalc = 0; blkCalc < compute_cu; blkCalc++) {
            uint32_t nblocks = (hostChunk_cu - 1) / block_size_in_bytes + 1;
            total_blocks_cu = nblocks;
            std::memcpy(h_buf_in.data(), &in[inIdx + blkCalc * HOST_BUFFER_SIZE], hostChunk_cu);
        }

        // Fill the host block size buffer with various block sizes per chunk/cu
        for (int cuBsize = 0; cuBsize < compute_cu; cuBsize++) {
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
            bufSize_in_bytes_cu = ((hostChunk_cu - 1) / BLOCK_SIZE_IN_KB + 1) * BLOCK_SIZE_IN_KB;
        }
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
        int narg = 0;
        compress_kernel_snappy->setArg(narg++, *(buffer_input));
        compress_kernel_snappy->setArg(narg++, *(buffer_output));
        compress_kernel_snappy->setArg(narg++, *(buffer_compressed_size));
        compress_kernel_snappy->setArg(narg++, *(buffer_block_size));
        compress_kernel_snappy->setArg(narg++, block_size_in_kb);
        compress_kernel_snappy->setArg(narg++, hostChunk_cu);
        std::vector<cl::Memory> inBufVec;

        inBufVec.push_back(*(buffer_input));
        inBufVec.push_back(*(buffer_block_size));

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_q->finish();

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // Fire kernel execution
        m_q->enqueueTask(*compress_kernel_snappy);
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

        for (int cuCopy = 0; cuCopy < compute_cu; cuCopy++) {
            // Copy data into out buffer
            // Include compress and block size data
            // Copy data block by block within a chunk example 2MB (64block size) - 32 blocks data
            // Do the same for all the compute units
            uint32_t idx = 0;
            for (uint32_t bIdx = 0; bIdx < total_blocks_cu; bIdx++, idx += block_size_in_bytes) {
                // Default block size in bytes i.e., 64 * 1024
                uint32_t block_size = block_size_in_bytes;
                if (idx + block_size > hostChunk_cu) {
                    block_size = hostChunk_cu - idx;
                }
                uint32_t compressed_size = h_compressSize.data()[bIdx];
                assert(compressed_size != 0);

                int orig_block_size = hostChunk_cu;
                int perc_cal = orig_block_size * 10;
                perc_cal = perc_cal / block_size;
                if (compressed_size < block_size && perc_cal >= 10) {
                    // Chunk Type Identifier
                    out[outIdx++] = 0x00;
                    // 3 Bytes to represent compress block length + 4;
                    uint32_t f_csize = compressed_size + 4;
                    std::memcpy(&out[outIdx], &f_csize, 3);
                    outIdx += 3;

                    // CRC - for now 0s
                    uint32_t crc_value = 0;
                    std::memcpy(&out[outIdx], &crc_value, 4);
                    outIdx += 4;
                    // Compressed data of this block with preamble
                    std::memcpy(&out[outIdx], (h_buf_out.data() + bIdx * block_size_in_bytes), compressed_size);
                    outIdx += compressed_size;
                } else {
                    // Chunk Type Identifier
                    out[outIdx++] = 0x01;
                    // 3 Bytes to represent uncompress block length + 4;
                    uint32_t f_csize = block_size + 4;
                    std::memcpy(&out[outIdx], &f_csize, 3);
                    outIdx += 3;

                    // CRC -for now 0s
                    uint32_t crc_value = 0;
                    std::memcpy(&out[outIdx], &crc_value, 4);
                    outIdx += 4;

                    // Uncompressed data copy
                    std::memcpy(&out[outIdx], &in[inIdx + (cuCopy * HOST_BUFFER_SIZE) + idx], block_size);
                    outIdx += block_size;
                } // End of else - uncompressed stream update
            }     // End of chunk (block by block) copy to output buffer
        }         // End of CU loop - Each CU/chunk block by block copy
        // Buffer deleted
        delete (buffer_input);
        delete (buffer_output);
        delete (buffer_compressed_size);
        delete (buffer_block_size);
    }
    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    return outIdx;
}
