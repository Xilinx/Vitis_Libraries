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

#define RESIDUE_4K 4096

/* File descriptors to open the input and output files with O_DIRECT option
 * These descriptors used in P2P case only
 */
int fd_p2p_c_out = 0;
int fd_p2p_c_in = 0;

// XXHASH for K2
uint32_t xxhash_val;

uint64_t xil_lz4::get_event_duration_ns(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    return (end_time - start_time);
}

uint32_t xil_lz4::compress_file(std::string& inFile_name, std::string& outFile_name, int enable_p2p) {
    if (m_switch_flow == 0) { // Xilinx FPGA compression flow
        std::ifstream inFile;
        std::ofstream outFile;
        uint32_t input_size;
        uint32_t out_size;

        if (enable_p2p) {
            fd_p2p_c_in = open(inFile_name.c_str(), O_RDONLY | O_DIRECT);
            if (fd_p2p_c_in <= 0) {
                std::cout << "P2P: Unable to open input file, fd: " << fd_p2p_c_in << std::endl;
                exit(1);
            }
            input_size = lseek(fd_p2p_c_in, 0, SEEK_END);
            lseek(fd_p2p_c_in, 0, SEEK_SET);

            fd_p2p_c_out =
                open(outFile_name.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_APPEND | O_DIRECT, S_IRWXG | S_IRWXU);
            if (fd_p2p_c_out <= 0) {
                std::cout << "P2P: Unable to open output file, exited!, ret: " << fd_p2p_c_out << std::endl;
                close(fd_p2p_c_in);
                exit(1);
            }
        } else {
            inFile.open(inFile_name.c_str(), std::ifstream::binary);
            if (!inFile) {
                std::cout << "Non P2P: Unable to open input file\n";
                exit(1);
            }
            input_size = get_file_size(inFile);

            outFile.open(outFile_name.c_str(), std::ofstream::binary);
            if (!outFile) {
                std::cout << "Non P2P: Unable to open output file\n";
                inFile.close();
                exit(1);
            }
        }

        // Default value set to 64K
        uint8_t block_size_header = 0;
        switch (m_block_size_in_kb) {
            case 64:
                block_size_header = BSIZE_STD_64KB;
                break;
            case 256:
                block_size_header = BSIZE_STD_256KB;
                break;
            case 1024:
                block_size_header = BSIZE_STD_1024KB;
                break;
            case 4096:
                block_size_header = BSIZE_STD_4096KB;
                break;
            default:
                block_size_header = BSIZE_STD_64KB;
                std::cout << "Invalid Block Size given, so setting to 64K" << std::endl;
                break;
        }

        uint32_t host_buffer_size = HOST_BUFFER_SIZE;
        uint32_t acc_buff_size = m_block_size_in_kb * 1024 * PARALLEL_BLOCK;
        if (acc_buff_size > host_buffer_size) {
            host_buffer_size = acc_buff_size;
        }
        if (host_buffer_size > input_size) {
            host_buffer_size = input_size;
        }
        uint8_t temp_buff[10] = {
            FLG_BYTE, block_size_header, input_size, input_size >> 8, input_size >> 16, input_size >> 24, 0, 0, 0, 0};

        // xxhash is used to calculate hash value
        uint32_t xxh = XXH32(temp_buff, 10, 0);
        // This value is sent to Kernel 2
        xxhash_val = (xxh >> 8);

        uint32_t enbytes;
        // LZ4 overlap & multiple compute unit compress
        if (enable_p2p) {
            /* Pass NULL values for input and output to the compress() function,
             * since these buffers are not needed in P2P.
             */
            enbytes = compress(NULL, NULL, input_size, host_buffer_size, enable_p2p);
            close(fd_p2p_c_in);
            close(fd_p2p_c_out);
        } else {
            if (input_size < 4096)
                out_size = 4096;
            else
                out_size = input_size;
            std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
            inFile.read((char*)in.data(), input_size);
            std::vector<uint8_t, aligned_allocator<uint8_t> > out(out_size);
            enbytes = compress(in.data(), out.data(), input_size, host_buffer_size, enable_p2p);
            outFile.write((char*)out.data(), enbytes);
            inFile.close();
            outFile.close();
        }
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

void xil_lz4::bufferExtensionAssignments(bool flow) {
    for (int i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            if (flow) {
                inExt[i][j].flags = comp_ddr_nums[i];
                inExt[i][j].obj = h_buf_in[i][j].data();
                inExt[i][j].param = NULL;

                outExt[i][j].flags = comp_ddr_nums[i];
                outExt[i][j].obj = h_buf_out[i][j].data();
                outExt[i][j].param = NULL;

                csExt[i][j].flags = comp_ddr_nums[i];
                csExt[i][j].obj = h_compressSize[i][j].data();
                csExt[i][j].param = NULL;

                bsExt[i][j].flags = comp_ddr_nums[i];
                bsExt[i][j].obj = h_blksize[i][j].data();
                bsExt[i][j].param = NULL;

                lz4SizeExt[i][j].flags = comp_ddr_nums[0];
                lz4SizeExt[i][j].obj = h_lz4OutSize[i][j].data();
                lz4SizeExt[i][j].param = NULL;

                lz4Ext[i][j].flags = comp_ddr_nums[0];
                lz4Ext[i][j].obj = h_enc_out[i][j].data();
                lz4Ext[i][j].param = NULL;

            } else {
                inExt[i][j].flags = decomp_ddr_nums[i];
                inExt[i][j].obj = h_buf_in[i][j].data();

                outExt[i][j].flags = decomp_ddr_nums[i];
                outExt[i][j].obj = h_buf_out[i][j].data();

                csExt[i][j].flags = decomp_ddr_nums[i];
                csExt[i][j].obj = h_compressSize[i][j].data();

                bsExt[i][j].flags = decomp_ddr_nums[i];
                bsExt[i][j].obj = h_blksize[i][j].data();
            }
        }
    }

    if (flow) {
        headExt.flags = comp_ddr_nums[0];
        headExt.obj = h_header.data();
        headExt.param = NULL;
    }
}

// Constructor
xil_lz4::xil_lz4() {
    for (int i = 0; i < MAX_COMPUTE_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            h_buf_in[i][j].resize(HOST_BUFFER_SIZE);
            h_buf_out[i][j].resize(HOST_BUFFER_SIZE);

            h_blksize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS);

            m_compressSize[i][j].reserve(MAX_NUMBER_BLOCKS);
            m_blkSize[i][j].reserve(MAX_NUMBER_BLOCKS);
            h_enc_out[i][j].resize(HOST_BUFFER_SIZE);
            h_lz4OutSize[i][j].resize(RESIDUE_4K);
        }
    }
    h_header.resize(RESIDUE_4K);
}

// Destructor
xil_lz4::~xil_lz4() {}

int xil_lz4::init(const std::string& binaryFileName, uint8_t device_id) {
    // unsigned fileBufSize;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();

    /* Multi board support: selecting the right device based on the device_id,
     * provided through command line args (-id <device_id>).
     */
    if (devices.size() < device_id) {
        std::cout << "Identfied devices = " << devices.size() << ", given device id = " << unsigned(device_id)
                  << std::endl;
        std::cout << "Error: Device ID should be within the range of number of Devices identified" << std::endl;
        std::cout << "Program exited.." << std::endl;
        exit(1);
    }
    devices.at(0) = devices.at(device_id);

    cl::Device device = devices.at(0);

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << ", device id = " << unsigned(device_id) << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFileName);

    // std::string binaryFile = xcl::find_binary_file(device_name, binaryFileName.c_str());
    // cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

    if (SINGLE_XCLBIN) {
        // Create Compress kernels
        for (int i = 0; i < C_COMPUTE_UNIT; i++) {
            compress_kernel_lz4[i] = new cl::Kernel(*m_program, compress_kernel_names[i].c_str());
            packer_kernel_lz4[i] = new cl::Kernel(*m_program, packer_kernel_names[i].c_str());
        }

        if (!m_bin_flow) {
            // Create Decompress kernels
            for (int i = 0; i < D_COMPUTE_UNIT; i++)
                decompress_kernel_lz4[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
        }
    } else {
        if (m_bin_flow) {
            // Create Compress kernels
            for (int i = 0; i < C_COMPUTE_UNIT; i++) {
                compress_kernel_lz4[i] = new cl::Kernel(*m_program, compress_kernel_names[i].c_str());
                packer_kernel_lz4[i] = new cl::Kernel(*m_program, packer_kernel_names[i].c_str());
            }
        } else {
            // Create Decompress kernels
            for (int i = 0; i < D_COMPUTE_UNIT; i++)
                decompress_kernel_lz4[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
        }
    }

    return 0;
}

int xil_lz4::release() {
    if (m_bin_flow) {
        for (int i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel_lz4[i]);
    } else {
        for (int i = 0; i < D_COMPUTE_UNIT; i++) delete (decompress_kernel_lz4[i]);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);

    return 0;
}

uint32_t xil_lz4::decompress_file(std::string& inFile_name, std::string& outFile_name) {
    if (m_switch_flow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint32_t input_size = get_file_size(inFile);

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);

        // Read magic header 4 bytes
        char c = 0;
        char magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
        for (int i = 0; i < MAGIC_HEADER_SIZE; i++) {
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

        // Original size
        uint32_t original_size = 0;
        inFile.read((char*)&original_size, 8);
        inFile.get(c);
        // Allocat output size
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(original_size);
        // Read block data from compressed stream .lz4
        inFile.read((char*)in.data(), (input_size - 15));

        uint32_t host_buffer_size = HOST_BUFFER_SIZE;
        uint32_t acc_buff_size = m_block_size_in_kb * 1024 * PARALLEL_BLOCK;
        if (acc_buff_size > host_buffer_size) {
            host_buffer_size = acc_buff_size;
        }
        if (host_buffer_size > original_size) {
            host_buffer_size = original_size;
        }

        // Decompression Overlapped multiple cu solution
        uint32_t debytes = decompress(in.data(), out.data(), (input_size - 15), original_size, host_buffer_size);

        // Decompression Sequential multiple cus.
        // uint32_t debytes = decompress_sequential(in.data(), out.data(), (input_size - 15), original_size);

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

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

uint32_t xil_lz4::decompress(
    uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t original_size, uint32_t host_buffer_size) {
    uint32_t block_size_in_bytes = m_block_size_in_kb * 1024;
    uint64_t total_kernel_time = 0;
#ifdef EVENT_PROFILE
    uint64_t total_read_time = 0;
    uint64_t total_write_time = 0;
#endif

    // Total number of blocks exist for this file
    int total_block_cnt = (original_size - 1) / block_size_in_bytes + 1;
    int block_cntr = 0;
    int done_block_cntr = 0;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;

    // Read, Write and Kernel events
    cl::Event kernel_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event read_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event write_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    // Assignment to the buffer extensions
    bufferExtensionAssignments(0);

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
    uint32_t idx = 0;
    for (uint32_t i = 0; i < original_size; i += host_buffer_size, idx++) {
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
    for (int cu = 0; cu < D_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            // Input:- This buffer contains input chunk data
            buffer_input[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size, &inExt[cu][flag]);

            // Output:- This buffer contains compressed data written by device
            buffer_output[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size, &outExt[cu][flag]);

            // Ouput:- This buffer contains compressed block sizes
            buffer_compressed_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &csExt[cu][flag]);

            // Input:- This buffer contains origianl input block sizes
            buffer_block_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &bsExt[cu][flag]);
        }
    }

    // Counter which helps in tracking output buffer index
    uint32_t outIdx = 0;

    // Track the flags of remaining chunks
    int chunk_flags[total_chunks];
    int cu_order[total_chunks];

    // Finished bricks
    int completed_bricks = 0;

    int flag = 0;
    int lcl_cu = 0;
    uint32_t inIdx = 0;
    uint32_t total_decompression_size = 0;

    uint32_t init_itr = 0;
    if (total_chunks < 2)
        init_itr = 1;
    else
        init_itr = 2 * D_COMPUTE_UNIT;

    auto total_start = std::chrono::high_resolution_clock::now();
    // Copy first few buffers
    for (uint32_t itr = 0, brick = 0; brick < init_itr; brick += D_COMPUTE_UNIT, itr++, flag = !flag) {
        lcl_cu = D_COMPUTE_UNIT;
        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;

        for (int cu = 0; cu < lcl_cu; cu++) {
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

                if (tmp == NO_COMPRESS_BIT) {
                    uint8_t b1 = compressed_size;
                    uint8_t b2 = compressed_size >> 8;
                    uint8_t b3 = compressed_size >> 16;

                    if (b3 == BSIZE_NCOMP_64 || b3 == BSIZE_NCOMP_4096 || b3 == BSIZE_NCOMP_256 ||
                        b3 == BSIZE_NCOMP_1024) {
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
        for (int cu = 0; cu < lcl_cu; cu++) {
            chunk_flags[brick + cu] = flag;
            cu_order[brick + cu] = cu;
            if (itr >= 2) {
                read_events[cu][flag].wait();

                completed_bricks++;

                // Accumulate Kernel time
                total_kernel_time += get_event_duration_ns(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
                // Accumulate Write time
                total_write_time += get_event_duration_ns(write_events[cu][flag]);
                // Accumulate Read time
                total_read_time += get_event_duration_ns(read_events[cu][flag]);
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

                    if (tmp == NO_COMPRESS_BIT) {
                        uint8_t b1 = compressed_size;
                        uint8_t b2 = compressed_size >> 8;
                        uint8_t b3 = compressed_size >> 16;

                        if (b3 == BSIZE_NCOMP_64 || b3 == BSIZE_NCOMP_4096 || b3 == BSIZE_NCOMP_256 ||
                            b3 == BSIZE_NCOMP_1024) {
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
            int narg = 0;
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_input[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_output[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
            decompress_kernel_lz4[cu]->setArg(narg++, m_block_size_in_kb);
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
    int stride = 0;

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
        for (int j = 0; j < lcl_cu; j++) {
            int cu = cu_order[brick + j];
            int flag = chunk_flags[brick + j];

            // Run over each block within brick
            int brick_flag_idx = brick + j;

            // Accumulate Kernel time
            total_kernel_time += get_event_duration_ns(kernel_events[cu][flag]);
#ifdef EVENT_PROFILE
            // Accumulate Write time
            total_write_time += get_event_duration_ns(write_events[cu][flag]);
            // Accumulate Read time
            total_read_time += get_event_duration_ns(read_events[cu][flag]);
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
    float kernel_throughput_in_mbps_1 = (float)original_size * 1000 / total_kernel_time;
#ifdef EVENT_PROFILE
    std::cout << "Total Kernel Time " << total_kernel_time << std::endl;
    std::cout << "Total Write Time " << total_write_time << std::endl;
    std::cout << "Total Read Time " << total_read_time << std::endl;
#endif
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;

    for (int dBuf = 0; dBuf < D_COMPUTE_UNIT; dBuf++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            delete (buffer_input[dBuf][flag]);
            delete (buffer_output[dBuf][flag]);
            delete (buffer_compressed_size[dBuf][flag]);
            delete (buffer_block_size[dBuf][flag]);
        }
    }
    return original_size;
} // Decompress Overlap

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint32_t xil_lz4::decompress_sequential(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t original_size) {
    uint32_t block_size_in_bytes = m_block_size_in_kb * 1024;

    // Total number of blocks exists for this file
    int total_block_cnt = (original_size - 1) / block_size_in_bytes + 1;
    int block_cntr = 0;
    int done_block_cntr = 0;

    uint32_t no_compress_case = 0;
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    uint32_t inIdx = 0;
    uint32_t total_decomression_size = 0;

    uint32_t hostChunk_cu[D_COMPUTE_UNIT];
    int compute_cu;
    int output_idx = 0;

    for (uint32_t outIdx = 0; outIdx < original_size; outIdx += HOST_BUFFER_SIZE * D_COMPUTE_UNIT) {
        compute_cu = 0;
        uint32_t chunk_size = HOST_BUFFER_SIZE;

        // Figure out the chunk size for each compute unit
        for (int bufCalc = 0; bufCalc < D_COMPUTE_UNIT; bufCalc++) {
            hostChunk_cu[bufCalc] = 0;
            if (outIdx + (chunk_size * (bufCalc + 1)) > original_size) {
                hostChunk_cu[bufCalc] = original_size - (outIdx + HOST_BUFFER_SIZE * bufCalc);
                compute_cu++;
                break;
            } else {
                hostChunk_cu[bufCalc] = chunk_size;
                compute_cu++;
            }
        }

        uint32_t nblocks[D_COMPUTE_UNIT];
        uint32_t bufblocks[D_COMPUTE_UNIT];
        uint32_t total_size[D_COMPUTE_UNIT];
        uint32_t buf_size[D_COMPUTE_UNIT];
        uint32_t block_size = 0;
        uint32_t compressed_size = 0;

        for (int cuProc = 0; cuProc < compute_cu; cuProc++) {
            nblocks[cuProc] = 0;
            buf_size[cuProc] = 0;
            bufblocks[cuProc] = 0;
            total_size[cuProc] = 0;
            for (uint32_t cIdx = 0; cIdx < hostChunk_cu[cuProc];
                 cIdx += block_size_in_bytes, nblocks[cuProc]++, total_size[cuProc] += block_size) {
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
                m_blkSize[cuProc][0].data()[nblocks[cuProc]] = block_size;
                m_compressSize[cuProc][0].data()[nblocks[cuProc]] = compressed_size;

                // If compressed size is less than original block size
                if (compressed_size < block_size) {
                    h_compressSize[cuProc][0].data()[bufblocks[cuProc]] = compressed_size;
                    h_blksize[cuProc][0].data()[bufblocks[cuProc]] = block_size;
                    std::memcpy(&(h_buf_in[cuProc][0].data()[buf_size[cuProc]]), &in[inIdx], compressed_size);
                    inIdx += compressed_size;
                    buf_size[cuProc] += block_size_in_bytes;
                    bufblocks[cuProc]++;
                } else if (compressed_size == block_size) {
                    no_compress_case++;
                    // No compression block
                    std::memcpy(&(out[outIdx + cuProc * HOST_BUFFER_SIZE + cIdx]), &in[inIdx], block_size);
                    inIdx += block_size;
                } else {
                    assert(0);
                }
                block_cntr++;
                done_block_cntr++;
            }
            assert(total_size[cuProc] <= original_size);

            if (nblocks[cuProc] == 1 && compressed_size == block_size) break;
        } // Cu process done

        for (int bufC = 0; bufC < compute_cu; bufC++) {
            // Device buffer allocation
            buffer_input[bufC][0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               buf_size[bufC], &inExt[bufC]);

            buffer_output[bufC][0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX,
                               buf_size[bufC], &outExt[bufC]);

            buffer_block_size[bufC][0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * bufblocks[bufC], &bsExt[bufC]);

            buffer_compressed_size[bufC][0] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               sizeof(uint32_t) * bufblocks[bufC], &csExt[bufC]);
        }

        // Set kernel arguments
        for (int sArg = 0; sArg < compute_cu; sArg++) {
            uint32_t narg = 0;
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_input[sArg][0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_output[sArg][0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_block_size[sArg][0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, *(buffer_compressed_size[sArg][0]));
            decompress_kernel_lz4[sArg]->setArg(narg++, m_block_size_in_kb);
            decompress_kernel_lz4[sArg]->setArg(narg++, bufblocks[sArg]);
        }

        std::vector<cl::Memory> inBufVec;
        for (int inVec = 0; inVec < compute_cu; inVec++) {
            inBufVec.push_back(*(buffer_input[inVec][0]));
            inBufVec.push_back(*(buffer_block_size[inVec][0]));
            inBufVec.push_back(*(buffer_compressed_size[inVec][0]));
        }

        // Migrate memory - Map host to device buffers
        m_q->enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_q->finish();

        auto kernel_start = std::chrono::high_resolution_clock::now();
        // Kernel invocation
        for (int task = 0; task < compute_cu; task++) {
            m_q->enqueueTask(*decompress_kernel_lz4[task]);
        }
        m_q->finish();

        auto kernel_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
        kernel_time_ns_1 += duration;

        std::vector<cl::Memory> outBufVec;
        for (int oVec = 0; oVec < compute_cu; oVec++) outBufVec.push_back(*(buffer_output[oVec][0]));

        // Migrate memory - Map device to host buffers
        m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q->finish();

        for (int cuRead = 0; cuRead < compute_cu; cuRead++) {
            uint32_t bufIdx = 0;
            for (uint32_t bIdx = 0, idx = 0; bIdx < nblocks[cuRead]; bIdx++, idx += block_size_in_bytes) {
                uint32_t block_size = m_blkSize[cuRead][0].data()[bIdx];
                uint32_t compressed_size = m_compressSize[cuRead][0].data()[bIdx];
                if (compressed_size < block_size) {
                    std::memcpy(&out[output_idx], &h_buf_out[cuRead][0].data()[bufIdx], block_size);
                    output_idx += block_size;
                    bufIdx += block_size;
                    total_decomression_size += block_size;
                } else if (compressed_size == block_size) {
                    output_idx += block_size;
                }
            }
        } // CU processing ends

        // Delete device buffers
        for (int dBuf = 0; dBuf < compute_cu; dBuf++) {
            delete (buffer_input[dBuf][0]);
            delete (buffer_output[dBuf][0]);
            delete (buffer_block_size[dBuf][0]);
            delete (buffer_compressed_size[dBuf][0]);
        }
    } // Top - Main loop ends here

    float throughput_in_mbps_1 = (float)total_decomression_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    return original_size;

} // End of decompress

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint32_t xil_lz4::compress(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t host_buffer_size, int enable_p2p) {
    uint32_t block_size_in_bytes = m_block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    uint64_t total_kernel_time = 0;
    uint64_t total_packer_kernel_time = 0;
#ifdef EVENT_PROFILE
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
#endif

    /* Input buffer pointer used in p2p case */
    uint8_t* h_buf_in_p2p[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    /* Packer output buffer pointer used in p2p case */
    uint8_t* h_buf_out_p2p[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    int ret = 0;

    // Read, Write and Kernel events
    cl::Event kernel_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event read_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event write_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];
    cl::Event packer_kernel_events[MAX_COMPUTE_UNITS][OVERLAP_BUF_COUNT];

    cl::Event cs_kernel_events;
    cl::Event pk_kernel_events;

    // Header information
    uint32_t head_size = 0;
    h_header[head_size++] = MAGIC_BYTE_1;
    h_header[head_size++] = MAGIC_BYTE_2;
    h_header[head_size++] = MAGIC_BYTE_3;
    h_header[head_size++] = MAGIC_BYTE_4;

    h_header[head_size++] = FLG_BYTE;

    // Value
    switch (m_block_size_in_kb) {
        case 64:
            h_header[head_size++] = BSIZE_STD_64KB;
            break;
        case 256:
            h_header[head_size++] = BSIZE_STD_256KB;
            break;
        case 1024:
            h_header[head_size++] = BSIZE_STD_1024KB;
            break;
        case 4096:
            h_header[head_size++] = BSIZE_STD_4096KB;
            break;
    }

    // Input size
    h_header[head_size++] = input_size;
    h_header[head_size++] = input_size >> 8;
    h_header[head_size++] = input_size >> 16;
    h_header[head_size++] = input_size >> 24;
    h_header[head_size++] = 0;
    h_header[head_size++] = 0;
    h_header[head_size++] = 0;
    h_header[head_size++] = 0;

    // XXHASH value
    h_header[head_size++] = xxhash_val;

    // Assignment to the buffer extensions
    bufferExtensionAssignments(1);

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
    uint32_t idx = 0;
    for (uint32_t i = 0; i < input_size; i += host_buffer_size, idx++) {
        uint32_t chunk_size = host_buffer_size;
        if (chunk_size + i > input_size) {
            chunk_size = input_size - i;
        }
        // Update size of each chunk buffer
        sizeOfChunk[idx] = chunk_size;
    }

    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / 64 + 1) * 64;

    // Device buffer allocation
    for (int cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            if (enable_p2p) {
                /* set cl_mem_ext_ptr flag to XCL_MEM_EXT_P2P_BUFFER for p2p to work */
                inExt[cu][flag].flags |= XCL_MEM_EXT_P2P_BUFFER;
                /* obj set to nullptr as we call enqueueMapBuffer explicitly for p2p to work */
                inExt[cu][flag].obj = nullptr;

                lz4Ext[cu][flag].flags |= XCL_MEM_EXT_P2P_BUFFER;
                lz4Ext[cu][flag].obj = nullptr;

                // K1 Input:- This buffer contains input chunk data
                buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                                                        host_buffer_size, &(inExt[cu][flag]));
                h_buf_in_p2p[cu][flag] = (uint8_t*)m_q->enqueueMapBuffer(*(buffer_input[cu][flag]), CL_TRUE,
                                                                         CL_MAP_WRITE, 0, host_buffer_size);

                // K2 Output:- This buffer contains compressed data written by device
                buffer_lz4out[cu][flag] = new cl::Buffer(*m_context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                                                         host_buffer_size, &(lz4Ext[cu][flag]));
                h_buf_out_p2p[cu][flag] = (uint8_t*)m_q->enqueueMapBuffer(
                    *(buffer_lz4out[cu][flag]), CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, host_buffer_size);
            } else {
                // K1 Input:- This buffer contains input chunk data
                buffer_input[cu][flag] =
                    new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                                   host_buffer_size, &(inExt[cu][flag]));
                // K2 Output:- This buffer contains compressed data written by device
                buffer_lz4out[cu][flag] =
                    new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                                   host_buffer_size, &(lz4Ext[cu][flag]));
            }
            // K1 Output:- This buffer contains compressed data written by device
            // K2 Input:- This is a input to data packer kernel
            buffer_output[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size, &(outExt[cu][flag]));

            // K2 input:- This buffer contains compressed data written by device
            buffer_lz4OutSize[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               10 * sizeof(uint32_t), &lz4SizeExt[cu][flag]);

            // K1 Ouput:- This buffer contains compressed block sizes
            // K2 Input:- This buffer is used in data packer kernel
            buffer_compressed_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &(csExt[cu][flag]));

            // Input:- This buffer contains original input block sizes
            buffer_block_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &(bsExt[cu][flag]));

            // Input:- Header buffer only used once
            buffer_header[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               head_size * sizeof(uint8_t), &headExt);
        }
    }

    // Counter which helps in tracking
    // Output buffer index
    uint32_t outIdx = 0;

    std::chrono::duration<double, std::nano> packer_kernel_time_ns_1(0);
    std::chrono::duration<double, std::nano> comp_kernel_time_ns_1(0);
    uint64_t total_p2p_read_time = 0, total_p2p_write_time = 0;

    int flag = 0;
    uint32_t offset = 0;
    uint32_t tail_bytes = 0;
    // Kernel wait events for writing & compute
    std::vector<cl::Event> kernelWriteWait;
    std::vector<cl::Event> kernelComputeWait;
    std::vector<cl::Event> kernelReadWait;
    int cu = 0;
    // Main loop of overlap execution
    // Loop below runs over total bricks i.e., host buffer size chunks
    auto total_start = std::chrono::high_resolution_clock::now();
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; brick += C_COMPUTE_UNIT, itr++, flag = !flag) {
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
        if (enable_p2p) {
            auto start = std::chrono::high_resolution_clock::now();
            /* Third arg in read() should be divisible by 4K */
            ret = read(fd_p2p_c_in, h_buf_in_p2p[cu][flag], HOST_BUFFER_SIZE);
            if (ret == -1) std::cout << "P2P: read() failed with error: " << ret << ", line: " << __LINE__ << std::endl;
            auto end = std::chrono::high_resolution_clock::now();
            auto p2p_time = std::chrono::duration<double, std::nano>(end - start);
            total_p2p_read_time += p2p_time.count();
        } else {
            std::memcpy(h_buf_in[cu][flag].data(), &in[(brick + cu) * host_buffer_size], sizeOfChunk[brick + cu]);
        }

        // Set kernel arguments
        int narg = 0;
        compress_kernel_lz4[cu]->setArg(narg++, *(buffer_input[cu][flag]));
        compress_kernel_lz4[cu]->setArg(narg++, *(buffer_output[cu][flag]));
        compress_kernel_lz4[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
        compress_kernel_lz4[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
        compress_kernel_lz4[cu]->setArg(narg++, m_block_size_in_kb);
        compress_kernel_lz4[cu]->setArg(narg++, sizeOfChunk[brick + cu]);

        /* Transfer data from host to device
         * In p2p case, no need to transfer buffer input to device from host.
         */
        if (enable_p2p)
            m_q->enqueueMigrateMemObjects({*(buffer_block_size[cu][flag])}, 0, NULL, &(write_events[cu][flag]));
        else
            m_q->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_block_size[cu][flag])}, 0, NULL,
                                          &(write_events[cu][flag]));

        // Kernel Write events update
        kernelWriteWait.push_back(write_events[cu][flag]);

        // Fire the kernel
        m_q->enqueueTask(*compress_kernel_lz4[cu], &kernelWriteWait, &(kernel_events[cu][flag]));

        // Update kernel events flag on computation
        kernelComputeWait.push_back(kernel_events[cu][flag]);

        tail_bytes = (brick == total_chunks - 1) ? 1 : 0;

        // K2 Set Kernel arguments
        narg = 0;
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_output[cu][flag]));
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_lz4out[cu][flag]));
        if (brick == 0) {
            packer_kernel_lz4[cu]->setArg(narg++, *(buffer_header[cu][flag]));
        } else {
            // Wait on previous packer operation to finish
            read_events[cu][!flag].wait();
            total_kernel_time += get_event_duration_ns(kernel_events[cu][!flag]);
            total_packer_kernel_time += get_event_duration_ns(packer_kernel_events[cu][!flag]);
#ifdef EVENT_PROFILE
            if (!enable_p2p) {
                total_read_time += get_event_duration_ns(read_events[cu][!flag]);
                total_write_time += get_event_duration_ns(write_events[cu][!flag]);
            }
#endif

            // Copy output from kernel/packer and dump as .lz4 stream
            uint32_t compressed_size = h_lz4OutSize[cu][!flag].data()[0];
            uint32_t packed_buffer = compressed_size / RESIDUE_4K;
            uint32_t outIdx_align = RESIDUE_4K * packed_buffer;
            head_size = compressed_size - outIdx_align;
            offset = outIdx_align;

            if (enable_p2p) {
                auto start = std::chrono::high_resolution_clock::now();
                ret = write(fd_p2p_c_out, h_buf_out_p2p[cu][!flag], outIdx_align);
                if (ret == -1)
                    std::cout << "P2P: write() failed with error: " << ret << ", line: " << __LINE__ << std::endl;
                auto end = std::chrono::high_resolution_clock::now();
                auto p2p_time = std::chrono::duration<double, std::nano>(end - start);
                total_p2p_write_time += p2p_time.count();
            } else {
                std::memcpy(&out[outIdx], &h_enc_out[cu][!flag].data()[0], outIdx_align);
            }
            outIdx += outIdx_align;
            packer_kernel_lz4[cu]->setArg(narg++, *(buffer_lz4out[cu][!flag]));
        }
        uint32_t no_blocks_calc = (sizeOfChunk[brick + cu] - 1) / (m_block_size_in_kb * 1024) + 1;
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_compressed_size[cu][flag]));
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_block_size[cu][flag]));
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_lz4OutSize[cu][flag]));
        packer_kernel_lz4[cu]->setArg(narg++, *(buffer_input[cu][flag]));
        packer_kernel_lz4[cu]->setArg(narg++, head_size);
        packer_kernel_lz4[cu]->setArg(narg++, offset);
        packer_kernel_lz4[cu]->setArg(narg++, m_block_size_in_kb);
        packer_kernel_lz4[cu]->setArg(narg++, no_blocks_calc);
        packer_kernel_lz4[cu]->setArg(narg++, tail_bytes);

        // Fire the kernel
        m_q->enqueueTask(*packer_kernel_lz4[cu], &kernelComputeWait, &(packer_kernel_events[cu][flag]));
        // Update kernel events flag on computation
        kernelComputeWait.push_back(packer_kernel_events[cu][flag]);

        /* Transfer data from device to host.
         * In p2p case, no need to transfer packer output data from device to host
         */
        if (enable_p2p)
            m_q->enqueueMigrateMemObjects({*(buffer_lz4OutSize[cu][flag])}, CL_MIGRATE_MEM_OBJECT_HOST,
                                          &kernelComputeWait, &(read_events[cu][flag]));
        else
            m_q->enqueueMigrateMemObjects({*(buffer_lz4out[cu][flag]), *(buffer_lz4OutSize[cu][flag])},
                                          CL_MIGRATE_MEM_OBJECT_HOST, &kernelComputeWait, &(read_events[cu][flag]));
    } // Main loop ends here

    read_events[cu][!flag].wait();
    total_kernel_time += get_event_duration_ns(kernel_events[cu][!flag]);
    total_packer_kernel_time += get_event_duration_ns(packer_kernel_events[cu][!flag]);
#ifdef EVENT_PROFILE
    if (!enable_p2p) {
        total_read_time += get_event_duration_ns(read_events[cu][!flag]);
        total_write_time += get_event_duration_ns(write_events[cu][!flag]);
    }
#endif
    m_q->finish();
    m_q->flush();

    uint32_t compressed_size = h_lz4OutSize[0][!flag].data()[0];
    uint32_t align_4k = compressed_size / RESIDUE_4K;
    uint32_t outIdx_align = RESIDUE_4K * align_4k;
    uint32_t residue_size = compressed_size - outIdx_align;
    uint8_t empty_buffer[4096] = {0};
    uint8_t* temp;

    if (enable_p2p)
        temp = (uint8_t*)h_buf_out_p2p[0][!flag];
    else
        temp = (uint8_t*)h_enc_out[0][!flag].data();

    /* Make last packer output block divisible by 4K by appending 0's */
    temp = temp + compressed_size;
    memcpy(temp, empty_buffer, RESIDUE_4K - residue_size);
    compressed_size = outIdx_align + RESIDUE_4K;

    if (enable_p2p) {
        ret = write(fd_p2p_c_out, h_buf_out_p2p[0][!flag], compressed_size);
        if (ret == -1) std::cout << "P2P: write() failed with error: " << ret << ", line: " << __LINE__ << std::endl;
    } else {
        std::memcpy(&out[outIdx], &h_enc_out[0][!flag].data()[0], compressed_size);
    }
    outIdx += compressed_size;
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
    float kernel_throughput_in_mbps_1 = (float)input_size * 1000 / (total_kernel_time + total_packer_kernel_time);
#ifdef EVENT_PROFILE
    float compress_kernel_mbps = (float)input_size * 1000 / total_kernel_time;
    float packer_kernel_mbps = (float)outIdx * 1000 / total_packer_kernel_time;

#endif
#ifdef EVENT_PROFILE
    std::cout << "Total compress Kernel Time (milli sec) = " << total_kernel_time / 1000000 << std::endl;
    std::cout << "Total Packer Kernel Time (milli sec) = " << total_packer_kernel_time / 1000000 << std::endl;
    if (enable_p2p) {
        std::cout << "Total Write Time (milli sec) = " << total_p2p_write_time / 1000000 << std::endl;
        std::cout << "Total Read Time (milli sec) = " << total_p2p_read_time / 1000000 << std::endl;
    } else {
        std::cout << "Total Write Time (milli sec) = " << total_write_time / 1000000 << std::endl;
        std::cout << "Total Read Time (milli sec) = " << total_read_time / 1000000 << std::endl;
    }
    std::cout << "Compression kernel throughput (MBps) = " << compress_kernel_mbps << std::endl;
    std::cout << "Packer kernel throughput (MBps) = " << packer_kernel_mbps << std::endl;
#endif
#ifdef VERBOSE
    std::cout << "Compression is done, compressed_size = " << outIdx << std::endl;
    std::cout << "\nE2E(MBps)\tKT(MBps)\tLZ4_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;
#endif

    for (int cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            delete (buffer_input[cu][flag]);
            delete (buffer_output[cu][flag]);
            delete (buffer_lz4out[cu][flag]);
            delete (buffer_compressed_size[cu][flag]);
            delete (buffer_block_size[cu][flag]);
            delete (buffer_lz4OutSize[cu][flag]);
        }
    }
    return outIdx;
} // Overlap end
