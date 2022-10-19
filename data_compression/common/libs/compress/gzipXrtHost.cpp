/*
 * (c) Copyright 2019-2022 Xilinx, Inc. All rights reserved.
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
#include "zlib.h"
#include "gzipXrtHost.hpp"

extern unsigned long crc32(unsigned long crc, const unsigned char* buf, uint32_t len);
extern unsigned long adler32(unsigned long crc, const unsigned char* buf, uint32_t len);

gzipXrtHost::gzipXrtHost(enum State flow,
                         const std::string& xclbin,
                         uint8_t device_id,
                         uint8_t decKernelType,
                         uint8_t dflow,
                         bool freeRunKernel) {
    m_cdflow = flow;
    m_deviceid = device_id;
    m_zlibFlow = (enum design_flow)dflow;
    m_kidx = decKernelType;
    m_pending = 0;
    m_device = xrt::device(m_deviceid);
    m_uuid = m_device.load_xclbin(xclbin);
    m_freeRunKernel = freeRunKernel;
    if (m_zlibFlow) {
        m_checksum = 1;
        m_isZlib = true;
    }
}

uint64_t gzipXrtHost::decompressEngineSeq(
    uint8_t* in, uint8_t* out, uint64_t input_size, size_t max_outbuf_size, int cu) {
#if (VERBOSE_LEVEL >= 1)
    std::cout << "CU" << cu << " ";
#endif
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);

    // Streaming based solution
    uint32_t inBufferSize = input_size;
    uint32_t isLast = 1;
    const uint32_t lim_4gb = (uint32_t)(((uint64_t)1024 * 1024 * 1024) - 2); // 4GB limit on output size
    uint32_t outBufferSize = 0;

    if (max_outbuf_size > lim_4gb) {
        outBufferSize = lim_4gb;
    } else {
        outBufferSize = (uint32_t)max_outbuf_size;
    }

    auto data_writer_kernel = xrt::kernel(m_device, m_uuid, data_writer_kernel_name.c_str());
    auto data_reader_kernel = xrt::kernel(m_device, m_uuid, data_reader_kernel_name.c_str());

    auto buffer_dec_input = xrt::bo(m_device, inBufferSize, data_writer_kernel.group_id(0));
    auto buffer_dec_zlib_output = xrt::bo(m_device, outBufferSize, data_reader_kernel.group_id(0));
    auto buffer_size = xrt::bo(m_device, sizeof(uint32_t) * 10, data_reader_kernel.group_id(1));
    auto buffer_status = xrt::bo(m_device, sizeof(uint32_t) * 10, data_reader_kernel.group_id(2));

    auto dbuf_in = buffer_dec_input.map<uint8_t*>();
    auto dbuf_out = buffer_dec_zlib_output.map<uint8_t*>();
    auto dbuf_outSize = buffer_size.map<uint32_t*>();
    auto h_dcompressStatus = buffer_status.map<uint32_t*>();
    h_dcompressStatus[0] = 0;

    // Copy input data
    std::memcpy(dbuf_in, in, inBufferSize); // must be equal to input_size
    buffer_dec_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_status.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // start parallel reader kernel enqueue thread
    uint32_t decmpSizeIdx = 0;
    xrt::run reader_run;
    xrt::run writer_run;
    if (this->is_freeRunKernel()) {
        reader_run = data_reader_kernel(buffer_dec_zlib_output, buffer_size, buffer_status, outBufferSize);
        auto decompress_API_start = std::chrono::high_resolution_clock::now();
        writer_run = data_writer_kernel(buffer_dec_input, inBufferSize, isLast);
        writer_run.wait();
        auto decompress_API_end = std::chrono::high_resolution_clock::now();
        reader_run.wait();
        auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
        decompress_API_time_ns_1 += duration;
    } else {
        auto decompress_kernel = xrt::kernel(m_device, m_uuid, stream_decompress_kernel_name[m_kidx].c_str());
        auto decompress_API_start = std::chrono::high_resolution_clock::now();
        auto decomp_run = xrt::run(decompress_kernel);
        decomp_run.start();
        decomp_run.wait();
        auto decompress_API_end = std::chrono::high_resolution_clock::now();
        writer_run.wait();
        reader_run = data_reader_kernel(buffer_dec_zlib_output, buffer_size, buffer_status, outBufferSize);
        writer_run = data_writer_kernel(buffer_dec_input, inBufferSize, isLast);
        auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
        decompress_API_time_ns_1 += duration;
    }

    // copy decompressed output data
    buffer_size.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    buffer_status.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // decompressed size
    decmpSizeIdx = dbuf_outSize[0];
    if (decmpSizeIdx >= max_outbuf_size) {
        auto brkloop = false;
        do {
            reader_run = data_reader_kernel(buffer_dec_zlib_output, buffer_size, buffer_status, outBufferSize);
            reader_run.wait();

            // copy decompressed output data
            buffer_size.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            buffer_status.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

            decmpSizeIdx += dbuf_outSize[0];
            brkloop = h_dcompressStatus[0];
        } while (!brkloop);
        std::cout << "\n" << std::endl;
        std::cout << "compressed size (.gz/.xz): " << input_size << std::endl;
        std::cout << "decompressed size : " << decmpSizeIdx << std::endl;
        std::cout << "maximum output buffer allocated: " << max_outbuf_size << std::endl;
        std::cout << "Output Buffer Size Exceeds as the Compression Ratio is High " << std::endl;
        std::cout << "Use -mcr option to increase the output buffer size (Default: 20) --> Aborting " << std::endl;
        abort();
    } else {
        // Sync zlib output buffer
        buffer_dec_zlib_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        // copy output decompressed data
        std::memcpy(out, dbuf_out, decmpSizeIdx);
    }
    float throughput_in_mbps_1 = (float)decmpSizeIdx * 1000 / decompress_API_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

    // printme("Done with decompress \n");
    return decmpSizeIdx;
}

// This version of compression does sequential execution between kernel an dhost
size_t gzipXrtHost::compressEngineSeq(
    uint8_t* in, uint8_t* out, size_t input_size, int level, int strategy, int window_bits, uint32_t* checksum) {
#ifdef GZIP_STREAM
    std::string compress_kname = compress_kernel_names[1];
#else
    std::string compress_kname = compress_kernel_names[2];
#endif
    xrt::kernel compressKernel;
    if (!this->is_freeRunKernel()) {
        compressKernel = xrt::kernel(m_device, m_uuid, compress_kname.c_str());
    }

    if (this->is_freeRunKernel()) datamoverKernel = xrt::kernel(m_device, m_uuid, datamover_kernel_name.c_str());

    auto c_inputSize = input_size;
    auto numItr = 1;
    if (this->is_freeRunKernel()) {
        if (c_inputSize < 0x40000000) {
            numItr = xcl::is_emulation() ? 1 : (0x40000000 / c_inputSize);
        }
    }

    bool checksum_type;
    auto blockSize = BLOCK_SIZE_IN_KB * 1024; // can be changed for custom testing of block sizes upto 4KB.
    auto blckNum = (input_size - 1) / blockSize + 1;
    auto output_size = 10 * blckNum * BLOCK_SIZE_IN_KB * 1024;
    // Host buffers
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_input_buffer(c_inputSize);
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_output_buffer(output_size);
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_compressSize(2 * blckNum);
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_checkSumData(1);

    if (m_zlibFlow) {
        h_checkSumData.data()[0] = 1;
        checksum_type = false;
    } else {
        h_checkSumData.data()[0] = ~0;
        checksum_type = true;
    }

    xrt::bo buffer_input, buffer_output, buffer_cSize, buffer_checkSum;

    if (this->is_freeRunKernel()) {
        buffer_input =
            xrt::bo(m_device, h_input_buffer.data(), sizeof(uint8_t) * c_inputSize, datamoverKernel.group_id(0));
        buffer_output =
            xrt::bo(m_device, h_output_buffer.data(), sizeof(uint8_t) * output_size, datamoverKernel.group_id(1));
        buffer_cSize =
            xrt::bo(m_device, h_compressSize.data(), sizeof(uint32_t) * blckNum, datamoverKernel.group_id(5));
    } else {
        buffer_input =
            xrt::bo(m_device, h_input_buffer.data(), sizeof(uint8_t) * c_inputSize, compressKernel.group_id(0));
        buffer_output =
            xrt::bo(m_device, h_output_buffer.data(), sizeof(uint8_t) * output_size, compressKernel.group_id(1));
        buffer_cSize = xrt::bo(m_device, h_compressSize.data(), sizeof(uint32_t) * blckNum, compressKernel.group_id(2));
        buffer_checkSum = xrt::bo(m_device, h_checkSumData.data(), sizeof(uint32_t) * 10, compressKernel.group_id(3));
    }

    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);
    auto enbytes = 0;
    auto outIdx = 0;

    // Process the data serially
    uint32_t buf_size = c_inputSize;

    // Copy input data
    std::memcpy(h_input_buffer.data(), in, buf_size);

    if (this->is_freeRunKernel()) {
        buffer_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        buffer_output.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        buffer_cSize.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    } else {
        buffer_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        buffer_checkSum.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }

    xrt::run datamover_run;
    xrt::run cKernel_run;
    xrt::run run;

    auto kernel_start = std::chrono::high_resolution_clock::now();

#ifndef PERF_DM
    for (auto z = 0; z < numItr; z++) {
#endif
        if (this->is_freeRunKernel()) {
            datamover_run = datamoverKernel(buffer_input, buffer_output, buf_size,
#ifdef PERF_DM
                                            numItr, blockSize,
#endif
                                            buffer_cSize);
        } else {
            // Running Kernel
            run = compressKernel(buffer_input, buffer_output, buffer_cSize, buffer_checkSum, buf_size, checksum_type);
        }
#ifndef PERF_DM
    }
#endif

    if (this->is_freeRunKernel()) {
        datamover_run.wait();
    } else {
        run.wait();
    }
    auto kernel_stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(kernel_stop - kernel_start);
    kernel_time_ns_1 += duration;

    if (!(this->is_freeRunKernel())) {
        // Copy data back
        buffer_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        buffer_cSize.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        buffer_checkSum.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    } else {
        // Copy data back
        buffer_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        buffer_cSize.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    uint32_t compSizeCntr = 0;

    if (this->is_freeRunKernel()) {
#ifdef FILE_WRITE
        auto index = 0;
#endif
#ifdef PERF_DM
        uint32_t compSize = 0;
        for (long unsigned i = 0; i < 2 * blckNum; i += 2) {
            compSize = h_compressSize[i];
            compSizeCntr += compSize;
// if (!xcl::is_sw_emulation()) {
//    uint32_t compAPISize = h_compressSize[i + 1];
// AXI size and calculated size using strobe should be equal
//    assert(compAPISize == compSize);
//}
#ifdef FILE_WRITE
            std::stringstream ss;
            ss << (int)(i / 2);
            std::ofstream fOut("compBlockFile_" + ss.str());
            fOut.write(reinterpret_cast<const char*>(&h_output_buffer.data()[index]), compSize);
            index += blockSize * 2;
#endif
        }
#endif
    } else {
        // This handling required only for zlib
        // This is for next iteration
        if (checksum_type) h_checkSumData.data()[0] = ~h_checkSumData.data()[0];
    }

    float throughput_in_mbps_1 = (float)input_size * 1000 * numItr / kernel_time_ns_1.count();
    std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;

#ifndef PERF_DM
    compSizeCntr = h_compressSize[0];
    std::memcpy(out + outIdx, &h_output_buffer[0], compSizeCntr);
#endif
    enbytes += compSizeCntr;
    outIdx += compSizeCntr;

    if (!is_freeRunKernel()) {
        // Add last block header
        long int last_block = 0xffff000001;
        std::memcpy(&(out[outIdx]), &last_block, 5);
        enbytes += 5;

        m_checksum = h_checkSumData[0];
        if (checksum_type) m_checksum = ~m_checksum;

        *checksum = m_checksum;
    }
    return enbytes;
} // Sequential end
