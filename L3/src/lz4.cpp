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
#include "xxhash.h"
#include <iostream>
#include <cassert>
#include <vector>
#include "lz4.hpp"
#include "lz4_specs.hpp"

using namespace xf::compression;

#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))
namespace lz4_specs = xf::compression;

// Get the duration of input event
uint64_t getEventDurationNs(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    uint64_t duration = end_time - start_time;
    return duration;
}

uint64_t xfLz4::compressFile(
    std::string& inFile_name, std::string& outFile_name, uint64_t input_size, bool file_list_flag, bool m_flow) {
    m_SwitchFlow = m_flow;
    if (m_SwitchFlow == 0) { // Xilinx FPGA compression flow
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in;
        std::vector<uint8_t, aligned_allocator<uint8_t> > out;

        MEM_ALLOC_CHECK(in.resize(input_size), input_size, "Input Buffer");
        MEM_ALLOC_CHECK(out.resize(input_size), input_size, "Output Buffer");

        inFile.read((char*)in.data(), input_size);

        // LZ4 header
        outFile.put(MAGIC_BYTE_1);
        outFile.put(MAGIC_BYTE_2);
        outFile.put(MAGIC_BYTE_3);
        outFile.put(MAGIC_BYTE_4);

        // FLG & BD bytes
        // --no-frame-crc flow
        // --content-size
        outFile.put(FLG_BYTE);

        // Default value 64K
        uint8_t block_size_header = 0;
        switch (m_BlockSizeInKb) {
            case 64:
                outFile.put(lz4_specs::BSIZE_STD_64KB);
                block_size_header = lz4_specs::BSIZE_STD_64KB;
                break;
            case 256:
                outFile.put(lz4_specs::BSIZE_STD_256KB);
                block_size_header = lz4_specs::BSIZE_STD_256KB;
                break;
            case 1024:
                outFile.put(lz4_specs::BSIZE_STD_1024KB);
                block_size_header = lz4_specs::BSIZE_STD_1024KB;
                break;
            case 4096:
                outFile.put(lz4_specs::BSIZE_STD_4096KB);
                block_size_header = lz4_specs::BSIZE_STD_4096KB;
                break;
            default:
                std::cout << "Invalid Block Size" << std::endl;
                break;
        }

        uint32_t host_buffer_size = (m_BlockSizeInKb * 1024) * 32;

        if ((m_BlockSizeInKb * 1024) > input_size) host_buffer_size = m_BlockSizeInKb * 1024;

        uint64_t temp_buff[10] = {FLG_BYTE,         block_size_header, input_size,       input_size >> 8,
                                  input_size >> 16, input_size >> 24,  input_size >> 32, input_size >> 40,
                                  input_size >> 48, input_size >> 56};

        // xxhash is used to calculate hash value
        uint32_t xxh = XXH32(temp_buff, 10, 0);
        uint64_t enbytes;
        outFile.write((char*)&temp_buff[2], 8);

        // Header CRC
        outFile.put((uint8_t)(xxh >> 8));
        // LZ4 overlap & multiple compute unit compress
        enbytes = compress(in.data(), out.data(), input_size, host_buffer_size, file_list_flag);
        // Writing compressed data
        outFile.write((char*)out.data(), enbytes);

        outFile.put(0);
        outFile.put(0);
        outFile.put(0);
        outFile.put(0);

        // Close file
        inFile.close();
        outFile.close();
        return enbytes;
    } else { // Standard LZ4 flow
        std::string command = "lz4 --content-size -f -q " + inFile_name;
        system(command.c_str());
        std::string output = inFile_name + ".lz4";
        std::string rout = inFile_name + ".std.lz4";
        std::string rename = "mv " + output + " " + rout;
        system(rename.c_str());
        return 0;
    }
}

// Constructor
xfLz4::xfLz4() {
    for (uint32_t i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (uint32_t j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            MEM_ALLOC_CHECK(h_buf_in[i][j].resize(HOST_BUFFER_SIZE), HOST_BUFFER_SIZE, "Input Host Buffer");
            MEM_ALLOC_CHECK(h_buf_out[i][j].resize(HOST_BUFFER_SIZE), HOST_BUFFER_SIZE, "Output Host Buffer");
            MEM_ALLOC_CHECK(h_blksize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS, "BlockSize Host Buffer");
            MEM_ALLOC_CHECK(h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS,
                            "CompressSize Host Buffer");

            MEM_ALLOC_CHECK(m_compressSize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS,
                            "CompressSize(m) Host Buffer");
            MEM_ALLOC_CHECK(m_blkSize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS, "BlockSize(m) Host Buffer");
        }
    }
}

// Destructor
xfLz4::~xfLz4() {}

int xfLz4::init(const std::string& binaryFile, uint8_t flow, uint32_t block_size_kb) {
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

    m_program = new cl::Program(*m_context, devices, bins);
    std::string cu_id;
    std::string comp_krnl_name = compress_kernel_names[0].c_str();
    std::string decomp_krnl_name = decompress_kernel_names[0].c_str();

    m_BlockSizeInKb = block_size_kb;
    m_BinFlow = flow;

    if (m_BinFlow) {
        // Create Compress kernels
        for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string krnl_name_full = comp_krnl_name + ":{" + comp_krnl_name + "_" + cu_id + "}";
            compress_kernel_lz4[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
        }
    } else {
        // Create Decompress kernels
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string krnl_name_full = decomp_krnl_name + ":{" + decomp_krnl_name + "_" + cu_id + "}";
            decompress_kernel_lz4[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
        }
    }

    return 0;
}

int xfLz4::release() {
    if (m_BinFlow) {
        for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
            if (compress_kernel_lz4[i]) {
                delete compress_kernel_lz4[i];
                compress_kernel_lz4[i] = nullptr;
            }
        }
    } else {
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) {
            if (decompress_kernel_lz4[i]) {
                delete decompress_kernel_lz4[i];
                decompress_kernel_lz4[i] = nullptr;
            }
        }
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);

    return 0;
}

uint64_t xfLz4::decompressFile(
    std::string& inFile_name, std::string& outFile_name, uint64_t input_size, bool file_list_flag, bool m_flow) {
    m_SwitchFlow = m_flow;
    if (m_SwitchFlow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        MEM_ALLOC_CHECK(in.resize(input_size), input_size, "Input Buffer");
        // Read magic header 4 bytes
        char c = 0;
        char magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
        for (uint32_t i = 0; i < MAGIC_HEADER_SIZE; i++) {
            inFile.get(c);
            if (c == magic_hdr[i])
                continue;
            else {
                std::cout << "Problem with magic header " << c << " " << i << std::endl;
                exit(1);
            }
        }

        // Header Checksum
        inFile.get(c);

        // Check if block size is 64 KB
        inFile.get(c);
        // printf("block_size %d \n", c);

        switch (c) {
            case lz4_specs::BSIZE_STD_64KB:
                m_BlockSizeInKb = 64;
                break;
            case lz4_specs::BSIZE_STD_256KB:
                m_BlockSizeInKb = 256;
                break;
            case lz4_specs::BSIZE_STD_1024KB:
                m_BlockSizeInKb = 1024;
                break;
            case lz4_specs::BSIZE_STD_4096KB:
                m_BlockSizeInKb = 4096;
                break;
            default:
                std::cout << "Invalid Block Size" << std::endl;
                break;
        }
        // printf("m_BlockSizeInKb %d \n", m_BlockSizeInKb);

        // Original size
        uint64_t original_size = 0;
        inFile.read((char*)&original_size, 8);
        inFile.get(c);
        // printf("original_size %d \n", original_size);

        // Allocat output size
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(original_size);
        MEM_ALLOC_CHECK(out.resize(original_size), original_size, "Output Buffer");

        // Read block data from compressed stream .lz4
        inFile.read((char*)in.data(), (input_size - 15));

        uint32_t host_buffer_size = (m_BlockSizeInKb * 1024) * 32;

        if ((m_BlockSizeInKb * 1024) > original_size) host_buffer_size = m_BlockSizeInKb * 1024;

        uint64_t debytes;
        // Decompression Overlapped multiple cu solution
        debytes = decompress(in.data(), out.data(), (input_size - 15), original_size, host_buffer_size, file_list_flag);
        outFile.write((char*)out.data(), debytes);
        // Close file
        inFile.close();
        outFile.close();
        return debytes;
    } else {
        std::string command = "lz4 --content-size -f -q -d " + inFile_name;
        system(command.c_str());
        return 0;
    }
}

uint64_t xfLz4::decompress(uint8_t* in,
                           uint8_t* out,
                           uint64_t input_size,
                           uint64_t original_size,
                           uint32_t host_buffer_size,
                           bool file_list_flag) {
    uint32_t max_num_blks = (host_buffer_size) / (m_BlockSizeInKb * 1024);

    for (uint32_t i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (uint32_t j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            MEM_ALLOC_CHECK(h_buf_in[i][j].resize(host_buffer_size), host_buffer_size, "Input Host Buffer");
            MEM_ALLOC_CHECK(h_buf_out[i][j].resize(host_buffer_size), host_buffer_size, "Output Host Buffer");
            MEM_ALLOC_CHECK(h_blksize[i][j].resize(max_num_blks), max_num_blks, "MaxBlockSize Host Buffer");
            MEM_ALLOC_CHECK(h_compressSize[i][j].resize(max_num_blks), max_num_blks, "MaxNumBlocks Host Buffer");

            MEM_ALLOC_CHECK(m_compressSize[i][j].resize(max_num_blks), max_num_blks, "MaxNumBlocks (m) Host Buffer");
            MEM_ALLOC_CHECK(m_blkSize[i][j].resize(max_num_blks), max_num_blks, "MaxBlockSize (m) Host Buffer");
        }
    }

    uint32_t block_size_in_bytes = m_BlockSizeInKb * 1024;
    uint64_t total_kernel_time = 0;

    // Total number of blocks exist for this file
    uint32_t total_block_cnt = (original_size - 1) / block_size_in_bytes + 1;
    uint32_t block_cntr = 0;
    uint32_t done_block_cntr = 0;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;

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

    uint32_t total_chunks = (original_size - 1) / host_buffer_size + 1;
    if (total_chunks < 2) overlap_buf_count = 1;

    // Find out the size of each chunk spanning entire file
    // For eaxmple: As mentioned in previous example there are 6 chunks
    // Code below finds out the size of chunk, in general all the chunks holds
    // HOST_BUFFER_SIZE except for the last chunk
    uint32_t sizeOfChunk[total_chunks];
    uint32_t blocksPerChunk[total_chunks];
    uint32_t computeBlocksPerChunk[total_chunks];
    uint64_t idx = 0;
    for (uint64_t i = 0; i < original_size; i += host_buffer_size, idx++) {
        uint32_t chunk_size = host_buffer_size;
        if (chunk_size + i > original_size) {
            chunk_size = original_size - i;
        }
        // Update size of each chunk buffer
        sizeOfChunk[idx] = chunk_size;
        // Calculate sub blocks of size BLOCK_SIZE_IN_KB for each chunk
        // 2MB(example)
        // Figure out blocks per chunk
        uint32_t nblocks = (chunk_size - 1) / block_size_in_bytes + 1;
        blocksPerChunk[idx] = nblocks;
        computeBlocksPerChunk[idx] = nblocks;
    }

    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / 64 + 1) * 64;

    // Device buffer allocation
    for (uint32_t cu = 0; cu < D_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            // Input:- This buffer contains input chunk data
            buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    host_buffer_size, h_buf_in[cu][flag].data());

            // Output:- This buffer contains compressed data written by device
            buffer_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                     host_buffer_size, h_buf_out[cu][flag].data());

            // Ouput:- This buffer contains compressed block sizes
            buffer_compressed_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, temp_nblocks * sizeof(uint32_t),
                               h_compressSize[cu][flag].data());

            // Input:- This buffer contains origianl input block sizes
            buffer_block_size[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                         temp_nblocks * sizeof(uint32_t), h_blksize[cu][flag].data());
        }
    }

    // Counter which helps in tracking output buffer index
    uint64_t outIdx = 0;

    // Track the flags of remaining chunks
    uint32_t chunk_flags[total_chunks];
    uint32_t cu_order[total_chunks];

    // Finished bricks
    uint32_t completed_bricks = 0;

    int flag = 0;
    uint32_t lcl_cu = 0;
    uint64_t inIdx = 0;
    uint64_t total_decompression_size = 0;

    uint32_t init_itr = 0;
    if (total_chunks < 2 * D_COMPUTE_UNIT)
        init_itr = total_chunks;
    else
        init_itr = 2 * D_COMPUTE_UNIT;

    auto total_start = std::chrono::high_resolution_clock::now();
    // Copy first few buffers
    for (uint32_t itr = 0, brick = 0; brick < init_itr; brick += D_COMPUTE_UNIT, itr++, flag = !flag) {
        lcl_cu = D_COMPUTE_UNIT;
        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;

        for (uint32_t cu = 0; cu < lcl_cu; cu++) {
            uint32_t total_size = 0;
            uint32_t compressed_size = 0;
            uint32_t block_size = 0;
            uint32_t nblocks = 0;
            uint32_t bufblocks = 0;
            uint32_t buf_size = 0;
            uint32_t no_compress_size = 0;

            for (uint32_t cIdx = 0; cIdx < sizeOfChunk[brick + cu];
                 cIdx += block_size_in_bytes, total_size += block_size) {
                if (block_cntr == (total_block_cnt - 1)) {
                    block_size = original_size - done_block_cntr * block_size_in_bytes;
                } else {
                    block_size = block_size_in_bytes;
                }

                std::memcpy(&compressed_size, &in[inIdx], 4);
                inIdx += 4;

                uint32_t tmp = compressed_size;
                tmp >>= 24;

                if (tmp == lz4_specs::NO_COMPRESS_BIT) {
                    uint8_t b1 = compressed_size;
                    uint8_t b2 = compressed_size >> 8;
                    uint8_t b3 = compressed_size >> 16;

                    if (b3 == lz4_specs::BSIZE_NCOMP_64 || b3 == lz4_specs::BSIZE_NCOMP_4096 ||
                        b3 == lz4_specs::BSIZE_NCOMP_256 || b3 == lz4_specs::BSIZE_NCOMP_1024) {
                        compressed_size = block_size_in_bytes;
                    } else {
                        uint32_t size = 0;
                        size = b3;
                        size <<= 16;
                        uint32_t temp = b2;
                        temp <<= 8;
                        size |= temp;
                        temp = b1;
                        size |= temp;
                        compressed_size = size;
                    }
                }

                m_blkSize[cu][flag].data()[nblocks] = block_size;
                m_compressSize[cu][flag].data()[nblocks] = compressed_size;
                nblocks++;

                if (compressed_size < block_size) {
                    h_compressSize[cu][flag].data()[bufblocks] = compressed_size;
                    h_blksize[cu][flag].data()[bufblocks] = block_size;
                    std::memcpy(&(h_buf_in[cu][flag].data()[buf_size]), &in[inIdx], compressed_size);
                    inIdx += compressed_size;
                    buf_size += block_size_in_bytes;
                    bufblocks++;
                } else if (compressed_size == block_size) {
                    no_compress_size++;

                    int outChunkIdx = brick + cu;
                    // No compression block
                    std::memcpy(&(out[outChunkIdx * host_buffer_size + cIdx]), &in[inIdx], block_size);
                    inIdx += block_size;
                    computeBlocksPerChunk[outChunkIdx]--;
                } else {
                    assert(0);
                }
                block_cntr++;
                done_block_cntr++;
            }
        }
    }
    flag = 0;
    // Main loop of overlap execution
    // Loop below runs over total bricks i.e., host buffer size chunks
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; brick += D_COMPUTE_UNIT, itr++, flag = !flag) {
        lcl_cu = D_COMPUTE_UNIT;
        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;

        // Loop below runs over number of compute units
        for (uint32_t cu = 0; cu < lcl_cu; cu++) {
            chunk_flags[brick + cu] = flag;
            cu_order[brick + cu] = cu;
            if (itr >= 2) {
                read_events[cu][flag].wait();

                completed_bricks++;

                // Accumulate Kernel time
                total_kernel_time += getEventDurationNs(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
                // Accumulate Write time
                total_write_time += getEventDurationNs(write_events[cu][flag]);
                // Accumulate Read time
                total_read_time += getEventDurationNs(read_events[cu][flag]);
#endif

                int brick_flag_idx = brick - (D_COMPUTE_UNIT * overlap_buf_count - cu);
                uint32_t bufIdx = 0;
                for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, idx += block_size_in_bytes) {
                    uint32_t block_size = m_blkSize[cu][flag].data()[bIdx];
                    uint32_t compressed_size = m_compressSize[cu][flag].data()[bIdx];
                    if (compressed_size < block_size) {
                        std::memcpy(&out[outIdx], &h_buf_out[cu][flag].data()[bufIdx], block_size);
                        outIdx += block_size;
                        bufIdx += block_size_in_bytes;
                        total_decompression_size += block_size;
                    } else if (compressed_size == block_size) {
                        outIdx += block_size;
                        total_decompression_size += block_size;
                    }
                } // For loop ends here

                uint32_t total_size = 0;
                uint32_t compressed_size = 0;
                uint32_t block_size = 0;
                uint32_t nblocks = 0;
                uint32_t bufblocks = 0;
                uint32_t buf_size = 0;
                uint32_t no_compress_size = 0;
                for (uint32_t cIdx = 0; cIdx < sizeOfChunk[brick + cu];
                     cIdx += block_size_in_bytes, total_size += block_size) {
                    if (block_cntr == (total_block_cnt - 1)) {
                        block_size = original_size - done_block_cntr * block_size_in_bytes;
                    } else {
                        block_size = block_size_in_bytes;
                    }

                    std::memcpy(&compressed_size, &in[inIdx], 4);
                    inIdx += 4;

                    uint32_t tmp = compressed_size;
                    tmp >>= 24;

                    if (tmp == lz4_specs::NO_COMPRESS_BIT) {
                        uint8_t b1 = compressed_size;
                        uint8_t b2 = compressed_size >> 8;
                        uint8_t b3 = compressed_size >> 16;

                        if (b3 == lz4_specs::BSIZE_NCOMP_64 || b3 == lz4_specs::BSIZE_NCOMP_4096 ||
                            b3 == lz4_specs::BSIZE_NCOMP_256 || b3 == lz4_specs::BSIZE_NCOMP_1024) {
                            compressed_size = block_size_in_bytes;
                        } else {
                            uint32_t size = 0;
                            size = b3;
                            size <<= 16;
                            uint32_t temp = b2;
                            temp <<= 8;
                            size |= temp;
                            temp = b1;
                            size |= temp;
                            compressed_size = size;
                        }
                    }

                    m_blkSize[cu][flag].data()[nblocks] = block_size;
                    m_compressSize[cu][flag].data()[nblocks] = compressed_size;
                    nblocks++;
                    if (compressed_size < block_size) {
                        h_compressSize[cu][flag].data()[bufblocks] = compressed_size;
                        h_blksize[cu][flag].data()[bufblocks] = block_size;
                        std::memcpy(&(h_buf_in[cu][flag].data()[buf_size]), &in[inIdx], compressed_size);
                        inIdx += compressed_size;
                        buf_size += block_size_in_bytes;
                        bufblocks++;
                    } else if (compressed_size == block_size) {
                        no_compress_size++;
                        int outChunkIdx = brick + cu;
                        // No compression block
                        std::memcpy(&(out[outChunkIdx * host_buffer_size + cIdx]), &in[inIdx], block_size);
                        inIdx += block_size;
                        computeBlocksPerChunk[outChunkIdx]--;
                    } else {
                        assert(0);
                    }
                    block_cntr++;
                    done_block_cntr++;
                } // Input forloop ends here
            }     // If condition ends here

            // Set kernel arguments
            uint32_t narg = 0;
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_input[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_output[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, m_BlockSizeInKb);
            decompress_kernel_lz4[cu]->setArg(narg++, computeBlocksPerChunk[brick + cu]);

            // Kernel wait events for writing & compute
            std::vector<cl::Event> kernelWriteWait;
            std::vector<cl::Event> kernelComputeWait;

            // Migrate memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects(
                {*(buffer_input[cu][flag]), *(buffer_compressed_size[cu][flag]), *(buffer_block_size[cu][flag])}, 0,
                NULL, &(write_events[cu][flag]) /* 0 means from host*/);

            // Kernel write events update
            kernelWriteWait.push_back(write_events[cu][flag]);

            // Launch kernel
            m_q->enqueueTask(*decompress_kernel_lz4[cu], &kernelWriteWait, &(kernel_events[cu][flag]));

            // Update kernel events flag on computation
            kernelComputeWait.push_back(kernel_events[cu][flag]);

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects({*(buffer_output[cu][flag])}, CL_MIGRATE_MEM_OBJECT_HOST, &kernelComputeWait,
                                          &(read_events[cu][flag]));

        } // Compute unit loop

    } // End of main loop
    m_q->flush();
    m_q->finish();

    uint32_t leftover = total_chunks - completed_bricks;
    uint32_t stride = 0;

    if ((total_chunks < overlap_buf_count * D_COMPUTE_UNIT))
        stride = overlap_buf_count * D_COMPUTE_UNIT;
    else
        stride = total_chunks;

    // Handle leftover bricks
    for (uint32_t ovr_itr = 0, brick = stride - overlap_buf_count * D_COMPUTE_UNIT; ovr_itr < leftover;
         ovr_itr += D_COMPUTE_UNIT, brick += D_COMPUTE_UNIT) {
        lcl_cu = D_COMPUTE_UNIT;
        if (ovr_itr + lcl_cu > leftover) lcl_cu = leftover - ovr_itr;

        // Handle multiple bricks with multiple CUs
        for (uint32_t j = 0; j < lcl_cu; j++) {
            int cu = cu_order[brick + j];
            int flag = chunk_flags[brick + j];

            // Run over each block within brick
            int brick_flag_idx = brick + j;

            // Accumulate Kernel time
            total_kernel_time += getEventDurationNs(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
            // Accumulate Write time
            total_write_time += getEventDurationNs(write_events[cu][flag]);
            // Accumulate Read time
            total_read_time += getEventDurationNs(read_events[cu][flag]);
#endif
            uint32_t bufIdx = 0;
            for (uint32_t bIdx = 0, idx = 0; bIdx < blocksPerChunk[brick_flag_idx];
                 bIdx++, idx += block_size_in_bytes) {
                uint32_t block_size = m_blkSize[cu][flag].data()[bIdx];
                uint32_t compressed_size = m_compressSize[cu][flag].data()[bIdx];
                if (compressed_size < block_size) {
                    std::memcpy(&out[outIdx], &h_buf_out[cu][flag].data()[bufIdx], block_size);
                    outIdx += block_size;
                    bufIdx += block_size_in_bytes;
                    total_decompression_size += block_size;
                } else if (compressed_size == block_size) {
                    outIdx += block_size;
                    total_decompression_size += block_size;
                }
            } // For loop ends here
        }     // End of multiple CUs
    }         // End of leftover bricks
    // Delete device buffers

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)original_size * 1000 / total_time_ns.count();
#ifdef EVENT_PROFILE
    std::cout << "Total Kernel Time " << total_kernel_time << std::endl;
    std::cout << "Total Write Time " << total_write_time << std::endl;
    std::cout << "Total Read Time " << total_read_time << std::endl;
#endif
    if (file_list_flag == 0) {
        std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }

    for (uint32_t dBuf = 0; dBuf < D_COMPUTE_UNIT; dBuf++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            if (buffer_input[dBuf][flag]) {
                delete buffer_input[dBuf][flag];
                buffer_input[dBuf][flag] = nullptr;
            }
            if (buffer_output[dBuf][flag]) {
                delete buffer_output[dBuf][flag];
                buffer_output[dBuf][flag] = nullptr;
            }
            if (buffer_compressed_size[dBuf][flag]) {
                delete buffer_compressed_size[dBuf][flag];
                buffer_compressed_size[dBuf][flag] = nullptr;
            }
            if (buffer_block_size[dBuf][flag]) {
                delete buffer_block_size[dBuf][flag];
                buffer_block_size[dBuf][flag] = nullptr;
            }
        }
    }
    return original_size;
} // Decompress Overlap

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint64_t xfLz4::compress(
    uint8_t* in, uint8_t* out, uint64_t input_size, uint32_t host_buffer_size, bool file_list_flag) {
    // printf("host_buffer_size %d \n", host_buffer_size);
    uint32_t max_num_blks = (host_buffer_size) / (m_BlockSizeInKb * 1024);

    for (uint32_t i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (uint32_t j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            MEM_ALLOC_CHECK(h_buf_in[i][j].resize(host_buffer_size), host_buffer_size, "Input Host Buffer");
            MEM_ALLOC_CHECK(h_buf_out[i][j].resize(host_buffer_size), host_buffer_size, "Output Host Buffer");
            MEM_ALLOC_CHECK(h_blksize[i][j].resize(max_num_blks), max_num_blks, "MaxNumBlocks Host Buffer");
            MEM_ALLOC_CHECK(h_compressSize[i][j].resize(max_num_blks), max_num_blks, "CompressSize Host Buffer");

            MEM_ALLOC_CHECK(m_compressSize[i][j].resize(max_num_blks), max_num_blks, "CompressSize(m) Host Buffer");
            MEM_ALLOC_CHECK(m_blkSize[i][j].resize(max_num_blks), max_num_blks, "BlockSize(m) Host Buffer");
        }
    }

    uint32_t block_size_in_bytes = m_BlockSizeInKb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    uint64_t total_kernel_time = 0;
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
    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            // Input:- This buffer contains input chunk data
            buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    host_buffer_size, h_buf_in[cu][flag].data());

            // Output:- This buffer contains compressed data written by device
            buffer_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
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
    uint32_t lcl_cu = 0;

    // Main loop of overlap execution
    // Loop below runs over total bricks i.e., host buffer size chunks
    auto total_start = std::chrono::high_resolution_clock::now();
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; brick += C_COMPUTE_UNIT, itr++, flag = !flag) {
        //  	printf("Brick %u started\n", brick);
        lcl_cu = C_COMPUTE_UNIT;
        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;
        // Loop below runs over number of compute units
        for (uint32_t cu = 0; cu < lcl_cu; cu++) {
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

                    int orig_chunk_size = sizeOfChunk[brick_flag_idx];
                    int perc_cal = orig_chunk_size * 10;
                    perc_cal = perc_cal / block_size;

                    // If compressed size is less than original block size
                    // It means better to dump encoded bytes
                    if (compressed_size < block_size && perc_cal >= 10) {
                        std::memcpy(&out[outIdx], &compressed_size, 4);
                        outIdx += 4;
                        std::memcpy(&out[outIdx], (h_buf_out[cu][flag]).data() + bIdx * block_size_in_bytes,
                                    compressed_size);
                        outIdx += compressed_size;
                    } else {
                        if (block_size == block_size_in_bytes) {
                            out[outIdx++] = 0;
                            out[outIdx++] = 0;

                            if (block_size == lz4_specs::MAX_BSIZE_64KB)
                                out[outIdx++] = lz4_specs::BSIZE_NCOMP_64;
                            else if (block_size == lz4_specs::MAX_BSIZE_256KB)
                                out[outIdx++] = lz4_specs::BSIZE_NCOMP_256;
                            else if (block_size == lz4_specs::MAX_BSIZE_1024KB)
                                out[outIdx++] = lz4_specs::BSIZE_NCOMP_1024;
                            else if (block_size == lz4_specs::MAX_BSIZE_4096KB)
                                out[outIdx++] = lz4_specs::BSIZE_NCOMP_4096;

                            out[outIdx++] = lz4_specs::NO_COMPRESS_BIT;
                        } else {
                            std::memcpy(&out[outIdx], &block_size, 3);
                            outIdx += 3;
                            out[outIdx++] = lz4_specs::NO_COMPRESS_BIT;
                        }
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
            uint32_t narg = 0;
            compress_kernel_lz4[cu]->setArg(narg++, *(buffer_input[cu][flag]));
            compress_kernel_lz4[cu]->setArg(narg++, *(buffer_output[cu][flag]));
            compress_kernel_lz4[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
            compress_kernel_lz4[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
            compress_kernel_lz4[cu]->setArg(narg++, m_BlockSizeInKb);
            compress_kernel_lz4[cu]->setArg(narg++, sizeOfChunk[brick + cu]);

            // Transfer data from host to device
            m_q->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_block_size[cu][flag])}, 0, NULL,
                                          &(write_events[cu][flag]));

            // Kernel wait events for writing & compute
            std::vector<cl::Event> kernelWriteWait;
            std::vector<cl::Event> kernelComputeWait;

            // Kernel Write events update
            kernelWriteWait.push_back(write_events[cu][flag]);

            // Fire the kernel
            m_q->enqueueTask(*compress_kernel_lz4[cu], &kernelWriteWait, &(kernel_events[cu][flag]));
            // Update kernel events flag on computation
            kernelComputeWait.push_back(kernel_events[cu][flag]);

            // Transfer data from device to host
            m_q->enqueueMigrateMemObjects({*(buffer_output[cu][flag]), *(buffer_compressed_size[cu][flag])},
                                          CL_MIGRATE_MEM_OBJECT_HOST, &kernelComputeWait, &(read_events[cu][flag]));
        } // Compute unit loop ends here

    } // Main loop ends here
    m_q->flush();
    m_q->finish();

    uint32_t leftover = total_chunks - completed_bricks;
    uint32_t stride = 0;

    if ((total_chunks < overlap_buf_count * C_COMPUTE_UNIT))
        stride = overlap_buf_count * C_COMPUTE_UNIT;
    else
        stride = total_chunks;

    // Handle leftover bricks
    for (uint32_t ovr_itr = 0, brick = stride - overlap_buf_count * C_COMPUTE_UNIT; ovr_itr < leftover;
         ovr_itr += C_COMPUTE_UNIT, brick += C_COMPUTE_UNIT) {
        lcl_cu = C_COMPUTE_UNIT;
        if (ovr_itr + lcl_cu > leftover) lcl_cu = leftover - ovr_itr;

        // Handle multiple bricks with multiple CUs
        for (uint32_t j = 0; j < lcl_cu; j++) {
            int cu = cu_order[brick + j];
            int flag = chunk_flags[brick + j];
            // Run over each block of the within brick
            uint32_t index = 0;
            uint32_t brick_flag_idx = brick + j;

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

                int orig_chunk_size = sizeOfChunk[brick_flag_idx];
                int perc_cal = orig_chunk_size * 10;
                perc_cal = perc_cal / block_size;

                // If compressed size is less than original block size
                // It means better to dump encoded bytes
                if (compressed_size < block_size && perc_cal >= 10) {
                    std::memcpy(&out[outIdx], &compressed_size, 4);
                    outIdx += 4;
                    std::memcpy(&out[outIdx], &h_buf_out[cu][flag].data()[bIdx * block_size_in_bytes], compressed_size);
                    outIdx += compressed_size;
                } else {
                    if (block_size == block_size_in_bytes) {
                        out[outIdx++] = 0;
                        out[outIdx++] = 0;

                        if (block_size == lz4_specs::MAX_BSIZE_64KB)
                            out[outIdx++] = lz4_specs::BSIZE_NCOMP_64;
                        else if (block_size == lz4_specs::MAX_BSIZE_256KB)
                            out[outIdx++] = lz4_specs::BSIZE_NCOMP_256;
                        else if (block_size == lz4_specs::MAX_BSIZE_1024KB)
                            out[outIdx++] = lz4_specs::BSIZE_NCOMP_1024;
                        else if (block_size == lz4_specs::MAX_BSIZE_4096KB)
                            out[outIdx++] = lz4_specs::BSIZE_NCOMP_4096;

                        out[outIdx++] = lz4_specs::NO_COMPRESS_BIT;
                    } else {
                        uint8_t temp = 0;
                        temp = block_size;
                        out[outIdx++] = temp;
                        temp = block_size >> 8;
                        out[outIdx++] = temp;
                        temp = block_size >> 16;
                        out[outIdx++] = temp;
                        out[outIdx++] = lz4_specs::NO_COMPRESS_BIT;
                    }
                    std::memcpy(&out[outIdx], &in[brick_flag_idx * host_buffer_size + index], block_size);
                    outIdx += block_size;
                } // End of else - uncompressed stream update

            } // For loop ends
        }     // cu loop ends here
    }         // Main loop ends here

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
#ifdef EVENT_PROFILE
    std::cout << "Total Kernel Time " << total_kernel_time << std::endl;
    std::cout << "Total Write Time " << total_write_time << std::endl;
    std::cout << "Total Read Time " << total_read_time << std::endl;
#endif
    if (file_list_flag == 0) {
        std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }

    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            if (buffer_input[cu][flag]) {
                delete buffer_input[cu][flag];
                buffer_input[cu][flag] = nullptr;
            }
            if (buffer_output[cu][flag]) {
                delete buffer_output[cu][flag];
                buffer_output[cu][flag] = nullptr;
            }
            if (buffer_compressed_size[cu][flag]) {
                delete buffer_compressed_size[cu][flag];
                buffer_compressed_size[cu][flag] = nullptr;
            }
            if (buffer_block_size[cu][flag]) {
                delete buffer_block_size[cu][flag];
                buffer_block_size[cu][flag] = nullptr;
            }
        }
    }

    return outIdx;
} // Overlap end
