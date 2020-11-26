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
#include "zlib_old.hpp"
#include "zlib.h"
using namespace xf::compression;
extern unsigned long crc32(unsigned long crc, const unsigned char* buf, uint32_t len);
extern unsigned long adler32(unsigned long crc, const unsigned char* buf, uint32_t len);

using namespace xf::compression;

void xf::compression::event_cb(cl_event event, cl_int cmd_status, void* data) {
    assert(data != NULL);
    ((xf::compression::buffers*)(data))->finish = true;
}

uint64_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    if (file_size == 0) {
        std::cout << "File is empty!" << std::endl;
        exit(0);
    }
    file.seekg(0, file.beg);
    return file_size;
}

void xfZlib::set_checksum(uint32_t cval) {
    m_checksum = cval;
}

uint32_t xfZlib::calculate_crc32(uint8_t* in, size_t input_size) {
    m_checksum = crc32(m_checksum, in, input_size);
    return m_checksum;
}

uint32_t xfZlib::calculate_adler32(uint8_t* in, size_t input_size) {
    m_checksum = adler32(m_checksum, in, input_size);
    return m_checksum;
}

void xfZlib::generate_checksum(uint8_t* in, size_t input_size) {
    if (m_zlibFlow) {
        m_future_checksum = std::async(std::launch::async, &xfZlib::calculate_adler32, this, in, input_size);
    } else {
        m_future_checksum = std::async(std::launch::async, &xfZlib::calculate_crc32, this, in, input_size);
    }
}

uint32_t xfZlib::get_checksum(void) {
#ifdef ENABLE_SW_CHECKSUM
    return m_future_checksum.get();
#elif ENABLE_HW_CHECKSUM
    return m_checksum;
#endif
}

uint64_t xfZlib::compress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size, int cu) {
    m_infile = inFile_name;
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

    auto compress_API_start = std::chrono::high_resolution_clock::now();
    uint64_t enbytes = 0;
    // zlib Compress
    enbytes = compress(zlib_in.data(), zlib_out.data(), input_size, cu);

    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(compress_API_end - compress_API_start);
    compress_API_time_ns_1 += duration;

    float throughput_in_mbps_1 = (float)input_size * 1000 / compress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(3) << throughput_in_mbps_1;

    outFile.write((char*)zlib_out.data(), enbytes);

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
#if (VERBOSE_LEVEL >= 2)
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
#if (VERBOSE_LEVEL >= 2)
    std::cout << "INFO: Reading " << binaryFileName << std::endl;
#endif
    if (access(binaryFileName.c_str(), R_OK) != 0) {
        std::cerr << "ERROR: " << binaryFileName.c_str() << " xclbin not available please build " << std::endl;
        m_err_code = 1;
        return 1;
    }

#if (VERBOSE_LEVEL >= 2)
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
    // OCL_CHECK(err, m_def_q = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
    OCL_CHECK(err, m_def_q = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
    if ((m_cdflow == BOTH) || (m_cdflow == COMP_ONLY)) {
#ifdef USE_SINGLE_KERNEL_ZLIBC
        for (int i = 0; i < C_COMPUTE_UNIT; i++) {
            OCL_CHECK(
                err, m_q[i] = new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
        }
#else
        for (int i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
            OCL_CHECK(err, m_q[i] = new cl::CommandQueue(*m_context, m_device, m_isProfile, &err));
        }
#endif

// Use map data structure to track the
// Cu instance and CU number combination for
// xrm generated CU allocation process
#if USE_SINGLE_KERNEL_ZLIBC
        for (int i = 0; i < C_COMPUTE_UNIT; i++) {
            auto cu_id = std::to_string(i + 1);
            std::string compress_stream_kname = compress_kernel_names[1] + "_" + cu_id;
            compressKernelMap.insert(std::pair<std::string, int>(compress_stream_kname, i));
        }
#else
#endif

        std::string cu_id;
#ifdef USE_SINGLE_KERNEL_ZLIBC
        std::string comp_krnl_name = compress_kernel_names[1].c_str();
#else
        std::string comp_krnl_name = compress_kernel_names[0].c_str();
        std::string huffman_krnl_name = huffman_kernel_names[0].c_str();
#endif

#ifdef ENABLE_HW_CHECKSUM
        std::string checksum_krnl_name = checksum_kernel_names[0].c_str();
#endif

#ifdef ENABLE_HW_CHECKSUM
        // Create Checksum Kernels
        OCL_CHECK(err, checksum_kernel = new cl::Kernel(*m_program, checksum_krnl_name.c_str(), &err));
#endif
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
               uint8_t d_type,
               enum design_flow dflow) {
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
    m_zlibFlow = dflow;
    m_kidx = d_type;
    m_pending = 0;
    if (m_zlibFlow) m_checksum = 1;
#if (VERBOSE_LEVEL >= 2)
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

#if (VERBOSE_LEVEL >= 2)
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
    m_memoryManager = new xf::compression::memoryManager(64, m_context);

    if ((cd_flow == BOTH) || (cd_flow == COMP_ONLY)) {
#if (VERBOSE_LEVEL >= 2)
        std::chrono::duration<double, std::milli> cons_API_time_ns_1(0);
        auto cons_API_start = std::chrono::high_resolution_clock::now();
#endif
#ifdef ENABLE_HW_CHECKSUM
        MEM_ALLOC_CHECK(h_buf_checksum_data.resize(1), 1, "Checksum Data");
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

#if (VERBOSE_LEVEL >= 2)
        auto cons_API_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(cons_API_end - cons_API_start);
        cons_API_time_ns_1 = duration;
        float cons_time = cons_API_time_ns_1.count();
        std::cout << "Compress Host Buffer Allocation Time = " << std::fixed << std::setprecision(2) << cons_time
                  << std::endl;
#endif
        cl_int err;
#ifdef ENABLE_HW_CHECKSUM
        OCL_CHECK(err, buffer_checksum_data = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                             sizeof(uint32_t), h_buf_checksum_data.data(), &err));
#endif
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
#if (VERBOSE_LEVEL >= 2)
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

#if (VERBOSE_LEVEL >= 2)
        std::cout << "Found Platform" << std::endl;
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

void xfZlib::release_dec_buffers(void) {
    // delete the allocated buffers
    for (int i = 0; i < DIN_BUFFERCOUNT; i++) {
        DELETE_OBJ(buffer_dec_input[i]);
    }

    for (int i = 0; i < DOUT_BUFFERCOUNT + 1; i++) {
        DELETE_OBJ(buffer_dec_zlib_output[i]);
    }
}

void xfZlib::release() {
    DELETE_OBJ(m_program);
    DELETE_OBJ(m_context);

    if ((m_cdflow == BOTH) || (m_cdflow == COMP_ONLY)) {
#ifdef USE_SINGLE_KERNEL_ZLIBC
        for (uint8_t i = 0; i < C_COMPUTE_UNIT; i++) {
#else
        for (uint8_t i = 0; i < C_COMPUTE_UNIT * OVERLAP_BUF_COUNT; i++) {
#endif
            DELETE_OBJ(m_q[i]);
        }

#ifdef ENABLE_HW_CHECKSUM
        DELETE_OBJ(checksum_kernel);
#endif

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
        }
    }

    if (m_memoryManager) {
        delete m_memoryManager;
    }

    if (m_def_q) {
        delete m_def_q;
    }

    if (compress_stream_kernel) {
        delete compress_stream_kernel;
    }
}

// Destructor
xfZlib::~xfZlib() {
    release();
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
    debytes = decompress(in.data(), out.data(), input_size, output_buf_size, cu);
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
    cl::Buffer* buffer_status; // single common buffer to capture the
                               // decompression status by kernel

    cl_int err;
    std::string cu_id = std::to_string((cu + 1));
    std::string data_reader_kname = data_reader_kernel_name + ":{" + data_reader_kernel_name + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel data_reader_kernel(*m_program, data_reader_kname.c_str(), &err));

    for (int i = 0; i < BUFCNT; i++) {
        OCL_CHECK(err, buffer_size[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                       2 * sizeof(uint32_t), h_dcompressSize_stream[i].data(), &err));
    }

    OCL_CHECK(err, buffer_status = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(uint32_t),
                                                  h_dcompressStatus.data(), &err));

    // set consistent buffer size to be read
    OCL_CHECK(err, err = data_reader_kernel.setArg(2, *buffer_status));
    OCL_CHECK(err, err = data_reader_kernel.setArg(3, bufSize));

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
                // if output data size is multiple of buffer size, then (buffer_size +
                // 1) is sent by reader kernel
                if (raw_size > bufSize) {
                    --raw_size;
                }
                if (raw_size != 0) {
                    if (!(m_derr_code) && (dcmpSize + raw_size < max_outbuf_size)) {
                        std::memcpy(out + dcmpSize, outP, raw_size);
                    }
                    dcmpSize += raw_size;
                }
                if (raw_size != bufSize) done = true;

                if ((dcmpSize > max_outbuf_size) && (max_outbuf_size != 0)) {
#if (VERBOSE_LEVEL >= 1)
                    std::cout << "\n" << std::endl;
                    std::cout << "\x1B[35mZIP BOMB: Exceeded output buffer size during "
                                 "decompression \033[0m \n"
                              << std::endl;
                    std::cout << "\x1B[35mUse -mcr option to increase the maximum "
                                 "compression ratio (Default: 10) \033[0m \n"
                              << std::endl;
                    std::cout << "\x1B[35mAborting .... \033[0m\n" << std::endl;
#endif
                    m_derr_code = true;
                }
                if ((kernelReadWait[cbf_idx]).size() > 0)
                    kernelReadWait[cbf_idx].pop_back(); // must always have single element
            }
            ++cpy_cnt;
        }
        // need to avoid full buffer copies for 0 bytes size data
        if (!done) {
            // set reader kernel arguments
            OCL_CHECK(err, err = data_reader_kernel.setArg(0, *(buffer_dec_zlib_output[cbf_idx])));
            OCL_CHECK(err, err = data_reader_kernel.setArg(1, *(buffer_size[cbf_idx])));

            // enqueue reader kernel
            OCL_CHECK(err, err = m_q_rd[cu]->enqueueTask(data_reader_kernel, NULL, &(kernelReadEvent[cbf_idx])));
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

    delete buffer_status;
}

// method to enqueue writes in parallel with reads from decompression kernel
void xfZlib::_enqueue_writes(uint32_t bufSize, uint8_t* in, uint32_t inputSize, int cu) {
    const int BUFCNT = DIN_BUFFERCOUNT;
    cl_int err;

    std::string cu_id = std::to_string(cu + 1);
    std::string data_writer_kname = data_writer_kernel_name + ":{" + data_writer_kernel_name + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel data_writer_kernel(*m_program, data_writer_kname.c_str(), &err));

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
        OCL_CHECK(err, err = data_writer_kernel.setArg(0, *(buffer_dec_input[cbf_idx])));
        OCL_CHECK(err, err = data_writer_kernel.setArg(1, cBufSize));

        // enqueue data migration to kernel
        OCL_CHECK(err, err = m_q_wr[cu]->enqueueMigrateMemObjects({*(buffer_dec_input[cbf_idx])}, 0, NULL, NULL));
        // enqueue the writer kernel dependent on corresponding bufffer migration
        OCL_CHECK(err, err = m_q_wr[cu]->enqueueTask(data_writer_kernel, NULL, &(kernelWriteEvent[cbf_idx])));
    }
    // wait for enqueued writer kernels to finish
    OCL_CHECK(err, err = m_q_wr[cu]->finish());
}

size_t xfZlib::decompress(uint8_t* in, uint8_t* out, size_t input_size, size_t max_outbuf_size, int cu) {
    cl_int err;
    uint8_t hidx = 0;
    if (in[hidx++] == 0x1F && in[hidx++] == 0x8B) {
        // Check for magic header
        // Check if method is deflate or not
        if (in[hidx++] != 0x08) {
            std::cerr << "\n";
            std::cerr << "Deflate Header Check Fails" << std::endl;
            release();
            m_err_code = c_headermismatch;
            return 0;
        }

        // Check if the FLAG has correct value
        // Supported file name or no file name
        // 0x00: No File Name
        // 0x08: File Name
        if (in[hidx] != 0 && in[hidx] != 0x08) {
            std::cerr << "\n";
            std::cerr << "Deflate -n option check failed" << std::endl;
            release();
            m_err_code = c_headermismatch;
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
    } else {
        hidx = 0;
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
                release();
                m_err_code = c_headermismatch;
                return 0;
            }
        } else {
            std::cerr << "\n";
            std::cerr << "Zlib Header mismatch" << std::endl;
            release();
            m_err_code = c_headermismatch;
            return 0;
        }
    }

#if (VERBOSE_LEVEL >= 1)
    std::cout << "CU" << cu << " ";
#endif
    // Streaming based solution
    uint32_t inBufferSize = INPUT_BUFFER_SIZE;
    uint32_t outBufferSize = OUTPUT_BUFFER_SIZE;

    for (int i = 0; i < DOUT_BUFFERCOUNT; i++) {
        ZOCL_CHECK_2(err,
                     buffer_dec_zlib_output[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                                outBufferSize, h_dbufstream_zlibout[i].data(), &err),
                     m_err_code, c_clOutOfHostMemory, c_clOutOfResource);
        if (error_code()) {
            release_dec_buffers();
            return 0;
        }
    }

    // Streaming based solution
    // Decompress bank index range [0,21]
    // Input Device Buffer allocation (__enqueue_writes)
    for (int i = 0; i < DIN_BUFFERCOUNT; i++) {
        ZOCL_CHECK_2(err, buffer_dec_input[i] = new cl::Buffer(*m_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                               inBufferSize, h_dbufstream_in[i].data(), &err),
                     m_err_code, c_clOutOfHostMemory, c_clOutOfResource);
        if (error_code()) {
            release_dec_buffers();
            return 0;
        }
    }
    // if input_size if greater than 2 MB, then buffer size must be 2MB
    if (input_size < inBufferSize) inBufferSize = input_size;
    if ((max_outbuf_size < outBufferSize) && (max_outbuf_size != 0)) outBufferSize = max_outbuf_size;

    uint8_t kidx = m_kidx;
    std::string cu_id = std::to_string((cu + 1));
    std::string decompress_kname =
        stream_decompress_kernel_name[kidx] + ":{" + stream_decompress_kernel_name[kidx] + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel decompress_kernel(*m_program, decompress_kname.c_str(), &err));

    // Set Kernel Args
    OCL_CHECK(err, err = decompress_kernel.setArg(0, (uint32_t)input_size));

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;
    std::thread decompWriter(&xfZlib::_enqueue_writes, this, inBufferSize, in, input_size, cu);
    std::thread decompReader(&xfZlib::_enqueue_reads, this, outBufferSize, out, &decmpSizeIdx, cu, max_outbuf_size);

    ZOCL_CHECK_2(err, err = m_q_dec[cu]->enqueueTask(decompress_kernel), m_err_code, c_clOutOfHostMemory,
                 c_clOutOfResource);
    if (error_code()) {
        release_dec_buffers();
        return 0;
    }

    OCL_CHECK(err, err = m_q_dec[cu]->finish());

    decompReader.join();
    decompWriter.join();

    release_dec_buffers();

    if (m_derr_code) {
        return 0;
    } else {
        return decmpSizeIdx;
    }
}

size_t xfZlib::deflate_buffer(
    uint8_t* in, uint8_t* out, size_t& input_size, bool& last_data, bool last_buffer, std::string cu_id) {
    cl_int err;
    bool initialize_checksum = false;
    bool actionDone = false;
    if (compress_stream_kernel == NULL) {
        std::string compress_kname = compress_kernel_names[1] + ":{" + cu_id + "}";
        OCL_CHECK(err, compress_stream_kernel = new cl::Kernel(*m_program, compress_kname.c_str(), &err));
        initialize_checksum = true;
    }
    bool checksum_type = false;

#ifdef ENABLE_HW_CHECKSUM
    if (m_zlibFlow) {
        h_buf_checksum_data.data()[0] = 1;
        checksum_type = false;
    } else {
        h_buf_checksum_data.data()[0] = ~0;
        checksum_type = true;
    }
#endif
    m_lastData = (last_buffer) ? true : false;

    buffers* lastBuffer = m_memoryManager->getLastBuffer();
    if (input_size) {
        auto buffer = m_memoryManager->createBuffer(input_size);
        if (buffer != NULL) {
#ifdef ENABLE_SW_CHECKSUM
            // Trigger CRC calculation
            generate_checksum(in, input_size);
#endif
            std::memcpy(buffer->h_buf_in, &in[0], input_size);
            uint32_t inSize = input_size;
            uint32_t chkSize = input_size;
            buffer->store_size = 0;

            uint32_t last_buffer_size = inSize % (BLOCK_SIZE_IN_KB * 1024);

            if ((last_buffer_size != 0) && last_buffer_size < MIN_BLOCK_SIZE) {
                inSize -= last_buffer_size;
                buffer->store_size = last_buffer_size;
            }

            int narg = 0;
            if (inSize > MIN_BLOCK_SIZE) {
                // Set kernel arguments
                compress_stream_kernel->setArg(narg++, *(buffer->buffer_input));
                compress_stream_kernel->setArg(narg++, *(buffer->buffer_zlib_output));
                compress_stream_kernel->setArg(narg++, *(buffer->buffer_compress_size));
                compress_stream_kernel->setArg(narg++, inSize);
            }

#ifdef ENABLE_HW_CHECKSUM
            narg = 0;

            checksum_kernel->setArg(narg++, *(buffer->buffer_input));
            checksum_kernel->setArg(narg++, *buffer_checksum_data);
            checksum_kernel->setArg(narg++, chkSize);
            checksum_kernel->setArg(narg++, checksum_type);
#endif

            // Migrate memory - Map host to device buffers
            OCL_CHECK(err, err = m_def_q->enqueueMigrateMemObjects({*(buffer->buffer_input)}, 0 /* 0 means from host*/,
                                                                   NULL, &(buffer->wr_event)));

            std::vector<cl::Event> wrEvents = {buffer->wr_event};
#ifdef ENABLE_HW_CHECKSUM
            std::vector<cl::Event> wrChkEvents = {buffer->wr_event};
            if (initialize_checksum) {
                OCL_CHECK(err, err = m_def_q->enqueueMigrateMemObjects(
                                   {*(buffer_checksum_data)}, 0 /* 0 means from host*/, NULL, &(buffer->chk_wr_event)));
                wrChkEvents.push_back(buffer->chk_wr_event);
            }
#endif

            std::vector<cl::Event> cmpEvents;
            if (inSize > MIN_BLOCK_SIZE) {
                // kernel write events update
                // LZ77 Compress Fire Kernel invocation
                OCL_CHECK(err, err = m_def_q->enqueueTask(*compress_stream_kernel, &(wrEvents), &(buffer->cmp_event)));

                cmpEvents = {buffer->cmp_event};
            }

#ifdef ENABLE_HW_CHECKSUM
            if (lastBuffer != NULL) {
                wrChkEvents.push_back(lastBuffer->chk_event);
            }
            // checksum kernel
            OCL_CHECK(err, err = m_def_q->enqueueTask(*checksum_kernel, &(wrChkEvents), &(buffer->chk_event)));
#endif

            if (inSize > MIN_BLOCK_SIZE) {
                OCL_CHECK(err, err = m_def_q->enqueueMigrateMemObjects(
                                   {*(buffer->buffer_compress_size), *(buffer->buffer_zlib_output)},
                                   CL_MIGRATE_MEM_OBJECT_HOST, &(cmpEvents), &(buffer->rd_event)));
            }

            cl_int err;
            if (inSize > MIN_BLOCK_SIZE) {
                OCL_CHECK(err,
                          err = buffer->rd_event.setCallback(CL_COMPLETE, xf::compression::event_cb, (void*)buffer));
            } else {
                OCL_CHECK(err,
                          err = buffer->chk_event.setCallback(CL_COMPLETE, xf::compression::event_cb, (void*)buffer));
            }
            input_size = 0;
            actionDone = true;
        }
    }

    if ((m_memoryManager->peekBuffer() != NULL) && m_memoryManager->peekBuffer()->finish) {
        buffers* buffer;
        uint32_t compSize = 0;
        auto size = 0;
        buffer = m_memoryManager->getBuffer();
        uint32_t block_size_in_bytes = BLOCK_SIZE_IN_KB * 1024;
        if (buffer != NULL) {
            actionDone = true;
            double accelerated_size = buffer->input_size - buffer->store_size;
            auto n_blocks = (uint8_t)ceil(accelerated_size / block_size_in_bytes);
            uint32_t bIdx = 0;
            for (; bIdx < n_blocks; bIdx++) {
                compSize = (buffer->h_compressSize)[bIdx];
                if (compSize > block_size_in_bytes) {
                    uint32_t block_size = block_size_in_bytes;
                    uint8_t zeroData = 0x00;
                    uint8_t len_low = (uint8_t)block_size;
                    uint8_t len_high = (uint8_t)(block_size >> 8);
                    uint8_t len_low_n = ~len_low;
                    uint8_t len_high_n = ~len_high;
                    out[size++] = zeroData;
                    out[size++] = len_low;
                    out[size++] = len_high;
                    out[size++] = len_low_n;
                    out[size++] = len_high_n;
                    std::memcpy(out + size, &buffer->h_buf_in[bIdx * block_size_in_bytes], block_size);
                    size += block_size;
                } else {
                    std::memcpy(out + size, &buffer->h_buf_zlibout[bIdx * block_size_in_bytes], compSize);
                    size += compSize;
                }
            }

            if (buffer->store_size > 0) {
                uint32_t block_size = buffer->store_size;
                uint8_t zeroData = 0x00;
                uint8_t len_low = (uint8_t)block_size;
                uint8_t len_high = (uint8_t)(block_size >> 8);
                uint8_t len_low_n = ~len_low;
                uint8_t len_high_n = ~len_high;
                out[size++] = zeroData;
                out[size++] = len_low;
                out[size++] = len_high;
                out[size++] = len_low_n;
                out[size++] = len_high_n;
                std::memcpy(out + size, &buffer->h_buf_in[bIdx * block_size_in_bytes], block_size);
                size += block_size;
            }
            //            m_pending--;
        }

        if (m_lastData && (input_size == 0) && (m_memoryManager->isPending() == false))
            last_data = true;
        else
            last_data = false;

#ifdef ENABLE_HW_CHECKSUM
        if (last_data) {
            OCL_CHECK(err,
                      err = m_def_q->enqueueMigrateMemObjects({*(buffer_checksum_data)}, CL_MIGRATE_MEM_OBJECT_HOST));
            OCL_CHECK(err, err = m_def_q->finish());

            m_checksum = h_buf_checksum_data.data()[0];
            if (checksum_type) m_checksum = ~m_checksum;
        }
#endif
        return size;
    }

    if ((actionDone == false) && (m_memoryManager->isPending() == true)) {
        // wait for a queue to finish to save CPU cycles
        auto buffer = m_memoryManager->peekBuffer();
        if (buffer != NULL) {
            buffer->rd_event.wait();
        }
    }

    if (m_lastData && (input_size == 0) && (m_memoryManager->isPending() == false))
        last_data = true;
    else
        last_data = false;
    return 0;
}

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
size_t xfZlib::compress_buffer(
    uint8_t* in, uint8_t* out, size_t input_size, bool last_buffer, uint32_t host_buffer_size, int cu) {
#if (VERBOSE_LEVEL >= 1)
    std::cout << "CU: " << cu << " ";
#endif

#ifdef ENABLE_SW_CHECKSUM
    // Trigger CRC calculation
    generate_checksum(in, input_size);
#endif

    cl_int err;

#ifdef USE_SINGLE_KERNEL_ZLIBC
    // Create kernel
    std::string cu_id = std::to_string((cu + 1));
    std::string compress_kname = compress_kernel_names[1] + ":{" + compress_kernel_names[1] + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel compress_stream_kernel(*m_program, compress_kname.c_str(), &err));

#else
    // Create kernel
    std::string cu_id = std::to_string((cu + 1));
    std::string lz77_kname = compress_kernel_names[0] + ":{" + compress_kernel_names[0] + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel lz77_kernel(*m_program, lz77_kname.c_str(), &err));

    std::string huff_kname = huffman_kernel_names[0] + ":{" + huffman_kernel_names[0] + "_" + cu_id + "}";
    OCL_CHECK(err, cl::Kernel huff_kernel(*m_program, huff_kname.c_str(), &err));

#endif

#ifdef USE_SINGLE_KERNEL_ZLIBC
    // Read, Write and Kernel events
    cl::Event comp_write_events[OVERLAP_BUF_COUNT];
    cl::Event comp_kernel_events[OVERLAP_BUF_COUNT];
    cl::Event comp_read_events[OVERLAP_BUF_COUNT];

    // Kernel wait events for writing & compute
    std::vector<cl::Event> compKernelWriteWait[OVERLAP_BUF_COUNT];
    std::vector<cl::Event> compKernelComputeWait[OVERLAP_BUF_COUNT];
#endif

#ifdef ENABLE_HW_CHECKSUM
    cl::Event cs_write_event;
    cl::Event cs_kernel_event;
    cl::Event cs_read_event;
    std::vector<cl::Event> csKernelWriteWait;
    std::vector<cl::Event> csKernelComputeWait;
#endif

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

    // Common variables
    int completed_bricks = 0;
    uint32_t input_index = 0;
    uint16_t tail_block_size = 0;
    bool flag_smallblk = false;

#ifdef USE_SINGLE_KERNEL_ZLIBC

    uint8_t buffIdx = 0;
    bool checksumDataTransfer = false;

#ifdef ENABLE_HW_CHECKSUM
    checksumDataTransfer = true;
    bool checksum_type = false;

    if (m_zlibFlow)
        checksum_type = false;
    else
        checksum_type = true;

    h_buf_checksum_data.data()[0] = 1;
    // if checksum type call crc32
    if (checksum_type) h_buf_checksum_data.data()[0] = ~0;

#endif

overlap:
    for (uint32_t brick = 0, itr = 0; brick < total_chunks; itr++, buffIdx = (itr % OVERLAP_BUF_COUNT)) {
        chunk_flags[brick] = buffIdx;

        if (itr >= OVERLAP_BUF_COUNT) {
            // Wait for read events
            comp_read_events[buffIdx].wait();

            // Completed bricks counter
            completed_bricks++;

            uint32_t brick_flag_idx = brick - overlap_buf_count;

            // Copy the data from various blocks in concatinated manner
            for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++) {
                uint32_t compressed_size = h_compressSize[cu][buffIdx].data()[bIdx];
                if (compressed_size <= block_size_in_bytes) {
                    std::memcpy(&out[outIdx], &h_buf_zlibout[cu][buffIdx].data()[bIdx * block_size_in_bytes],
                                compressed_size);
                    outIdx += compressed_size;
                } else {
                    uint8_t zeroData = 0x00;
                    uint8_t len_low = (uint8_t)block_size_in_bytes;
                    uint8_t len_high = (uint8_t)(block_size_in_bytes >> 8);
                    uint8_t len_low_n = ~len_low;
                    uint8_t len_high_n = ~len_high;
                    out[outIdx++] = zeroData;
                    out[outIdx++] = len_low;
                    out[outIdx++] = len_high;
                    out[outIdx++] = len_low_n;
                    out[outIdx++] = len_high_n;
                    std::memcpy(&out[outIdx], &h_buf_in[cu][buffIdx].data()[bIdx * block_size_in_bytes],
                                block_size_in_bytes);
                    outIdx += block_size_in_bytes;
                }
            }

        } // If condition which reads compress output for 0 or 1 location
        // Figure out block sizes per brick
        uint32_t idxblk = 0;
        for (uint32_t i = 0; i < sizeOfChunk[brick]; i += block_size_in_bytes, idxblk++) {
            uint32_t block_size = block_size_in_bytes;

            if (i + block_size > sizeOfChunk[brick]) {
                block_size = sizeOfChunk[brick] - i;
            }
            if (block_size < MIN_BLOCK_SIZE) {
                input_index = idxblk * block_size_in_bytes;
                flag_smallblk = true;
                tail_block_size = block_size;
                sizeOfChunk[brick] -= block_size;
                blocksPerChunk[brick]--;
            }
        }

        if (sizeOfChunk[brick] != 0) {
            std::memcpy(h_buf_in[cu][buffIdx].data(), &in[brick * host_buffer_size], sizeOfChunk[brick]);

            // Set compress kernel arguments
            int narg = 0;

            compress_stream_kernel.setArg(narg++, *buffer_input[cu][buffIdx]);
            compress_stream_kernel.setArg(narg++, *buffer_zlib_output[cu][buffIdx]);
            compress_stream_kernel.setArg(narg++, *buffer_compress_size[cu][buffIdx]);
            compress_stream_kernel.setArg(narg++, sizeOfChunk[brick]);
        }

#ifdef ENABLE_HW_CHECKSUM
        int narg = 0;
        checksum_kernel->setArg(narg++, *buffer_input[cu][buffIdx]);
        checksum_kernel->setArg(narg++, *buffer_checksum_data);
        checksum_kernel->setArg(narg++, sizeOfChunk[brick]);
        checksum_kernel->setArg(narg++, checksum_type);
#endif
        // Migrate memory - Map host to device buffers
        OCL_CHECK(err, err = m_q[cu]->enqueueMigrateMemObjects({*(buffer_input[cu][buffIdx])}, 0, NULL,
                                                               &comp_write_events[buffIdx]));
        if (sizeOfChunk[brick] != 0) compKernelWriteWait[buffIdx].push_back(comp_write_events[buffIdx]);

// Migrate checksum data
#ifdef ENABLE_HW_CHECKSUM
        csKernelWriteWait.push_back(comp_write_events[buffIdx]);
        if (checksumDataTransfer) {
            OCL_CHECK(err,
                      err = m_q[cu]->enqueueMigrateMemObjects({*(buffer_checksum_data)}, 0, NULL, &cs_write_event));
            csKernelWriteWait.push_back(cs_write_event);

            // Checksum Fire Kernel invocation
            OCL_CHECK(err, err = m_q[cu]->enqueueTask(*checksum_kernel, &csKernelWriteWait, &cs_kernel_event));
            csKernelWriteWait.push_back(cs_kernel_event);
            checksumDataTransfer = false;
        } else {
            // Checksum Fire Kernel invocation
            OCL_CHECK(err, err = m_q[cu]->enqueueTask(*checksum_kernel, &csKernelWriteWait, &cs_kernel_event));
            csKernelWriteWait.push_back(cs_kernel_event);
        }
#endif
        if (sizeOfChunk[brick] != 0) {
            // Compress Fire Kernel invocation
            OCL_CHECK(err, err = m_q[cu]->enqueueTask(compress_stream_kernel, &compKernelWriteWait[buffIdx],
                                                      &comp_kernel_events[buffIdx]));

            // Update kernel events flag on computation
            compKernelComputeWait[buffIdx].push_back(comp_kernel_events[buffIdx]);

            // Migrate memory - Map device to host buffers
            OCL_CHECK(err,
                      err = m_q[cu]->enqueueMigrateMemObjects(
                          {*(buffer_zlib_output[cu][buffIdx]), *(buffer_compress_size[cu][buffIdx])},
                          CL_MIGRATE_MEM_OBJECT_HOST, &compKernelComputeWait[buffIdx], &comp_read_events[buffIdx]));
        }
        brick++;

    } // Main overlap loop

    OCL_CHECK(err, err = m_q[cu]->finish());

#ifdef ENABLE_HW_CHECKSUM
    // read back check sum data
    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = m_q[cu]->enqueueMigrateMemObjects({*(buffer_checksum_data)}, CL_MIGRATE_MEM_OBJECT_HOST));
    OCL_CHECK(err, err = m_q[cu]->finish());

    m_checksum = h_buf_checksum_data.data()[0];
    if (checksum_type) m_checksum = ~m_checksum;
#endif
    uint32_t leftover = total_chunks - completed_bricks;
    uint32_t stride = 0;
    if ((total_chunks < overlap_buf_count))
        stride = overlap_buf_count;
    else
        stride = total_chunks;
    // Handle leftover bricks
    for (uint32_t ovr_itr = 0, brick = stride - overlap_buf_count; ovr_itr < leftover; ovr_itr++, brick++) {
        uint8_t buffIdx = chunk_flags[brick];

        // Run over each block within brick
        uint32_t brick_flag_idx = brick;
        // Copy the data from various blocks in concatinated manner
        for (uint32_t bIdx = 0; bIdx < blocksPerChunk[brick_flag_idx]; bIdx++) {
            uint32_t compressed_size = h_compressSize[cu][buffIdx].data()[bIdx];
            if (compressed_size <= block_size_in_bytes) {
                std::memcpy(&out[outIdx], &h_buf_zlibout[cu][buffIdx].data()[bIdx * block_size_in_bytes],
                            compressed_size);
                outIdx += compressed_size;
            } else {
                uint8_t zeroData = 0x00;
                uint8_t len_low = (uint8_t)block_size_in_bytes;
                uint8_t len_high = (uint8_t)(block_size_in_bytes >> 8);
                uint8_t len_low_n = ~len_low;
                uint8_t len_high_n = ~len_high;
                out[outIdx++] = zeroData;
                out[outIdx++] = len_low;
                out[outIdx++] = len_high;
                out[outIdx++] = len_low_n;
                out[outIdx++] = len_high_n;
                std::memcpy(&out[outIdx], &h_buf_in[cu][buffIdx].data()[bIdx * block_size_in_bytes],
                            block_size_in_bytes);
                outIdx += block_size_in_bytes;
            }
        }
    }
#else
    // Track the lags of respective chunks for left over handling
    int cu_order[total_chunks];

    // Finished bricks
    int flag = 0;
    uint32_t lcl_cu = 0;

    uint8_t cunits = (uint8_t)C_COMPUTE_UNIT;
    uint8_t queue_idx = 0;
    std::chrono::duration<double, std::milli> compress_API_time_ms_1(0);
overlap:
    for (uint32_t brick = 0, itr = 0; brick < total_chunks;
         /*brick += C_COMPUTE_UNIT,*/ itr++, flag = !flag) {
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
                if (block_size < MIN_BLOCK_SIZE) {
                    input_index = idxblk * block_size_in_bytes;
                    flag_smallblk = true;
                    tail_block_size = block_size;
                    sizeOfChunk[brick + cu] -= block_size;
                } else {
                    //////printme("sizeofChunk %d block_size %d cu %d \n",
                    /// sizeOfChunk[brick+cu], block_size, cu);
                    (h_blksize[cu][flag]).data()[idxblk++] = block_size;
                }
            }
            if (sizeOfChunk[brick + cu] != 0) {
                std::memcpy(h_buf_in[cu][flag].data(), &in[(brick + cu) * host_buffer_size], sizeOfChunk[brick + cu]);

                // Set kernel arguments
                int narg = 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_input[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_lz77_output[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_compress_size[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_inblk_size[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = lz77_kernel.setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                OCL_CHECK(err, err = lz77_kernel.setArg(narg++, block_size_in_kb));

                OCL_CHECK(err, err = lz77_kernel.setArg(narg++, sizeOfChunk[brick + cu]));

                narg = 0;
                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_lz77_output[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_dyn_ltree_freq[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_dyn_dtree_freq[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_zlib_output[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_compress_size[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, *(buffer_inblk_size[cu][flag])), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);
                if (error_code()) return 0;

                OCL_CHECK(err, err = huff_kernel.setArg(narg++, block_size_in_kb));
                ZOCL_CHECK_2(err, err = huff_kernel.setArg(narg++, sizeOfChunk[brick + cu]), m_err_code,
                             c_clOutOfResource, c_clOutOfHostMemory);

                // Migrate memory - Map host to device buffers
                ZOCL_CHECK_2(err,
                             err = m_q[queue_idx + cu]->enqueueMigrateMemObjects(
                                 {*(buffer_input[cu][flag]), *(buffer_inblk_size[cu][flag])}, 0 /* 0 means from host*/),
                             m_err_code, c_clOutOfHostMemory, c_clOutOfResource);

                // kernel write events update
                // LZ77 Compress Fire Kernel invocation
                ZOCL_CHECK_2(err, err = m_q[queue_idx + cu]->enqueueTask(lz77_kernel), m_err_code, c_clOutOfResource,
                             c_clOutOfHostMemory);

                // Huffman Fire Kernel invocation
                ZOCL_CHECK_2(err, err = m_q[queue_idx + cu]->enqueueTask(huff_kernel), m_err_code, c_clOutOfHostMemory,
                             c_clOutOfResource);

                ZOCL_CHECK_2(err, err = m_q[queue_idx + cu]->enqueueMigrateMemObjects(
                                      {*(buffer_compress_size[cu][flag])}, CL_MIGRATE_MEM_OBJECT_HOST),
                             m_err_code, c_clOutOfResource, c_clOutOfHostMemory);
            }
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
                if (compressed_size <= block_size_in_bytes) {
                    m_q[queue_idx + cu]->enqueueReadBuffer(*(buffer_zlib_output[cu][flag]), CL_TRUE, index,
                                                           compressed_size * sizeof(uint8_t), &out[outIdx]);
                    outIdx += compressed_size;
                } else {
                    uint8_t zeroData = 0x00;
                    uint8_t len_low = (uint8_t)block_size_in_bytes;
                    uint8_t len_high = (uint8_t)(block_size_in_bytes >> 8);
                    uint8_t len_low_n = ~len_low;
                    uint8_t len_high_n = ~len_high;
                    out[outIdx++] = zeroData;
                    out[outIdx++] = len_low;
                    out[outIdx++] = len_high;
                    out[outIdx++] = len_low_n;
                    out[outIdx++] = len_high_n;
                    std::memcpy(&out[outIdx], &h_buf_in[cu][flag].data()[bIdx * block_size_in_bytes],
                                block_size_in_bytes);
                    outIdx += block_size_in_bytes;
                }
            }
        }
    }
#endif
    if (flag_smallblk) {
        uint8_t len_low = (uint8_t)tail_block_size;
        uint8_t len_high = (uint8_t)(tail_block_size >> 8);
        out[outIdx++] = 0x00;
        out[outIdx++] = len_low;
        out[outIdx++] = len_high;
        out[outIdx++] = ~len_low;
        out[outIdx++] = ~len_high;
        for (int i = 0; i < tail_block_size; i++) {
            out[outIdx++] = in[input_index + i];
        }
    }

    // Add the last block information
    if (last_buffer) {
        long int last_block = 0xffff000001;
        // zlib special block based on Z_SYNC_FLUSH
        std::memcpy(&out[outIdx], &last_block, 5);
        outIdx += 5;
    }

    return outIdx;
}

// Add zlib header
size_t xfZlib::add_header(uint8_t* out, int level, int strategy, int window_bits) {
    size_t outIdx = 0;
    if (m_zlibFlow) {
        // Compression method
        uint8_t CM = DEFLATE_METHOD;

        // Compression Window information
        uint8_t CINFO = window_bits - 8;

        // Create CMF header
        uint16_t header = (CINFO << 4);
        header |= CM;
        header <<= 8;

        if (level < 2 || strategy > 2)
            level = 0;
        else if (level < 6)
            level = 1;
        else if (level == 6)
            level = 2;
        else
            level = 3;

        // CreatE FLG header based on level
        // Strategy information
        header |= (level << 6);

        // Ensure that Header (CMF + FLG) is
        // divisible by 31
        header += 31 - (header % 31);

        out[outIdx] = (uint8_t)(header >> 8);
        out[outIdx + 1] = (uint8_t)(header);
        outIdx += 2;
    } else {
        const uint16_t c_format_0 = 31;
        const uint16_t c_format_1 = 139;
        const uint16_t c_variant = 8;
        const uint16_t c_real_code = 8;
        const uint16_t c_opcode = 3;

        // 2 bytes of magic header
        out[outIdx++] = c_format_0;
        out[outIdx++] = c_format_1;

        // 1 byte Compression method
        out[outIdx++] = c_variant;

        // 1 byte flags
        uint8_t flags = 0;
        flags |= c_real_code;
        out[outIdx++] = flags;

        // 4 bytes file modification time in unit format
        unsigned long time_stamp = 0;
        struct stat istat;
        stat(m_infile.c_str(), &istat);
        time_stamp = istat.st_mtime;
        out[outIdx++] = time_stamp;
        out[outIdx++] = time_stamp >> 8;
        out[outIdx++] = time_stamp >> 16;
        out[outIdx++] = time_stamp >> 24;

        // 1 byte extra flag (depend on compression method)
        uint8_t deflate_flags = 0;
        out[outIdx++] = deflate_flags;

        // 1 byte OPCODE - 0x03 for Unix
        out[outIdx++] = c_opcode;

        // Dump file name
        for (int i = 0; m_infile[i] != '\0'; i++) {
            out[outIdx++] = m_infile[i];
        }
        out[outIdx++] = 0;
    }
    return outIdx;
}

std::string xfZlib::getCompressKernel(int idx) {
    return compress_kernel_names[idx];
}

std::string xfZlib::getDeCompressKernel(int idx) {
    return stream_decompress_kernel_name[idx];
}

// Add zlib footer
size_t xfZlib::add_footer(uint8_t* out, size_t compressSize) {
    size_t outIdx = compressSize;
    uint32_t checksum = 0;

    outIdx = compressSize;
#ifdef ENABLE_SW_CHECKSUM
    checksum = get_checksum();
#elif ENABLE_HW_CHECKSUM
    checksum = m_checksum;
#endif

    if (m_zlibFlow) {
        out[outIdx++] = checksum >> 24;
        out[outIdx++] = checksum >> 16;
        out[outIdx++] = checksum >> 8;
        out[outIdx++] = checksum;
    } else {
        struct stat istat;
        stat(m_infile.c_str(), &istat);
        unsigned long ifile_size = istat.st_size;
        out[outIdx++] = checksum;
        out[outIdx++] = checksum >> 8;
        out[outIdx++] = checksum >> 16;
        out[outIdx++] = checksum >> 24;

        out[outIdx++] = ifile_size;
        out[outIdx++] = ifile_size >> 8;
        out[outIdx++] = ifile_size >> 16;
        out[outIdx++] = ifile_size >> 24;

        out[outIdx++] = 0;
        out[outIdx++] = 0;
        out[outIdx++] = 0;
        out[outIdx++] = 0;
        out[outIdx++] = 0;
    }

    return outIdx;
}

// This version of compression does overlapped execution between
// Kernel and Host. I/O operations between Host and Device are
// overlapped with Kernel execution between multiple compute units
size_t xfZlib::compress(
    uint8_t* in, uint8_t* out, size_t input_size, int cu, int level, int strategy, int window_bits) {
    size_t total_size = 0;
    size_t host_buffer_size = HOST_BUFFER_SIZE;
    // Add header
    total_size += add_header(out, level, strategy, window_bits);

    // Call compress buffer
    total_size += compress_buffer(in, &out[total_size], input_size, true, host_buffer_size, cu);

    // Add footer
    total_size = add_footer(out, total_size);

    return total_size;
} // Overlap end

memoryManager::memoryManager(uint8_t max_buffer, cl::Context* context) {
    maxBufCount = max_buffer;
    mContext = context;
    bufCount = 0;
}

memoryManager::~memoryManager() {
    release();
}

buffers* memoryManager::createBuffer(size_t size) {
    if (!(this->freeBuffers.empty())) {
        if (this->freeBuffers.front()->allocated_size >= size) {
            auto buffer = this->freeBuffers.front();
            this->busyBuffers.push(buffer);
            this->freeBuffers.pop();
            lastBuffer = buffer;
            buffer->input_size = size;
            buffer->finish = false;
            return buffer;
        } else {
            auto buffer = this->freeBuffers.front();
            zlib_aligned_allocator<uint8_t> alocator;
            zlib_aligned_allocator<uint32_t> alocator_1;

            alocator.deallocate(buffer->h_buf_in);
            alocator.deallocate(buffer->h_buf_zlibout);
            alocator_1.deallocate(buffer->h_compressSize);

            DELETE_OBJ(buffer->buffer_compress_size);
            DELETE_OBJ(buffer->buffer_input);
            DELETE_OBJ(buffer->buffer_zlib_output);

            this->freeBuffers.pop();
            DELETE_OBJ(buffer);
            bufCount -= 1;
        }
    }

    if (bufCount < maxBufCount) {
        auto buffer = new buffers();
        buffer->finish = false;
        zlib_aligned_allocator<uint8_t> alocator;
        zlib_aligned_allocator<uint32_t> alocator_1;
        MEM_ALLOC_CHECK((buffer->h_buf_in = alocator.allocate(size)), size, "Input Host Buffer");
        MEM_ALLOC_CHECK((buffer->h_buf_zlibout = alocator.allocate(size)), size, "Output Host Buffer");
        uint16_t max_blocks = (size / BLOCK_SIZE_IN_KB) + 1;
        MEM_ALLOC_CHECK((buffer->h_compressSize = alocator_1.allocate(max_blocks)), max_blocks,
                        "CompressSize Host Buffer");

        cl_int err;
        OCL_CHECK(err, buffer->buffer_input = new cl::Buffer(*(this->mContext), CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                             size, buffer->h_buf_in, &err));

        OCL_CHECK(err, buffer->buffer_compress_size =
                           new cl::Buffer(*mContext, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          max_blocks * sizeof(uint32_t), buffer->h_compressSize, &err));

        OCL_CHECK(err, buffer->buffer_zlib_output = new cl::Buffer(*mContext, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                                   size, buffer->h_buf_zlibout, &err));

        buffer->input_size = size;
        buffer->allocated_size = size;
        this->busyBuffers.push(buffer);
        bufCount++;
        lastBuffer = buffer;
        return buffer;
    }
    return NULL;
}

buffers* memoryManager::getBuffer() {
    if (this->busyBuffers.empty()) return NULL;
    auto buffer = this->busyBuffers.front();
    this->freeBuffers.push(buffer);
    this->busyBuffers.pop();
    return buffer;
}

buffers* memoryManager::peekBuffer() {
    if (this->busyBuffers.empty()) return NULL;
    auto buffer = this->busyBuffers.front();
    return buffer;
}

buffers* memoryManager::getLastBuffer() {
    return lastBuffer;
}

void memoryManager::release() {
    while (!(this->freeBuffers.empty())) {
        auto buffer = this->freeBuffers.front();
        zlib_aligned_allocator<uint8_t> alocator;
        zlib_aligned_allocator<uint32_t> alocator_1;

        alocator.deallocate(buffer->h_buf_in);
        alocator.deallocate(buffer->h_buf_zlibout);
        alocator_1.deallocate(buffer->h_compressSize);

        DELETE_OBJ(buffer->buffer_compress_size);
        DELETE_OBJ(buffer->buffer_input);
        DELETE_OBJ(buffer->buffer_zlib_output);

        this->freeBuffers.pop();
        DELETE_OBJ(buffer);
    }
}

bool memoryManager::isPending() {
    return (this->busyBuffers.empty() == false);
}
