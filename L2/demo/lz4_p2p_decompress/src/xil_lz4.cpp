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
#include "xil_lz4.hpp"
#include "xxhash.h"

#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

int fd_p2p_c_out = 0;
int fd_p2p_c_in = 0;

uint64_t xfLz4::get_event_duration_ns(const cl::Event& event) {
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

void xfLz4::bufferExtensionAssignments(bool flow) {
    for (uint32_t i = 0; i < OVERLAP_BUF_COUNT; i++) {
        if (flow) {
#if 0
                inExt[i][j].flags = comp_ddr_nums[i];
                inExt[i][j].obj   = h_buf_in[i][j].data();
                inExt[i][j].param   = 0;
                
                outExt[i][j].flags = comp_ddr_nums[i];
                outExt[i][j].obj   = h_buf_out[i][j].data();
                outExt[i][j].param   = 0;
                
                csExt[i][j].flags = comp_ddr_nums[i];
                csExt[i][j].obj   = h_compressSize[i][j].data();
                csExt[i][j].param   = 0;
                
                bsExt[i][j].flags = comp_ddr_nums[i];
                bsExt[i][j].obj   = h_blksize[i][j].data();
                bsExt[i][j].param   = 0;
#endif
            printf("compression flow\n");
        } else {
            inExt[i].flags = decomp_ddr_nums[i];
            inExt[i].obj = h_buf_in[i].data();
            inExt[i].param = 0;

            for (uint32_t j = 0; j < MAX_COMPUTE_UNITS; j++) {
                outExt[i][j].flags = decomp_ddr_nums[j];
                outExt[i][j].obj = h_buf_out[i][j].data();
                outExt[i][j].param = 0;
            }

            pbExt[i].flags = decomp_ddr_nums[0];
            pbExt[i].obj = h_no_blocks[i].data();
            pbExt[i].param = 0;

            siExt[i].flags = decomp_ddr_nums[0];
            siExt[i].obj = h_in_start_index[i].data();
            siExt[i].param = 0;

            osExt[i].flags = decomp_ddr_nums[0];
            osExt[i].obj = h_original_size[i].data();
            osExt[i].param = 0;

            bsiExt[i].flags = decomp_ddr_nums[0];
            bsiExt[i].obj = h_block_start_idx[i].data();
            bsiExt[i].param = 0;

            pbpcExt[i].flags = decomp_ddr_nums[0];
            pbpcExt[i].obj = h_no_blocks_per_cu[i].data();
            pbpcExt[i].param = 0;

            csExt[i].flags = decomp_ddr_nums[0];
            csExt[i].obj = h_compressSize[i].data();
            csExt[i].param = 0;

            bsExt[i].flags = decomp_ddr_nums[0];
            bsExt[i].obj = h_blksize[i].data();
            bsExt[i].param = 0;
        }
    }
}

// Constructor
xfLz4::xfLz4(const std::string& binaryFile) {
    m_block_size_in_kb = BLOCK_SIZE_IN_KB;
    host_buffer_size = HOST_BUFFER_SIZE;

    for (uint32_t i = 0; i < OVERLAP_BUF_COUNT; i++) {
        // Index calculation
        h_buf_in[i].resize(MAX_IN_BUFFER_SIZE);
        h_blksize[i].resize(MAX_NUMBER_BLOCKS * MAX_COMPUTE_UNITS);
        h_compressSize[i].resize(MAX_NUMBER_BLOCKS * MAX_COMPUTE_UNITS);

        for (uint32_t j = 0; j < MAX_COMPUTE_UNITS; j++) {
            m_compressSize[i][j].reserve(MAX_NUMBER_BLOCKS);
            m_blkSize[i][j].reserve(MAX_NUMBER_BLOCKS);
            h_buf_out[i][j].resize(HOST_BUFFER_SIZE);
        }
    }

    max_num_blks = (host_buffer_size) / (m_block_size_in_kb * 1024);

    unsigned fileBufSize;
    std::cout << "Binary: " << binaryFile << std::endl;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    for (uint8_t flag = 0; flag < OVERLAP_BUF_COUNT; flag++) {
        m_q[flag] = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    }
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
        // Create Decompress kernels

        unpacker_kernel_lz4 = new cl::Kernel(*m_program, unpacker_kernel_names[0].c_str());
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++)
            decompress_kernel_lz4[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
    } else {
        // Create Decompress kernels
        unpacker_kernel_lz4 = new cl::Kernel(*m_program, unpacker_kernel_names[0].c_str());
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++)
            decompress_kernel_lz4[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
    }

    for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) {
        for (uint32_t j = 0; j < OVERLAP_BUF_COUNT; j++) {
            h_buf_out[i][j].resize(host_buffer_size);
        }
    }

    h_no_blocks[0].resize(MAX_COMPUTE_UNITS * OVERLAP_BUF_COUNT);
    h_original_size[0].resize(MAX_COMPUTE_UNITS * OVERLAP_BUF_COUNT);
    h_no_blocks_per_cu[0].resize(MAX_COMPUTE_UNITS);
    for (uint8_t bufc = 0; bufc < OVERLAP_BUF_COUNT; bufc++) {
        h_in_start_index[bufc].resize(MAX_COMPUTE_UNITS * OVERLAP_BUF_COUNT);
        h_blksize[bufc].resize(MAX_COMPUTE_UNITS * max_num_blks);
        h_compressSize[bufc].resize(MAX_COMPUTE_UNITS * max_num_blks);
        h_block_start_idx[bufc].resize(MAX_COMPUTE_UNITS * max_num_blks);
    }

    bufferExtensionAssignments(0);
    uint32_t buf_size_in = MAX_IN_BUFFER_SIZE;
    buf_size_in_partitions = MAX_IN_BUFFER_PARTITION;
    uint32_t buf_size_out[D_COMPUTE_UNIT] = {0};

    for (uint32_t cuProc = 0; cuProc < D_COMPUTE_UNIT; cuProc++) {
        buf_size_out[cuProc] = host_buffer_size;
    }

    for (uint32_t cu = 0; cu < 1; cu++) {
        for (uint8_t overlap_buffer = 0; overlap_buffer < OVERLAP_BUF_COUNT; overlap_buffer++) {
            inExt[0].flags |= XCL_MEM_EXT_P2P_BUFFER;
            inExt[0].obj = nullptr;
            buffer_input[0] =
                new cl::Buffer(*m_context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, buf_size_in, &inExt[0]);

            // for (uint32_t sbuf = 0; sbuf < buf_size_in_partitions; sbuf++) {
            //    h_buf_in_p2p[sbuf] = (uint8_t*)m_q[0]->enqueueMapBuffer(*(buffer_input[0]), CL_TRUE, CL_MAP_WRITE,
            //                                                            sbuf * host_buffer_size, host_buffer_size);
            //}

            buffer_no_blocks[0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t), &pbExt[0]);

            buffer_in_start_index[0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t), &siExt[0]);

            buffer_original_size[0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t), &osExt[0]);

            buffer_compressed_size[overlap_buffer] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * D_COMPUTE_UNIT * max_num_blks, &csExt[overlap_buffer]);

            buffer_block_size[overlap_buffer] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * D_COMPUTE_UNIT * max_num_blks, &bsExt[overlap_buffer]);

            buffer_block_start_idx[overlap_buffer] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * D_COMPUTE_UNIT * max_num_blks, &bsiExt[overlap_buffer]);

            buffer_no_blocks_per_cu[0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * D_COMPUTE_UNIT, &pbpcExt[0]);
        }
    }

    for (uint8_t overlap_buffer = 0; overlap_buffer < OVERLAP_BUF_COUNT; overlap_buffer++) {
        for (uint32_t cu = 0; cu < D_COMPUTE_UNIT; cu++) {
            buffer_output[overlap_buffer][cu] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               buf_size_out[cu], &outExt[overlap_buffer][cu]);
            m_q[0]->enqueueMigrateMemObjects({*(buffer_output[overlap_buffer][cu])}, 0); // 0 means from host
            m_q[0]->finish();
        }
    }
}

// Destructor
xfLz4::~xfLz4() {
    int ret = 0;

    if (m_bin_flow) {
        for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel_lz4[i]);
    } else {
        delete (unpacker_kernel_lz4);
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) delete (decompress_kernel_lz4[i]);
    }
    delete (m_program);
    for (uint8_t flag = 0; flag < OVERLAP_BUF_COUNT; flag++) {
        delete (m_q[flag]);
    }
    delete (m_context);
    delete (buffer_in_start_index[0]);
    delete (buffer_input[0]);
    delete (buffer_no_blocks[0]);
    delete (buffer_original_size[0]);
    delete (buffer_no_blocks_per_cu[0]);

    for (uint32_t dBuf = 0; dBuf < 1; dBuf++) {
        for (uint8_t overlap_buffer = 0; overlap_buffer < OVERLAP_BUF_COUNT; overlap_buffer++) {
            delete (buffer_compressed_size[overlap_buffer]);
            delete (buffer_block_size[overlap_buffer]);
            delete (buffer_block_start_idx[overlap_buffer]);
        }
    }
    for (uint8_t overlap_buffer = 0; overlap_buffer < OVERLAP_BUF_COUNT; overlap_buffer++) {
        for (uint32_t bufC = 0; bufC < D_COMPUTE_UNIT; bufC++) {
            delete (buffer_output[overlap_buffer][bufC]);
        }
    }
}

uint64_t xfLz4::decompress_ssd_file(std::string& inFile_name, std::string& outFile_name) {
    uint64_t debytes;
    uint64_t original_size = 0;
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = get_file_size(inFile);
    inFile.close();

    int fd_p2p_c_in = open(inFile_name.c_str(), O_RDONLY | O_DIRECT);
    if (fd_p2p_c_in <= 0) {
        std::cout << "P2P: Unable to open input file, fd: " << fd_p2p_c_in << std::endl;
        exit(1);
    }
    std::vector<uint8_t, aligned_allocator<uint8_t> > in_4kbytes(4 * KB);
    read(fd_p2p_c_in, (char*)in_4kbytes.data(), 4 * KB);
    lseek(fd_p2p_c_in, 0, SEEK_SET);
    std::memcpy(&original_size, &in_4kbytes[6], 4);
    std::vector<uint8_t, aligned_allocator<uint8_t> > out(original_size);
    debytes = p2p_inline_decompress(fd_p2p_c_in, out.data(), input_size, original_size);
    outFile.write((char*)out.data(), debytes);
    close(fd_p2p_c_in);
    outFile.close();
    return debytes;
}

uint64_t xfLz4::p2p_inline_decompress(int fd_p2p_c_in, uint8_t* out, uint64_t input_size, uint64_t original_size) {
    // Input file size supported is <128MB
    assert(input_size < MAX_IN_BUFFER_SIZE);

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    uint64_t inIdx = 0;
    uint64_t total_decomression_size = 0;
    uint32_t compute_cu = D_COMPUTE_UNIT;
    uint8_t total_no_cu = D_COMPUTE_UNIT;
    uint8_t first_chunk = 1;
    uint8_t copy_chunk = 0;
    uint8_t copy_chunk_idx = 0;
    uint8_t migrate_chunk = 0;
    uint32_t max_buffer_size_in_bytes = MAX_IN_BUFFER_SIZE;

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;
    uint32_t block_size_in_bytes = BLOCK_SIZE_IN_KB * 1024;
    uint32_t num_blocks = host_buffer_size / block_size_in_bytes;

    int ret = 0;

    h_in_start_index[0].data()[0] = 0;
    uint32_t in_start_index = h_in_start_index[0].data()[0];

    uint32_t next_in_start_index;
    uint8_t s_idx, e_idx;
    uint64_t out_index = 0;

    int q_id = 0;
    uint32_t itr = 0;
    uint64_t outIdx = 0;
    cl::Event unpacker_read_events[OVERLAP_BUF_COUNT];
    for (uint32_t sbuf = 0; sbuf < buf_size_in_partitions; sbuf++) {
        h_buf_in_p2p[sbuf] = (uint8_t*)m_q[0]->enqueueMapBuffer(*(buffer_input[0]), CL_TRUE, CL_MAP_WRITE,
                                                                sbuf * host_buffer_size, host_buffer_size);
    }
    auto total_start = std::chrono::high_resolution_clock::now();
    for (outIdx = 0; outIdx < original_size; outIdx += host_buffer_size * D_COMPUTE_UNIT, q_id++, itr++) {
        q_id = q_id % OVERLAP_BUF_COUNT;
        compute_cu = 0;
        while ((outIdx + (compute_cu * host_buffer_size)) < original_size && (compute_cu < D_COMPUTE_UNIT))
            compute_cu++;
        // printf("Iter:%d\tFlag:%d\n",itr,q_id);
        for (uint32_t cuProc = 0; cuProc < 1; cuProc++) {
            if (first_chunk) {
                uint32_t sbuf = 0;
                while (inIdx < input_size && sbuf < 2) {
                    ret = read(fd_p2p_c_in, h_buf_in_p2p[sbuf], HOST_BUFFER_SIZE);
                    if (ret == -1)
                        std::cout << "P2P: compress(): read() failed, err: " << ret << ", line: " << __LINE__
                                  << std::endl;
                    inIdx = (inIdx + host_buffer_size < input_size) ? (inIdx + host_buffer_size) : input_size;
                    sbuf++;
                    copy_chunk_idx++;
                }
            } else {
                while (copy_chunk && (inIdx < input_size)) {
                    ret = read(fd_p2p_c_in, h_buf_in_p2p[copy_chunk_idx], HOST_BUFFER_SIZE);
                    if (ret == -1)
                        std::cout << "P2P: compress(): read() failed, err: " << ret << ", line: " << __LINE__
                                  << std::endl;
                    inIdx = (inIdx + host_buffer_size < input_size) ? (inIdx + host_buffer_size) : input_size;
                    copy_chunk--;
                    copy_chunk_idx++;
                }
            }
        }

        // printf("input_size %d inIdx %d\n", input_size, inIdx);
        // printf("set unpacker kernel args\n");
        uint32_t narg = 0;
        unpacker_kernel_lz4->setArg(narg++, *(buffer_input[0]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_block_size[q_id]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_compressed_size[q_id]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_block_start_idx[q_id]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_no_blocks_per_cu[0]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_original_size[0]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_in_start_index[0]));
        unpacker_kernel_lz4->setArg(narg++, *(buffer_no_blocks[0]));
        unpacker_kernel_lz4->setArg(narg++, m_block_size_in_kb);
        unpacker_kernel_lz4->setArg(narg++, first_chunk);
        unpacker_kernel_lz4->setArg(narg++, total_no_cu);
        unpacker_kernel_lz4->setArg(narg++, num_blocks);

        // printf("set decompress kernel args\n");
        for (uint32_t sArg = 0; sArg < compute_cu; sArg++) {
            narg = 0;
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_input[0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_output[q_id][sArg]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_block_size[q_id]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_compressed_size[q_id]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_block_start_idx[q_id]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_no_blocks_per_cu[0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, m_block_size_in_kb);
            decompress_kernel_lz4[sArg]->setArg(narg++, sArg);
            decompress_kernel_lz4[sArg]->setArg(narg++, total_no_cu);
            decompress_kernel_lz4[sArg]->setArg(narg++, num_blocks);
        }
        // printf("host to device\n");
        if (itr >= OVERLAP_BUF_COUNT) {
            m_q[q_id]->finish();
            if (first_chunk) original_size = h_original_size[0].data()[0];

            // printf("original_size %d\n", original_size);
            for (uint32_t cuRead = 0; cuRead < compute_cu; cuRead++) {
                // printf("out_index:%d \n",out_index);
                // printf("outIdx + (cuRead*host_buffer_size) %d \n", outIdx + (cuRead*host_buffer_size));

                if ((out_index + host_buffer_size) < original_size) {
                    std::memcpy(&out[out_index], h_buf_out[q_id][cuRead].data(), host_buffer_size);
                } else {
                    std::memcpy(&out[out_index], h_buf_out[q_id][cuRead].data(), original_size - out_index);
                }
                out_index = out_index + host_buffer_size;
                // temp = temp + host_buffer_size;

                // printf("out_index:%d\n",out_index);
            }
        }
        m_q[q_id]->enqueueMigrateMemObjects({*(buffer_in_start_index[0])}, 0 /* 0 means from host*/);

        auto kernel_start = std::chrono::high_resolution_clock::now();
        // printf("unpacker kernel call\n");
        m_q[q_id]->enqueueTask(*unpacker_kernel_lz4);

        // printf("Migrate data device to host\n");
        m_q[q_id]->enqueueMigrateMemObjects({*(buffer_in_start_index[0])}, CL_MIGRATE_MEM_OBJECT_HOST, nullptr,
                                            &(unpacker_read_events[q_id]));

        // printf("decompress call\n");
        for (uint32_t task = 0; task < compute_cu; task++) {
            m_q[q_id]->enqueueTask(*decompress_kernel_lz4[task]);
        }
        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;
        std::vector<cl::Memory> outBufVec;
        for (uint32_t oVec = 0; oVec < compute_cu; oVec++) {
            outBufVec.push_back(*(buffer_output[q_id][oVec]));
            if (first_chunk) outBufVec.push_back(*(buffer_original_size[0]));
        }
        m_q[q_id]->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        // printf("read to host buffer\n");

        // wait for in_start_index for next iteration
        unpacker_read_events[q_id].wait();

        next_in_start_index = h_in_start_index[0].data()[0];
        // printf("next_in_start_index:%d\n",next_in_start_index);
        migrate_chunk = 0;
        s_idx = in_start_index / host_buffer_size;
        e_idx = next_in_start_index / host_buffer_size;
        // printf("s_idx:%d\t e_idx:%d\n",s_idx,e_idx);
        while (s_idx != e_idx) {
            copy_chunk++;
            s_idx++;
            migrate_chunk = 1;
        }

        in_start_index = h_in_start_index[0].data()[0];
        first_chunk = 0;
    }
    // printf("done...\n");

    uint32_t remaining_itr = (itr < OVERLAP_BUF_COUNT) ? itr : 4;
    q_id = 0;

    for (uint8_t i = 0; i < remaining_itr; i++, q_id++) {
        q_id = q_id % OVERLAP_BUF_COUNT;
        m_q[q_id]->finish();
        for (uint32_t cuRead = 0; cuRead < compute_cu; cuRead++) {
            if ((out_index + host_buffer_size) < original_size) {
                std::memcpy(&out[out_index], h_buf_out[q_id][cuRead].data(), host_buffer_size);
            } else {
                std::memcpy(&out[out_index], h_buf_out[q_id][cuRead].data(), original_size - out_index);
            }
            out_index = out_index + host_buffer_size;
        }
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    for (uint32_t sbuf = 0; sbuf < MAX_IN_BUFFER_PARTITION; sbuf++)
        ret = m_q[0]->enqueueUnmapMemObject(*(buffer_input[0]), h_buf_in_p2p[sbuf], nullptr, nullptr);

    total_decomression_size = original_size;
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)original_size * 1000 / total_time_ns.count();
    float kernel_throughput_in_mbps_1 = (float)total_decomression_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;
    std::cout << "\t\t" << (double)original_size / 1000000;
    return original_size;
}
