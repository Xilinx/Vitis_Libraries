/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
#include "zstd.hpp"
#include <fcntl.h>  /* For O_RDWR */
#include <unistd.h> /* For open(), creat() */
#include <sys/stat.h>

int fd_p2p_c_in = 0;
const int RESIDUE_4K = 4096;

uint64_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

constexpr uint32_t roundoff(uint32_t x, uint32_t y) {
    return ((x - 1) / (y) + 1);
}

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

// Constructor
xil_zstd::xil_zstd(const std::string& binaryFileName, uint8_t max_cr, uint8_t device_id) {
    m_max_cr = max_cr;
    m_deviceid = device_id;
    init(binaryFileName);
}

// Destructor
xil_zstd::~xil_zstd() {
    release();
}

int xil_zstd::init(const std::string& binaryFileName) {
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[m_deviceid];

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Using Device: " << device_name << std::endl;

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);

    m_q_dec = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_rd = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_wr = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    // import_binary() command will find the OpenCL binary file created using the
    // v++ compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFileName);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    m_program = new cl::Program(*m_context, {device}, bins);

    // Create Decompress kernel
    decompress_kernel = new cl::Kernel(*m_program, decompress_kernel_name.c_str());
    data_writer_kernel = new cl::Kernel(*m_program, data_writer_kernel_name.c_str());
    data_reader_kernel = new cl::Kernel(*m_program, data_reader_kernel_name.c_str());

    return 0;
}

int xil_zstd::release() {
    delete decompress_kernel;
    delete data_writer_kernel;
    delete data_reader_kernel;

    delete (m_q_dec);
    delete (m_q_rd);
    delete (m_q_wr);

    delete (m_program);

    delete (m_context);

    return 0;
}

uint64_t xil_zstd::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
    uint8_t c_max_cr = m_max_cr;

    std::vector<uint8_t, aligned_allocator<uint8_t> > in;
    // Allocat output size
    // 8 - Max CR per file expected, if this size is big
    // Decompression crashes
    std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size * c_max_cr);
    uint32_t debytes = 0;

    MEM_ALLOC_CHECK(out.resize(input_size * c_max_cr), input_size * c_max_cr, "Output Buffer");

    // printme("In decompress_file \n");
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    MEM_ALLOC_CHECK(in.resize(input_size), input_size, "Input Buffer");
    inFile.read((char*)in.data(), input_size);
    // printme("Call to zlib_decompress \n");
    // Call decompress
    debytes = decompressSeq(in.data(), out.data(), input_size);
    // Close file
    inFile.close();

    outFile.write((char*)out.data(), debytes);
    outFile.close();

    return debytes;
}

uint64_t xil_zstd::decompressSeq(uint8_t* in, uint8_t* out, uint64_t input_size) {
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);
    uint32_t inBufferSize = input_size;
    const uint64_t max_outbuf_size = input_size * m_max_cr;
    const uint32_t lim_4gb = (uint32_t)(((uint64_t)4 * 1024 * 1024 * 1024) - 2); // 4GB limit on output size
    uint32_t outBufferSize = 0;
    // allocate < 4GB size for output buffer
    if (max_outbuf_size > lim_4gb) {
        outBufferSize = lim_4gb;
    } else {
        outBufferSize = (uint32_t)max_outbuf_size;
    }
    // host allocated aligned memory
    std::vector<uint8_t, aligned_allocator<uint8_t> > dbuf_in(inBufferSize);
    std::vector<uint8_t, aligned_allocator<uint8_t> > dbuf_out(outBufferSize);
    std::vector<uint64_t, aligned_allocator<uint64_t> > dbuf_outSize(2);
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_dcompressStatus(2);

    // opencl buffer creation
    cl::Buffer* buffer_dec_input =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, inBufferSize, dbuf_in.data());
    cl::Buffer* buffer_dec_output =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, outBufferSize, dbuf_out.data());

    cl::Buffer* buffer_size =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), dbuf_outSize.data());
    cl::Buffer* buffer_status =
        new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(uint32_t), h_dcompressStatus.data());

    // Set Kernel Args
    decompress_kernel->setArg(0, input_size);

    data_writer_kernel->setArg(0, *(buffer_dec_input));
    data_writer_kernel->setArg(1, inBufferSize);

    data_reader_kernel->setArg(0, *(buffer_dec_output));
    data_reader_kernel->setArg(1, *(buffer_size));
    data_reader_kernel->setArg(2, *(buffer_status));
    data_reader_kernel->setArg(3, outBufferSize);

    // Copy input data
    std::memcpy(dbuf_in.data(), in, inBufferSize); // must be equal to input_size
    m_q_wr->enqueueMigrateMemObjects({*(buffer_dec_input)}, 0, NULL, NULL);
    m_q_wr->finish();

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;

    // make sure that command queue is empty before decompression kernel enqueue
    m_q_dec->finish();

    // enqueue data movers
    m_q_wr->enqueueTask(*data_writer_kernel);
    m_q_rd->enqueueTask(*data_reader_kernel);

    // enqueue decompression kernel
    auto decompress_API_start = std::chrono::high_resolution_clock::now();

    m_q_dec->enqueueTask(*decompress_kernel);
    m_q_dec->finish();

    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    decompress_API_time_ns_1 += duration;

    // wait for reader to finish
    m_q_rd->finish();
    // copy decompressed output data
    m_q_rd->enqueueMigrateMemObjects({*(buffer_size), *(buffer_dec_output)}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, NULL);
    m_q_rd->finish();
    // decompressed size
    decmpSizeIdx = dbuf_outSize[0];
    // copy output decompressed data
    std::memcpy(out, dbuf_out.data(), decmpSizeIdx);

    float throughput_in_mbps_1 = (float)decmpSizeIdx * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

    // free the cl buffers
    delete buffer_dec_input;
    delete buffer_dec_output;
    delete buffer_size;
    delete buffer_status;

    // printme("Done with decompress \n");
    return decmpSizeIdx;
}
