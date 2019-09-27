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
#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include "snappy.hpp"

#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

using namespace xf::compression;

uint64_t getEventDurationNs(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    return (end_time - start_time);
}

uint64_t xilSnappy::compressFile(std::string& inFile_name,
                                 std::string& outFile_name,
                                 uint64_t input_size,
                                 bool file_list_flag) {
    if (m_switch_flow == 0) { // Xilinx FPGA compression flow
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
        uint32_t acc_buff_size = m_block_size_in_kb * 1024 * PARALLEL_BLOCK;
        if (acc_buff_size > host_buffer_size) {
            host_buffer_size = acc_buff_size;
        }
        if (host_buffer_size > input_size) {
            host_buffer_size = input_size;
        }
        if (host_buffer_size > HOST_BUFFER_SIZE) {
            host_buffer_size = HOST_BUFFER_SIZE;
        }

        // Snappy overlap & multiple compute unit compress
        uint64_t enbytes = compress(in.data(), out.data(), input_size, host_buffer_size, file_list_flag);

        // Writing compressed data
        outFile.write((char*)out.data(), enbytes);

        // Close file
        inFile.close();
        outFile.close();
        return enbytes;
    } else { // Standard Snappy flow
        // Build Java based snappy source code
        std::string command =
            "java -cp "
            "\".:snappy-0.5-SNAPSHOT-bin.jar:commons-io-2.6."
            "jar\" MainClass -c " +
            inFile_name;
        system(command.c_str());
        return 0;
    }
}

uint64_t xilSnappy::decompressFile(std::string& inFile_name,
                                   std::string& outFile_name,
                                   uint64_t input_size,
                                   bool file_list_flag) {
    if (m_switch_flow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(6 * input_size);

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

        // Decompression Overlapped multiple cu solution
        // uint64_t debytes = decompress(in.data(), out.data(), (input_size - 15),
        // original_size, host_buffer_size);

        // Decompression Sequential multiple cus.
        uint64_t debytes = decompressSequential(in.data(), out.data(), (input_size - 10), file_list_flag);
        outFile.write((char*)out.data(), debytes);

        // Close file
        inFile.close();
        outFile.close();

        return debytes;
    } else {
        // Use standard snappy compress/decompress below
        std::string command =
            "java -cp "
            "\".:snappy-0.5-SNAPSHOT-bin.jar:commons-io-2.6."
            "jar\" MainClass -d " +
            inFile_name;
        system(command.c_str());
        return 0;
    }
}

uint64_t xilSnappy::decompressSequential(uint8_t* in, uint8_t* out, uint64_t input_size, bool file_list_flag) {
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    uint32_t compute_cu = 1;
    uint32_t buf_size = BLOCK_SIZE_IN_KB * 1024;
    uint32_t blocksPerChunk = HOST_BUFFER_SIZE / buf_size;
    uint32_t host_buffer_size = ((HOST_BUFFER_SIZE - 1) / BLOCK_SIZE_IN_KB + 1) * BLOCK_SIZE_IN_KB;

    // Allocate global buffers
    for (uint32_t bufC = 0; bufC < compute_cu; bufC++) {
        // Device buffer allocation
        buffer_input[bufC][0] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, host_buffer_size,
                                               h_buf_in[bufC][0].data());

        buffer_output[bufC][0] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, host_buffer_size,
                                                h_buf_out[bufC][0].data());

        buffer_block_size[bufC][0] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    sizeof(uint32_t) * blocksPerChunk, h_blksize[bufC][0].data());

        buffer_compressed_size[bufC][0] =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(uint32_t) * blocksPerChunk,
                           h_compressSize[bufC][0].data());
    }

    // Set Kernel Arguments
    uint32_t narg = 0;
    decompress_kernel_snappy[0]->setArg(narg++, *(buffer_input[0][0]));
    decompress_kernel_snappy[0]->setArg(narg++, *(buffer_output[0][0]));
    decompress_kernel_snappy[0]->setArg(narg++, *(buffer_block_size[0][0]));
    decompress_kernel_snappy[0]->setArg(narg++, *(buffer_compressed_size[0][0]));
    decompress_kernel_snappy[0]->setArg(narg++, m_block_size_in_kb);
    decompress_kernel_snappy[0]->setArg(narg++, blocksPerChunk);

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
    uint32_t completed_bricks = 0;
    uint16_t stride_cidsize = 4;
    bool blkDecomExist = false;
    uint32_t blkUnComp = 0;

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
            m_compressSize[0][0].data()[over_block_cntr] = chunk_size - 4;
            m_blkSize[0][0].data()[over_block_cntr] = block_size;

            h_compressSize[0][0].data()[bufblocks] = chunk_size - 4;
            h_blksize[0][0].data()[bufblocks] = block_size;
            bufblocks++;

            // Copy data
            std::memcpy(&(h_buf_in[0][0].data()[block_cntr * buf_size]), &in[idxSize + 8], chunk_size - 4);
            block_cntr++;
            blkDecomExist = true;
        } else if (chunk_idx == 0x01) {
            m_compressSize[0][0].data()[over_block_cntr] = chunk_size - 4;
            m_blkSize[0][0].data()[over_block_cntr] = chunk_size - 4;
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
            decompress_kernel_snappy[0]->setArg(5, block_cntr);

            // For big files go ahead do it here
            std::vector<cl::Memory> inBufVec;
            inBufVec.push_back(*(buffer_input[0][0]));
            inBufVec.push_back(*(buffer_block_size[0][0]));
            inBufVec.push_back(*(buffer_compressed_size[0][0]));

            // Migrate memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects(inBufVec, 0 /*0 means from host*/);
            m_q->finish();

            // Measure kernel execution time
            auto kernel_start = std::chrono::high_resolution_clock::now();

            // Kernel invocation
            m_q->enqueueTask(*decompress_kernel_snappy[0]);
            m_q->finish();

            auto kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;

            std::vector<cl::Memory> outBufVec;
            outBufVec.push_back(*(buffer_output[0][0]));

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();

            bufIdx = 0;
            // copy output
            for (uint32_t bIdx = 0; bIdx < over_block_cntr; bIdx++) {
                uint32_t block_size = m_blkSize[0][0].data()[bIdx];
                uint32_t compressed_size = m_compressSize[0][0].data()[bIdx];
                if (compressed_size < block_size) {
                    std::memcpy(&out[output_idx], &h_buf_out[0][0].data()[bufIdx], block_size);
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
        decompress_kernel_snappy[0]->setArg(5, block_cntr);

        std::vector<cl::Memory> inBufVec;
        inBufVec.push_back(*(buffer_input[0][0]));
        inBufVec.push_back(*(buffer_block_size[0][0]));
        inBufVec.push_back(*(buffer_compressed_size[0][0]));

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /*0 means from host*/);
        m_q->finish();

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // Kernel invocation
        m_q->enqueueTask(*decompress_kernel_snappy[0]);
        m_q->finish();

        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;

        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*(buffer_output[0][0]));

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        bufIdx = 0;

        // copy output
        for (uint32_t bIdx = 0; bIdx < over_block_cntr; bIdx++) {
            uint32_t block_size = m_blkSize[0][0].data()[bIdx];
            uint32_t compressed_size = m_compressSize[0][0].data()[bIdx];
            if (compressed_size < block_size) {
                std::memcpy(&out[output_idx], &h_buf_out[0][0].data()[bufIdx], block_size);
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
    if (file_list_flag == 0) {
        std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:" << kernel_throughput_in_mbps_1 << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;
    }

    delete (buffer_input[0][0]);
    delete (buffer_output[0][0]);
    delete (buffer_block_size[0][0]);
    delete (buffer_compressed_size[0][0]);

    return output_idx;
    return 0;
}

// Constructor
xilSnappy::xilSnappy() {
    for (int i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            h_buf_in[i][j].resize(HOST_BUFFER_SIZE);
            h_buf_out[i][j].resize(HOST_BUFFER_SIZE);
            h_blksize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS);

            m_compressSize[i][j].reserve(MAX_NUMBER_BLOCKS);
            m_blkSize[i][j].reserve(MAX_NUMBER_BLOCKS);
        }
    }
}

// Destructor
xilSnappy::~xilSnappy() {}

int xilSnappy::init(const std::string& binaryFile) {
    unsigned fileBufSize;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

    if (SINGLE_XCLBIN) {
        std::string cu_id;
        std::string comp_krnl_name = compress_kernel_names[0].c_str();
        std::string decomp_krnl_name = decompress_kernel_names[0].c_str();
        // Create Compress kernels
        for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string krnl_name_full = comp_krnl_name + ":{" + comp_krnl_name + "_" + cu_id + "}";
            compress_kernel_snappy[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
        }

        // Create Decompress kernels
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++)
            decompress_kernel_snappy[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
    } else {
        std::string cu_id;
        std::string comp_krnl_name = compress_kernel_names[0].c_str();
        std::string decomp_krnl_name = decompress_kernel_names[0].c_str();
        if (m_bin_flow) {
            // Create Compress kernels
            for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
                cu_id = std::to_string(i + 1);
                std::string krnl_name_full = comp_krnl_name + ":{" + comp_krnl_name + "_" + cu_id + "}";
                compress_kernel_snappy[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
            }
        } else {
            // Create Decompress kernels
            for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++)
                decompress_kernel_snappy[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
        }
    }
    return 0;
}

int xilSnappy::release() {
    if (m_bin_flow) {
        for (int i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel_snappy[i]);
    } else {
        for (int i = 0; i < D_COMPUTE_UNIT; i++) delete (decompress_kernel_snappy[i]);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);

    return 0;
}

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint64_t xilSnappy::compress(
    uint8_t* in, uint8_t* out, uint64_t input_size, uint32_t host_buffer_size, bool file_list_flag) {
    uint32_t block_size_in_bytes = m_block_size_in_kb * 1024;

    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    uint64_t total_kernel_time = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;

    // Read, Write and Kernel events
    cl::Event kernel_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event read_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event write_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    // Total chunks in input file
    // For example: Input file size is 12MB and Host buffer size is 2MB
    // Then we have 12/2 = 6 chunks exists
    // Calculate the count of total chunks based on input size
    // This count is used to overlap the execution between chunks and file
    // operations
    uint32_t total_chunks = (input_size - 1) / host_buffer_size + 1;

    if (total_chunks < 2) overlap_buf_count = 1;

    // Find out the size of each chunk spanning entire file
    // For eaxmple: As mentioned in previous example there are 6 chunks
    // Code below finds out the size of chunk, in general all the chunks holds
    // HOST_BUFFER_SIZE except for the last chunk
    uint32_t sizeOfChunk[total_chunks];
    uint32_t blocksPerChunk[total_chunks];
    uint32_t idx = 0;
    for (uint64_t i = 0; i < input_size; i += host_buffer_size, idx++) {
        uint32_t chunk_size = host_buffer_size;
        if (chunk_size + i > input_size) {
            chunk_size = input_size - i;
        }
        // Update size of each chunk buffer
        sizeOfChunk[idx] = chunk_size;
        // Calculate sub blocks of size BLOCK_SIZE_IN_KB for each chunk
        // 2MB(example)
        // Figure out blocks per chunk
        uint32_t nblocks = (chunk_size - 1) / block_size_in_bytes + 1;
        blocksPerChunk[idx] = nblocks;
    }

    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / 64 + 1) * 64;

    // Device buffer allocation
    for (int cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            // Input:- This buffer contains input chunk data
            buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    host_buffer_size, h_buf_in[cu][flag].data());

            // Output:- This buffer contains compressed data written by device
            buffer_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                     host_buffer_size, h_buf_out[cu][flag].data());

            // Ouput:- This buffer contains compressed block sizes
            buffer_compressed_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, temp_nblocks * sizeof(uint32_t),
                               h_compressSize[cu][flag].data());

            // Input:- This buffer contains origianl input block sizes
            buffer_block_size[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                         temp_nblocks * sizeof(uint32_t), h_blksize[cu][flag].data());
        }
    }

    // Counter which helps in tracking
    // Output buffer index
    uint64_t outIdx = 0;

    // Track the lags of respective chunks for left over handling
    int chunk_flags[total_chunks];
    int cu_order[total_chunks];

    // Finished bricks
    int completed_bricks = 0;

    int flag = 0;
    int lcl_cu = 0;

    // Main loop of overlap execution
    // Loop below runs over total bricks i.e., host buffer size chunks
    auto total_start = std::chrono::high_resolution_clock::now();
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; brick += C_COMPUTE_UNIT, itr++, flag = !flag) {
        lcl_cu = C_COMPUTE_UNIT;
        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;
        // Loop below runs over number of compute units
        for (int cu = 0; cu < lcl_cu; cu++) {
            chunk_flags[brick + cu] = flag;
            cu_order[brick + cu] = cu;
            // Wait on read events
            if (itr >= 2) {
                // Wait on current flag previous operation to finish
                read_events[cu][flag].wait();

                // Completed bricks counter
                completed_bricks++;

                // Accumulate Kernel time
                total_kernel_time += getEventDurationNs(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
                // Accumulate Write time
                total_write_time += getEventDurationNs(write_events[cu][flag]);
                // Accumulate Read time
                total_read_time += getEventDurationNs(read_events[cu][flag]);
#endif
                // Run over each block of the within brick
                uint32_t index = 0;
                int brick_flag_idx = brick - (C_COMPUTE_UNIT * overlap_buf_count - cu);
                for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                    uint32_t block_size = block_size_in_bytes;
                    if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                        block_size = sizeOfChunk[brick_flag_idx] - index;
                    }
                    // Figure out the compressed size
                    uint32_t compressed_size = (h_compressSize[cu][flag]).data()[bIdx];
                    assert(compressed_size != 0);
                    assert(compressed_size <= block_size_in_bytes);

                    int orig_chunk_size = sizeOfChunk[brick_flag_idx];
                    int perc_cal = orig_chunk_size * 10;
                    perc_cal = perc_cal / block_size;

                    // If compressed size is less than original block size
                    // It means better to dump encoded bytes
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
                        std::memcpy(&out[outIdx], (h_buf_out[cu][flag]).data() + bIdx * block_size_in_bytes,
                                    compressed_size);
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
                        std::memcpy(&out[outIdx], &in[brick_flag_idx * host_buffer_size + index], block_size);
                        outIdx += block_size;
                    } // End of else - uncompressed stream update
                }
            }
            // Figure out block sizes per brick
            uint32_t bIdx = 0;
            for (uint32_t i = 0; i < sizeOfChunk[brick + cu]; i += block_size_in_bytes) {
                uint32_t block_size = block_size_in_bytes;
                if (i + block_size > sizeOfChunk[brick + cu]) {
                    block_size = sizeOfChunk[brick + cu] - i;
                }
                (h_blksize[cu][flag]).data()[bIdx++] = block_size;
            }

            // Copy data from input buffer to host
            std::memcpy(h_buf_in[cu][flag].data(), &in[(brick + cu) * host_buffer_size], sizeOfChunk[brick + cu]);

            // Set kernel arguments
            int narg = 0;
            compress_kernel_snappy[cu]->setArg(narg++, *(buffer_input[cu][flag]));
            compress_kernel_snappy[cu]->setArg(narg++, *(buffer_output[cu][flag]));
            compress_kernel_snappy[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
            compress_kernel_snappy[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
            compress_kernel_snappy[cu]->setArg(narg++, m_block_size_in_kb);
            compress_kernel_snappy[cu]->setArg(narg++, sizeOfChunk[brick + cu]);

            // Transfer data from host to device
            m_q->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_block_size[cu][flag])}, 0, NULL,
                                          &(write_events[cu][flag]));

            // Kernel wait events for writing & compute
            std::vector<cl::Event> kernelWriteWait;
            std::vector<cl::Event> kernelComputeWait;

            // Kernel Write events update
            kernelWriteWait.push_back(write_events[cu][flag]);

            // Fire the kernel
            m_q->enqueueTask(*compress_kernel_snappy[cu], &kernelWriteWait, &(kernel_events[cu][flag]));

            // Update kernel events flag on computation
            kernelComputeWait.push_back(kernel_events[cu][flag]);

            // Transfer data from device to host
            m_q->enqueueMigrateMemObjects({*(buffer_output[cu][flag]), *(buffer_compressed_size[cu][flag])},
                                          CL_MIGRATE_MEM_OBJECT_HOST, &kernelComputeWait, &(read_events[cu][flag]));
        } // Compute unit loop ends here

    } // Main loop ends here
    m_q->flush();
    m_q->finish();

    int leftover = total_chunks - completed_bricks;
    int stride = 0;

    if ((total_chunks < overlap_buf_count * C_COMPUTE_UNIT))
        stride = overlap_buf_count * C_COMPUTE_UNIT;
    else
        stride = total_chunks;

    // Handle leftover bricks
    for (int ovr_itr = 0, brick = stride - overlap_buf_count * C_COMPUTE_UNIT; ovr_itr < leftover;
         ovr_itr += C_COMPUTE_UNIT, brick += C_COMPUTE_UNIT) {
        lcl_cu = C_COMPUTE_UNIT;
        if (ovr_itr + lcl_cu > leftover) lcl_cu = leftover - ovr_itr;

        // Handlue multiple bricks with multiple CUs
        for (int j = 0; j < lcl_cu; j++) {
            int cu = cu_order[brick + j];
            int flag = chunk_flags[brick + j];
            // Run over each block of the within brick
            uint32_t index = 0;
            int brick_flag_idx = brick + j;

            // Accumulate Kernel time
            total_kernel_time += getEventDurationNs(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
            // Accumulate Write time
            total_write_time += getEventDurationNs(write_events[cu][flag]);
            // Accumulate Read time
            total_read_time += getEventDurationNs(read_events[cu][flag]);
#endif
            for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                uint32_t block_size = block_size_in_bytes;
                if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                    block_size = sizeOfChunk[brick_flag_idx] - index;
                }

                // Figure out the compressed size
                uint32_t compressed_size = (h_compressSize[cu][flag]).data()[bIdx];
                assert(compressed_size != 0);
                assert(compressed_size <= block_size_in_bytes);

                int orig_chunk_size = sizeOfChunk[brick_flag_idx];
                int perc_cal = orig_chunk_size * 10;
                perc_cal = perc_cal / block_size;

                // If compressed size is less than original block size
                // It means better to dump encoded bytes
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
                    std::memcpy(&out[outIdx], (h_buf_out[cu][flag]).data() + bIdx * block_size_in_bytes,
                                compressed_size);
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

                    std::memcpy(&out[outIdx], &in[brick_flag_idx * host_buffer_size + index], block_size);
                    outIdx += block_size;
                } // End of else - uncompressed stream update

            } // For loop ends
        }     // cu loop ends here
    }         // Main loop ends here

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
    float kernel_throughput_in_mbps_1 = (float)input_size * 1000 / total_kernel_time;
#ifdef EVENT_PROFILE
    std::cout << "Total Kernel Time " << total_kernel_time << std::endl;
    std::cout << "Total Write Time " << total_write_time << std::endl;
    std::cout << "Total Read Time " << total_read_time << std::endl;
#endif
    if (file_list_flag == 0) {
        std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:" << throughput_in_mbps_1 << std::endl
                  << "KT(MBps)\t\t:" << kernel_throughput_in_mbps_1 << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
        std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;
    }

    for (int cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            delete (buffer_input[cu][flag]);
            delete (buffer_output[cu][flag]);
            delete (buffer_compressed_size[cu][flag]);
            delete (buffer_block_size[cu][flag]);
        }
    }

    return outIdx;
} // Overlap end
