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
#include "xil_snappy_streaming.hpp"
#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

uint64_t xfSnappyStreaming::getEventDurationNs(const cl::Event& event) {
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
xfSnappyStreaming::xfSnappyStreaming(const std::string& binaryFile, uint8_t flow) {
    m_bin_flow = flow;
    // Index calculation
    h_buf_in.resize(HOST_BUFFER_SIZE);
    h_buf_out.resize(HOST_BUFFER_SIZE);
    h_blksize.resize(MAX_NUMBER_BLOCKS);
    h_compressSize.resize(MAX_NUMBER_BLOCKS);

    m_compressSize.reserve(MAX_NUMBER_BLOCKS);
    m_blkSize.reserve(MAX_NUMBER_BLOCKS);

    unsigned fileBufSize;
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
        compress_kernel_snappy = new cl::Kernel(*m_program, compress_kernel_name.c_str());
        // Create Compress datamover kernels
        compress_data_mover_kernel = new cl::Kernel(*m_program, compress_dm_kernel_name.c_str());
    } else {
        // Create Decompress kernels
        decompress_kernel_snappy = new cl::Kernel(*m_program, decompress_kernel_name.c_str());
        // Create Decompress datamover kernels
        decompress_data_mover_kernel = new cl::Kernel(*m_program, decompress_dm_kernel_name.c_str());
    }
}

// Destructor
xfSnappyStreaming::~xfSnappyStreaming() {
    if (m_bin_flow) {
        delete (compress_kernel_snappy);
        delete (compress_data_mover_kernel);
    } else {
        delete (decompress_kernel_snappy);
        delete (decompress_data_mover_kernel);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);
}

uint64_t xfSnappyStreaming::compressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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

        /*uint32_t host_buffer_size = HOST_BUFFER_SIZE;
        uint32_t acc_buff_size = m_block_size_in_kb * 1024 * PARALLEL_BLOCK;
        if (acc_buff_size > host_buffer_size) {
            host_buffer_size = acc_buff_size;
        }
        if (host_buffer_size > input_size) {
            host_buffer_size = input_size;
        }
        if (host_buffer_size > HOST_BUFFER_SIZE) {
            host_buffer_size = HOST_BUFFER_SIZE;
        }*/

        // Snappy Sequential compress
        uint64_t enbytes = compress(in.data(), out.data(), input_size);

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

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint64_t xfSnappyStreaming::compress(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;
    uint32_t total_block_count = (input_size - 1) / host_buffer_size + 1;

    // Index calculation
    h_buf_in.resize(host_buffer_size * total_block_count);
    h_buf_out.resize(host_buffer_size * total_block_count);
    h_compressSize.resize(total_block_count);

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

        compress_kernel_snappy->setArg(2, c_input_size);
        // Migrate Memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects({*(buffer_input)}, 0);
        m_q->finish();

        // Measure kernel execution time
        auto kernel_start = std::chrono::high_resolution_clock::now();

        // enqueue the kernels and wait for them to finish
        m_q->enqueueTask(*compress_data_mover_kernel);
        m_q->enqueueTask(*compress_kernel_snappy);
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
            out[outIdx++] = 0x00;

            // 3 Bytes to represent compressed block length + 4
            uint32_t f_csize = compressedSize + 4;
            std::memcpy(out + outIdx, &f_csize, 3);
            outIdx += 3;

            // CRC - for now 0s
            uint32_t crc_value = 0;
            std::memcpy(out + outIdx, &crc_value, 4);
            outIdx += 4;

            std::memcpy(out + outIdx, h_buf_out.data() + i * host_buffer_size, compressedSize);
            outIdx += compressedSize;
        } else {
            // Chunk Type Identifier
            out[outIdx++] = 0x01;
            // 3 Bytes to represent uncompress block length + 4;
            uint32_t f_csize = c_input_size + 4;
            std::memcpy(out + outIdx, &f_csize, 3);
            outIdx += 3;
            // CRC -for now 0s
            uint32_t crc_value = 0;
            std::memcpy(out + outIdx, &crc_value, 4);
            outIdx += 4;

            std::memcpy(out + outIdx, in + (host_buffer_size * i), c_input_size);
            outIdx += c_input_size;
        }
    }
    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    return outIdx;
}

uint64_t xfSnappyStreaming::decompressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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

        // Decompression Sequential multiple cus.
        uint64_t debytes = decompress(in.data(), out.data(), (input_size - 10));
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

uint64_t xfSnappyStreaming::decompress(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;

    // Index calculation
    h_buf_in.resize(host_buffer_size);
    h_buf_out.resize(host_buffer_size);

    uint32_t compSize;
    uint32_t decompSize;

    // uint64_t cIdx = 0;
    uint32_t chunk_size = 0;
    uint8_t chunk_idx = 0;
    uint32_t block_size = 0;
    uint32_t bufIdx = 0;
    uint16_t stride_cidsize = 4;

    uint64_t total_decompressed_size = 0;
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    for (uint32_t cIdx = 0; cIdx < input_size; cIdx += stride_cidsize) {
        // Chunk identifier
        chunk_idx = in[cIdx];
        chunk_size = 0;

        // Chunk Compressed size
        uint8_t cbyte_1 = in[cIdx + 1];
        uint8_t cbyte_2 = in[cIdx + 2];
        uint8_t cbyte_3 = in[cIdx + 3];

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
            uint8_t bval1 = in[cIdx + 8];
            uint32_t final_size = 0;

            if ((bval1 >> 7) == 1) {
                uint8_t b1 = bval1 & 0x7F;
                bval1 = in[cIdx + 9];
                uint8_t b2 = bval1 & 0x7F;
                if ((bval1 >> 7) == 1) {
                    bval1 = in[cIdx + 10];
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
            compSize = chunk_size - 4;
            decompSize = block_size;

            std::memcpy(h_buf_in.data(), in + cIdx + 8, compSize);
            // Device buffer allocation
            buffer_input =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, compSize, h_buf_in.data());

            buffer_output =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, decompSize, h_buf_out.data());

            // set kernel arguments
            int narg = 0;
            decompress_data_mover_kernel->setArg(narg++, *(buffer_input));
            decompress_data_mover_kernel->setArg(narg++, *(buffer_output));
            decompress_data_mover_kernel->setArg(narg++, decompSize);
            decompress_data_mover_kernel->setArg(narg, compSize);

            decompress_kernel_snappy->setArg(2, compSize);
            decompress_kernel_snappy->setArg(3, decompSize);

            // Migrate Memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects({*(buffer_input)}, 0);
            m_q->finish();

            auto kernel_start = std::chrono::high_resolution_clock::now();

            // enqueue the kernels and wait for them to finish
            m_q->enqueueTask(*decompress_data_mover_kernel);
            m_q->enqueueTask(*decompress_kernel_snappy);
            m_q->finish();

            auto kernel_end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
            kernel_time_ns_1 += duration;

            // Migrate memory - Map device to host buffers
            m_q->enqueueMigrateMemObjects({*(buffer_output)}, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();

            std::memcpy(out + bufIdx, h_buf_out.data(), decompSize);
            bufIdx += decompSize;
        } else if (chunk_idx == 0x01) {
            compSize = chunk_size - 4;
            decompSize = chunk_size - 4;
            std::memcpy(out + bufIdx, in + cIdx + 8, compSize);
        }

        cIdx += chunk_size;
        total_decompressed_size += decompSize;
    }
    float throughput_in_mbps_1 = (float)total_decompressed_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    return total_decompressed_size;
}
