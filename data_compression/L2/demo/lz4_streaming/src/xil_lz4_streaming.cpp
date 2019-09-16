/*
 * Copyright 2019 Xilinx, Inc.
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
 */

#include "xil_lz4_streaming.hpp"
#include "xxhash.h"

#define BLOCK_SIZE 64
#define KB 1024
#define MAGIC_HEADER_SIZE 4
#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

// Declaration of custom stream APIs that binds to Xilinx Streaming APIs.
decltype(&clCreateStream) xcl::Stream::createStream = nullptr;
decltype(&clReleaseStream) xcl::Stream::releaseStream = nullptr;
decltype(&clReadStream) xcl::Stream::readStream = nullptr;
decltype(&clWriteStream) xcl::Stream::writeStream = nullptr;
decltype(&clPollStreams) xcl::Stream::pollStreams = nullptr;

uint64_t xfLz4Streaming::getEventDurationNs(const cl::Event& event) {
    uint64_t start_time = 0, end_time = 0;

    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &start_time);
    event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &end_time);
    return (end_time - start_time);
}

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

void xfLz4Streaming::mapStreams(bool flow) {
    if (flow) {
        cExt.obj = NULL;                         // null since no buffer transfer required
        cExt.param = compress_kernel_lz4->get(); // the kernel

        cExt.flags = 0;                               // argument number
        cpr_input_stream = xcl::Stream::createStream( // write stream for input data
            m_device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &cExt, &cRet);

        cExt.flags = 1;
        cpr_output_stream = xcl::Stream::createStream( // read stream for compressed output data
            m_device.get(), CL_STREAM_READ_ONLY, CL_STREAM, &cExt, &cRet);

        cExt.flags = 2;
        cpr_csize_stream = xcl::Stream::createStream( // read stream for compressed output data
            m_device.get(), CL_STREAM_READ_ONLY, CL_STREAM, &cExt, &cRet);
    } else {
        dExt.obj = NULL;
        dExt.param = decompress_kernel_lz4->get();

        dExt.flags = 0;                               // argument number
        dcp_input_stream = xcl::Stream::createStream( // write stream for input data
            m_device.get(), CL_STREAM_WRITE_ONLY, CL_STREAM, &dExt, &dRet);

        dExt.flags = 1;
        dcp_output_stream = xcl::Stream::createStream( // read stream for compressed output data
            m_device.get(), CL_STREAM_READ_ONLY, CL_STREAM, &dExt, &dRet);
    }
}

// Constructor
xfLz4Streaming::xfLz4Streaming() {
    // Index calculation
    first_init = true;
    h_buf_in.resize(HOST_BUFFER_SIZE);
    h_buf_out.resize(HOST_BUFFER_SIZE);
    h_compressSize.resize(MAX_NUMBER_BLOCKS);
}

// Destructor
xfLz4Streaming::~xfLz4Streaming() {}

int xfLz4Streaming::init(const std::string& binaryFile) {
    // unsigned fileBufSize;
    // The get_xil_devices will return vector of Xilinx Devices
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    m_device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = new cl::Context(m_device);
    m_q =
        new cl::CommandQueue(*m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = m_device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // import_binary() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return as Binaries
    // OpenCL and it can contain many functions which can be executed on the
    // device.
    // char* fileBuf = xcl::read_binary_file(binaryFile, fileBufSize);
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);

    m_program = new cl::Program(*m_context, devices, bins);

    cl_int err;
    auto platform_id = m_device.getInfo<CL_DEVICE_PLATFORM>(&err);
    // Initialization of streaming class is needed before using it.

    if (first_init) {
        first_init = false;
        xcl::Stream::init(platform_id);
    }
    if (SINGLE_XCLBIN) {
        // Create Compress kernels
        compress_kernel_lz4 = new cl::Kernel(*m_program, compress_kernel_name.c_str());

        // Create Decompress kernels
        decompress_kernel_lz4 = new cl::Kernel(*m_program, decompress_kernel_name.c_str());

        // Assignment to the buffer extensions and create streams
        mapStreams(0);
        mapStreams(1);
    } else {
        if (m_bin_flow) {
            // Create Compress kernels
            compress_kernel_lz4 = new cl::Kernel(*m_program, compress_kernel_name.c_str());
            mapStreams(1);
        } else {
            // Create Decompress kernels
            decompress_kernel_lz4 = new cl::Kernel(*m_program, decompress_kernel_name.c_str());
            mapStreams(0);
        }
    }

    return 0;
}

int xfLz4Streaming::release() {
    if (m_bin_flow) {
        delete compress_kernel_lz4;
        // Releasing Streams
        xcl::Stream::releaseStream(cpr_input_stream);
        xcl::Stream::releaseStream(cpr_output_stream);
        xcl::Stream::releaseStream(cpr_csize_stream);
    } else {
        delete decompress_kernel_lz4;
        // Releasing Streams
        xcl::Stream::releaseStream(dcp_input_stream);
        xcl::Stream::releaseStream(dcp_output_stream);
    }
    delete (m_program);
    delete (m_q);
    delete (m_context);

    return 0;
}

uint64_t xfLz4Streaming::compressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    if (m_switch_flow == 0) { // Xilinx FPGA compression flow
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }
        // allocate memory for input and output files
        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(input_size);

        inFile.read((char*)in.data(), input_size);

        // LZ4 header
        outFile.put(MAGIC_BYTE_1);
        outFile.put(MAGIC_BYTE_2);
        outFile.put(MAGIC_BYTE_3);
        outFile.put(MAGIC_BYTE_4);

        // FLG & BD bytes
        // --no-frame-crc flow
        // --content-size
        outFile.put(FLG_BYTE);

        // Default value 64K
        uint8_t block_size_header = 0;
        switch (m_block_size_in_kb) {
            case 64:
                outFile.put(BSIZE_STD_64KB);
                block_size_header = BSIZE_STD_64KB;
                break;
            case 256:
                outFile.put(BSIZE_STD_256KB);
                block_size_header = BSIZE_STD_256KB;
                break;
            case 1024:
                outFile.put(BSIZE_STD_1024KB);
                block_size_header = BSIZE_STD_1024KB;
                break;
            case 4096:
                outFile.put(BSIZE_STD_4096KB);
                block_size_header = BSIZE_STD_4096KB;
                break;
            default:
                std::cout << "Invalid Block Size" << std::endl;
                break;
        }

        uint8_t temp_buff[10] = {FLG_BYTE,         block_size_header, input_size,       input_size >> 8,
                                 input_size >> 16, input_size >> 24,  input_size >> 32, input_size >> 40,
                                 input_size >> 48, input_size >> 56};

        // xxhash is used to calculate hash value
        uint32_t xxh = XXH32(temp_buff, 10, 0);
        uint64_t enbytes;
        outFile.write((char*)&temp_buff[2], 8);

        // Header CRC
        outFile.put((uint8_t)(xxh >> 8));
        // LZ4 compression in streaming mode
        enbytes = compress(in.data(), out.data(), input_size);
        // Writing compressed data
        outFile.write((char*)out.data(), enbytes);

        outFile.put(0);
        outFile.put(0);
        outFile.put(0);
        outFile.put(0);

        // Close file
        inFile.close();
        outFile.close();
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

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xfLz4Streaming::compress(uint8_t* in, uint8_t* out, uint64_t input_size) {
    /**
     *  > Resize host buffers to appropriate sizes
     *  > Calculate 64K blocks required for entire file
     *  > For each CU:
     *      > Create read and write streams
     *      > Do non-blocking stream IO and kernel execution:
     *          > Launch the kernel and enqueue tasks
     *          > Write data to kernel input streams
     *          > Read data from kernel output streams
     *          > Poll the streams for completed operations
     *          > On complete, map the next input block
     *      > On complete copy the compressed data to output buffer
     */
    // Allocate host buffers
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;
    uint32_t total_block_count = (input_size - 1) / host_buffer_size + 1;
    uint64_t total_kernel_time = 0;

    // h_buf_in.resize(host_buffer_size * total_block_count);
    h_buf_out.resize(host_buffer_size * total_block_count);
    h_compressSize.resize(total_block_count);

    // write and read the streams
    cl_stream_xfer_req rd_req{0};
    // cl_stream_xfer_req rd_req2{0};
    cl_stream_xfer_req wr_req{0};

    rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;

    // kernel wait event
    cl::Event k_wait_event[total_block_count];

    int32_t req_cnt = 3;

    auto total_start = std::chrono::high_resolution_clock::now();

    for (uint64_t buf_indx = 0, blk_idx = 0; buf_indx < input_size; buf_indx += host_buffer_size, ++blk_idx) {
        // reset stream polling request
        cl_streams_poll_req_completions poll_req[req_cnt]{0, 0, 0};

        // current block input size
        uint32_t c_input_size = host_buffer_size;
        if (blk_idx == total_block_count - 1) c_input_size = input_size - buf_indx;

        // set kernel arguments
        compress_kernel_lz4->setArg(3, c_input_size);

        // launch the kernels
        m_q->enqueueTask(*compress_kernel_lz4, NULL, k_wait_event + blk_idx);

        // write original file input data to stream
        wr_req.priv_data = (void*)((std::string("idata_") + std::to_string(blk_idx)).c_str());
        xcl::Stream::writeStream(cpr_input_stream, in + buf_indx, c_input_size, &wr_req, &cRet);

        // read compressed data from stream
        rd_req.priv_data = (void*)((std::string("cdata_") + std::to_string(blk_idx)).c_str());
        xcl::Stream::readStream(cpr_output_stream, h_buf_out.data() + buf_indx, c_input_size, &rd_req, &cRet);

        // read compressed data size from stream
        rd_req.priv_data = (void*)((std::string("csize_") + std::to_string(blk_idx)).c_str());
        xcl::Stream::readStream(cpr_csize_stream, h_compressSize.data() + blk_idx, 4, &rd_req, &cRet);

        // poll the requests for completion, this is a blocking call till all the streams are done
        xcl::Stream::pollStreams(m_device.get(), poll_req, req_cnt, req_cnt, &req_cnt, 20000, &cRet);
    }

    // wait for individual items to complete and then copy compressed data to output buffers
    /*int32_t req_cnt = 3*total_block_count;
    cl_streams_poll_req_completions poll_req[req_cnt];
    std::memset(poll_req, 0, sizeof(cl_streams_poll_req_completions) * req_cnt);
    // poll the requests for completion, this is a blocking call till all the streams are done
    xcl::Stream::pollStreams(m_device.get(), poll_req, req_cnt, req_cnt, &req_cnt, 50000, &cRet);
    */
    // after the compression is done, copy the buffers
    uint64_t outIdx = 0;
    for (uint64_t i = 0; i < total_block_count; ++i) {
        total_kernel_time += getEventDurationNs(k_wait_event[i]);
        // copy the compressed data to out pointer
        uint32_t compressedSize = h_compressSize.data()[i];
        // std::cout << "Compressed size: " << compressedSize << std::endl;
        // current block input size
        uint32_t c_input_size = host_buffer_size;
        if (i == total_block_count - 1) c_input_size = input_size - (host_buffer_size * i);

        if (c_input_size > compressedSize) {
            // copy the compressed data
            std::memcpy(out + outIdx, &compressedSize, 4);
            outIdx += 4;
            std::memcpy(out + outIdx, h_buf_out.data() + i * host_buffer_size, compressedSize);
            outIdx += compressedSize;
        } else {
            // copy the original data, since no compression
            if (c_input_size == host_buffer_size) {
                out[outIdx++] = 0;
                out[outIdx++] = 0;

                if (c_input_size == MAX_BSIZE_64KB)
                    out[outIdx++] = BSIZE_NCOMP_64;
                else if (c_input_size == MAX_BSIZE_256KB)
                    out[outIdx++] = BSIZE_NCOMP_256;
                else if (c_input_size == MAX_BSIZE_1024KB)
                    out[outIdx++] = BSIZE_NCOMP_1024;
                else if (c_input_size == MAX_BSIZE_4096KB)
                    out[outIdx++] = BSIZE_NCOMP_4096;

                out[outIdx++] = NO_COMPRESS_BIT;
            } else {
                uint8_t tmp = c_input_size;
                out[outIdx++] = tmp;
                tmp = c_input_size >> 8;
                out[outIdx++] = tmp;
                tmp = c_input_size >> 16;
                out[outIdx++] = tmp;
                out[outIdx++] = NO_COMPRESS_BIT;
            }
            std::memcpy(out + outIdx, in + (host_buffer_size * i), c_input_size);
            outIdx += c_input_size;
        }
    }
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
    float kernel_throughput_in_mbps_1 = (float)input_size * 1000 / total_kernel_time;
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;

    return outIdx;
}

uint64_t xfLz4Streaming::decompressFile(std::string& inFile_name, std::string& outFile_name, uint64_t input_size) {
    if (m_switch_flow == 0) {
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);

        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        std::vector<uint8_t, aligned_allocator<uint8_t> > in(input_size);

        // Read magic header 4 bytes
        char c = 0;
        char magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
        for (uint32_t i = 0; i < MAGIC_HEADER_SIZE; i++) {
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
        // printf("block_size %d \n", c);

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
        // printf("m_block_size_in_kb %d \n", m_block_size_in_kb);

        // Original size
        uint64_t original_size = 0;
        inFile.read((char*)&original_size, 8);
        inFile.get(c);
        // printf("original_size %d \n", original_size);

        // Allocate output size
        std::vector<uint8_t, aligned_allocator<uint8_t> > out(original_size);

        // Read block data from compressed stream .lz4
        inFile.read((char*)in.data(), (input_size - 15));

        uint64_t debytes;
        // Decompression
        debytes = decompress(in.data(), out.data(), (input_size - 15), original_size);

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

// Note: Various block sizes supported by LZ4 standard are not applicable to
// this function. It just supports Block Size 64KB
uint64_t xfLz4Streaming::decompress(uint8_t* in, uint8_t* out, uint64_t input_size, uint64_t original_size) {
    uint64_t total_kernel_time = 0;
    // Allocate host buffers
    uint32_t host_buffer_size = m_block_size_in_kb * 1024;
    uint32_t total_block_count = (original_size - 1) / host_buffer_size + 1;

    cl::Event k_wait_event[total_block_count];
    // bool kExecFlag[total_block_count];

    // write and read the stream request flags
    cl_stream_xfer_req rd_req{0};
    cl_stream_xfer_req wr_req{0};

    rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;

    // poll request items
    int32_t req_cnt = 2;
    uint64_t cIdx = 0;

    auto total_start = std::chrono::high_resolution_clock::now();

    for (uint64_t buf_indx = 0, blk_idx = 0; buf_indx < original_size; buf_indx += host_buffer_size, ++blk_idx) {
        // get the size of the compressed block
        uint32_t compressedSize = 0;

        std::memcpy(&compressedSize, in + cIdx, 4);
        cIdx += 4;

        uint32_t tmp = compressedSize;
        tmp >>= 24;

        if (tmp == 128) {
            uint8_t b1 = compressedSize;
            uint8_t b2 = compressedSize >> 8;
            uint8_t b3 = compressedSize >> 16;
            // uint8_t b4 = compressedSize >> 24;

            if (b3 == 1) {
                compressedSize = host_buffer_size;
            } else {
                uint16_t size = 0;
                size = b2;
                size <<= 8;
                uint16_t temp = b1;
                size |= temp;
                compressedSize = size;
            }
        }
        // decompressed block input size
        uint32_t dBlockSize = host_buffer_size;
        if (blk_idx == total_block_count - 1) dBlockSize = original_size - (host_buffer_size * blk_idx);

        // copy data to buffers
        // If compressed size is less than original block size
        if (compressedSize < dBlockSize) {
            // set kernel arguments
            decompress_kernel_lz4->setArg(2, compressedSize);
            decompress_kernel_lz4->setArg(3, dBlockSize);

            // launch the kernels
            m_q->enqueueTask(*decompress_kernel_lz4, NULL, k_wait_event + blk_idx);

            // write compressed file data to stream
            xcl::Stream::writeStream(dcp_input_stream, in + cIdx, compressedSize, &wr_req, &dRet);

            // read decompressed data from stream
            xcl::Stream::readStream(dcp_output_stream, out + buf_indx, dBlockSize, &rd_req, &dRet);

            // for polling the decompression request made here
            // req_cnt += 2;
            cl_streams_poll_req_completions poll_req[req_cnt];
            xcl::Stream::pollStreams(m_device.get(), poll_req, req_cnt, req_cnt, &req_cnt, 20000, &dRet);

            total_kernel_time += getEventDurationNs(k_wait_event[blk_idx]);

            cIdx += compressedSize;
            // kExecFlag[blk_idx] = true;
            // std::cout << "Compressed size: " << compressedSize << std::endl;
        } else {
            // no compression, copy as it is to output
            std::memcpy(out + buf_indx, in + cIdx, dBlockSize);
            cIdx += dBlockSize;
            // kExecFlag[blk_idx] = false;
        }
    }
    /*
    cl_streams_poll_req_completions poll_req[req_cnt];
    std::memset(poll_req, 0, sizeof(cl_streams_poll_req_completions) * req_cnt);
    // poll the requests for completion, this is a blocking call till all the streams are done
    xcl::Stream::pollStreams(m_device.get(), poll_req, req_cnt, req_cnt, &req_cnt, 50000, &dRet);
    */
    auto total_end = std::chrono::high_resolution_clock::now();
    /*for(uint64_t i = 0; i < total_block_count; ++i){
        if(kExecFlag[i]) total_kernel_time += getEventDurationNs(k_wait_event[i]);
    }*/
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
    float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
    float kernel_throughput_in_mbps_1 = (float)input_size * 1000 / total_kernel_time;
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "\t\t";
    std::cout << std::fixed << std::setprecision(2) << kernel_throughput_in_mbps_1;

    return original_size;
} // End of decompress
