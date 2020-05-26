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

using namespace xf::compression;

uint64_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void zip(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
    ////printme("In ZLIB flow");
    outFile.put(120);
    outFile.put(1);
    outFile.write((char*)zip_out, enbytes);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

void gzip_headers(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
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

void zlib_headers(std::string& inFile_name, std::ofstream& outFile, uint8_t* zip_out, uint32_t enbytes) {
    outFile.put(120);
    outFile.put(1);
    outFile.write((char*)zip_out, enbytes);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
    outFile.put(0);
}

uint64_t xfZlib::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    std::chrono::duration<double, std::nano> compress_API_time_ns_1(0);
    std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > zlib_in(input_size);
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > zlib_out(input_size * 2);

    MEM_ALLOC_CHECK(zlib_in.resize(input_size), input_size, "Input Buffer");
    MEM_ALLOC_CHECK(zlib_out.resize(input_size * 2), input_size * 2, "Output Buffer");

    inFile.read((char*)zlib_in.data(), input_size);

    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    auto compress_API_start = std::chrono::high_resolution_clock::now();
    uint64_t enbytes = 0;

    // zlib Compress
    enbytes = compress(zlib_in.data(), zlib_out.data(), input_size, host_buffer_size);

    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(compress_API_end - compress_API_start);
    compress_API_time_ns_1 += duration;

    float throughput_in_mbps_1 = (float)input_size * 1000 / compress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(3) << throughput_in_mbps_1;

    if (enbytes > 0) {
#ifdef GZIP_MODE
        // Pack gzip encoded stream .gz file
        gzip_headers(inFile_name, outFile, zlib_out.data(), enbytes);
#else
        // Pack zlib encoded stream .zlib file
        zlib_headers(inFile_name, outFile, zlib_out.data(), enbytes);
#endif
    } else {
        exit(EXIT_FAILURE);
    }

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

// OpenCL setup initialization
int xfZlib::init(const std::string& binaryFileName, uint8_t kidx) {
    // Error handling in OpenCL is performed using the cl_int specifier. OpenCL
    // functions either return or accept pointers to cl_int types to indicate if
    // an error occurred.
    cl_int err;

    // Look for platform
    std::vector<cl::Platform> platforms;
    OCL_CHECK(err, err = cl::Platform::get(&platforms));
    auto num_platforms = platforms.size();
    if (num_platforms == 0) {
        std::cerr << "No Platforms were found this could be cased because of the OpenCL \
                      ICD not installed at /etc/OpenCL/vendors directory"
                  << std::endl;
        m_err_code = -32;
        return -32;
    }

    std::string platformName;
    cl::Platform platform;
    bool foundFlag = false;
    for (size_t i = 0; i < platforms.size(); i++) {
        platform = platforms[i];
        OCL_CHECK(err, platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
        if (platformName == "Xilinx") {
            foundFlag = true;
#ifdef VERBOSE
            std::cout << "Found Platform" << std::endl;
            std::cout << "Platform Name: " << platformName.c_str() << std::endl;
#endif
            break;
        }
    }
    if (foundFlag == false) {
        std::cerr << "Error: Failed to find Xilinx platform" << std::endl;
        m_err_code = 1;
        return 1;
    }
    // Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
    m_device = devices[m_deviceid];

    // OpenCL Setup Start
    // Creating Context and Command Queue for selected Device
    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
    OCL_CHECK(err, m_context = new cl::Context(m_device, props, NULL, NULL, &err));
    if (err) {
        std::cerr << "Context creation Failed " << std::endl;
        m_err_code = 1;
        return 1;
    }
// Import_binary() command will find the OpenCL binary file created using the
// v++ compiler load into OpenCL Binary and return as Binaries
// OpenCL and it can contain many functions which can be executed on the
// device.
// auto fileBuf = xcl::read_binary_file(binaryFileName);
#ifdef VERBOSE
    std::cout << "INFO: Reading " << binaryFileName << std::endl;
#endif
    if (access(binaryFileName.c_str(), R_OK) != 0) {
        std::cerr << "ERROR: " << binaryFileName.c_str() << " xclbin not available please build " << std::endl;
        m_err_code = 1;
        return 1;
    }

#ifdef VERBOSE
    // Loading XCL Bin into char buffer
    std::cout << "Loading: '" << binaryFileName.c_str() << "'\n";
#endif
    std::ifstream bin_file(binaryFileName.c_str(), std::ifstream::binary);
    if (bin_file.fail()) {
        std::cerr << "Unable to open binary file" << std::endl;
        m_err_code = 1;
        return 1;
    }

    bin_file.seekg(0, bin_file.end);
    auto nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);

    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > buf;
    MEM_ALLOC_CHECK(buf.resize(nb), nb, "XCLBIN Buffer");
    bin_file.read(reinterpret_cast<char*>(buf.data()), nb);

    cl::Program::Binaries bins{{buf.data(), buf.size()}};
    ZOCL_CHECK_2(err, m_program = new cl::Program(*m_context, {m_device}, bins, NULL, &err), m_err_code, c_clinvalidbin,
                 c_clinvalidvalue);
    if (error_code()) {
        std::cerr << "Failed to program the device " << std::endl;
        return m_err_code;
    }

    // Create Command Queue
    // Compress Command Queue & Kernel Setup
    if ((m_cdflow == BOTH) || (m_cdflow == COMP_ONLY)) {
        for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
            OCL_CHECK(err, m_q[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
        }

        std::string cu_id;
        std::string comp_krnl_name = compress_kernel_names[0].c_str();
        std::string huffman_krnl_name = huffman_kernel_names[0].c_str();

        // Create Compress Kernels
        for (uint32_t i = 0; i < C_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string krnl_name_full = comp_krnl_name + ":{" + comp_krnl_name + "_" + cu_id + "}";
            OCL_CHECK(err, compress_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str(), &err));
        }

        // Create Huffman Kernel
        for (uint32_t i = 0; i < H_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string krnl_name_full = huffman_krnl_name + ":{" + huffman_krnl_name + "_" + cu_id + "}";
            OCL_CHECK(err, huffman_kernel[i] = new cl::Kernel(*m_program, krnl_name_full.c_str(), &err));
        }
    }

    // DeCompress Command Queue & Kernel Setup
    if ((m_cdflow == BOTH) || (m_cdflow == DECOMP_ONLY)) {
        for (uint8_t i = 0; i < D_COMPUTE_UNIT; i++) {
            OCL_CHECK(err, m_q_dec[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
        }

        for (uint8_t i = 0; i < D_COMPUTE_UNIT; i++) {
            OCL_CHECK(err, m_q_rd[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
            OCL_CHECK(err, m_q_rdd[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
            OCL_CHECK(err, m_q_wr[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
            OCL_CHECK(err, m_q_wrd[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
        }

        std::string cu_id;
        // Create Decompress Kernel
        for (uint32_t i = 0; i < D_COMPUTE_UNIT; i++) {
            cu_id = std::to_string(i + 1);
            std::string data_writer_kname =
                data_writer_kernel_name + ":{" + data_writer_kernel_name + "_" + cu_id + "}";
            std::string data_reader_kname =
                data_reader_kernel_name + ":{" + data_reader_kernel_name + "_" + cu_id + "}";
            std::string decompress_kname =
                stream_decompress_kernel_name[kidx] + ":{" + stream_decompress_kernel_name[kidx] + "_" + cu_id + "}";

            OCL_CHECK(err, data_writer_kernel[i] = new cl::Kernel(*m_program, data_writer_kname.c_str(), &err));
            OCL_CHECK(err, data_reader_kernel[i] = new cl::Kernel(*m_program, data_reader_kname.c_str(), &err));
            OCL_CHECK(err, decompress_kernel[i] = new cl::Kernel(*m_program, decompress_kname.c_str(), &err));
        }
    }
    // OpenCL Host / Device Buffer Setup Start
    return 0;
}

// Constructor
xfZlib::xfZlib(const std::string& binaryFileName,
               uint8_t max_cr,
               uint8_t cd_flow,
               uint8_t device_id,
               uint8_t profile,
               uint8_t d_type) {
    for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
        for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
            buffer_input[i][j] = nullptr;
            buffer_lz77_output[i][j] = nullptr;
            buffer_zlib_output[i][j] = nullptr;
            buffer_compress_size[i][j] = nullptr;
            buffer_inblk_size[i][j] = nullptr;
            buffer_dyn_ltree_freq[i][j] = nullptr;
            buffer_dyn_dtree_freq[i][j] = nullptr;
        }
    }
    // Zlib Compression Binary Name
    m_cdflow = cd_flow;
    m_isProfile = profile;
    m_deviceid = device_id;
    m_max_cr = max_cr;

#ifdef VERBOSE
    std::chrono::duration<double, std::milli> device_API_time_ns_1(0);
    auto device_API_start = std::chrono::high_resolution_clock::now();
#endif
    uint8_t kidx = d_type;

    // OpenCL setup
    int err = init(binaryFileName, kidx);
    if (err) {
        std::cerr << "\nOpenCL Setup Failed" << std::endl;
        release();
        return;
    }

#ifdef VERBOSE
    auto device_API_end = std::chrono::high_resolution_clock::now();
    auto duration_devscan = std::chrono::duration<double, std::milli>(device_API_end - device_API_start);
    device_API_time_ns_1 = duration_devscan;
    float device_scan = device_API_time_ns_1.count();
    std::cout << "OpenCL Setup = " << std::fixed << std::setprecision(2) << device_scan << std::endl;
#endif

    uint32_t block_size_in_kb = BLOCK_SIZE_IN_KB;
    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;
    uint32_t temp_nblocks = (host_buffer_size - 1) / block_size_in_bytes + 1;
    host_buffer_size = ((host_buffer_size - 1) / block_size_in_kb + 1) * block_size_in_kb;

    const uint16_t c_ltree_size = 1024;
    const uint16_t c_dtree_size = 64;
    const uint16_t c_bltree_size = 64;
    const uint16_t c_maxcode_size = 16;

    if ((cd_flow == BOTH) || (cd_flow == COMP_ONLY)) {
#ifdef VERBOSE
        std::chrono::duration<double, std::milli> cons_API_time_ns_1(0);
        auto cons_API_start = std::chrono::high_resolution_clock::now();
#endif
        for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
            for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
                // Host Buffer Allocation
                MEM_ALLOC_CHECK(h_buf_in[i][j].resize(HOST_BUFFER_SIZE), HOST_BUFFER_SIZE, "Input Host Buffer");
                MEM_ALLOC_CHECK(h_buf_zlibout[i][j].resize(HOST_BUFFER_SIZE * 2), HOST_BUFFER_SIZE * 2,
                                "Output Host Buffer");
                MEM_ALLOC_CHECK(h_blksize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS, "BlockSize Host Buffer");
                MEM_ALLOC_CHECK(h_compressSize[i][j].resize(MAX_NUMBER_BLOCKS), MAX_NUMBER_BLOCKS,
                                "CompressSize Host Buffer");
            }
        }

#ifdef VERBOSE
        auto cons_API_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(cons_API_end - cons_API_start);
        cons_API_time_ns_1 = duration;
        float cons_time = cons_API_time_ns_1.count();
        std::cout << "Compress Host Buffer Allocation Time = " << std::fixed << std::setprecision(2) << cons_time
                  << std::endl;
#endif
        cl_int err;
        for (int i = 0; i < MAX_CCOMP_UNITS; i++) {
            for (int j = 0; j < OVERLAP_BUF_COUNT; j++) {
                // Device Buffer Allocation
                OCL_CHECK(err, buffer_input[i][j] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                                   host_buffer_size, h_buf_in[i][j].data(), &err));

                OCL_CHECK(err,
                          buffer_lz77_output[i][j] = new cl::Buffer(
                              *m_context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, host_buffer_size * 4, NULL, &err));

                OCL_CHECK(err, buffer_compress_size[i][j] =
                                   new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                  temp_nblocks * sizeof(uint32_t), h_compressSize[i][j].data(), &err));

                OCL_CHECK(err, buffer_zlib_output[i][j] =
                                   new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                  host_buffer_size * 2, h_buf_zlibout[i][j].data(), &err));

                OCL_CHECK(err, buffer_inblk_size[i][j] =
                                   new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                  temp_nblocks * sizeof(uint32_t), h_blksize[i][j].data(), &err));

                OCL_CHECK(err, buffer_dyn_ltree_freq[i][j] =
                                   new cl::Buffer(*m_context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
                                                  PARALLEL_ENGINES * sizeof(uint32_t) * c_ltree_size, NULL, &err));

                OCL_CHECK(err, buffer_dyn_dtree_freq[i][j] =
                                   new cl::Buffer(*m_context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
                                                  PARALLEL_ENGINES * sizeof(uint32_t) * c_dtree_size, NULL, &err));
            }
        }
    }

    if ((cd_flow == BOTH) || (cd_flow == DECOMP_ONLY)) {
#ifdef VERBOSE
        std::chrono::duration<double, std::milli> cons_API_time_ns_1(0);
        auto cons_API_start = std::chrono::high_resolution_clock::now();
#endif
        // Decompression host buffer allocation
        for (int j = 0; j < DIN_BUFFERCOUNT; ++j)
            MEM_ALLOC_CHECK(h_dbufstream_in[j].resize(INPUT_BUFFER_SIZE), INPUT_BUFFER_SIZE, "Input Buffer");

        for (int j = 0; j < DOUT_BUFFERCOUNT; ++j) {
            MEM_ALLOC_CHECK(h_dbufstream_zlibout[j].resize(OUTPUT_BUFFER_SIZE), OUTPUT_BUFFER_SIZE,
                            "Output Host Buffer");
            MEM_ALLOC_CHECK(h_dcompressSize_stream[j].resize(sizeof(uint32_t)), sizeof(uint32_t),
                            "DecompressSize Host Buffer");
        }
        MEM_ALLOC_CHECK(h_dcompressStatus.resize(sizeof(uint32_t)), sizeof(uint32_t), "DecompressStatus Host Buffer");

        uint32_t inBufferSize = INPUT_BUFFER_SIZE;
        uint32_t outBufferSize = OUTPUT_BUFFER_SIZE;
        // Input Device Buffer allocation (__enqueue_writes)
        for (int i = 0; i < DIN_BUFFERCOUNT; i++) {
            OCL_CHECK(err, buffer_dec_input[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                                inBufferSize, h_dbufstream_in[i].data(), &err));
        }

        // Output Device Buffer allocation (__enqueue_reads)
        for (int i = 0; i < DOUT_BUFFERCOUNT; i++) {
            OCL_CHECK(err,
                      buffer_dec_zlib_output[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                                 outBufferSize, h_dbufstream_zlibout[i].data(), &err));
        }

#ifdef VERBOSE
        auto cons_API_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(cons_API_end - cons_API_start);
        cons_API_time_ns_1 = duration;
        float cons_time = cons_API_time_ns_1.count();
        std::cout << "DeCompress Host Buffer Allocation Time = " << std::fixed << std::setprecision(2) << cons_time
                  << std::endl;
#endif
    }
    // OpenCL Host / Device Buffer Setup End
}

int xfZlib::error_code(void) {
    return m_err_code;
}

void xfZlib::release() {
    DELETE_OBJ(m_program);
    DELETE_OBJ(m_context);

    if ((m_cdflow == BOTH) || (m_cdflow == COMP_ONLY)) {
        for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
            DELETE_OBJ(m_q[i]);
        }

        for (int i = 0; i < C_COMPUTE_UNIT; i++) {
            DELETE_OBJ(compress_kernel[i]);
        }

        for (int i = 0; i < H_COMPUTE_UNIT; i++) {
            DELETE_OBJ(huffman_kernel[i]);
        }

        uint32_t overlap_buf_count = OVERLAP_BUF_COUNT;
        for (uint32_t cu = 0; cu < C_COMPUTE_UNIT; cu++) {
            for (uint32_t flag = 0; flag < overlap_buf_count; flag++) {
                DELETE_OBJ(buffer_input[cu][flag]);
                DELETE_OBJ(buffer_lz77_output[cu][flag]);
                DELETE_OBJ(buffer_zlib_output[cu][flag]);
                DELETE_OBJ(buffer_compress_size[cu][flag]);
                DELETE_OBJ(buffer_inblk_size[cu][flag]);
                DELETE_OBJ(buffer_dyn_ltree_freq[cu][flag]);
                DELETE_OBJ(buffer_dyn_dtree_freq[cu][flag]);
            }
        }
    }

    // Decompress release
    if ((m_cdflow == BOTH) || (m_cdflow == DECOMP_ONLY)) {
        for (uint8_t i = 0; i < D_COMPUTE_UNIT; i++) {
            // Destroy command queues
            DELETE_OBJ(m_q_dec[i]);
            DELETE_OBJ(m_q_rd[i]);
            DELETE_OBJ(m_q_rdd[i]);
            DELETE_OBJ(m_q_wr[i]);
            DELETE_OBJ(m_q_wrd[i]);
            // Destroy kernels
            DELETE_OBJ(decompress_kernel[i]);
            DELETE_OBJ(data_writer_kernel[i]);
            DELETE_OBJ(data_reader_kernel[i]);
        }
        // delete the allocated buffers
        for (int i = 0; i < DIN_BUFFERCOUNT; i++) {
            DELETE_OBJ(buffer_dec_input[i]);
        }

        for (int i = 0; i < DOUT_BUFFERCOUNT; i++) {
            DELETE_OBJ(buffer_dec_zlib_output[i]);
        }
    }
}

// Destructor
xfZlib::~xfZlib() {
    release();
}

int xfZlib::decompress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size, uint8_t dcu_id = 0) {
    // Zlib deCompress
    uint32_t debytes = 0;

    std::random_device rand_hw;
    std::uniform_int_distribution<> range(0, (D_COMPUTE_UNIT - 1));
    uint8_t cu_id = range(rand_hw);

    if (dcu_id != 0)
        debytes = decompress(in, out, input_size, dcu_id);
    else
        debytes = decompress(in, out, input_size, cu_id);

    return debytes;
}

uint64_t xfZlib::compress_buffer(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint32_t host_buffer_size = HOST_BUFFER_SIZE;

    out[0] = 120;
    out[1] = 1;

    // Call to compress
    // Zlib Compress
    uint64_t enbytes = compress(in, out + 2, input_size, host_buffer_size);
    if (enbytes != 0) {
        out[enbytes + 1] = 0;
        out[enbytes + 2] = 0;
        out[enbytes + 3] = 0;
        out[enbytes + 4] = 0;
        out[enbytes + 5] = 0;

        enbytes += 5;
    }
    return enbytes;
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

    uint32_t output_buf_size = input_size * m_max_cr;

    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > in;
    // Allocat output size
    // 8 - Max CR per file expected, if this size is big
    // Decompression crashes
    std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > out(output_buf_size);

    MEM_ALLOC_CHECK(in.resize(input_size), input_size, "Input Buffer");
    MEM_ALLOC_CHECK(in.resize(output_buf_size), output_buf_size, "Output Buffer");

    uint32_t debytes = 0;
    inFile.read((char*)in.data(), input_size);

    // printme("Call to zlib_decompress \n");
    // Call decompress
    auto decompress_API_start = std::chrono::high_resolution_clock::now();
    debytes = decompress(in.data(), out.data(), input_size, cu);
    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    decompress_API_time_ns_1 = duration;

    if (debytes == 0) {
        std::cerr << "Decompression Failed" << std::endl;
        return 0;
    }

    float throughput_in_mbps_1 = (float)debytes * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(3) << throughput_in_mbps_1;

    outFile.write((char*)out.data(), debytes);

    // Close file
    inFile.close();
    outFile.close();

    return debytes;
}

// method to enqueue reads in parallel with writes to decompression kernel
void xfZlib::_enqueue_reads(uint32_t bufSize, uint8_t* out, uint32_t* decompSize, int cu, uint32_t max_outbuf_size) {
    const int BUFCNT = DOUT_BUFFERCOUNT;
    cl::Event hostReadEvent[BUFCNT];
    cl::Event kernelReadEvent[BUFCNT];

    std::vector<cl::Event> kernelReadWait[BUFCNT];

    uint8_t* outP = nullptr;
    uint32_t* outSize = nullptr;
    uint32_t dcmpSize = 0;
    cl::Buffer* buffer_size[BUFCNT];
    cl::Buffer* buffer_status; // single common buffer to capture the decompression status by kernel
    cl_int err;
    for (int i = 0; i < BUFCNT; i++) {
        OCL_CHECK(err, buffer_size[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                       2 * sizeof(uint32_t), h_dcompressSize_stream[i].data(), &err));
    }

    OCL_CHECK(err, buffer_status = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(uint32_t),
                                                  h_dcompressStatus.data(), &err));

    // set consistent buffer size to be read
    OCL_CHECK(err, err = data_reader_kernel[cu]->setArg(2, *buffer_status));
    OCL_CHECK(err, err = data_reader_kernel[cu]->setArg(3, bufSize));

    // enqueue first set of buffers
    uint8_t cbf_idx = 0;
    uint32_t keq_idx = 0;
    uint32_t raw_size = 0;
    uint32_t cpy_cnt = 0;
    bool done = false;

    do {
        cbf_idx = keq_idx % BUFCNT;
        if (((kernelReadWait[cbf_idx]).size() > 0) || (done)) {
            (hostReadEvent[cbf_idx]).wait(); // wait for previous data migration to complete
            if (!done) {
                outSize = h_dcompressSize_stream[cbf_idx].data();
                raw_size = *outSize;
                outP = h_dbufstream_zlibout[cbf_idx].data();
                // if output data size is multiple of buffer size, then (buffer_size + 1) is sent by reader kernel
                if (raw_size > bufSize) {
                    --raw_size;
                }
                if (raw_size != 0) {
                    std::memcpy(out + dcmpSize, outP, raw_size);
                    dcmpSize += raw_size;
                }
                if (raw_size != bufSize) done = true;

                if ((dcmpSize > max_outbuf_size) && (max_outbuf_size != 0)) {
                    std::cout << "\n" << std::endl;
                    std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during decompression \033[0m \n"
                              << std::endl;
                    std::cout
                        << "\x1B[35mUse -mcr option to increase the maximum compression ratio (Default: 10) \033[0m \n"
                        << std::endl;
                    std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
                    exit(1);
                }
                if ((kernelReadWait[cbf_idx]).size() > 0)
                    kernelReadWait[cbf_idx].pop_back(); // must always have single element
            }
            ++cpy_cnt;
        }
        // need to avoid full buffer copies for 0 bytes size data
        if (!done) {
            // set reader kernel arguments
            OCL_CHECK(err, err = data_reader_kernel[cu]->setArg(0, *(buffer_dec_zlib_output[cbf_idx])));
            OCL_CHECK(err, err = data_reader_kernel[cu]->setArg(1, *(buffer_size[cbf_idx])));

            // enqueue reader kernel
            OCL_CHECK(err, err = m_q_rd[cu]->enqueueTask(*data_reader_kernel[cu], NULL, &(kernelReadEvent[cbf_idx])));
            kernelReadWait[cbf_idx].push_back(kernelReadEvent[cbf_idx]); // event to wait for

            OCL_CHECK(err, err = m_q_rdd[cu]->enqueueMigrateMemObjects(
                               {*(buffer_size[cbf_idx]), *(buffer_dec_zlib_output[cbf_idx])},
                               CL_MIGRATE_MEM_OBJECT_HOST, &(kernelReadWait[cbf_idx]), &(hostReadEvent[cbf_idx])));
            ++keq_idx;
        }
    } while (!(done && keq_idx == cpy_cnt));
    // wait for data transfer queue to finish
    OCL_CHECK(err, err = m_q_rdd[cu]->finish());
    *decompSize = dcmpSize;

    // free the buffers
    for (int i = 0; i < BUFCNT; i++) delete (buffer_size[i]);
}

// method to enqueue writes in parallel with reads from decompression kernel
void xfZlib::_enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize, int cu) {
    const int BUFCNT = DIN_BUFFERCOUNT;
    cl_int err;
    uint32_t bufferCount = 1 + (inputSize - 1) / bufSize;

    uint8_t* inP = nullptr;
    cl::Event hostWriteEvent[BUFCNT];
    cl::Event kernelWriteEvent[BUFCNT];
    std::vector<cl::Event> hostWriteWait[BUFCNT];

    uint32_t cBufSize = bufSize;
    uint8_t cbf_idx = 0;  // index indicating current buffer(0-4) being used in loop
    uint32_t keq_idx = 0; // index indicating the number of writer kernel enqueues

    for (keq_idx = 0; keq_idx < bufferCount; ++keq_idx) {
        cbf_idx = keq_idx % BUFCNT;

        if (keq_idx > BUFCNT - 1) {
            // wait for (current - BUFCNT) kernel to finish
            (kernelWriteEvent[cbf_idx]).wait();
        }
        inP = h_dbufstream_in[cbf_idx].data();
        // set for last and other buffers
        if (keq_idx == bufferCount - 1) {
            if (bufferCount > 1) {
                cBufSize = inputSize - (bufSize * keq_idx);
            }
        }

        // copy the data
        std::memcpy(inP, in + (keq_idx * bufSize), cBufSize);

        // set kernel arguments
        OCL_CHECK(err, err = data_writer_kernel[cu]->setArg(0, *(buffer_dec_input[cbf_idx])));
        OCL_CHECK(err, err = data_writer_kernel[cu]->setArg(1, cBufSize));

        // enqueue data migration to kernel
        OCL_CHECK(err, err = m_q_wr[cu]->enqueueMigrateMemObjects({*(buffer_dec_input[cbf_idx])}, 0, NULL, NULL));
        // enqueue the writer kernel dependent on corresponding bufffer migration
        OCL_CHECK(err, err = m_q_wr[cu]->enqueueTask(*data_writer_kernel[cu], NULL, &(kernelWriteEvent[cbf_idx])));
    }
    // wait for enqueued writer kernels to finish
    OCL_CHECK(err, err = m_q_wr[cu]->finish());
}

uint32_t xfZlib::decompress(uint8_t* in, uint8_t* out, uint32_t input_size, int cu) {
    cl_int err;
#ifdef GZIP_MODE

    uint8_t hidx = 0;

    // Check for magic header
    if (in[hidx++] != 0x1F || in[hidx++] != 0x8B) {
        std::cerr << "\n";
        std::cerr << "Magic Header Fails" << std::endl;
        // We must set error_flag true and return it helps
        // for CISCO use case
        return 0;
    }

    // Check if method is deflate or not
    if (in[hidx++] != 0x08) {
        std::cerr << "\n";
        std::cerr << "Deflate Header Check Fails" << std::endl;
        // We must set error_flag true and return it helps
        // for CISCO use case
        return 0;
    }

    // Check if the FLAG has correct value
    // Supported file name or no file name
    // 0x00: No File Name
    // 0x08: File Name
    if (in[hidx] != 0 && in[hidx] != 0x08) {
        std::cerr << "\n";
        std::cerr << "Deflate -n option check failed" << std::endl;
        // We must set error_flag true and return it helps
        // for CISCO use case
        return 0;
    }

    hidx++;

    // Skip time stamp bytes
    // time stamp contains 4 bytes
    hidx += 4;

    // One extra 0  ending byte
    hidx += 1;

    // Check the operating system code
    // for Unix its 3
    uint8_t oscode_in = in[hidx];
    std::vector<uint8_t> oscodes{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    bool ochck = std::find(oscodes.cbegin(), oscodes.cend(), oscode_in) == oscodes.cend();
    if (ochck) {
        std::cerr << "\n";
        std::cerr << "GZip header mismatch: OS code is unknown" << std::endl;
        return 0;
    }

#else

    uint8_t hidx = 0;
    // ZLIB Header Checks
    // CMF
    // FLG
    uint8_t cmf = 0x78;
    // 0x01: Fast Mode
    // 0x5E: 1 to 5 levels
    // 0x9C: Default compression: level 6
    // 0xDA: High compression
    std::vector<uint8_t> zlib_flags{0x01, 0x5E, 0x9C, 0xDA};
    if (in[hidx++] == cmf) {
        uint8_t flg = in[hidx];
        bool hchck = std::find(zlib_flags.cbegin(), zlib_flags.cend(), flg) == zlib_flags.cend();
        if (hchck) {
            std::cerr << "\n";
            std::cerr << "Header check fails" << std::endl;
            // Set error_flag to true here for cisco usecase
            return 0;
        }
    } else {
        // Set the error flag to true for Cisco Usecase
        // and return
        std::cerr << "\n";
        std::cerr << "Zlib Header mismatch" << std::endl;
        return 0;
    }

#endif
    // Streaming based solution
    uint32_t inBufferSize = INPUT_BUFFER_SIZE;
    uint32_t outBufferSize = OUTPUT_BUFFER_SIZE;
    uint32_t max_outbuf_size = input_size * m_max_cr;
    const int c_bufcnt = DIN_BUFFERCOUNT;
    const int c_outBufCnt = DOUT_BUFFERCOUNT;

    // Set kernel args for input buffer
    for (int i = 0; i < c_bufcnt; i++) {
        ZOCL_CHECK(err, err = data_writer_kernel[cu]->setArg(0, *(buffer_dec_input[i])), m_err_code, c_clOutOfResource);
        if (error_code()) return 0;
    }

    // Set kernel args for output buffer
    for (int i = 0; i < c_outBufCnt; i++) {
        ZOCL_CHECK(err, err = data_reader_kernel[cu]->setArg(0, *(buffer_dec_zlib_output[i])), m_err_code,
                   c_clOutOfResource);
        if (error_code()) return 0;
    }

    // if input_size if greater than 2 MB, then buffer size must be 2MB
    if (input_size < inBufferSize) inBufferSize = input_size;
    if ((max_outbuf_size < outBufferSize) && (max_outbuf_size != 0)) outBufferSize = max_outbuf_size;

    // Set Kernel Args
    OCL_CHECK(err, err = decompress_kernel[cu]->setArg(0, input_size));

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;
    std::thread decompWriter(&xfZlib::_enqueue_writes, this, inBufferSize, in, input_size, cu);
    std::thread decompReader(&xfZlib::_enqueue_reads, this, outBufferSize, out, &decmpSizeIdx, cu, max_outbuf_size);

    OCL_CHECK(err, err = m_q_dec[cu]->enqueueTask(*decompress_kernel[cu]));
    OCL_CHECK(err, err = m_q_dec[cu]->finish());

    decompReader.join();
    decompWriter.join();
    return decmpSizeIdx;
}
// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
uint64_t xfZlib::compress(uint8_t* in, uint8_t* out, uint64_t input_size, uint32_t host_buffer_size) {
    cl_int err;
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

    for (uint64_t i = 0; i < input_size; i += host_buffer_size, idx++) {
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
    std::chrono::duration<double, std::milli> compress_API_time_ms_1(0);
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
                OCL_CHECK(err, err = m_q[queue_idx + cu]->finish());

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
                    OCL_CHECK(err, err = m_q[queue_idx + cu]->enqueueReadBuffer(
                                       *(buffer_zlib_output[cu][flag]), CL_TRUE, index,
                                       compressed_size * sizeof(uint8_t), &out[outIdx]));
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

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_input[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_lz77_output[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_compress_size[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_inblk_size[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            OCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, block_size_in_kb));

            OCL_CHECK(err, err = compress_kernel[cu]->setArg(narg++, sizeOfChunk[brick + cu]));

            narg = 0;
            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_lz77_output[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_zlib_output[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_compress_size[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            ZOCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, *(buffer_inblk_size[cu][flag])), m_err_code,
                       c_clOutOfResource);
            if (error_code()) return 0;

            OCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, block_size_in_kb));
            OCL_CHECK(err, err = huffman_kernel[cu]->setArg(narg++, sizeOfChunk[brick + cu]));

            // Migrate memory - Map host to device buffers
            OCL_CHECK(err, err = m_q[queue_idx + cu]->enqueueMigrateMemObjects(
                               {*(buffer_input[cu][flag]), *(buffer_inblk_size[cu][flag])}, 0 /* 0 means from host*/));

            // kernel write events update
            // LZ77 Compress Fire Kernel invocation
            OCL_CHECK(err, err = m_q[queue_idx + cu]->enqueueTask(*compress_kernel[cu]));

            // Huffman Fire Kernel invocation
            OCL_CHECK(err, err = m_q[queue_idx + cu]->enqueueTask(*huffman_kernel[cu]));

            OCL_CHECK(err, err = m_q[queue_idx + cu]->enqueueMigrateMemObjects({*(buffer_compress_size[cu][flag])},
                                                                               CL_MIGRATE_MEM_OBJECT_HOST));
        } // Internal loop runs on compute units

        if (total_chunks > 2)
            brick += C_COMPUTE_UNIT;
        else
            brick++;

    } // Main overlap loop

    for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
        OCL_CHECK(err, err = m_q[i]->flush());
        OCL_CHECK(err, err = m_q[i]->finish());
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
                m_q[queue_idx + cu]->enqueueReadBuffer(*(buffer_zlib_output[cu][flag]), CL_TRUE, index,
                                                       compressed_size * sizeof(uint8_t), &out[outIdx]);
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
