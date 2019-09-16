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
#include "xxhash.h"
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

cl_mem_ext_ptr_t get_buffer_extension(int ddr_no) {
    cl_mem_ext_ptr_t ext;
    switch (ddr_no) {
        case 0:
            ext.flags = XCL_MEM_DDR_BANK0;
            break;
        case 1:
            ext.flags = XCL_MEM_DDR_BANK1;
            break;
        case 2:
            ext.flags = XCL_MEM_DDR_BANK2;
            break;
        default:
            ext.flags = XCL_MEM_DDR_BANK3;
    };
    return ext;
}
void zip(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
#ifdef GZIP_FLOW
    // printf("In GZIP FLOW \n");
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
    // printf("In ZLIB flow");
    outFile.put(120);
    outFile.put(1);
#endif
    outFile.write((char*)zip_out, enbytes);
#ifdef GZIP_FLOW
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

uint64_t xil_zlib::get_event_duration_ns(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    return (end_time - start_time);
}

int xil_zlib::compress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size) {
    ////printf("In compress buffer \n");
    int output_length = 0;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    out[0] = 120;
    out[1] = 1;

    // Call to compress
    // GZip Compress
    uint32_t enbytes = compress(in, out + 2, input_size, host_buffer_size);

    out[enbytes + 1] = 0;
    out[enbytes + 2] = 0;
    out[enbytes + 3] = 0;
    out[enbytes + 4] = 0;
    out[enbytes + 5] = 0;

    // for (int i = 0; i < enbytes; i++)
    //    //printf("out %d / %c \n", out[i], out[i]);

    return enbytes;
}

uint32_t xil_zlib::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    std::vector<uint8_t, aligned_allocator<uint8_t> > gzip_in(input_size);
    std::vector<uint8_t, aligned_allocator<uint8_t> > gzip_out(input_size * 2);

    inFile.read((char*)gzip_in.data(), input_size);

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    // GZip Compress
    uint32_t enbytes = compress(gzip_in.data(), gzip_out.data(), input_size, host_buffer_size);

    // Pack GZip encoded stream .gz file
    zip(inFile_name, outFile, gzip_out.data(), enbytes);

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

void xil_zlib::bufferExtensionAssignments(bool flow) {
    for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            if (flow) {
                inExt[i][j].flags = comp_ddr_nums[i];
                inExt[i][j].obj = h_buf_in[i][j].data();
                inExt[i][j].param = NULL;

                outExt[i][j].flags = comp_ddr_nums[i];
                outExt[i][j].obj = h_buf_out[i][j].data();
                outExt[i][j].param = NULL;

                gzoutExt[i][j].flags = comp_ddr_nums[i];
                gzoutExt[i][j].obj = h_buf_gzipout[i][j].data();
                gzoutExt[i][j].param = NULL;

                cssizeExt[i][j].flags = comp_ddr_nums[i];
                cssizeExt[i][j].obj = h_compressSize[i][j].data();
                cssizeExt[i][j].param = NULL;

                insizeExt[i][j].flags = comp_ddr_nums[i];
                insizeExt[i][j].obj = h_blksize[i][j].data();
                insizeExt[i][j].param = NULL;

                dltreeFreqExt[i][j].flags = comp_ddr_nums[i];
                dltreeFreqExt[i][j].obj = h_dyn_ltree_freq[i][j].data();
                dltreeFreqExt[i][j].param = NULL;

                dltreeCodesExt[i][j].flags = comp_ddr_nums[i];
                dltreeCodesExt[i][j].obj = h_dyn_ltree_codes[i][j].data();
                dltreeCodesExt[i][j].param = NULL;

                dltreeBlenExt[i][j].flags = comp_ddr_nums[i];
                dltreeBlenExt[i][j].obj = h_dyn_ltree_blen[i][j].data();
                dltreeBlenExt[i][j].param = NULL;

                ddtreeFreqExt[i][j].flags = comp_ddr_nums[i];
                ddtreeFreqExt[i][j].obj = h_dyn_dtree_freq[i][j].data();
                ddtreeFreqExt[i][j].param = NULL;

                ddtreeCodesExt[i][j].flags = comp_ddr_nums[i];
                ddtreeCodesExt[i][j].obj = h_dyn_dtree_codes[i][j].data();
                ddtreeCodesExt[i][j].param = NULL;

                ddtreeBlenExt[i][j].flags = comp_ddr_nums[i];
                ddtreeBlenExt[i][j].obj = h_dyn_dtree_blen[i][j].data();
                ddtreeBlenExt[i][j].param = NULL;

                dbltreeFreqExt[i][j].flags = comp_ddr_nums[i];
                dbltreeFreqExt[i][j].obj = h_dyn_bltree_freq[i][j].data();
                dbltreeFreqExt[i][j].param = NULL;

                dbltreeCodesExt[i][j].flags = comp_ddr_nums[i];
                dbltreeCodesExt[i][j].obj = h_dyn_bltree_codes[i][j].data();
                dbltreeCodesExt[i][j].param = NULL;

                dbltreeBlenExt[i][j].flags = comp_ddr_nums[i];
                dbltreeBlenExt[i][j].obj = h_dyn_bltree_blen[i][j].data();
                dbltreeBlenExt[i][j].param = NULL;

                maxCodeExt[i][j].flags = comp_ddr_nums[i];
                maxCodeExt[i][j].obj = h_buff_max_codes[i][j].data();
                maxCodeExt[i][j].param = NULL;
            }
        }
    }
}

// Constructor
xil_zlib::xil_zlib() {
    for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            h_buf_in[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
            h_buf_out[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 4);
            h_buf_gzipout[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 2);
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
}

// Destructor
xil_zlib::~xil_zlib() {}

int xil_zlib::init(const std::string& binaryFileName) {
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
    // std::string binaryFile = xcl::find_binary_file(device_name,binaryFileName.c_str());
    // cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    auto fileBuf = xcl::read_binary_file(binaryFileName);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

    if (m_bin_flow == 0 || SINGLE_XCLBIN) {
        // Create Compress & Huffman kernels
        for (int i = 0; i < C_COMPUTE_UNIT; i++) {
            compress_kernel[i] = new cl::Kernel(*m_program, compress_kernel_names[i].c_str());
        }
        // Create Compress & Huffman kernels
        for (int i = 0; i < H_COMPUTE_UNIT; i++) {
            huffman_kernel[i] = new cl::Kernel(*m_program, huffman_kernel_names[i].c_str());
        }

        // Create Tree generation kernel
        for (int i = 0; i < T_COMPUTE_UNIT; i++) {
            treegen_kernel[i] = new cl::Kernel(*m_program, treegen_kernel_names[i].c_str());
        }
    }
    if (m_bin_flow == 1 || SINGLE_XCLBIN) {
        // Create Decompress kernel
        for (int i = 0; i < D_COMPUTE_UNIT; i++) {
            decompress_kernel[i] = new cl::Kernel(*m_program, decompress_kernel_names[i].c_str());
        }
    }
    return 0;
}

int xil_zlib::release() {
    if (m_bin_flow == 0) {
        for (int i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel[i]);

        for (int i = 0; i < H_COMPUTE_UNIT; i++) delete (huffman_kernel[i]);

        for (int i = 0; i < T_COMPUTE_UNIT; i++) delete (treegen_kernel[i]);
    } else {
        for (int i = 0; i < D_COMPUTE_UNIT; i++) delete (decompress_kernel[i]);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);

    return 0;
}

uint32_t xil_zlib::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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
#ifdef GZIP_FLOW
    // printf("In GZIP_flow\n");
    char c = 0;
    uint8_t d_cntr = 0;

    // Magic header
    inFile.get(c);
    d_cntr++;
    inFile.get(c);
    d_cntr++;

    // 1 Byte compress method
    inFile.get(c);
    d_cntr++;

    // 1 Byte flags
    inFile.get(c);
    d_cntr++;

    // 4bytes file modification
    inFile.get(c);
    d_cntr++;
    inFile.get(c);
    d_cntr++;
    inFile.get(c);
    d_cntr++;
    inFile.get(c);
    d_cntr++;

    // 1 Byte extra flag
    inFile.get(c);
    d_cntr++;

    // 1 Byte opcode
    inFile.get(c);
    d_cntr++;

    // Read file name
    do {
        inFile.get(c);
        d_cntr++;
    } while (c != '\0');

    inFile.get(c);
    d_cntr++;
    ////printf("%d \n", c);

    // READ ZLIB header 2 bytes
    inFile.read((char*)in.data(), (input_size - d_cntr));

    // Call decompress
    // printf("Decompress called.\n");
    debytes = decompress(in.data(), out.data(), (input_size - d_cntr));
// printf("Decompress %d bytes.\n", debytes);
#else
    // READ ZLIB header 2 bytes
    inFile.read((char*)in.data(), input_size);

    // Call decompress
    debytes = decompress(in.data(), out.data(), input_size);
#endif

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

uint32_t xil_zlib::decompress(uint8_t* in, uint8_t* out, uint32_t input_size) {
    h_buf_in[0][0].resize(input_size);
    h_buf_gzipout[0][0].resize(input_size * 10);

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // Assignment to the buffer extensions
    bufferExtensionAssignments(1);

    buffer_input[0][0] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                                        input_size, &inExt[0][0]);

    buffer_gzip_output[0][0] = new cl::Buffer(
        *m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX, input_size * 10, &gzoutExt[0][0]);

    buffer_compress_size[0][0] =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX,
                       10 * sizeof(uint32_t), &cssizeExt[0][0]);

    // Copy compressed input to h_buf_in
    std::memcpy(h_buf_in[0][0].data(), &in[0], input_size);

    int narg = 0;
    // Set Kernel Args
    (decompress_kernel[0])->setArg(narg++, *(buffer_input[0][0]));
    (decompress_kernel[0])->setArg(narg++, *(buffer_gzip_output[0][0]));
    (decompress_kernel[0])->setArg(narg++, *(buffer_compress_size[0][0]));
    (decompress_kernel[0])->setArg(narg++, input_size);

    std::vector<cl::Memory> inBufVec;
    inBufVec.push_back(*(buffer_input[0][0]));

    // Migrate Memory - Map host to device buffers
    m_q->enqueueMigrateMemObjects(inBufVec, 0);
    m_q->finish();

    auto kernel_start = std::chrono::high_resolution_clock::now();

    // Kernel invocation
    // printf("Enqueue task\n");
    m_q->enqueueTask(*decompress_kernel[0]);
    m_q->finish();

    auto kernel_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(kernel_end - kernel_start);
    kernel_time_ns_1 += duration;

    std::vector<cl::Memory> outBufVec;
    outBufVec.push_back(*(buffer_gzip_output[0][0]));
    outBufVec.push_back(*(buffer_compress_size[0][0]));

    // Migrate memory - Map device to host buffers
    // printf("Migrate memory\n");
    m_q->enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
    m_q->finish();

    uint32_t raw_size = h_compressSize[0][0].data()[0];
    // printf("raw_size %d \n", raw_size);
    // Copy FPGA output to outbuffer
    std::memcpy(&out[0], &h_buf_gzipout[0][0].data()[0], raw_size);
    float throughput_in_mbps_1 = (float)input_size * 1000 / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

    delete (buffer_input[0][0]);
    delete (buffer_gzip_output[0][0]);
    delete (buffer_compress_size[0][0]);

    return raw_size;
}

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint32_t xil_zlib::compress(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t host_buffer_size) {
    ////printf("In compress \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;

    uint64_t total_compress_kernel_time = 0;
    uint64_t total_treegen_kernel_time = 0;
    uint64_t total_huffman_kernel_time = 0;

    // Read, Write and Kernel events
    cl::Event kernel_events[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Event read_events[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Event write_events[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Event tree_kevents[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];
    cl::Event huffkernel_events[MAX_CCOMP_UNITS][OVERLAP_BUF_COUNT];

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

    std::chrono::duration<double, std::nano> lz77_kernel_time(0);
    std::chrono::duration<double, std::nano> tree_kernel_time(0);
    std::chrono::duration<double, std::nano> huffman_kernel_time(0);

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

    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / block_size_in_kb + 1) * block_size_in_kb;

    // Device buffer allocation
    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            buffer_input[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size, &inExt[cu][flag]);

            buffer_lz77_output[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size * 4, &outExt[cu][flag]);

            buffer_gzip_output[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX,
                               host_buffer_size * 2, &gzoutExt[cu][flag]);

            buffer_compress_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &cssizeExt[cu][flag]);

            buffer_inblk_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX,
                               temp_nblocks * sizeof(uint32_t), &insizeExt[cu][flag]);

            buffer_dyn_ltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, &dltreeFreqExt[cu][flag]);

            buffer_dyn_dtree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, &ddtreeFreqExt[cu][flag]);

            buffer_dyn_bltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, &dbltreeFreqExt[cu][flag]);

            buffer_dyn_ltree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, &dltreeCodesExt[cu][flag]);

            buffer_dyn_dtree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, &ddtreeCodesExt[cu][flag]);

            buffer_dyn_bltree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, &dbltreeCodesExt[cu][flag]);

            buffer_dyn_ltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * LTREE_SIZE, &dltreeBlenExt[cu][flag]);

            buffer_dyn_dtree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * DTREE_SIZE, &ddtreeBlenExt[cu][flag]);

            buffer_dyn_bltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               PARALLEL_ENGINES * sizeof(uint32_t) * BLTREE_SIZE, &dbltreeBlenExt[cu][flag]);

            buffer_max_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                               (PARALLEL_ENGINES * MAXCODE_SIZE) * sizeof(uint32_t), &maxCodeExt[cu][flag]);
        }
    }

    auto dw_s = std::chrono::high_resolution_clock::now();

    // Counter which helps in tracking
    // Output buffer index
    uint32_t outIdx = 0;

    // Track the lags of respective chunks for left over handling
    int chunk_flags[total_chunks];
    int cu_order[total_chunks];

    // Finished bricks
    int completed_bricks = 0;

    int flag = 0;
    int lcl_cu = 0;

    auto total_start = std::chrono::high_resolution_clock::now();

// int chunk_flags[total_chunks];
// int cu_order[total_chunks];

overlap:
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; /*brick += C_COMPUTE_UNIT,*/ itr++, flag = !flag) {
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
                read_events[cu][flag].wait();

                // Completed bricks counter
                completed_bricks++;

                // Accumulater kernel time
                total_compress_kernel_time += get_event_duration_ns(kernel_events[cu][flag]);
                total_treegen_kernel_time += get_event_duration_ns(tree_kevents[cu][flag]);
                total_huffman_kernel_time += get_event_duration_ns(huffkernel_events[cu][flag]);

                uint32_t index = 0;
                uint32_t brick_flag_idx = brick - (C_COMPUTE_UNIT * overlap_buf_count - cu);

                ////printf("blocksPerChunk %d \n", blocksPerChunk[brick]);
                // Copy the data from various blocks in concatinated manner
                for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                    uint32_t block_size = block_size_in_bytes;
                    if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                        block_size = sizeOfChunk[brick_flag_idx] - index;
                    }

                    uint32_t compressed_size = (h_compressSize[cu][flag].data())[bIdx];
                    std::memcpy(&out[outIdx], &h_buf_gzipout[cu][flag].data()[bIdx * block_size_in_bytes],
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
                ////printf("sizeofChunk %d block_size %d cu %d \n", sizeOfChunk[brick+cu], block_size, cu);
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
            (huffman_kernel[cu])->setArg(narg++, *(buffer_gzip_output[cu][flag]));
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
            m_q->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_inblk_size[cu][flag])},
                                          0 /* 0 means from host*/, NULL, &(write_events[cu][flag]));

            // kernel wait events for writing and compute
            std::vector<cl::Event> kernelWriteWait;
            std::vector<cl::Event> kernelComputeWait;
            std::vector<cl::Event> TreekernelComputeWait;
            std::vector<cl::Event> HuffkernelComputeWait;

            // kernel write events update
            kernelWriteWait.push_back(write_events[cu][flag]);

            // LZ77 Compress Fire Kernel invocation
            m_q->enqueueTask(*compress_kernel[cu], &kernelWriteWait, &kernel_events[cu][flag]);

            // update kernel events flag on computation
            kernelComputeWait.push_back(kernel_events[cu][flag]);

            // TreeGen Fire Kernel invocation
            m_q->enqueueTask(*treegen_kernel[cu], &kernelComputeWait, &tree_kevents[cu][flag]);

            TreekernelComputeWait.push_back(tree_kevents[cu][flag]);

            // Huffman Fire Kernel invocation
            m_q->enqueueTask(*huffman_kernel[cu], &TreekernelComputeWait, &huffkernel_events[cu][flag]);

            HuffkernelComputeWait.push_back(huffkernel_events[cu][flag]);

            m_q->enqueueMigrateMemObjects({*(buffer_gzip_output[cu][flag]), *(buffer_compress_size[cu][flag])},
                                          CL_MIGRATE_MEM_OBJECT_HOST, &HuffkernelComputeWait, &(read_events[cu][flag]));
        } // Internal loop runs on compute units

        if (total_chunks > 2)
            brick += C_COMPUTE_UNIT;
        else
            brick++;

    } // Main overlap loop
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

            // Run over each block within brick
            uint32_t index = 0;
            uint32_t brick_flag_idx = brick + j;

            ////printf("blocksPerChunk %d \n", blocksPerChunk[brick]);
            // Copy the data from various blocks in concatinated manner
            for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++, index += block_size_in_bytes) {
                uint32_t block_size = block_size_in_bytes;
                if (index + block_size > sizeOfChunk[brick_flag_idx]) {
                    block_size = sizeOfChunk[brick_flag_idx] - index;
                }

                uint32_t compressed_size = (h_compressSize[cu][flag].data())[bIdx];
                std::memcpy(&out[outIdx], &h_buf_gzipout[cu][flag].data()[bIdx * block_size_in_bytes], compressed_size);
                outIdx += compressed_size;
            }
        }
    }

    // GZip special block based on Z_SYNC_FLUSH
    int xarg = 0;
    out[outIdx + xarg++] = 0x01;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0xff;
    out[outIdx + xarg++] = 0xff;
    outIdx += xarg;

    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            delete (buffer_input[cu][flag]);
            delete (buffer_lz77_output[cu][flag]);
            delete (buffer_gzip_output[cu][flag]);
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

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();

    float compress_kt_in_mbps_1 = (float)input_size * 1000 / total_compress_kernel_time;
    float treegen_kt_in_mbps_1 = (float)input_size * 1000 / total_treegen_kernel_time;
    float huffman_kt_in_mbps_1 = (float)input_size * 1000 / total_huffman_kernel_time;

#ifdef EVENT_PROFILE
    std::cout << "Total Kernel Time " << total_kernel_time << std::endl;
    std::cout << "Total Write Time " << total_write_time << std::endl;
    std::cout << "Total Read Time " << total_read_time << std::endl;
#endif
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    std::cout << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << compress_kt_in_mbps_1;

    // float lz77_kt = (float)input_size * 1000 / lz77_kernel_time.count();
    // float tree_kt = (float)input_size * 1000 / tree_kernel_time.count();
    // float huff_kt = (float)input_size * 1000 / huffman_kernel_time.count();

    // std::cout << std::fixed << std::setprecision(2) << lz77_kt << "\t";
    // std::cout << std::fixed << std::setprecision(2) << tree_kt << "\t";
    // std::cout << std::fixed << std::setprecision(2) << huff_kt;
    ////printf("Done with compress \n");
    ////printf("outIdx %d \n", outIdx);
    return outIdx;
} // Overlap end
