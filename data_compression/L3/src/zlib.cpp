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
#include "zlib.hpp"
#define FORMAT_0 31
#define FORMAT_1 139
#define VARIANT 8
#define REAL_CODE 8
#define OPCODE 3
#define CHUNK_16K 16384

using namespace xf::compression;

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

uint32_t xfZlib::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

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
xfZlib::xfZlib(const std::string& binaryFileName) {
    // Zlib Compression Binary Name
    init(binaryFileName);

    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;
    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / block_size_in_kb + 1) * block_size_in_kb;

    for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            h_buf_in[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
            h_buf_out[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 4);
            h_buf_zlibout[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 2);
            h_blksize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_dyn_ltree_freq[i][j].resize(PARALLEL_ENGINES * LTREE_SIZE);
            h_dyn_dtree_freq[i][j].resize(PARALLEL_ENGINES * DTREE_SIZE);
            h_dyn_bltree_freq[i][j].resize(PARALLEL_ENGINES * BLTREE_SIZE);
            h_dyn_ltree_codes[i][j].resize(PARALLEL_ENGINES * LTREE_SIZE);
            h_dyn_dtree_codes[i][j].resize(PARALLEL_ENGINES * DTREE_SIZE);
            h_dyn_bltree_codes[i][j].resize(PARALLEL_ENGINES * BLTREE_SIZE);
            h_dyn_ltree_blen[i][j].resize(PARALLEL_ENGINES * LTREE_SIZE);
            h_dyn_dtree_blen[i][j].resize(PARALLEL_ENGINES * DTREE_SIZE);
            h_dyn_bltree_blen[i][j].resize(PARALLEL_ENGINES * BLTREE_SIZE);

            h_buff_max_codes[i][j].resize(PARALLEL_ENGINES * MAXCODE_SIZE);
        }
    }

    // Device buffer allocation
    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    host_buffer_size, h_buf_in[cu][flag].data());

            buffer_lz77_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                          host_buffer_size * 4, h_buf_out[cu][flag].data());

            buffer_zlib_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                          host_buffer_size * 2, h_buf_zlibout[cu][flag].data());

            buffer_compress_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, temp_nblocks * sizeof(uint32_t),
                               h_compressSize[cu][flag].data());

            buffer_inblk_size[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                         temp_nblocks * sizeof(uint32_t), h_blksize[cu][flag].data());

            buffer_dyn_ltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_freq[cu][flag].data());

            buffer_dyn_dtree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_freq[cu][flag].data());

            buffer_dyn_bltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_freq[cu][flag].data());

            buffer_dyn_ltree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_codes[cu][flag].data());

            buffer_dyn_dtree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_codes[cu][flag].data());

            buffer_dyn_bltree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_codes[cu][flag].data());

            buffer_dyn_ltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, h_dyn_ltree_blen[cu][flag].data());

            buffer_dyn_dtree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, h_dyn_dtree_blen[cu][flag].data());

            buffer_dyn_bltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, h_dyn_bltree_blen[cu][flag].data());

            buffer_max_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               (PARALLEL_ENGINES * MAXCODE_SIZE) * sizeof(uint32_t), h_buff_max_codes[cu][flag].data());
        }
    }

    for (int i = 0; i < MAX_DDCOMP_UNITS; i++) {
        h_dbuf_in[i].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
        h_dbuf_zlibout[i].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 10);
        h_dcompressSize[i].resize(MAX_NUMBER_BLOCKS);
    }
}

// Destructor
xfZlib::~xfZlib() {
    release();
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            delete (buffer_input[cu][flag]);
            delete (buffer_lz77_output[cu][flag]);
            delete (buffer_zlib_output[cu][flag]);
            delete (buffer_compress_size[cu][flag]);
            delete (buffer_inblk_size[cu][flag]);

            delete (buffer_dyn_ltree_freq[cu][flag]);
            delete (buffer_dyn_dtree_freq[cu][flag]);
            delete (buffer_dyn_bltree_freq[cu][flag]);

            delete (buffer_dyn_ltree_codes[cu][flag]);
            delete (buffer_dyn_dtree_codes[cu][flag]);
            delete (buffer_dyn_bltree_codes[cu][flag]);

            delete (buffer_dyn_ltree_blen[cu][flag]);
            delete (buffer_dyn_dtree_blen[cu][flag]);
            delete (buffer_dyn_bltree_blen[cu][flag]);
        }
    }
}

int xfZlib::decompress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size) {
    int output_length = 0;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    // Call to compress
    // Zlib deCompress
    uint32_t debytes = decompress(in, out, input_size, 0);

    return debytes;
}

int xfZlib::compress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size) {
    int output_length = 0;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    out[0] = 120;
    out[1] = 1;

    // Call to compress
    // Zlib Compress
    uint32_t enbytes = compress(in, out + 2, input_size, host_buffer_size);

    out[enbytes + 1] = 0;
    out[enbytes + 2] = 0;
    out[enbytes + 3] = 0;
    out[enbytes + 4] = 0;
    out[enbytes + 5] = 0;

    enbytes += 5;

    return enbytes;
}

int xfZlib::init(const std::string& binaryFileName) {
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    // m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        m_q[i] = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    }

    for (uint8_t flag = 0; flag < D_COMPUTE_UNIT; flag++) {
        m_q_dec[flag] = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    }
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

    std::string cu_id;
    std::string comp_krnl_name = compress_kernel_names[0].c_str();
    std::string huffman_krnl_name = huffman_kernel_names[0].c_str();
    std::string treegen_krnl_name = treegen_kernel_names[0].c_str();
    std::string decomp_krnl_name = decompress_kernel_names[0].c_str();
    // Create Compress Kernels
    for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
        cu_id = std::to_string(i + 1);
        std::string krnl_name_full = comp_krnl_name + ":{" + comp_krnl_name + "_" + cu_id + "}";
        compress_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
    }

    // Create Huffman Kernel
    for (uint32_t i = 0; i < H_COMPUTE_UNIT; i++) {
        cu_id = std::to_string(i + 1);
        std::string krnl_name_full = huffman_krnl_name + ":{" + huffman_krnl_name + "_" + cu_id + "}";
        huffman_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
    }

    // Create TreeGen Kernel
    for (uint32_t i = 0; i < T_COMPUTE_UNIT; i++) {
        cu_id = std::to_string(i + 1);
        std::string krnl_name_full = treegen_krnl_name + ":{" + treegen_krnl_name + "_" + cu_id + "}";
        treegen_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
    }

    // Create Decompress Kernel
    for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) {
        cu_id = std::to_string(i + 1);
        std::string krnl_name_full = decomp_krnl_name + ":{" + decomp_krnl_name + "_" + cu_id + "}";
        decompress_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str());
    }

    return 0;
}

int xfZlib::release() {
    delete (m_program);
    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        delete (m_q[i]);
    }

    for (uint8_t flag = 0; flag < D_COMPUTE_UNIT; flag++) {
        delete (m_q_dec[flag]);
    }
    delete (m_context);

    for (int i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel[i]);

    for (int i = 0; i < H_COMPUTE_UNIT; i++) delete (huffman_kernel[i]);

    for (int i = 0; i < T_COMPUTE_UNIT; i++) delete (treegen_kernel[i]);

    for (int i = 0; i < D_COMPUTE_UNIT; i++) delete (decompress_kernel[i]);

    return 0;
}

uint32_t xfZlib::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu) {
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
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

uint32_t xfZlib::decompress(uint8_t* in, uint8_t* out, uint32_t input_size, int cu) {
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
        inP = (uint8_t*)m_q_dec[cu]->enqueueMapBuffer(*(buffer_in), CL_TRUE, CL_MAP_READ, 0, input_size);
        outP = (uint8_t*)m_q_dec[cu]->enqueueMapBuffer(*(buffer_out), CL_TRUE, CL_MAP_WRITE, 0, input_size * 10);
        outSize =
            (uint32_t*)m_q_dec[cu]->enqueueMapBuffer(*(buffer_size), CL_TRUE, CL_MAP_WRITE, 0, 10 * sizeof(uint32_t));
    } else {
        // printme("before buffer creation \n");
        buffer_in =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_size, h_dbuf_in[cu].data());

        buffer_out = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, input_size * 10,
                                    h_dbuf_zlibout[cu].data());

        buffer_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 10 * sizeof(uint32_t),
                                     h_dcompressSize[cu].data());

        inP = h_dbuf_in[cu].data();
        outP = h_dbuf_zlibout[cu].data();
        outSize = h_dcompressSize[cu].data();
    }
    // printme("Entered incopy \n");
    // Copy compressed input to h_buf_in
    std::memcpy(inP, &in[0], input_size);

    int narg = 0;
    // Set Kernel Args
    // printme("Setargs \n");
    (decompress_kernel[cu])->setArg(narg++, *(buffer_in));
    (decompress_kernel[cu])->setArg(narg++, *(buffer_out));
    (decompress_kernel[cu])->setArg(narg++, *(buffer_size));
    (decompress_kernel[cu])->setArg(narg++, input_size);

    // Migrate Memory - Map host to device buffers
    m_q_dec[cu]->enqueueMigrateMemObjects({*(buffer_in)}, 0);
    m_q_dec[cu]->finish();

    // Kernel invocation
    m_q_dec[cu]->enqueueTask(*decompress_kernel[cu]);
    m_q_dec[cu]->finish();

    // Migrate memory - Map device to host buffers
    m_q_dec[cu]->enqueueMigrateMemObjects({*(buffer_size)}, CL_MIGRATE_MEM_OBJECT_HOST);
    m_q_dec[cu]->finish();

    uint32_t raw_size = *outSize;

    // If raw size is greater than 3GB
    // Limit it to 3GB
    if (raw_size > (3U << (3 * 10))) raw_size = (3U << (3 * 10));

    m_q_dec[cu]->enqueueReadBuffer(*(buffer_out), CL_TRUE, 0, raw_size * sizeof(uint8_t), &out[0]);

    if (flag) {
        m_q_dec[cu]->enqueueUnmapMemObject(*buffer_in, inP, nullptr, nullptr);
        m_q_dec[cu]->enqueueUnmapMemObject(*buffer_out, outP, nullptr, nullptr);
        m_q_dec[cu]->enqueueUnmapMemObject(*buffer_size, outSize, nullptr, nullptr);
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
uint32_t xfZlib::compress(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t host_buffer_size) {
    //////printme("In compress \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;

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

    // Track the lags of respective chunks for left over handling
    int chunk_flags[total_chunks];
    int cu_order[total_chunks];

    // Finished bricks
    int completed_bricks = 0;

    int flag = 0;
    uint32_t lcl_cu = 0;

    uint8_t cunits = (uint8_t)C_COMPUTE_UNIT;
    uint8_t queue_idx = 0;
overlap:
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; /*brick += C_COMPUTE_UNIT,*/ itr++, flag = !flag) {
        if (cunits > 1)
            queue_idx = flag * OVERLAP_BUF_COUNT;
        else
            queue_idx = flag;

        if (total_chunks > 2)
            lcl_cu = C_COMPUTE_UNIT;
        else
            lcl_cu = 1;

        if (brick + lcl_cu > total_chunks) lcl_cu = total_chunks - brick;

        for (uint32_t cu = 0; cu < lcl_cu; cu++) {
            chunk_flags[brick + cu] = flag;
            cu_order[brick + cu] = cu;

            // Wait for read events
            if (itr >= 2) {
                // Wait on current flag previous operation to finish
                m_q[queue_idx + cu]->finish();

                // Completed bricks counter
                completed_bricks++;

                uint32_t index = 0;
                uint32_t brick_flag_idx = brick - (C_COMPUTE_UNIT * overlap_buf_count - cu);

                //////printme("blocksPerChunk %d \n", blocksPerChunk[brick]);
                // Copy the data from various blocks in concatinated manner
                for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                    uint32_t block_size = block_size_in_bytes;
                    if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                        block_size = sizeOfChunk[brick_flag_idx] - index;
                    }

                    uint32_t compressed_size = (h_compressSize[cu][flag].data())[bIdx];
                    std::memcpy(&out[outIdx], &h_buf_zlibout[cu][flag].data()[bIdx * block_size_in_bytes],
                                compressed_size);
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
                (h_blksize[cu][flag]).data()[idxblk++] = block_size;
            }

            std::memcpy(h_buf_in[cu][flag].data(), &in[(brick + cu) * host_buffer_size], sizeOfChunk[brick + cu]);

            // Set kernel arguments
            int narg = 0;

            (compress_kernel[cu])->setArg(narg++, *(buffer_input[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, *(buffer_lz77_output[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, *(buffer_compress_size[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, *(buffer_inblk_size[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag]));
            (compress_kernel[cu])->setArg(narg++, block_size_in_kb);
            (compress_kernel[cu])->setArg(narg++, sizeOfChunk[brick + cu]);

            narg = 0;
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_bltree_freq[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_codes[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_codes[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_bltree_codes[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_blen[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_blen[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_dyn_bltree_blen[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, *(buffer_max_codes[cu][flag]));
            (treegen_kernel[cu])->setArg(narg++, block_size_in_kb);
            (treegen_kernel[cu])->setArg(narg++, sizeOfChunk[brick + cu]);
            (treegen_kernel[cu])->setArg(narg++, blocksPerChunk[brick]);

            narg = 0;
            (huffman_kernel[cu])->setArg(narg++, *(buffer_lz77_output[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_zlib_output[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_compress_size[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_inblk_size[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_codes[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_codes[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_bltree_codes[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_ltree_blen[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_dtree_blen[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_dyn_bltree_blen[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, *(buffer_max_codes[cu][flag]));
            (huffman_kernel[cu])->setArg(narg++, block_size_in_kb);
            (huffman_kernel[cu])->setArg(narg++, sizeOfChunk[brick + cu]);

            // Migrate memory - Map host to device buffers
            m_q[queue_idx + cu]->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_inblk_size[cu][flag])},
                                                          0 /* 0 means from host*/);

            // kernel write events update
            // LZ77 Compress Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*compress_kernel[cu]);

            // TreeGen Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*treegen_kernel[cu]);

            // Huffman Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*huffman_kernel[cu]);

            m_q[queue_idx + cu]->enqueueMigrateMemObjects(
                {*(buffer_zlib_output[cu][flag]), *(buffer_compress_size[cu][flag])}, CL_MIGRATE_MEM_OBJECT_HOST);
        } // Internal loop runs on compute units

        if (total_chunks > 2)
            brick += C_COMPUTE_UNIT;
        else
            brick++;

    } // Main overlap loop

    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        m_q[i]->flush();
        m_q[i]->finish();
    }

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

                uint32_t compressed_size = (h_compressSize[cu][flag].data())[bIdx];
                std::memcpy(&out[outIdx], &h_buf_zlibout[cu][flag].data()[bIdx * block_size_in_bytes], compressed_size);
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
