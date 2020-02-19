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
#include "gzip.hpp"

uint32_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint32_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void zip(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
    const uint16_t c_format_0 = 31;
    const uint16_t c_format_1 = 139;
    const uint16_t c_variant = 8;
    const uint16_t c_real_code = 8;
    const uint16_t c_opcode = 3;

    // 2 bytes of magic header
    outFile.put(c_format_0);
    outFile.put(c_format_1);

    // 1 byte Compression method
    outFile.put(c_variant);

    // 1 byte flags
    uint8_t flags = 0;
    flags |= c_real_code;
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
    outFile.put(c_opcode);

    // Dump file name
    for (int i = 0; inFile_name[i] != '\0'; i++) {
        outFile.put(inFile_name[i]);
    }

    outFile.put(0);
    outFile.write((char*)zip_out, enbytes);

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

    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

uint32_t xil_gzip::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::chrono::duration<double, std::nano> compress_API_time_ns_1(0);
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

    // gzip Compress
    uint32_t enbytes = compress(gzip_in.data(), gzip_out.data(), input_size, host_buffer_size);

    // Pack gzip encoded stream .gz file
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

// Constructor
xil_gzip::xil_gzip(const std::string& binaryFileName, uint8_t max_cr) {
    // GZip Compression Binary Name
    init(binaryFileName);

    m_max_cr = max_cr;

    // printf("C_COMPUTE_UNIT \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;
    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / block_size_in_kb + 1) * block_size_in_kb;

    const uint16_t c_ltree_size = 1024;
    const uint16_t c_dtree_size = 64;
    const uint16_t c_bltree_size = 64;
    const uint16_t c_maxcode_size = 16;

    for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            // Index calculation
            h_buf_in[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE);
            h_buf_out[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 4);
            h_buf_gzipout[i][j].resize(PARALLEL_ENGINES * HOST_BUFFER_SIZE * 2);
            h_blksize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS);
            h_dyn_ltree_freq[i][j].resize(PARALLEL_ENGINES * c_ltree_size);
            h_dyn_dtree_freq[i][j].resize(PARALLEL_ENGINES * c_dtree_size);
            h_dyn_bltree_freq[i][j].resize(PARALLEL_ENGINES * c_bltree_size);
            h_dyn_ltree_codes[i][j].resize(PARALLEL_ENGINES * c_ltree_size);
            h_dyn_dtree_codes[i][j].resize(PARALLEL_ENGINES * c_dtree_size);
            h_dyn_bltree_codes[i][j].resize(PARALLEL_ENGINES * c_bltree_size);
            h_dyn_ltree_blen[i][j].resize(PARALLEL_ENGINES * c_ltree_size);
            h_dyn_dtree_blen[i][j].resize(PARALLEL_ENGINES * c_dtree_size);
            h_dyn_bltree_blen[i][j].resize(PARALLEL_ENGINES * c_bltree_size);

            h_buff_max_codes[i][j].resize(PARALLEL_ENGINES * c_maxcode_size);
        }
    }
    // Device buffer allocation
    for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
        for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
            buffer_input[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    host_buffer_size, h_buf_in[cu][flag].data());

            buffer_lz77_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                          host_buffer_size * 4, h_buf_out[cu][flag].data());

            buffer_gzip_output[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                          host_buffer_size * 2, h_buf_gzipout[cu][flag].data());

            buffer_compress_size[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, temp_nblocks * sizeof(uint32_t),
                               h_compressSize[cu][flag].data());

            buffer_inblk_size[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                         temp_nblocks * sizeof(uint32_t), h_blksize[cu][flag].data());

            buffer_dyn_ltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_ltree_size, h_dyn_ltree_freq[cu][flag].data());

            buffer_dyn_dtree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_dtree_size, h_dyn_dtree_freq[cu][flag].data());

            buffer_dyn_bltree_freq[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_bltree_size, h_dyn_bltree_freq[cu][flag].data());

            buffer_dyn_ltree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_ltree_size, h_dyn_ltree_codes[cu][flag].data());

            buffer_dyn_dtree_codes[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_dtree_size, h_dyn_dtree_codes[cu][flag].data());

            buffer_dyn_bltree_codes[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                               PARALLEL_ENGINES * sizeof(uint32_t) * c_bltree_size,
                                                               h_dyn_bltree_codes[cu][flag].data());

            buffer_dyn_ltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_ltree_size, h_dyn_ltree_blen[cu][flag].data());

            buffer_dyn_dtree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_dtree_size, h_dyn_dtree_blen[cu][flag].data());

            buffer_dyn_bltree_blen[cu][flag] =
                new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               PARALLEL_ENGINES * sizeof(uint32_t) * c_bltree_size, h_dyn_bltree_blen[cu][flag].data());

            buffer_max_codes[cu][flag] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                        (PARALLEL_ENGINES * c_maxcode_size) * sizeof(uint32_t),
                                                        h_buff_max_codes[cu][flag].data());
        }
    }

    // initialize the buffers
    for (int j = 0; j < DIN_BUFFERCOUNT; ++j) h_dbuf_in[j].resize(INPUT_BUFFER_SIZE);

    for (int j = 0; j < DOUT_BUFFERCOUNT; ++j) {
        h_dbuf_gzipout[j].resize(OUTPUT_BUFFER_SIZE);
        h_dcompressSize[j].resize(sizeof(uint32_t));
    }
}

// Destructor
xil_gzip::~xil_gzip() {
    release();
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
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
}

int xil_gzip::init(const std::string& binaryFileName) {
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(device);
    // m_q = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        m_q[i] = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    }

    m_q_dec = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_rd = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_rdd = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_wr = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);
    m_q_wrd = new cl::CommandQueue(*m_context, device, CL_QUEUE_PROFILING_ENABLE);

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // v++ compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    // std::string binaryFile = xcl::find_binary_file(device_name,binaryFileName.c_str());
    // cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    auto fileBuf = xcl::read_binary_file(binaryFileName);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    m_program = new cl::Program(*m_context, devices, bins);

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

    // Create Decompress kernel
    for (int i = 0; i < D_COMPUTE_UNIT; i++) {
        decompress_kernel = new cl::Kernel(*m_program, decompress_kernel_names[0].c_str());
        data_writer_kernel = new cl::Kernel(*m_program, data_writer_kernel_names[0].c_str());
        data_reader_kernel = new cl::Kernel(*m_program, data_reader_kernel_names[0].c_str());
    }

    return 0;
}

int xil_gzip::release() {
    delete (m_program);
    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        delete (m_q[i]);
    }

    delete (m_context);

    for (int i = 0; i < C_COMPUTE_UNIT; i++) delete (compress_kernel[i]);

    for (int i = 0; i < H_COMPUTE_UNIT; i++) delete (huffman_kernel[i]);

    for (int i = 0; i < T_COMPUTE_UNIT; i++) delete (treegen_kernel[i]);

    for (int i = 0; i < D_COMPUTE_UNIT; i++) {
        delete decompress_kernel;
        delete data_writer_kernel;
        delete data_reader_kernel;
        delete m_q_dec;
        delete m_q_rd;
        delete m_q_rdd;
        delete m_q_wr;
        delete m_q_wrd;
    }

    return 0;
}

uint32_t xil_gzip::decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu) {
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
    std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size * m_max_cr);
    uint32_t debytes = 0;
    int infile_cntr = 0;

    // Read Magic Header
    inFile.get();
    inFile.get();
    inFile.get();
    inFile.get();

    // Read opcodes
    inFile.get();
    inFile.get();
    inFile.get();
    inFile.get();

    // Read flags
    inFile.get();
    inFile.get();

    char c;
    inFile.get(c);
    infile_cntr = 11;
    // Read the file name data
    // dont use it at present
    // In future we may need it.
    while (c != '\0') {
        inFile.get(c);
        infile_cntr++;
    }

    // Read GZIP header 2 bytes
    // Skip first two bytes as per Zlib in decompress kernel
    // this is not required for GZip kernel, to avoid modifications to
    // decompress kernel.
    inFile.read((char*)in.data() + 2, input_size - (infile_cntr));

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

// method to enqueue reads in parallel with writes to decompression kernel
void xil_gzip::_enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize, uint32_t max_outbuf_size) {
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
            new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, bufSize, h_dbuf_gzipout[i].data());

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
            outP = h_dbuf_gzipout[abs(cbf_idx - 1)].data();
            uint32_t sz2read = raw_size;
            if (sz2read > bufSize) {
                sz2read--;
            }
            hostReadEvent.wait(); // wait for previous data migration to complete
            std::memcpy(out + dcmpSize, outP, sz2read);
            dcmpSize += sz2read;
            if (dcmpSize > max_outbuf_size) {
                std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during decompression \033[0m \n"
                          << std::endl;
                std::cout
                    << "\x1B[35mUse -mcr option to increase the maximum compression ratio (Default: 10) \033[0m \n"
                    << std::endl;
                std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
                exit(1);
            }
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
    outP = h_dbuf_gzipout[abs(cbf_idx - 1)].data();
    if (raw_size > bufSize) {
        raw_size--;
    }
    hostReadEvent.wait();
    if (raw_size > max_outbuf_size) {
        std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during decompression \033[0m \n" << std::endl;
        std::cout << "\x1B[35mUse -mcr option to increase the maximum compression ratio (Default: 10) \033[0m \n"
                  << std::endl;
        std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
        exit(1);
    }
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
void xil_gzip::_enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize) {
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

uint32_t xil_gzip::decompress(uint8_t* in, uint8_t* out, uint32_t input_size, int cu) {
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);
    uint32_t inBufferSize = INPUT_BUFFER_SIZE;
    uint32_t outBufferSize = OUTPUT_BUFFER_SIZE;
    const uint32_t max_outbuf_size = input_size * m_max_cr;
    // if input_size if greater than 2 MB, then buffer size must be 2MB
    if (input_size < inBufferSize) inBufferSize = input_size;

    // Set Kernel Args
    decompress_kernel->setArg(0, input_size);

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;
    std::thread decompWriter(&xil_gzip::_enqueue_writes, this, inBufferSize, in, input_size);
    std::thread decompReader(&xil_gzip::_enqueue_reads, this, outBufferSize, out, &decmpSizeIdx, max_outbuf_size);

    // enqueue decompression kernel
    m_q_dec->finish();

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
// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint32_t xil_gzip::compress(uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t host_buffer_size) {
    //////printme("In compress \n");
    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;

    std::chrono::duration<double, std::nano> lz77_kernel_time_ns_1(0);
    std::chrono::duration<double, std::nano> treegen_kernel_time_ns_1(0);
    std::chrono::duration<double, std::nano> huffman_kernel_time_ns_1(0);

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
            m_q[queue_idx + cu]->enqueueMigrateMemObjects({*(buffer_input[cu][flag]), *(buffer_inblk_size[cu][flag])},
                                                          0 /* 0 means from host*/);
            m_q[queue_idx + cu]->finish();

            auto lz77_kernel_start = std::chrono::high_resolution_clock::now();
            // kernel write events update
            // LZ77 Compress Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*compress_kernel[cu]);
            m_q[queue_idx + cu]->finish();

            auto lz77_kernel_end = std::chrono::high_resolution_clock::now();
            auto lz77_duration = std::chrono::duration<double, std::nano>(lz77_kernel_end - lz77_kernel_start);
            lz77_kernel_time_ns_1 += lz77_duration;

            auto treegen_kernel_start = std::chrono::high_resolution_clock::now();

            // TreeGen Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*treegen_kernel[cu]);
            m_q[queue_idx + cu]->finish();
            auto treegen_kernel_end = std::chrono::high_resolution_clock::now();
            auto treegen_duration = std::chrono::duration<double, std::nano>(treegen_kernel_end - treegen_kernel_start);

            auto huffman_kernel_start = std::chrono::high_resolution_clock::now();
            // Huffman Fire Kernel invocation
            m_q[queue_idx + cu]->enqueueTask(*huffman_kernel[cu]);
            m_q[queue_idx + cu]->finish();

            auto huffman_kernel_end = std::chrono::high_resolution_clock::now();
            auto huffman_duration = std::chrono::duration<double, std::nano>(huffman_kernel_end - huffman_kernel_start);

            m_q[queue_idx + cu]->enqueueMigrateMemObjects(
                {*(buffer_gzip_output[cu][flag]), *(buffer_compress_size[cu][flag])}, CL_MIGRATE_MEM_OBJECT_HOST);
            m_q[queue_idx + cu]->finish();
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
                std::memcpy(&out[outIdx], &h_buf_gzipout[cu][flag].data()[bIdx * block_size_in_bytes], compressed_size);
                outIdx += compressed_size;
            }
        }
    }

    std::chrono::duration<double, std::nano> temp_kernel_time_ns_1(0);
    if (lz77_kernel_time_ns_1.count() > huffman_kernel_time_ns_1.count())
        temp_kernel_time_ns_1 = lz77_kernel_time_ns_1;
    else
        temp_kernel_time_ns_1 = huffman_kernel_time_ns_1;

    float throughput_in_mbps_1 = (float)input_size * 1000 / temp_kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    // gzip special block based on Z_SYNC_FLUSH
    int xarg = 0;
    out[outIdx + xarg++] = 0x01;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0x00;
    out[outIdx + xarg++] = 0xff;
    out[outIdx + xarg++] = 0xff;
    outIdx += xarg;
    return outIdx;
} // Overlap end
