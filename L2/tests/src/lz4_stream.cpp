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
#include "lz4_stream.hpp"
#include "xxhash.h"

#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

uint64_t xfLz4Streaming::getEventDurationNs(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    return (end_time - start_time);
}

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

// Constructor
xfLz4Streaming::xfLz4Streaming(const std::string& binaryFile, uint8_t flow) {
    m_bin_flow = flow;
    // Index calculation
    h_buf_in.resize(BLOCK_SIZE_IN_KB * 1024);
    h_buf_out.resize(BLOCK_SIZE_IN_KB * 1024);
    // h_blksize.resize(1);
    // h_compressSize.resize(1);

    // m_compressSize.reserve1);
    m_blkSize.reserve(1);

    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    m_device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(m_device);
    m_q =
        new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = m_device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);

    m_program = new cl::Program(*m_context, devices, bins);

    if (m_bin_flow) {
        // Create Compress kernels
        compress_kernel_lz4 = new cl::Kernel(*m_program, compress_kernel_name.c_str());
        // Create Compress datamover kernels
        compress_data_mover_kernel = new cl::Kernel(*m_program, compress_dm_kernel_name.c_str());
    } else {
        // Create Decompress kernels
        decompress_kernel_lz4 = new cl::Kernel(*m_program, decompress_kernel_name.c_str());
        // Create Decompress datamover kernels
        decompress_data_mover_kernel = new cl::Kernel(*m_program, decompress_dm_kernel_name.c_str());
    }
}

// Destructor
xfLz4Streaming::~xfLz4Streaming() {
    if (m_bin_flow) {
        delete (compress_kernel_lz4);
        delete (compress_data_mover_kernel);
    } else {
        delete (decompress_kernel_lz4);
        delete (decompress_data_mover_kernel);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);
}

uint64_t xfLz4Streaming::compressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    if (m_switch_flow == 0) { // Xilinx FPGA compression flow
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size);

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
        switch (m_block_size_in_kb) {
            case 64:
                outFile.put(BSIZE_STD_64KB);
                block_size_header = BSIZE_STD_64KB;
                break;
            case 256:
                outFile.put(BSIZE_STD_256KB);
                block_size_header = BSIZE_STD_256KB;
                break;
            case 1024:
                outFile.put(BSIZE_STD_1024KB);
                block_size_header = BSIZE_STD_1024KB;
                break;
            case 4096:
                outFile.put(BSIZE_STD_4096KB);
                block_size_header = BSIZE_STD_4096KB;
                break;
            default:
                std::cout << "Invalid Block Size" << std::endl;
                break;
        }

        // uint32_t host_buffer_size = (m_block_size_in_kb * 1024) * 32;
        // if ((m_block_size_in_kb * 1024) > input_size) host_buffer_size = m_block_size_in_kb * 1024;

        uint8_t temp_buff[10] = {FLG_BYTE,         block_size_header, input_size,       input_size >> 8,
                                 input_size >> 16, input_size >> 24,  input_size >> 32, input_size >> 40,
                                 input_size >> 48, input_size >> 56};

        // xxhash is used to calculate hash value
        uint32_t xxh = XXH32(temp_buff, 10, 0);
        uint64_t enbytes;
        outFile.write((char*)&temp_buff[2], 8);

        // Header CRC
        outFile.put((uint8_t)(xxh >> 8));
        // LZ4 streaming compression
        enbytes = compressStream(in.data(), out.data(), input_size);
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
        std::string command = "../../../common/lz4/lz4 --content-size -f -q " + inFile_name;
        system(command.c_str());
        std::string output = inFile_name + ".lz4";
        std::string rout = inFile_name + ".std.lz4";
        std::string rename = "mv " + output + " " + rout;
        system(rename.c_str());
        return 0;
    }
}

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xfLz4Streaming::compressStream(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;
    uint32_t total_block_count = (input_size - 1) / host_buffer_size + 1;

    // Index calculation
    h_buf_in.resize(host_buffer_size * total_block_count);
    h_buf_out.resize(host_buffer_size * total_block_count);
    h_compressSize.resize(total_block_count);

    // m_compressSize.reserve(total_block_count);

    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // copy input to input buffer
    std::memcpy(h_buf_in.data(), in, input_size);
    // sequentially copy block sized buffers to kernel and wait for them to finish before enqueueing
    for (uint32_t blkIndx = 0, bufIndx = 0; blkIndx < total_block_count; blkIndx++, bufIndx += host_buffer_size) {
        // current block input size
        uint32_t c_input_size = host_buffer_size;
        if (blkIndx == total_block_count - 1) c_input_size = input_size - bufIndx;

        // device buffer allocation
        buffer_input =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, c_input_size, h_buf_in.data() + bufIndx);

        buffer_output = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, c_input_size,
                                       h_buf_out.data() + bufIndx);

        buffer_compressed_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t),
                                                h_compressSize.data() + blkIndx);
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

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // enqueue the kernels and wait for them to finish
        m_q->enqueueTask(*compress_data_mover_kernel);
        m_q->enqueueTask(*compress_kernel_lz4);
        m_q->finish();

        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;

        // Setup output buffer vectors
        std::vector<cl::Memory> outBufVec;
        outBufVec.push_back(*buffer_output);
        outBufVec.push_back(*buffer_compressed_size);

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        // Free CL buffers
        delete (buffer_input);
        delete (buffer_output);
        delete (buffer_compressed_size);
    }
    // read the data to output buffer
    uint64_t outIdx = 0;
    for (uint64_t i = 0; i < total_block_count; ++i) {
        // copy the compressed data to out pointer
        uint32_t compressedSize = h_compressSize.data()[i];
        // std::cout << "Compressed size: " << compressedSize << std::endl;
        // current block input size
        uint32_t c_input_size = host_buffer_size;
        if (i == total_block_count - 1) c_input_size = input_size - (host_buffer_size * i);

        if (c_input_size > compressedSize) {
            // copy the compressed data
            std::memcpy(out + outIdx, &compressedSize, 4);
            outIdx += 4;
            std::memcpy(out + outIdx, h_buf_out.data() + i * host_buffer_size, compressedSize);
            outIdx += compressedSize;
        } else {
            // copy the original data, since no compression
            if (c_input_size == host_buffer_size) {
                out[outIdx++] = 0;
                out[outIdx++] = 0;

                switch (c_input_size) {
                    case MAX_BSIZE_64KB:
                        out[outIdx++] = BSIZE_NCOMP_64;
                        break;
                    case MAX_BSIZE_256KB:
                        out[outIdx++] = BSIZE_NCOMP_256;
                        break;
                    case MAX_BSIZE_1024KB:
                        out[outIdx++] = BSIZE_NCOMP_1024;
                        break;
                    case MAX_BSIZE_4096KB:
                        out[outIdx++] = BSIZE_NCOMP_4096;
                        break;
                    default:
                        out[outIdx++] = BSIZE_NCOMP_64;
                        break;
                }

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
            std::memcpy(out + outIdx, in + (host_buffer_size * i), c_input_size);
            outIdx += c_input_size;
        }
    }

    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    return outIdx;
}

uint64_t xfLz4Streaming::decompressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    if (m_switch_flow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);

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
            case BSIZE_STD_64KB:
                m_block_size_in_kb = 64;
                break;
            case BSIZE_STD_256KB:
                m_block_size_in_kb = 256;
                break;
            case BSIZE_STD_1024KB:
                m_block_size_in_kb = 1024;
                break;
            case BSIZE_STD_4096KB:
                m_block_size_in_kb = 4096;
                break;
            default:
                std::cout << "Invalid Block Size" << std::endl;
                break;
        }
        // printf("m_block_size_in_kb %d \n", m_block_size_in_kb);

        // Original size
        uint64_t original_size = 0;
        inFile.read((char*)&original_size, 8);
        inFile.get(c);
        // printf("original_size %d \n", original_size);

        // Allocat output size
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(original_size);

        // Read block data from compressed stream .lz4
        inFile.read((char*)in.data(), (input_size - 15));

        // uint32_t host_buffer_size = (m_block_size_in_kb * 1024) * 32;

        // if ((m_block_size_in_kb * 1024) > original_size) host_buffer_size = m_block_size_in_kb * 1024;

        uint64_t debytes;
        // Decompression Streaming
        debytes = decompressStream(in.data(), out.data(), (input_size - 15), original_size);
        outFile.write((char*)out.data(), debytes);
        // Close file
        inFile.close();
        outFile.close();
        return debytes;
    } else {
        std::string command = "../../../common/lz4/lz4 --content-size -f -q -d " + inFile_name;
        system(command.c_str());
        return 0;
    }
}

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xfLz4Streaming::decompressStream(uint8_t* in, uint8_t* out, uint64_t input_size, uint64_t original_size) {
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;
    uint32_t total_block_count = (original_size - 1) / host_buffer_size + 1;

    // Index calculation
    h_buf_in.resize(host_buffer_size * total_block_count);
    h_buf_out.resize(host_buffer_size * total_block_count);
    h_compressSize.resize(total_block_count);

    // m_compressSize.reserve(total_block_count);
    // m_blkSize.reserve(total_block_count);

    // uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    // uint32_t block_size_in_bytes = block_size_in_kb * 1024;

    uint64_t cIdx = 0;
    uint64_t total_decompressed_size = 0;
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    for (uint64_t buf_indx = 0, blk_idx = 0; buf_indx < original_size; buf_indx += host_buffer_size, ++blk_idx) {
        // get the size of the compressed block
        uint32_t compressedSize = 0;

        std::memcpy(&compressedSize, in + cIdx, 4);
        cIdx += 4;

        uint32_t tmp = compressedSize;
        tmp >>= 24;

        if (tmp == 128) {
            uint8_t b1 = compressedSize;
            uint8_t b2 = compressedSize >> 8;
            uint8_t b3 = compressedSize >> 16;
            // uint8_t b4 = compressedSize >> 24;

            if (b3 == 1) {
                compressedSize = host_buffer_size;
            } else {
                uint16_t size = 0;
                size = b2;
                size <<= 8;
                uint16_t temp = b1;
                size |= temp;
                compressedSize = size;
            }
        }
        // decompressed block input size
        uint32_t dBlockSize = host_buffer_size;
        if (blk_idx == total_block_count - 1) dBlockSize = original_size - (host_buffer_size * blk_idx);

        // If compressed size is less than original block size
        if (compressedSize < dBlockSize) {
            // copy data to buffer
            std::memcpy(h_buf_in.data() + buf_indx, in + cIdx, compressedSize);
            // Device buffer allocation
            buffer_input = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, compressedSize,
                                          h_buf_in.data() + buf_indx);

            buffer_output = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, dBlockSize,
                                           h_buf_out.data() + buf_indx);

            // set kernel arguments
            int narg = 0;
            decompress_data_mover_kernel->setArg(narg++, *(buffer_input));
            decompress_data_mover_kernel->setArg(narg++, *(buffer_output));
            decompress_data_mover_kernel->setArg(narg++, dBlockSize);
            decompress_data_mover_kernel->setArg(narg, compressedSize);

            decompress_kernel_lz4->setArg(2, compressedSize);
            decompress_kernel_lz4->setArg(3, dBlockSize);

            // Migrate Memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects({*(buffer_input)}, 0);
            m_q->finish();

            auto kernel_start = std::chrono::high_resolution_clock::now();

            // enqueue the kernels and wait for them to finish
            m_q->enqueueTask(*decompress_data_mover_kernel);
            m_q->enqueueTask(*decompress_kernel_lz4);
            m_q->finish();

            auto kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects({*(buffer_output)}, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();

            // copy data to output
            std::memcpy(out + buf_indx, h_buf_out.data() + buf_indx, dBlockSize);
            cIdx += compressedSize;

            // free buffers
            delete (buffer_input);
            delete (buffer_output);
            // std::cout << "Compressed size: " << compressedSize << std::endl;
        } else if (compressedSize == dBlockSize) {
            // no compression, copy as it is to output
            std::memcpy(out + buf_indx, in + cIdx, dBlockSize);
            cIdx += dBlockSize;
        } else {
            assert(0);
        }
        total_decompressed_size += dBlockSize;
    }
    float throughput_in_mbps_1 = (float)total_decompressed_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    return original_size;
} // End of decompress
