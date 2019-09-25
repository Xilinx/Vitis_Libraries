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
#include "xil_zlib.hpp"
#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

#define FORMAT_0 31
#define FORMAT_1 139
#define VARIANT 8
#define REAL_CODE 8
#define OPCODE 3
#define CHUNK_16K 16384

uint32_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint32_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void zip(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
#ifdef zlib_FLOW
    // printme("In zlib FLOW \n");
    // 2 bytes of magic header
    outFile.put(FORMAT_0);
    outFile.put(FORMAT_1);

    // 1 byte Compression method
    outFile.put(VARIANT);

    // 1 byte flags
    uint8_t flags = 0;
    flags |= REAL_CODE;
    outFile.put(flags);

    // 4 bytes file modification time in unit format
    unsigned long time_stamp = 0;
    struct stat istat;
    stat(inFile_name.c_str(), &istat);
    time_stamp = istat.st_mtime;
    // put_long(time_stamp, outFile);
    uint8_t time_byte = 0;
    time_byte = time_stamp;
    outFile.put(time_byte);
    time_byte = time_stamp >> 8;
    outFile.put(time_byte);
    time_byte = time_stamp >> 16;
    outFile.put(time_byte);
    time_byte = time_stamp >> 24;
    outFile.put(time_byte);

    // 1 byte extra flag (depend on compression method)
    uint8_t deflate_flags = 0;
    outFile.put(deflate_flags);

    // 1 byte OPCODE - 0x03 for Unix
    outFile.put(OPCODE);

    // Dump file name
    for (int i = 0; inFile_name[i] != '\0'; i++) {
        outFile.put(inFile_name[i]);
    }
    outFile.put(0);
#else
    ////printme("In ZLIB flow");
    outFile.put(120);
    outFile.put(1);
#endif
    outFile.write((char*)zip_out, enbytes);
#ifdef zlib_FLOW
    unsigned long ifile_size = istat.st_size;
    uint8_t crc_byte = 0;
    long crc_val = 0;
    crc_byte = crc_val;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 8;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 16;
    outFile.put(crc_byte);
    crc_byte = crc_val >> 24;
    outFile.put(crc_byte);

    uint8_t len_byte = 0;
    len_byte = ifile_size;
    outFile.put(len_byte);
    len_byte = ifile_size >> 8;
    outFile.put(len_byte);
    len_byte = ifile_size >> 16;
    outFile.put(len_byte);
    len_byte = ifile_size >> 24;
    outFile.put(len_byte);
#endif
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

uint32_t xil_zlib::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::chrono::duration<double, std::nano> compress_API_time_ns_1(0);
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    std::vector<uint8_t, aligned_allocator<uint8_t> > zlib_in(input_size);
    std::vector<uint8_t, aligned_allocator<uint8_t> > zlib_out(input_size * 2);

    inFile.read((char*)zlib_in.data(), input_size);

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    auto compress_API_start = std::chrono::high_resolution_clock::now();
    // zlib Compress
    uint32_t enbytes = compress(zlib_in.data(), zlib_out.data(), input_size, host_buffer_size);
    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(compress_API_end - compress_API_start);
    compress_API_time_ns_1 += duration;

    float throughput_in_mbps_1 = (float)input_size * 1000 / compress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    // Pack zlib encoded stream .gz file
    zip(inFile_name, outFile, zlib_out.data(), enbytes);

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
xil_zlib::xil_zlib(const std::string& binaryFileName) {
    // Zlib Compression Binary Name
    init(binaryFileName);

    // printf("C_COMPUTE_UNIT \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;
    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / block_size_in_kb + 1) * block_size_in_kb;

    // Index calculation
    h_buf_in.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
    h_buf_out.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 4);
    h_buf_zlibout.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 2);
    h_blksize.resize(MAX_NUMBER_BLOCKS);
    h_compressSize.resize(MAX_NUMBER_BLOCKS);
    h_dyn_ltree_freq.resize(PARALLEL_ENGINES * LTREE_SIZE);
    h_dyn_dtree_freq.resize(PARALLEL_ENGINES * DTREE_SIZE);
    h_dyn_bltree_freq.resize(PARALLEL_ENGINES * BLTREE_SIZE);
    h_dyn_ltree_codes.resize(PARALLEL_ENGINES * LTREE_SIZE);
    h_dyn_dtree_codes.resize(PARALLEL_ENGINES * DTREE_SIZE);
    h_dyn_bltree_codes.resize(PARALLEL_ENGINES * BLTREE_SIZE);
    h_dyn_ltree_blen.resize(PARALLEL_ENGINES * LTREE_SIZE);
    h_dyn_dtree_blen.resize(PARALLEL_ENGINES * DTREE_SIZE);
    h_dyn_bltree_blen.resize(PARALLEL_ENGINES * BLTREE_SIZE);

    h_buff_max_codes.resize(PARALLEL_ENGINES * MAXCODE_SIZE);
    // Device buffer allocation
    buffer_input =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, host_buffer_size, h_buf_in.data());

    buffer_lz77_output =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, host_buffer_size * 4, h_buf_out.data());

    buffer_zlib_output =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, host_buffer_size * 2, h_buf_zlibout.data());

    buffer_compress_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          temp_nblocks * sizeof(uint32_t), h_compressSize.data());

    buffer_inblk_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       temp_nblocks * sizeof(uint32_t), h_blksize.data());

    buffer_dyn_ltree_freq = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_freq.data());

    buffer_dyn_dtree_freq = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_freq.data());

    buffer_dyn_bltree_freq =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                       PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_freq.data());

    buffer_dyn_ltree_codes = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_codes.data());

    buffer_dyn_dtree_codes = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_codes.data());

    buffer_dyn_bltree_codes =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                       PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_codes.data());

    buffer_dyn_ltree_blen = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_blen.data());

    buffer_dyn_dtree_blen = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_blen.data());

    buffer_dyn_bltree_blen =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                       PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_blen.data());

    buffer_max_codes = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      (PARALLEL_ENGINES * MAXCODE_SIZE) * sizeof(uint32_t), h_buff_max_codes.data());

    h_dbuf_in.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
    h_dbuf_zlibout.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 10);
    h_dcompressSize.resize(MAX_NUMBER_BLOCKS);
}

// Destructor
xil_zlib::~xil_zlib() {
    release();
    delete (buffer_input);
    delete (buffer_lz77_output);
    delete (buffer_zlib_output);
    delete (buffer_compress_size);
    delete (buffer_inblk_size);

    delete (buffer_dyn_ltree_freq);
    delete (buffer_dyn_dtree_freq);
    delete (buffer_dyn_bltree_freq);

    delete (buffer_dyn_ltree_codes);
    delete (buffer_dyn_dtree_codes);
    delete (buffer_dyn_bltree_codes);

    delete (buffer_dyn_ltree_blen);
    delete (buffer_dyn_dtree_blen);
    delete (buffer_dyn_bltree_blen);
}

int xil_zlib::init(const std::string& binaryFileName) {
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    m_q_dec = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    // std::string binaryFile = xcl::find_binary_file(device_name,binaryFileName.c_str());
    // cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    auto fileBuf = xcl::read_binary_file(binaryFileName);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

    // Create Compress & Huffman kernels
    compress_kernel = new cl::Kernel(*m_program, compress_kernel_names[0].c_str());
    // Create Compress & Huffman kernels
    huffman_kernel = new cl::Kernel(*m_program, huffman_kernel_names[0].c_str());

    // Create Tree generation kernel
    treegen_kernel = new cl::Kernel(*m_program, treegen_kernel_names[0].c_str());

    // Create Decompress kernel
    decompress_kernel = new cl::Kernel(*m_program, decompress_kernel_names[0].c_str());

    return 0;
}

int xil_zlib::release() {
    delete (m_program);
    delete (m_q);

    delete (m_q_dec);
    delete (m_context);

    delete (compress_kernel);

    delete (huffman_kernel);

    delete (treegen_kernel);

    delete (decompress_kernel);

    return 0;
}

uint32_t xil_zlib::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu) {
    // printme("In decompress_file \n");
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);

    // Allocat output size
    // 8 - Max CR per file expected, if this size is big
    // Decompression crashes
    std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size * 10);
    uint32_t debytes = 0;
    // READ ZLIB header 2 bytes
    inFile.read((char*)in.data(), input_size);
    // printme("Call to zlib_decompress \n");
    // Call decompress
    auto decompress_API_start = std::chrono::high_resolution_clock::now();
    debytes = decompress(in.data(), out.data(), input_size, cu);
    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    decompress_API_time_ns_1 += duration;

    float throughput_in_mbps_1 = (float)debytes * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:" << throughput_in_mbps_1 << std::endl;

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

uint32_t xil_zlib::decompress(uint8_t* in, uint8_t* out, uint32_t input_size, int cu) {
    bool flag = false;
    if (input_size > 128 * 1024 * 1024) flag = true;
    // printme("Entered zlib decop \n");

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    uint8_t* inP = nullptr;
    uint8_t* outP = nullptr;
    uint32_t* outSize = nullptr;
    cl::Buffer* buffer_in;
    cl::Buffer* buffer_out;
    cl::Buffer* buffer_size;
    if (flag) {
        // printme("before buffer creation \n");
        buffer_in = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, input_size);
        buffer_out = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, input_size * 10);
        buffer_size = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, 10 * sizeof(uint32_t));
        inP = (uint8_t*)m_q_dec->enqueueMapBuffer(*(buffer_in), CL_TRUE, CL_MAP_READ, 0, input_size);
        outP = (uint8_t*)m_q_dec->enqueueMapBuffer(*(buffer_out), CL_TRUE, CL_MAP_WRITE, 0, input_size * 10);
        outSize = (uint32_t*)m_q_dec->enqueueMapBuffer(*(buffer_size), CL_TRUE, CL_MAP_WRITE, 0, 10 * sizeof(uint32_t));
    } else {
        // printme("before buffer creation \n");
        buffer_in = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_size, h_dbuf_in.data());

        buffer_out =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, input_size * 10, h_dbuf_zlibout.data());

        buffer_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 10 * sizeof(uint32_t),
                                     h_dcompressSize.data());

        inP = h_dbuf_in.data();
        outP = h_dbuf_zlibout.data();
        outSize = h_dcompressSize.data();
    }
    // printme("Entered incopy \n");
    // Copy compressed input to h_buf_in
    std::memcpy(inP, &in[0], input_size);

    int narg = 0;
    // Set Kernel Args
    // printme("Setargs \n");
    decompress_kernel->setArg(narg++, *(buffer_in));
    decompress_kernel->setArg(narg++, *(buffer_out));
    decompress_kernel->setArg(narg++, *(buffer_size));
    decompress_kernel->setArg(narg++, input_size);

    // Migrate Memory - Map host to device buffers
    m_q_dec->enqueueMigrateMemObjects({*(buffer_in)}, 0);
    m_q_dec->finish();

    // Kernel invocation
    m_q_dec->enqueueTask(*decompress_kernel);
    m_q_dec->finish();

    // Migrate memory - Map device to host buffers
    m_q_dec->enqueueMigrateMemObjects({*(buffer_size)}, CL_MIGRATE_MEM_OBJECT_HOST);
    m_q_dec->finish();

    uint32_t raw_size = *outSize;

    // If raw size is greater than 3GB
    // Limit it to 3GB
    if (raw_size > (uint32_t)(3 * 1024 * 1024 * 1024)) raw_size = (uint32_t)(3 * 1024 * 1024 * 1024);

    m_q_dec->enqueueReadBuffer(*(buffer_out), CL_TRUE, 0, raw_size * sizeof(uint8_t), &out[0]);

    if (flag) {
        m_q_dec->enqueueUnmapMemObject(*buffer_in, inP, nullptr, nullptr);
        m_q_dec->enqueueUnmapMemObject(*buffer_out, outP, nullptr, nullptr);
        m_q_dec->enqueueUnmapMemObject(*buffer_size, outSize, nullptr, nullptr);
    }
    delete (buffer_in);
    delete (buffer_out);
    delete (buffer_size);

    // printme("Done with decompress \n");
    return raw_size;
}
// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint32_t xil_zlib::compress(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t host_buffer_size) {
    //////printme("In compress \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;

    // For example: Input file size is 12MB and Host buffer size is 2MB
    // Then we have 12/2 = 6 chunks exists
    // Calculate the count of total chunks based on input size
    // This count is used to overlap the execution between chunks and file
    // operations

    uint32_t total_chunks = (input_size - 1) / host_buffer_size + 1;

    // Find out the size of each chunk spanning entire file
    // For eaxmple: As mentioned in previous example there are 6 chunks
    // Code below finds out the size of chunk, in general all the chunks holds
    // HOST_BUFFER_SIZE except for the last chunk
    uint32_t sizeOfChunk[total_chunks];
    uint32_t blocksPerChunk[total_chunks];
    uint32_t idx = 0;

    for (uint32_t i = 0; i < input_size; i += host_buffer_size, idx++) {
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

    // Counter which helps in tracking
    // Output buffer index
    uint32_t outIdx = 0;

    // Finished bricks
    int completed_bricks = 0;

    int flag = 0;
    uint32_t lcl_cu = 0;

overlap:
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; itr++, flag = !flag) {
        lcl_cu = 1;

        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;

        for (uint32_t cu = 0; cu < lcl_cu; cu++) {
            if (itr >= 2) {
                // Wait on current flag previous operation to finish
                m_q->finish();

                // Completed bricks counter
                completed_bricks++;

                uint32_t index = 0;
                uint32_t brick_flag_idx = brick - 1;

                //////printme("blocksPerChunk %d \n", blocksPerChunk[brick]);
                // Copy the data from various blocks in concatinated manner
                for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                    uint32_t block_size = block_size_in_bytes;
                    if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                        block_size = sizeOfChunk[brick_flag_idx] - index;
                    }

                    uint32_t compressed_size = (h_compressSize.data())[bIdx];
                    std::memcpy(&out[outIdx], &h_buf_zlibout.data()[bIdx * block_size_in_bytes], compressed_size);
                    outIdx += compressed_size;
                }
            } // If condition which reads huffman output for 0 or 1 location

            // Figure out block sizes per brick
            uint32_t idxblk = 0;
            for (uint32_t i = 0; i < sizeOfChunk[brick + cu]; i += block_size_in_bytes) {
                uint32_t block_size = block_size_in_bytes;

                if (i + block_size > sizeOfChunk[brick + cu]) {
                    block_size = sizeOfChunk[brick + cu] - i;
                }
                //////printme("sizeofChunk %d block_size %d cu %d \n", sizeOfChunk[brick+cu], block_size, cu);
                (h_blksize).data()[idxblk++] = block_size;
            }

            std::memcpy(h_buf_in.data(), &in[(brick + cu) * host_buffer_size], sizeOfChunk[brick + cu]);

            // Set kernel arguments
            int narg = 0;

            compress_kernel->setArg(narg++, *(buffer_input));
            compress_kernel->setArg(narg++, *(buffer_lz77_output));
            compress_kernel->setArg(narg++, *(buffer_compress_size));
            compress_kernel->setArg(narg++, *(buffer_inblk_size));
            compress_kernel->setArg(narg++, *(buffer_dyn_ltree_freq));
            compress_kernel->setArg(narg++, *(buffer_dyn_dtree_freq));
            compress_kernel->setArg(narg++, block_size_in_kb);
            compress_kernel->setArg(narg++, sizeOfChunk[brick + cu]);

            narg = 0;
            treegen_kernel->setArg(narg++, *(buffer_dyn_ltree_freq));
            treegen_kernel->setArg(narg++, *(buffer_dyn_dtree_freq));
            treegen_kernel->setArg(narg++, *(buffer_dyn_bltree_freq));
            treegen_kernel->setArg(narg++, *(buffer_dyn_ltree_codes));
            treegen_kernel->setArg(narg++, *(buffer_dyn_dtree_codes));
            treegen_kernel->setArg(narg++, *(buffer_dyn_bltree_codes));
            treegen_kernel->setArg(narg++, *(buffer_dyn_ltree_blen));
            treegen_kernel->setArg(narg++, *(buffer_dyn_dtree_blen));
            treegen_kernel->setArg(narg++, *(buffer_dyn_bltree_blen));
            treegen_kernel->setArg(narg++, *(buffer_max_codes));
            treegen_kernel->setArg(narg++, block_size_in_kb);
            treegen_kernel->setArg(narg++, sizeOfChunk[brick + cu]);
            treegen_kernel->setArg(narg++, blocksPerChunk[brick]);

            narg = 0;
            huffman_kernel->setArg(narg++, *(buffer_lz77_output));
            huffman_kernel->setArg(narg++, *(buffer_zlib_output));
            huffman_kernel->setArg(narg++, *(buffer_compress_size));
            huffman_kernel->setArg(narg++, *(buffer_inblk_size));
            huffman_kernel->setArg(narg++, *(buffer_dyn_ltree_codes));
            huffman_kernel->setArg(narg++, *(buffer_dyn_dtree_codes));
            huffman_kernel->setArg(narg++, *(buffer_dyn_bltree_codes));
            huffman_kernel->setArg(narg++, *(buffer_dyn_ltree_blen));
            huffman_kernel->setArg(narg++, *(buffer_dyn_dtree_blen));
            huffman_kernel->setArg(narg++, *(buffer_dyn_bltree_blen));
            huffman_kernel->setArg(narg++, *(buffer_max_codes));
            huffman_kernel->setArg(narg++, block_size_in_kb);
            huffman_kernel->setArg(narg++, sizeOfChunk[brick + cu]);

            // Migrate memory - Map host to device buffers
            m_q->enqueueMigrateMemObjects({*(buffer_input), *(buffer_inblk_size)}, 0 /* 0 means from host*/);
            m_q->finish();

            // LZ77 Compress Fire Kernel invocation
            m_q->enqueueTask(*compress_kernel);
            m_q->finish();

            // TreeGen Fire Kernel invocation
            m_q->enqueueTask(*treegen_kernel);
            m_q->finish();

            // Huffman Fire Kernel invocation
            m_q->enqueueTask(*huffman_kernel);
            m_q->finish();

            m_q->enqueueMigrateMemObjects({*(buffer_zlib_output), *(buffer_compress_size)}, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q->finish();
        } // Internal loop runs on compute units

        brick++;

    } // Main overlap loop

    m_q->flush();
    m_q->finish();

    uint32_t leftover = total_chunks - completed_bricks;
    uint32_t stride = total_chunks;

    // Handle leftover bricks
    for (uint32_t ovr_itr = 0, brick = stride - 1; ovr_itr < leftover; ovr_itr += 1, brick += 1) {
        lcl_cu = 1;
        if (ovr_itr + lcl_cu > leftover) lcl_cu = leftover - ovr_itr;

        // Handle multiple bricks with multiple CUs
        for (uint32_t j = 0; j < lcl_cu; j++) {
            // Run over each block within brick
            uint32_t index = 0;
            uint32_t brick_flag_idx = brick + j;

            //////printme("blocksPerChunk %d \n", blocksPerChunk[brick]);
            // Copy the data from various blocks in concatinated manner
            for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                uint32_t block_size = block_size_in_bytes;
                if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                    block_size = sizeOfChunk[brick_flag_idx] - index;
                }

                uint32_t compressed_size = (h_compressSize.data())[bIdx];
                std::memcpy(&out[outIdx], &h_buf_zlibout.data()[bIdx * block_size_in_bytes], compressed_size);
                outIdx += compressed_size;
            }
        }
    }

    // zlib special block based on Z_SYNC_FLUSH
    int xarg = 0;
    out[outIdx + xarg++] = 0x01;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0xff;
    out[outIdx + xarg++] = 0xff;
    outIdx += xarg;
    return outIdx;
} // Overlap end
