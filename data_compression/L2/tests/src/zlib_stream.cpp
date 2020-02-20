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
xfZlibStream::xfZlibStream(const std::string& binaryFile) {
    // initialize the device
    init(binaryFile);
    // initialize the buffers
    for (int j = 0; j < DIN_BUFFERCOUNT; ++j) h_dbuf_in[j].resize(INPUT_BUFFER_SIZE);

    for (int j = 0; j < DOUT_BUFFERCOUNT; ++j) {
        h_dbuf_zlibout[j].resize(OUTPUT_BUFFER_SIZE);
        h_dcompressSize[j].resize(sizeof(uint32_t));
    }
}

// Destructor
xfZlibStream::~xfZlibStream() {
    release();
}

int xfZlibStream::init(const std::string& binaryFileName) {
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    m_device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(m_device);

    m_q_dec = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_PROFILING_ENABLE);
    m_q_rd = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_PROFILING_ENABLE);
    m_q_rdd = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_PROFILING_ENABLE);
    m_q_wr = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_PROFILING_ENABLE);
    m_q_wrd = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_PROFILING_ENABLE);

    std::string device_name = m_device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // v++ compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    auto fileBuf = xcl::read_binary_file(binaryFileName);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

    // Create Decompress kernel
    data_writer_kernel = new cl::Kernel(*m_program, data_writer_kernel_name.c_str());
    data_reader_kernel = new cl::Kernel(*m_program, data_reader_kernel_name.c_str());
    decompress_kernel = new cl::Kernel(*m_program, decompress_kernel_name.c_str());

    return 0;
}

int xfZlibStream::release() {
    delete (m_program);

    delete (m_q_dec);
    delete (m_q_rd);
    delete (m_q_rdd);
    delete (m_q_wr);
    delete (m_q_wrd);

    delete (m_context);

    delete (decompress_kernel);
    delete (data_writer_kernel);
    delete (data_reader_kernel);

    return 0;
}

uint32_t xfZlibStream::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
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
    // Call decompress
    debytes = decompress(in.data(), out.data(), input_size);

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

// method to enqueue reads in parallel with writes to decompression kernel
void xfZlibStream::_enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize) {
    const int BUFCNT = DOUT_BUFFERCOUNT; // mandatorily 2
    cl::Event hostReadEvent;
    cl::Event kernelReadEvent;
    cl::Event sizeReadEvent;

    uint8_t* outP = nullptr;
    uint32_t* outSize = nullptr;
    uint32_t dcmpSize = 0;
    cl::Buffer* buffer_out[BUFCNT];
    cl::Buffer* buffer_size[BUFCNT];
    for (int i = 0; i < BUFCNT; i++) {
        buffer_out[i] =
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, bufSize, h_dbuf_zlibout[i].data());

        buffer_size[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 2 * sizeof(uint32_t),
                                        h_dcompressSize[i].data());
    }

    // enqueue first set of buffers
    uint8_t cbf_idx = 0;
    uint32_t keq_idx = 0;
    uint32_t raw_size = 0;
    // set consistent buffer size to be read
    data_reader_kernel->setArg(2, bufSize);
    do {
        // set reader kernel arguments
        data_reader_kernel->setArg(0, *(buffer_out[cbf_idx]));
        data_reader_kernel->setArg(1, *(buffer_size[cbf_idx]));

        // enqueue reader kernel
        m_q_rd->enqueueTask(*data_reader_kernel, NULL, &kernelReadEvent);

        // copy previous data
        if (keq_idx > 0) {
            outP = h_dbuf_zlibout[abs(cbf_idx - 1)].data();
            uint32_t sz2read = raw_size;
            if (sz2read > bufSize) {
                sz2read--;
            }
            hostReadEvent.wait(); // wait for previous data migration to complete
            std::memcpy(out + dcmpSize, outP, sz2read);
            dcmpSize += sz2read;
        }
        // wait for kernel read to complete after copying previous data
        kernelReadEvent.wait();

        m_q_rdd->enqueueMigrateMemObjects({*(buffer_size[cbf_idx])}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &sizeReadEvent);
        sizeReadEvent.wait();

        outSize = h_dcompressSize[cbf_idx].data();
        raw_size = *outSize;
        if (raw_size > 0) {
            m_q_rdd->enqueueMigrateMemObjects({*(buffer_out[cbf_idx])}, CL_MIGRATE_MEM_OBJECT_HOST, NULL,
                                              &hostReadEvent);
        }
        ++keq_idx;
        cbf_idx = ((cbf_idx == 0) ? 1 : 0); // since only two buffers
    } while (raw_size == bufSize);

    // read the last block of data
    outP = h_dbuf_zlibout[abs(cbf_idx - 1)].data();
    if (raw_size > bufSize) {
        raw_size--;
    }
    hostReadEvent.wait();
    std::memcpy(out + dcmpSize, outP, raw_size);
    dcmpSize += raw_size;

    *decompSize = dcmpSize;

    // free the buffers
    for (int i = 0; i < BUFCNT; i++) {
        delete (buffer_out[i]);
        delete (buffer_size[i]);
    }
}

// method to enqueue writes in parallel with reads from decompression kernel
void xfZlibStream::_enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize) {
    const int BUFCNT = DIN_BUFFERCOUNT;

    uint32_t bufferCount = 1 + (inputSize - 1) / bufSize;

    uint8_t* inP = nullptr;
    cl::Buffer* buffer_in[BUFCNT];
    cl::Event hostWriteEvent[bufferCount];
    cl::Event kernelWriteEvent[bufferCount];

    for (int i = 0; i < BUFCNT; i++) {
        buffer_in[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, bufSize, h_dbuf_in[i].data());
    }

    uint32_t cBufSize = bufSize;
    uint8_t cbf_idx = 0;  // index indicating current buffer(0-4) being used in loop
    uint32_t keq_idx = 0; // index indicating the number of writer kernel enqueues

    // enqueue first set of buffers
    for (uint32_t bnum = 0; bnum < BUFCNT && bnum < bufferCount; ++bnum) {
        std::vector<cl::Event> hostWriteWait;
        inP = h_dbuf_in[bnum].data();
        // set for last and other buffers
        if (bnum == bufferCount - 1) {
            if (bufferCount > 1) {
                cBufSize = inputSize - (bufSize * bnum);
            }
        }
        std::memcpy(inP, in + (bnum * bufSize), cBufSize);

        // set kernel arguments
        data_writer_kernel->setArg(0, *(buffer_in[bnum]));
        data_writer_kernel->setArg(1, cBufSize);

        m_q_wrd->enqueueMigrateMemObjects({*(buffer_in[bnum])}, 0, NULL, &(hostWriteEvent[bnum]));

        hostWriteWait.push_back(hostWriteEvent[bnum]);
        m_q_wr->enqueueTask(*data_writer_kernel, &hostWriteWait, &(kernelWriteEvent[bnum]));
    }

    if (bufferCount >= BUFCNT) {
        // sequencially enqueue data transfers as the write kernels finish in order
        for (keq_idx = BUFCNT; keq_idx < bufferCount; ++keq_idx) {
            cbf_idx = keq_idx % BUFCNT;
            inP = h_dbuf_in[cbf_idx].data();
            // set for last and other buffers
            if (keq_idx == bufferCount - 1) {
                if (bufferCount > 1) {
                    cBufSize = inputSize - (bufSize * keq_idx);
                }
            }
            std::vector<cl::Event> hostWriteWait;
            std::vector<cl::Event> kernelWriteWait;

            // copy the data
            std::memcpy(inP, in + (keq_idx * bufSize), cBufSize);

            // wait for (current - BUFCNT) kernel to finish
            // cl::Event::waitForEvents(kernelWriteWait);
            (kernelWriteEvent[keq_idx - BUFCNT]).wait();

            // set kernel arguments
            data_writer_kernel->setArg(0, *(buffer_in[cbf_idx]));
            data_writer_kernel->setArg(1, cBufSize);

            m_q_wrd->enqueueMigrateMemObjects({*(buffer_in[cbf_idx])}, 0, NULL, &(hostWriteEvent[keq_idx]));

            hostWriteWait.push_back(hostWriteEvent[keq_idx]); // data tranfer to wait for
            m_q_wr->enqueueTask(*data_writer_kernel, &hostWriteWait, &(kernelWriteEvent[keq_idx]));
        }
    }
    // std::cout << "Writer Enqueued" << std::endl;
    m_q_wr->finish();
    // std::cout << "Data Write successfull" << std::endl;
    for (int i = 0; i < BUFCNT; i++) delete (buffer_in[i]);
}

uint32_t xfZlibStream::decompress(uint8_t* in, uint8_t* out, uint32_t input_size) {
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);
    uint32_t inBufferSize = INPUT_BUFFER_SIZE;
    uint32_t outBufferSize = OUTPUT_BUFFER_SIZE;

    // if input_size if greater than 2 MB, then buffer size must be 2MB
    if (input_size < inBufferSize) inBufferSize = input_size;

    // Set Kernel Args
    decompress_kernel->setArg(0, input_size);

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;
    std::thread decompWriter(&xfZlibStream::_enqueue_writes, this, inBufferSize, in, input_size);
    std::thread decompReader(&xfZlibStream::_enqueue_reads, this, outBufferSize, out, &decmpSizeIdx);

    // enqueue decompression kernel
    m_q_dec->finish();
    sleep(2);
    auto decompress_API_start = std::chrono::high_resolution_clock::now();

    m_q_dec->enqueueTask(*decompress_kernel);
    m_q_dec->finish();

    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    decompress_API_time_ns_1 += duration;

    decompReader.join();
    decompWriter.join();

    float throughput_in_mbps_1 = (float)decmpSizeIdx * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

    // printme("Done with decompress \n");
    return decmpSizeIdx;
}
