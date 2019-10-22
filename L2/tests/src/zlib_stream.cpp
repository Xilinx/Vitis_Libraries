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
#include "zlib_stream.hpp"
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

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

uint32_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint32_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

// Constructor
xfZlibStream::xfZlibStream() {
    h_dbuf_in.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
    h_dbuf_gzipout.resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 10);
    h_dcompressSize.resize(MAX_NUMBER_BLOCKS);
}

// Destructor
xfZlibStream::~xfZlibStream() {}

int xfZlibStream::init(const std::string& binaryFileName) {
    // cl_int err;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    m_device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(m_device);
    m_q_dec =
        new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);

    m_q_dm =
        new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = m_device.getInfo<CL_DEVICE_NAME>();
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

    // Create Decompress kernel
    data_mover_kernel = new cl::Kernel(*m_program, data_mover_kernel_name.c_str());
    decompress_kernel = new cl::Kernel(*m_program, decompress_kernel_name.c_str());

    return 0;
}

int xfZlibStream::release() {
    delete (m_program);
    delete (m_q_dec);
    delete (m_context);

    delete (decompress_kernel);
    delete (data_mover_kernel);

    return 0;
}

uint32_t xfZlibStream::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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
#ifdef GZIP_FLOW
    ////printme("In GZIP_flow");
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
    //////printme("%d \n", c);

    // READ ZLIB header 2 bytes
    inFile.read((char*)in.data(), (input_size - d_cntr));

    // Call decompress
    debytes = decompress(in.data(), out.data(), (input_size - d_cntr));

#else
    // READ ZLIB header 2 bytes
    inFile.read((char*)in.data(), input_size);
    // printme("Call to zlib_decompress \n");
    // Call decompress
    auto decompress_API_start = std::chrono::high_resolution_clock::now();
    debytes = decompress(in.data(), out.data(), input_size);
    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    decompress_API_time_ns_1 += duration;

    float throughput_in_mbps_1 = (float)debytes * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << "Throughput E2E:" << throughput_in_mbps_1 << "MBps" << std::endl;

#endif

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

uint32_t xfZlibStream::decompress(uint8_t* in, uint8_t* out, uint32_t input_size) {
    // cl_int err;
    uint32_t inBufferSize = 2 * 1024 * 1024;
    uint32_t bufferCount = 1 + (input_size - 1) / inBufferSize;

    // if input_size if greater than 2 MB, then buffer size must be 2MB
    if (input_size < inBufferSize) inBufferSize = input_size;

    h_dbuf_in.resize(inBufferSize);
    h_dbuf_gzipout.resize(inBufferSize * 10);
    h_dcompressSize.resize(10);

    uint8_t* inP = nullptr;
    uint8_t* outP = nullptr;
    uint32_t* outSize = nullptr;
    cl::Buffer* buffer_in;
    cl::Buffer* buffer_out;
    cl::Buffer* buffer_size;

    buffer_in = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, inBufferSize, h_dbuf_in.data());

    buffer_out =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, inBufferSize * 10, h_dbuf_gzipout.data());

    buffer_size = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 10 * sizeof(uint32_t),
                                 h_dcompressSize.data());

    inP = h_dbuf_in.data();
    outP = h_dbuf_gzipout.data();
    outSize = h_dcompressSize.data();

    // Set Kernel Args
    int narg = 0;
    data_mover_kernel->setArg(narg++, *(buffer_in));
    data_mover_kernel->setArg(narg++, *(buffer_out));
    data_mover_kernel->setArg(narg++, *(buffer_size));

    decompress_kernel->setArg(0, input_size);

    // enqueue decompression kernel
    m_q_dec->enqueueTask(*decompress_kernel);

    uint32_t cBufSize = inBufferSize;
    uint32_t decmpSizeIdx = 0;
    for (uint32_t bufIdx = 0; bufIdx < bufferCount; bufIdx++) {
        if (bufferCount > 1 && bufIdx == bufferCount - 1) {
            cBufSize = input_size - (inBufferSize * bufIdx);
            std::memset(inP, '\0', inBufferSize);
        }
        // Copy compressed input to h_buf_in
        std::memcpy(inP, in + (bufIdx * inBufferSize), cBufSize);

        // set input_size as current block size for data mover, so that it can read/write from host in chunks
        data_mover_kernel->setArg(narg, cBufSize);

        // Migrate Memory - Map host to device buffers
        m_q_dm->enqueueMigrateMemObjects({*(buffer_in)}, 0);
        m_q_dm->finish();

        // Kernel invocation
        m_q_dm->enqueueTask(*data_mover_kernel);
        m_q_dm->finish();
        // printme("kernel done \n");

        // Migrate memory - Map device to host buffers
        m_q_dm->enqueueMigrateMemObjects({*(buffer_out), *(buffer_size)}, CL_MIGRATE_MEM_OBJECT_HOST);
        m_q_dm->finish();

        uint32_t raw_size = *outSize;
        std::memcpy(out, outP + decmpSizeIdx, raw_size);
        decmpSizeIdx += raw_size;
    }
    // wait for decompression kernel
    m_q_dec->finish();

    delete (buffer_in);
    delete (buffer_out);
    delete (buffer_size);

    // printme("Done with decompress \n");
    return decmpSizeIdx;
}
